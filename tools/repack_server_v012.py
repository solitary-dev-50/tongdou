"""Repackage the original v0.1.2 server ZIP with deployment-only Linux fixes.

Never reads a running server, never invokes Docker, never overwrites an archive.
Stage first, run the separate validation suite, then archive that same directory.
"""
import argparse
import ast
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import zipfile

from fix_server_linux_startup import install, TEMPLATES

NAME = 'TongDou_Server_v0.1.2_Early_Tester'
ORIGINAL_SHA = 'a7bf8385768298fe1231adad7ad7efc758fdb08bfc6c3a4a96294316f6dbb215'
OVERRIDES = {'docker-compose.yml', 'README.md'}
ADDITIONS = {'START_TONGDOU_SERVER.sh', 'scripts/start_tongdou_server.py',
             'LINUX_FIRST_START_EN.md', 'REPACK_INFO.json'}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def original_files(source):
    if digest(source.read_bytes()) != ORIGINAL_SHA:
        raise ValueError('Original v0.1.2 ZIP digest mismatch.')
    with zipfile.ZipFile(source) as archive:
        result = {}
        for item in archive.infolist():
            if item.is_dir():
                continue
            path = PurePosixPath(item.filename)
            if path.parts[0] != NAME or '..' in path.parts or path.is_absolute():
                raise ValueError('Unexpected original ZIP path.')
            rel = '/'.join(path.parts[1:])
            if rel in result:
                raise ValueError('Duplicate original ZIP path.')
            result[rel] = archive.read(item)
    manifest = result['SHA256SUMS.txt'].decode('utf-8').splitlines()
    expected = {}
    for line in manifest:
        checksum, rel = line.split('  ', 1)
        expected[rel] = checksum
    if set(expected) != set(result) - {'SHA256SUMS.txt'}:
        raise ValueError('Original manifest does not cover every file.')
    if any(digest(result[rel]) != checksum for rel, checksum in expected.items()):
        raise ValueError('Original per-file checksum mismatch.')
    return result


def stage(source, destination):
    files = original_files(source)
    destination.mkdir(parents=True, exist_ok=False)
    for rel, data in files.items():
        path = destination / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    for directory in ('mysql', 'redis', 'uploadfile'):
        (destination / directory).mkdir(exist_ok=True)
    install(destination)
    shutil.copyfile(TEMPLATES / 'README.md', destination / 'README.md')
    info = {'server_version': '0.1.2', 'packaging_revision': 'linux-startup-20260915',
            'original_zip': source.name, 'original_sha256': ORIGINAL_SHA,
            'replaced_files': sorted(OVERRIDES), 'added_files': sorted(ADDITIONS),
            'business_source_changed': False, 'real_docker_startup_verified': False,
            'validation_scope': 'Static checks, simulated startup, native Compose config and archive integrity only.'}
    (destination / 'REPACK_INFO.json').write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
    write_checksums(destination)
    return audit(source, destination)


def write_checksums(folder):
    paths = sorted(p for p in folder.rglob('*') if p.is_file() and p.name != 'SHA256SUMS.txt')
    (folder / 'SHA256SUMS.txt').write_text(''.join(
        digest(p.read_bytes()) + '  ' + p.relative_to(folder).as_posix() + '\n' for p in paths), encoding='utf-8')


def audit(source, folder):
    original = original_files(source)
    current = {}
    for path in folder.rglob('*'):
        if path.is_symlink():
            raise ValueError('Symlink not allowed in release.')
        if path.is_file():
            current[path.relative_to(folder).as_posix()] = path.read_bytes()
    if set(current) != set(original) | ADDITIONS:
        raise ValueError('Unexpected file added to or removed from release.')
    changed = {rel for rel in original if original[rel] != current[rel]}
    if changed - OVERRIDES - {'SHA256SUMS.txt'}:
        raise ValueError('Business source or Windows launcher changed unexpectedly.')
    manifest = dict(line.split('  ', 1)[::-1] for line in current['SHA256SUMS.txt'].decode().splitlines())
    if set(manifest) != set(current) - {'SHA256SUMS.txt'}:
        raise ValueError('Incomplete new checksum manifest.')
    if any(digest(current[rel]) != checksum for rel, checksum in manifest.items()):
        raise ValueError('New file checksum mismatch.')
    python_count = 0
    for rel, data in current.items():
        path = PurePosixPath(rel)
        if path.name in {'.env', '.config.yaml', '.mcp_server_settings.json', '.voiceprint.yaml'} or path.suffix in {'.db', '.sqlite', '.pyc', '.log'}:
            raise ValueError('Runtime/private file in package: ' + rel)
        if path.suffix == '.py':
            ast.parse(data, filename=rel, feature_version=(3, 11))
            python_count += 1
    for rel in ('START_TONGDOU_SERVER.sh', 'scripts/start_tongdou_server.py', 'LINUX_FIRST_START_EN.md'):
        if current[rel] != (TEMPLATES / rel).read_bytes():
            raise ValueError('Launcher template mismatch: ' + rel)
    # Inspect COPY sources for all three build definitions; build-stage copies are internal.
    copy_count = 0
    for rel in ('Dockerfile-server', 'Dockerfile-web', 'main/xiaozhi-server/docker/voiceprint/Dockerfile'):
        for line in current[rel].decode('utf-8-sig').splitlines():
            if not line.startswith('COPY ') or '--from=' in line:
                continue
            for pattern in line.split()[1:-1]:
                if not list(folder.glob(pattern)):
                    raise ValueError('Missing COPY input: ' + pattern)
                copy_count += 1
    return {'files': len(current), 'python_syntax_files': python_count,
            'dockerfile_copy_inputs': copy_count, 'changed_original_files': sorted(changed),
            'windows_and_business_files_unchanged': True, 'runtime_files_absent': True}


def archive(source, folder):
    result = audit(source, folder)
    target = folder.parent / (NAME + '.zip')
    partial = target.with_suffix('.zip.partial')
    if target.exists() or partial.exists():
        raise FileExistsError('Archive output already exists; refusing to overwrite.')
    with zipfile.ZipFile(partial, 'x', zipfile.ZIP_DEFLATED, compresslevel=6) as output:
        for path in sorted(folder.rglob('*')):
            rel = path.relative_to(folder).as_posix()
            item = zipfile.ZipInfo(NAME + '/' + rel + ('/' if path.is_dir() else ''))
            item.create_system = 3
            mode = 0o40755 if path.is_dir() else (0o100755 if path.suffix == '.sh' else 0o100644)
            item.external_attr = mode << 16
            item.compress_type = zipfile.ZIP_DEFLATED
            output.writestr(item, b'' if path.is_dir() else path.read_bytes())
    with zipfile.ZipFile(partial) as output:
        expected = {NAME + '/' + p.relative_to(folder).as_posix(): p for p in folder.rglob('*') if p.is_file()}
        if {n for n in output.namelist() if not n.endswith('/')} != set(expected):
            raise ValueError('Archive file inventory mismatch.')
        for name, path in expected.items():
            if output.read(name) != path.read_bytes():
                raise ValueError('Archive content mismatch.')
        if output.testzip() is not None:
            raise ValueError('ZIP integrity failure.')
    partial.rename(target)
    checksum = digest(target.read_bytes())
    text = checksum + '  ' + target.name + '\n'
    target.with_suffix('.zip.sha256.txt').write_text(text, encoding='utf-8')
    (target.parent / 'SHA256SUMS').write_text(text, encoding='utf-8')
    return dict(result, zip=str(target), sha256=checksum, zip_bytes=target.stat().st_size)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=('stage', 'audit', 'archive'))
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--folder', type=Path, required=True)
    args = parser.parse_args()
    operation = {'stage': stage, 'audit': audit, 'archive': archive}[args.operation]
    print(json.dumps(operation(args.source.resolve(), args.folder.resolve()), indent=2))
