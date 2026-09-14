"""Check the actual release ZIP with native Linux Compose, without a Docker daemon.

Uses a new /tmp directory, empty Docker credentials and a nonexistent socket.
Build/start failures due to the missing daemon are recorded, never called passes.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import secrets
import subprocess
import tempfile
import zipfile

TOOLS = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('fix', TOOLS / 'fix_server_linux_startup.py')
fix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fix)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('zip', type=Path)
    parser.add_argument('--compose', required=True, type=Path)
    parser.add_argument('--evidence-dir', type=Path, help='Keep redacted command logs outside /tmp')
    parser.add_argument('--patch-zip', type=Path, help='Apply the actual four-file patch instead of templates')
    args = parser.parse_args()
    root = Path(tempfile.mkdtemp(prefix='tongdou-v012-linux-'))
    evidence = args.evidence_dir.resolve() if args.evidence_dir else root
    evidence.mkdir(parents=True, exist_ok=True)
    print('Isolated evidence directory:', root)
    print('Original ZIP SHA256:', hashlib.sha256(args.zip.read_bytes()).hexdigest())
    with zipfile.ZipFile(args.zip) as archive:
        archive.extractall(root / 'original')
        archive.extractall(root / 'patched')
    original = next((root / 'original').iterdir())
    patched = next((root / 'patched').iterdir())
    docker_config = root / 'docker-config'
    docker_config.mkdir()
    env = {'PATH': '/usr/local/bin:/usr/bin:/bin', 'HOME': str(root),
           'DOCKER_CONFIG': str(docker_config), 'DOCKER_HOST': 'unix://' + str(root / 'unavailable.sock')}

    def compose(folder, command):
        return subprocess.run([str(args.compose.resolve()), '--project-directory', str(folder),
                               '-f', str(folder / 'docker-compose.yml'), '-p', 'tongdou-linux-isolated'] + command,
                              cwd=root, env=env, text=True, capture_output=True, timeout=90)

    result = compose(original, ['config', '--format', 'json'])
    assert result.returncode == 0, result.stderr
    model = json.loads(result.stdout)
    assert model['services']['tongdou-db']['environment']['MYSQL_ROOT_PASSWORD'] == ''
    assert model['services']['voiceprint-api']['environment']['VOICEPRINT_API_TOKEN'] == ''
    (evidence / 'original-config-warnings.txt').write_text(result.stderr)
    print('PASS: original ZIP reproduces both missing-variable warnings and empty container settings.')
    if args.patch_zip:
        with zipfile.ZipFile(args.patch_zip) as archive:
            assert set(archive.namelist()) == {'docker-compose.yml', 'START_TONGDOU_SERVER.sh',
                                              'scripts/start_tongdou_server.py', 'LINUX_FIRST_START_EN.md'}
            archive.extractall(patched)
    else:
        fix.install(patched)
    result = compose(patched, ['config', '--quiet'])
    assert result.returncode != 0
    print('PASS: patched Compose rejects missing credentials before build/start.')
    # Synthetic private fixture, NOT a claim that a real daemon ran initialization.
    text = (patched / '.env.example').read_text().replace('COMPOSE_PROJECT_NAME=tongdou-v010',
                                                        'COMPOSE_PROJECT_NAME=tongdou-linux-isolated')
    credentials = {}
    for key in ('MYSQL_ROOT_PASSWORD', 'VOICEPRINT_API_TOKEN'):
        credentials[key] = secrets.token_urlsafe(32)
        text = text.replace(key + '=GENERATED_ON_FIRST_START', key + '=' + credentials[key])
    path = patched / '.env'
    with os.fdopen(os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600), 'w') as stream:
        stream.write(text)
    result = compose(patched, ['config', '--format', 'json'])
    assert result.returncode == 0 and not result.stderr, result.stderr
    services = json.loads(result.stdout)['services']
    assert services['tongdou-db']['environment']['MYSQL_ROOT_PASSWORD'] == credentials['MYSQL_ROOT_PASSWORD']
    assert services['tongdou-manager']['environment']['SPRING_DATASOURCE_DRUID_PASSWORD'] == credentials['MYSQL_ROOT_PASSWORD']
    assert services['voiceprint-api']['environment']['VOICEPRINT_MYSQL_PASSWORD'] == credentials['MYSQL_ROOT_PASSWORD']
    assert services['voiceprint-api']['environment']['VOICEPRINT_API_TOKEN'] == credentials['VOICEPRINT_API_TOKEN']
    assert services['tongdou-manager']['environment']['VOICEPRINT_API_ENDPOINT'].endswith('key=' + credentials['VOICEPRINT_API_TOKEN'])
    for service in ('tongdou-manager', 'tongdou-voice', 'voiceprint-api'):
        assert services[service]['pull_policy'] == 'never'
        build = services[service]['build']
        assert Path(build['context']) == patched
        assert (patched / build['dockerfile']).is_file()
    print('PASS: native Compose resolves all credential consumers, three build contexts and never-pull policies.')
    for label, command in [('build', ['build', 'tongdou-manager', 'tongdou-voice', 'voiceprint-api']),
                           ('start', ['up', '-d', '--no-build']), ('status', ['ps', '--all'])]:
        result = compose(patched, command)
        output = result.stdout + result.stderr
        for value in credentials.values():
            assert value not in output
        (evidence / (label + '.txt')).write_text(output)
        assert result.returncode != 0
        print(f'UNVERIFIED: actual {label} exited {result.returncode}. See {evidence / (label + ".txt")}.')
        print(output[-1200:])
    result = subprocess.run(['sh', '-n', str(patched / 'START_TONGDOU_SERVER.sh')], capture_output=True)
    assert result.returncode == 0
    print('PASS: POSIX shell entry syntax.')


if __name__ == '__main__':
    main()
