"""Run the real shell/Python launcher in fresh Linux folders with simulated Docker.

Only `compose config` uses the real native Compose binary. Every container, SQL
and port-probe operation is simulated and fails closed for unexpected commands.
No host Docker socket, existing images, credentials or volumes are used.
"""
import argparse
import hashlib
import http.server
import json
import os
from pathlib import Path
import secrets
import shutil
import subprocess
import tempfile
import threading

FAKE_DOCKER = r'''#!/usr/bin/python3
import base64, json, os, pathlib, subprocess, sys
args = sys.argv[1:]
root = pathlib.Path(os.environ['SIM_PACKAGE'])
state = pathlib.Path(os.environ['SIM_EVENTS'])
events = json.loads(state.read_text()) if state.exists() else []
failure = os.environ.get('SIM_FAIL', '')
def record(event):
    events.append(event)
    state.write_text(json.dumps(events))
def stop():
    print('SIMULATED Docker failure', file=sys.stderr)
    sys.exit(1)
if args == ['info']:
    if failure == 'daemon': stop()
    record('info'); sys.exit(0)
if args == ['volume', 'ls', '--format', '{{.Name}}']:
    record('empty-volume-inventory'); sys.exit(0)
assert args[0] == 'compose', 'Only simulated Compose operations are allowed'
assert args[1:9] == ['--project-directory', str(root), '--env-file', str(root / '.env'), '-f', str(root / 'docker-compose.yml'), '-p', 'tongdou-simulation']
cmd = args[9:]
if cmd == ['config', '--format', 'json']:
    record('real-compose-config')
    os.execv(os.environ['SIM_COMPOSE'], [os.environ['SIM_COMPOSE']] + args[1:])
values = dict(line.split('=', 1) for line in (root / '.env').read_text().splitlines() if line and not line.startswith('#'))
for key in ('MYSQL_ROOT_PASSWORD', 'VOICEPRINT_API_TOKEN'):
    assert values[key] not in ' '.join(args), 'Secret in command arguments'
if cmd == ['build', 'tongdou-manager', 'tongdou-voice', 'voiceprint-api']:
    record('simulated-build')
    if failure == 'build': stop()
elif cmd[:6] == ['up', '-d', '--no-build', '--wait', '--wait-timeout', '900']:
    assert 'simulated-build' in events
    if len(cmd) > 6:
        assert cmd[6:] == ['tongdou-db', 'tongdou-redis', 'tongdou-manager']
        record('simulated-manager-up')
    else:
        assert 'simulated-cache-clear' in events
        text = (root / 'data/.config.yaml').read_text()
        assert os.environ['SIM_SECRET'] in text
        assert (root / 'data/.mcp_server_settings.json').is_file()
        record('simulated-all-up')
elif cmd[:3] == ['exec', '-T', 'tongdou-db']:
    assert 'simulated-manager-up' in events
    assert cmd[3:6] == ['sh', '-c', 'export MYSQL_PWD="$MYSQL_ROOT_PASSWORD"; exec mysql -uroot -N -B "$@"']
    sql = sys.stdin.read()
    if sql.startswith('SELECT param_value'):
        record('simulated-secret-read'); print(os.environ['SIM_SECRET'])
    elif sql.startswith('CREATE DATABASE IF NOT EXISTS'):
        record('simulated-voiceprint-schema')
    elif sql.startswith('UPDATE sys_params'):
        endpoint = 'http://voiceprint-api:8005/voiceprint/health?key=' + values['VOICEPRINT_API_TOKEN']
        assert base64.b64encode(endpoint.encode()).decode() in sql
        assert 'server.websocket' in sql and 'server.ota' in sql
        assert 'simulated-voiceprint-schema' in events
        record('simulated-endpoint-update')
    else: raise AssertionError('Unexpected SQL operation')
elif cmd == ['exec', '-T', 'tongdou-redis', 'redis-cli', 'HDEL', 'sys:params', 'server.voice_print', 'server.websocket', 'server.ota']:
    assert 'simulated-endpoint-update' in events
    record('simulated-cache-clear')
elif cmd[:5] == ['exec', '-T', 'tongdou-voice', 'python', '-c']:
    assert 'simulated-all-up' in events
    assert "socket.create_connection(('127.0.0.1',8000),5)" in cmd[5]
    record('simulated-voice-probe')
elif cmd == ['ps', '--all']:
    record('simulated-status'); print('SIMULATED service status; no real Docker containers started.')
else: raise AssertionError('Unrecognized simulated operation')
'''


class DashboardFixture(http.server.BaseHTTPRequestHandler):
    status_code = 200

    def do_GET(self):
        self.send_response(self.status_code)
        self.end_headers()
        self.wfile.write(b'SIMULATED dashboard')

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--compose', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    sandbox = Path(tempfile.mkdtemp(prefix='tongdou simulation '))
    binaries = sandbox / 'bin'
    binaries.mkdir()
    fake = binaries / 'docker'
    fake.write_text(FAKE_DOCKER, encoding='utf-8')
    fake.chmod(0o755)
    (sandbox / 'docker-config').mkdir()
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), DashboardFixture)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    results = []

    def fresh(name):
        root = sandbox / name
        shutil.copytree(args.package, root)
        assert not (root / '.env').exists()
        assert not (root / 'data/.config.yaml').exists()
        example = root / '.env.example'
        example.write_text(example.read_text().replace('COMPOSE_PROJECT_NAME=tongdou-v010', 'COMPOSE_PROJECT_NAME=tongdou-simulation')
                           .replace('TONGDOU_DASHBOARD_PORT=8102', 'TONGDOU_DASHBOARD_PORT=' + str(server.server_port)))
        return root

    def invoke(root, failure='', configure_only=False, expect=0, ip='192.168.1.20'):
        env = {'PATH': str(binaries) + ':/usr/bin:/bin', 'HOME': str(sandbox),
               'DOCKER_HOST': 'unix://' + str(sandbox / 'does-not-exist.sock'),
               'DOCKER_CONFIG': str(sandbox / 'docker-config'), 'PYTHONDONTWRITEBYTECODE': '1',
               'SIM_PACKAGE': str(root), 'SIM_EVENTS': str(root / 'simulation-events.json'),
               'SIM_COMPOSE': str(args.compose.resolve()), 'SIM_SECRET': manager_secret,
               'SIM_FAIL': failure, 'MYSQL_ROOT_PASSWORD': 'deliberately-wrong-export',
               'VOICEPRINT_API_TOKEN': 'deliberately-wrong-export', 'COMPOSE_FILE': '/invalid-compose.yml'}
        command = ['sh', str(root / 'START_TONGDOU_SERVER.sh'), '--lan-ip', ip]
        if configure_only:
            command.append('--configure-only')
        result = subprocess.run(command, cwd=sandbox, env=env, capture_output=True, text=True, timeout=90)
        output = result.stdout + result.stderr
        sensitive = [manager_secret]
        if (root / '.env').exists():
            values = dict(line.split('=', 1) for line in (root / '.env').read_text().splitlines() if line and not line.startswith('#'))
            sensitive.extend(values[k] for k in ('MYSQL_ROOT_PASSWORD', 'VOICEPRINT_API_TOKEN'))
        assert all(value not in output for value in sensitive), 'Secret leaked in launcher output'
        if result.returncode != expect:
            raise AssertionError('Unexpected launcher status: ' + output)
        if expect:
            assert 'Services passed startup checks' not in output
        return json.loads((root / 'simulation-events.json').read_text()) if (root / 'simulation-events.json').exists() else []

    manager_secret = secrets.token_urlsafe(32)
    try:
        first = fresh('first install with spaces')
        events = invoke(first)
        assert events[-1] == 'simulated-status'
        assert events.index('simulated-manager-up') < events.index('simulated-secret-read') < events.index('simulated-all-up')
        paths = [first / '.env', first / 'data/.config.yaml', first / 'data/.mcp_server_settings.json']
        before = [hashlib.sha256(p.read_bytes()).hexdigest() for p in paths]
        assert all(p.stat().st_mode & 0o777 == 0o600 for p in paths)
        results.append('first-start: real shell, random credentials, real Compose config, simulated services, private files, no secret output')
        invoke(first)
        assert before == [hashlib.sha256(p.read_bytes()).hexdigest() for p in paths]
        results.append('repeat-start: credentials and user configuration preserved')
        only = fresh('configure only')
        events = invoke(only, configure_only=True)
        assert 'simulated-build' not in events and not (only / 'data/.config.yaml').exists()
        results.append('configure-only: no simulated build or service startup')
        interrupted = fresh('build interrupted')
        events = invoke(interrupted, failure='build', expect=1)
        assert 'simulated-manager-up' not in events
        saved = (interrupted / '.env').read_bytes()
        invoke(interrupted)
        assert saved == (interrupted / '.env').read_bytes()
        results.append('build-failure-and-retry: no premature startup, same credentials reused')
        unavailable = fresh('daemon unavailable')
        invoke(unavailable, failure='daemon', expect=1)
        assert not (unavailable / '.env').exists()
        results.append('daemon-unavailable: failure without creating credentials')
        invoke(unavailable, expect=1, ip='127.0.0.1')
        assert not (unavailable / '.env').exists()
        results.append('invalid-address: failure before touching configuration')
        DashboardFixture.status_code = 503
        invoke(first, expect=1)
        results.append('dashboard-http-failure: no false success')
    finally:
        server.shutdown()
        server.server_close()
    report = {'mode': 'SIMULATION ONLY; real Compose config, fake Docker and fixture HTTP',
              'real_docker_started': False, 'existing_volumes_touched': False,
              'passed': results, 'failed': [], 'sandbox': str(sandbox)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
