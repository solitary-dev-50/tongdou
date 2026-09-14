"""Bootstrap regression tests; Docker operations are simulated, not integration tests."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('launcher', TOOLS / 'release_templates/server/scripts/start_tongdou_server.py')
launcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launcher)
fix_spec = importlib.util.spec_from_file_location('fix', TOOLS / 'fix_server_linux_startup.py')
fix = importlib.util.module_from_spec(fix_spec)
fix_spec.loader.exec_module(fix)


class BootstrapTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='tongdou linux test ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.root_patch = patch.object(launcher, 'ROOT', self.root)
        self.root_patch.start()
        self.addCleanup(self.root_patch.stop)
        (self.root / '.env.example').write_text('COMPOSE_PROJECT_NAME=tongdou-test\nMYSQL_ROOT_PASSWORD=GENERATED_ON_FIRST_START\nVOICEPRINT_API_TOKEN=GENERATED_ON_FIRST_START\n')
        sql = self.root / 'main/xiaozhi-server/mysql-init/01-voiceprint.sql'
        sql.parent.mkdir(parents=True)
        sql.write_text('CREATE DATABASE IF NOT EXISTS voiceprint_db;')
        self.calls = []
        self.volumes = ''
        self.empty_volume = False
        self.failed_build = False
        self.failed_start = False

    def docker(self, args, env, quiet=False, input_text=None, check=True):
        self.calls.append((args, env, input_text))
        out = ''
        if args[1:3] == ['volume', 'ls']:
            out = self.volumes
        if args[1] == 'run':
            return subprocess.CompletedProcess(args, 0 if self.empty_volume else 1, '', '')
        if 'config' in args:
            values = launcher.read_env(self.root / '.env')
            password, token = (values[k] for k in launcher.CREDENTIALS)
            services = {name: {'build': {}, 'pull_policy': 'never'} for name in launcher.LOCAL_SERVICES}
            for service in services.values():
                service['build'] = {'context': str(self.root)}
            services['tongdou-db'] = {'environment': {'MYSQL_ROOT_PASSWORD': password}}
            services['tongdou-manager']['environment'] = {
                'SPRING_DATASOURCE_DRUID_PASSWORD': password,
                'VOICEPRINT_API_ENDPOINT': 'http://voiceprint-api:8005/voiceprint/health?key=' + token}
            services['voiceprint-api']['environment'] = {
                'VOICEPRINT_MYSQL_PASSWORD': password, 'VOICEPRINT_API_TOKEN': token}
            out = json.dumps({'services': services})
        if input_text and input_text.startswith('SELECT '):
            out = 'private-manager-secret\n'
        if ('build' in args and self.failed_build) or ('up' in args and self.failed_start):
            raise RuntimeError('simulated Docker failure')
        return subprocess.CompletedProcess(args, 0, out, '')

    def invoke(self, configure_only=False):
        argv = ['launcher', '--lan-ip', '192.168.1.20']
        if configure_only:
            argv.append('--configure-only')
        stream = io.StringIO()
        with patch.object(launcher, 'run', self.docker), patch('sys.argv', argv), \
                patch.object(launcher.urllib.request, 'urlopen') as http, contextlib.redirect_stdout(stream):
            http.return_value.__enter__.return_value.status = 200
            launcher.main()
        return stream.getvalue()

    def test_first_start_full_order_and_no_secret_output(self):
        output = self.invoke()
        values = launcher.read_env(self.root / '.env')
        for key in launcher.CREDENTIALS:
            self.assertRegex(values[key], r'^[A-Za-z0-9_-]{43}$')
            self.assertNotIn(values[key], output)
            self.assertFalse(any(values[key] in ' '.join(args) for args, _, _ in self.calls))
        self.assertNotIn('private-manager-secret', output)
        self.assertNotEqual(values[launcher.CREDENTIALS[0]], values[launcher.CREDENTIALS[1]])
        commands = [args for args, _, _ in self.calls]
        build = next(i for i, args in enumerate(commands) if 'build' in args)
        start = next(i for i, args in enumerate(commands) if 'up' in args)
        self.assertLess(build, start)
        self.assertEqual(commands[build][-3:], list(launcher.LOCAL_SERVICES))
        self.assertTrue(all('--no-build' in args and '--wait' in args for args in commands if 'up' in args))
        self.assertIn('private-manager-secret', (self.root / 'data/.config.yaml').read_text())
        if os.name == 'posix':
            self.assertEqual((self.root / '.env').stat().st_mode & 0o777, 0o600)

    def test_repeat_preserves_credentials_and_configuration(self):
        self.invoke()
        files = [self.root / '.env', self.root / 'data/.config.yaml', self.root / 'data/.mcp_server_settings.json']
        before = [p.read_bytes() for p in files]
        self.volumes = 'tongdou-test-mysql-data\n'
        self.invoke()
        self.assertEqual(before, [p.read_bytes() for p in files])

    def test_existing_volumes_missing_credentials_stop_without_writes(self):
        self.volumes = 'tongdou-test-mysql-data\n'
        with self.assertRaisesRegex(RuntimeError, 'Restore the matching .env'):
            self.invoke()
        self.assertFalse((self.root / '.env').exists())
        self.assertFalse(any('build' in args or 'up' in args for args, _, _ in self.calls))

    def test_template_env_is_repaired_only_on_empty_install(self):
        shutil.copyfile(self.root / '.env.example', self.root / '.env')
        self.invoke(configure_only=True)
        self.assertNotIn('GENERATED_ON_FIRST_START', (self.root / '.env').read_text())

    def test_empty_volumes_from_failed_first_attempt_are_reused_read_only(self):
        self.volumes = 'tongdou-test-mysql-data\ntongdou-test-voiceprint-data\n'
        self.empty_volume = True
        self.invoke(configure_only=True)
        checks = [args for args, _, _ in self.calls if args[1] == 'run']
        self.assertEqual(len(checks), 2)
        self.assertTrue(all(any(arg.endswith(',readonly') for arg in args) for args in checks))
        self.assertTrue((self.root / '.env').exists())

    def test_configure_only_does_not_start_or_build(self):
        self.invoke(configure_only=True)
        self.assertFalse(any('build' in args or 'up' in args for args, _, _ in self.calls))
        self.assertFalse((self.root / 'data').exists())

    def test_exported_environment_does_not_override_package(self):
        with patch.dict(os.environ, {'MYSQL_ROOT_PASSWORD': 'wrong', 'COMPOSE_FILE': '/wrong.yml',
                                    'COMPOSE_PROJECT_NAME': 'wrong', 'TONGDOU_WS_PORT': '9999'}):
            self.invoke()
        for _, env, _ in self.calls:
            for name in ('MYSQL_ROOT_PASSWORD', 'COMPOSE_FILE', 'COMPOSE_PROJECT_NAME', 'TONGDOU_WS_PORT'):
                self.assertNotIn(name, env)

    def test_build_failure_never_starts_services(self):
        self.failed_build = True
        with self.assertRaisesRegex(RuntimeError, 'simulated'):
            self.invoke()
        self.assertFalse(any('up' in args for args, _, _ in self.calls))

    def test_service_start_failure_stops_bootstrap(self):
        self.failed_start = True
        with self.assertRaisesRegex(RuntimeError, 'simulated'):
            self.invoke()
        self.assertFalse((self.root / 'data/.config.yaml').exists())

    def test_invalid_token_is_not_silently_rotated(self):
        text = 'MYSQL_ROOT_PASSWORD=existing-password\nVOICEPRINT_API_TOKEN=bad\n'
        (self.root / '.env').write_text(text)
        with self.assertRaisesRegex(RuntimeError, 'at least 32'):
            self.invoke()
        self.assertEqual(text, (self.root / '.env').read_text())

    def test_shell_code_and_duplicate_keys_rejected(self):
        for text in ('MYSQL_ROOT_PASSWORD=$(touch marker)\n', 'A=one\nA=two\n'):
            (self.root / '.env').write_text(text)
            with self.assertRaises(RuntimeError):
                launcher.read_env(self.root / '.env')
        self.assertFalse((self.root / 'marker').exists())

    def test_patch_is_idempotent_and_does_not_touch_windows_or_data(self):
        compose = 'services:\n' + ''.join(f'  {name}:\n    image: tongdou/{image}:v0.1.2\n' for name, image in (
            ('tongdou-voice', 'server'), ('tongdou-manager', 'manager'), ('voiceprint-api', 'voiceprint')))
        compose += '    environment:\n      MYSQL_ROOT_PASSWORD: ${MYSQL_ROOT_PASSWORD}\n'
        (self.root / 'docker-compose.yml').write_text(compose)
        (self.root / 'START_TONGDOU_SERVER.bat').write_bytes(b'unchanged')
        fix.install(self.root)
        before = (self.root / 'docker-compose.yml').read_bytes()
        fix.install(self.root)
        self.assertEqual(before, (self.root / 'docker-compose.yml').read_bytes())
        self.assertEqual((self.root / 'START_TONGDOU_SERVER.bat').read_bytes(), b'unchanged')
        self.assertFalse((self.root / '.env').exists())


if __name__ == '__main__':
    unittest.main()
