"""Install Linux startup files into an extracted source-build server package.

This edits deployment files only. It never runs Docker or touches runtime data.
"""
from pathlib import Path
import argparse
import re
import shutil

TEMPLATES = Path(__file__).resolve().parent / 'release_templates/server'


def install(package):
    compose_path = package / 'docker-compose.yml'
    text = compose_path.read_text(encoding='utf-8')
    # These are local build output tags, not published download locations.
    pattern = r'(?m)^(    image: tongdou/(?:server|manager|voiceprint):[^\n]+)\n(?:    pull_policy: never\n)?'
    text, count = re.subn(pattern, r'\1\n    pull_policy: never\n', text)
    if count != 3:
        raise ValueError('Expected the three TongDou local image tags; package was not changed.')
    for key in ('MYSQL_ROOT_PASSWORD', 'VOICEPRINT_API_TOKEN'):
        text = text.replace('${' + key + '}', '${' + key + ':?Run the platform startup script to initialize .env}')
    compose_path.write_text(text, encoding='utf-8', newline='\n')
    for name in ('START_TONGDOU_SERVER.sh', 'scripts/start_tongdou_server.py', 'LINUX_FIRST_START_EN.md'):
        destination = package / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(TEMPLATES / name, destination)
    (package / 'START_TONGDOU_SERVER.sh').chmod(0o755)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    install(parser.parse_args().package.resolve())
