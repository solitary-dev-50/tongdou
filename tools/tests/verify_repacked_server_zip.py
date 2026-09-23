"""Independently extract and verify a completed server ZIP on Linux, without Docker."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from repack_server_v012 import audit, NAME


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--zip', type=Path, required=True)
    parser.add_argument('--compose', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    sandbox = Path(tempfile.mkdtemp(prefix='tongdou-zip-verification-'))
    checksum = hashlib.sha256(args.zip.read_bytes()).hexdigest()
    expected = checksum + '  ' + args.zip.name + '\n'
    assert args.zip.with_suffix('.zip.sha256.txt').read_text() == expected
    assert (args.zip.parent / 'SHA256SUMS').read_text() == expected
    with zipfile.ZipFile(args.zip) as archive:
        assert archive.testzip() is None
        shell = archive.getinfo(NAME + '/START_TONGDOU_SERVER.sh')
        assert shell.external_attr >> 16 & 0o777 == 0o755
        archive.extractall(sandbox)
    package = sandbox / NAME
    result = audit(args.original, package)
    hashes = subprocess.run(['sha256sum', '--quiet', '-c', 'SHA256SUMS.txt'], cwd=package,
                            capture_output=True, text=True)
    assert hashes.returncode == 0, 'Extracted per-file checksums failed'
    config_dir = sandbox / 'empty-docker-config'
    config_dir.mkdir()
    env = {'PATH': '/usr/bin:/bin', 'HOME': str(sandbox), 'DOCKER_CONFIG': str(config_dir),
           'DOCKER_HOST': 'unix://' + str(sandbox / 'nonexistent.sock')}
    command = [str(args.compose), '-f', str(package / 'docker-compose.yml')]
    missing = subprocess.run(command + ['config', '--quiet'], cwd=package, env=env, capture_output=True)
    assert missing.returncode != 0, 'Missing credentials should fail validation'
    valid = subprocess.run(command + ['--env-file', str(package / '.env.example'), 'config', '--format', 'json'],
                           cwd=package, env=env, text=True, capture_output=True)
    assert valid.returncode == 0 and not valid.stderr, 'Compose schema validation failed'
    services = json.loads(valid.stdout)['services']
    for name in ('tongdou-voice', 'tongdou-manager', 'voiceprint-api'):
        assert services[name]['pull_policy'] == 'never'
        assert Path(services[name]['build']['context']) == package
    assert not (package / '.env').exists()
    # Native shell parser, without executing the launcher.
    for path in package.rglob('*.sh'):
        assert subprocess.run(['bash', '-n', str(path)], capture_output=True).returncode == 0
    result.update(zip_sha256=checksum, extracted_checksums_passed=True,
                  compose_schema_passed=True, missing_credentials_rejected=True,
                  real_docker_started=False, existing_volumes_touched=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
