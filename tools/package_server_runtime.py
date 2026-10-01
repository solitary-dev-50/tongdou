"""Create a small, build-free TongDou Server runtime package for review."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = ROOT / 'tools/release_templates/server_runtime'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True, help='Reviewed source tree containing mysql-init/01-voiceprint.sql')
    parser.add_argument('--version', required=True, help='Same fixed tag used to build all three images')
    parser.add_argument('--namespace', default='tongdou', help='Same repository prefix used for image builds')
    parser.add_argument('--output', type=Path, required=True, help='New output directory; never overwrites an installation')
    args = parser.parse_args()
    if not re.fullmatch(r'v?[0-9]+\.[0-9]+\.[0-9]+(?:[-.][A-Za-z0-9.-]+)?', args.version):
        parser.error('Version must be a fixed release tag.')
    if not re.fullmatch(r'[a-z0-9][a-z0-9._/-]*', args.namespace):
        parser.error('Invalid image repository prefix.')
    source = args.source.resolve()
    sql = source / 'main/xiaozhi-server/mysql-init/01-voiceprint.sql'
    if not sql.is_file():
        parser.error('Missing required voiceprint initialization SQL: ' + str(sql))
    output = args.output.resolve()
    archive = output.with_suffix('.zip')
    if output.exists() or archive.exists():
        parser.error('Output exists; choose a new path. Existing installations are never overwritten.')
    shutil.copytree(TEMPLATE, output, ignore=shutil.ignore_patterns(
        'Dockerfile-voiceprint-release', '__pycache__', '*.pyc'))
    (output / 'main/xiaozhi-server/mysql-init').mkdir(parents=True)
    shutil.copy2(sql, output / 'main/xiaozhi-server/mysql-init/01-voiceprint.sql')
    for name in ('LICENSE',):
        if (source / name).is_file():
            shutil.copy2(source / name, output / name)
    (output / 'data').mkdir()
    (output / 'uploadfile').mkdir()
    env = output / '.env.example'
    content = env.read_text(encoding='utf-8')
    content = re.sub(r'(?m)^TONGDOU_VERSION=.*$', 'TONGDOU_VERSION=' + args.version, content)
    content = re.sub(r'(?m)^TONGDOU_IMAGE_NAMESPACE=.*$', 'TONGDOU_IMAGE_NAMESPACE=' + args.namespace, content)
    env.write_text(content, encoding='utf-8', newline='\n')
    files = sorted(p for p in output.rglob('*') if p.is_file() and p.name != 'SHA256SUMS.txt')
    checksums = ''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(output).as_posix()}\n' for p in files)
    (output / 'SHA256SUMS.txt').write_text(checksums, encoding='utf-8', newline='\n')
    with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED) as bundle:
        for path in sorted(output.rglob('*')):
            bundle.write(path, (Path(output.name) / path.relative_to(output)).as_posix())
    print(f'Runtime folder: {output}')
    print(f'Candidate ZIP: {archive} ({archive.stat().st_size} bytes)')
    print(f'SHA256: {hashlib.sha256(archive.read_bytes()).hexdigest()}')


if __name__ == '__main__':
    main()
