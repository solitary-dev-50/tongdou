"""Run status or stop against this package's Compose project."""
import subprocess
import sys

from start_tongdou_server import ROOT, clean_environment, read_env


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in ('stop', 'status'):
        raise RuntimeError('Use stop or status.')
    path = ROOT / '.env'
    if not path.is_file():
        raise RuntimeError('No .env in this package. Start or restore the installation first.')
    values = read_env(path)
    project = values.get('COMPOSE_PROJECT_NAME') or 'tongdou-v010'
    command = ['docker', 'compose', '--project-directory', str(ROOT), '--env-file', str(path),
               '-f', str(ROOT / 'docker-compose.yml'), '-p', project]
    command += ['stop'] if sys.argv[1] == 'stop' else ['ps', '--all']
    return subprocess.run(command, cwd=ROOT, env=clean_environment(values), check=False).returncode


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (RuntimeError, OSError) as error:
        print('ERROR: ' + str(error), file=sys.stderr)
        sys.exit(1)
