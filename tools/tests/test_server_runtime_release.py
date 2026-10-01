"""Check the prebuilt runtime launcher without touching Docker or existing data."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
TEMPLATE = ROOT / 'tools/release_templates/server_runtime'
spec = importlib.util.spec_from_file_location('runtime_launcher', TEMPLATE / 'scripts/start_tongdou_server.py')
launcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(launcher)
control_spec = importlib.util.spec_from_file_location('runtime_control', TEMPLATE / 'scripts/control_tongdou_server.py')
import sys
with patch.dict(sys.modules, {'start_tongdou_server': launcher}):
    control = importlib.util.module_from_spec(control_spec)
    control_spec.loader.exec_module(control)


class RuntimeReleaseTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        shutil.copy2(TEMPLATE / '.env.example', self.root / '.env.example')
        sql = self.root / 'main/xiaozhi-server/mysql-init/01-voiceprint.sql'
        sql.parent.mkdir(parents=True)
        sql.write_text('CREATE DATABASE IF NOT EXISTS voiceprint_db;')
        patcher = patch.object(launcher, 'ROOT', self.root)
        patcher.start()
        self.addCleanup(patcher.stop)
        control_patcher = patch.object(control, 'ROOT', self.root)
        control_patcher.start()
        self.addCleanup(control_patcher.stop)
        self.commands = []
        self.volumes = ''

    def fake_run(self, args, env, quiet=False, input_text=None, check=True):
        self.commands.append(args)
        out = ''
        if args[1:3] == ['volume', 'ls']:
            out = self.volumes
        elif args[1] == 'run':
            return subprocess.CompletedProcess(args, 1, '', '')
        elif 'config' in args:
            values = launcher.read_env(self.root / '.env')
            password = values['MYSQL_ROOT_PASSWORD']
            token = values['VOICEPRINT_API_TOKEN']
            version = values['TONGDOU_VERSION']
            namespace = values['TONGDOU_IMAGE_NAMESPACE']
            out = json.dumps({'services': {
                'tongdou-voice': {'image': f'{namespace}/server:{version}'},
                'tongdou-manager': {'image': f'{namespace}/manager:{version}', 'environment': {
                    'SPRING_DATASOURCE_DRUID_PASSWORD': password,
                    'VOICEPRINT_API_ENDPOINT': 'http://voiceprint-api:8005/voiceprint/health?key=' + token}},
                'voiceprint-api': {'image': f'{namespace}/voiceprint:{version}', 'environment': {
                    'VOICEPRINT_MYSQL_PASSWORD': password, 'VOICEPRINT_API_TOKEN': token}},
                'tongdou-db': {'environment': {'MYSQL_ROOT_PASSWORD': password}},
            }})
        elif input_text and input_text.startswith('SELECT '):
            out = 'manager-secret\n'
        return subprocess.CompletedProcess(args, 0, out, '')

    def start(self, configure_only=False):
        args = ['launcher', '--lan-ip', '192.168.1.20']
        if configure_only:
            args.append('--configure-only')
        with patch.object(launcher, 'run', self.fake_run), patch('sys.argv', args), \
                patch.object(launcher.urllib.request, 'urlopen') as urlopen, \
                patch.object(launcher.time, 'sleep'), patch.object(launcher, 'warn_resources'):
            urlopen.return_value.__enter__.return_value.status = 200
            launcher.main()

    def test_first_start_pulls_before_up_and_never_builds(self):
        self.start()
        self.assertTrue((self.root / '.env').is_file())
        self.assertTrue((self.root / 'data/.config.yaml').is_file())
        verbs = [part for args in self.commands for part in args if part in ('pull', 'up', 'build')]
        self.assertEqual(verbs[0], 'pull')
        self.assertNotIn('build', verbs)
        self.assertTrue(all('--no-build' in args for args in self.commands if 'up' in args))
        self.assertEqual(self.commands[-1][-1], '--all')

    def test_repeat_preserves_credentials(self):
        self.start()
        previous = (self.root / '.env').read_bytes()
        self.volumes = 'tongdou-v010-mysql-data\n'
        self.start()
        self.assertEqual(previous, (self.root / '.env').read_bytes())

    def test_existing_volume_without_password_stops_before_pull(self):
        self.volumes = 'tongdou-v010-mysql-data\n'
        with self.assertRaisesRegex(RuntimeError, 'Restore the matching .env'):
            self.start()
        self.assertFalse((self.root / '.env').exists())
        self.assertFalse(any('pull' in args or 'up' in args for args in self.commands))

    def test_compose_template_contains_no_build(self):
        text = (TEMPLATE / 'docker-compose.yml').read_text()
        self.assertNotIn('build:', text)
        for name in ('server', 'manager', 'voiceprint'):
            self.assertIn('/' + name + ':${TONGDOU_VERSION:', text)
        self.assertIn('voiceprint-model-cache:/root/.cache/modelscope', text)

    def test_status_and_stop_keep_the_same_project_and_volumes(self):
        self.start(configure_only=True)
        with patch.object(control.subprocess, 'run') as command:
            command.return_value.returncode = 0
            for action, expected in [('status', ['ps', '--all']), ('stop', ['stop'])]:
                with patch('sys.argv', ['control', action]):
                    self.assertEqual(control.main(), 0)
                args = command.call_args.args[0]
                self.assertEqual(args[-len(expected):], expected)
                self.assertIn('tongdou-v010', args)
                self.assertNotIn('-v', args)


if __name__ == '__main__':
    unittest.main()
