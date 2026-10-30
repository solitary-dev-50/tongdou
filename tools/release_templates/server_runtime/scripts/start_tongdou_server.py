"""Initialize and start the prebuilt-image Linux server package (Python 3 stdlib)."""
import argparse
import base64
import ipaddress
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import subprocess
import sys
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
CREDENTIALS = ('MYSQL_ROOT_PASSWORD', 'VOICEPRINT_API_TOKEN')
LOCAL_SERVICES = ('tongdou-manager', 'tongdou-voice', 'voiceprint-api')


def fail(message):
    raise RuntimeError(message)


def read_env(path):
    """Read literal package settings; never execute a user's .env as shell code."""
    values = {}
    for number, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        match = re.fullmatch(r'([A-Za-z_][A-Za-z0-9_]*)=(.*)', line)
        if not match or match[1] in values:
            fail(f'Unsupported or duplicate setting in {path.name}, line {number}.')
        key, value = match.groups()
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        if any(c in value for c in ('$', '\\', '"', "'", '#')):
            fail(f'{key}: use a literal value without interpolation, escapes or inline comments.')
        values[key] = value
    return values


def private_write(path, content, replace=False):
    path.parent.mkdir(parents=True, exist_ok=True)
    # Create privately even before chmod, and never overwrite an existing credential file.
    flags = os.O_WRONLY | os.O_CREAT | (os.O_TRUNC if replace else os.O_EXCL)
    fd = os.open(path, flags, 0o600)
    with os.fdopen(fd, 'w', encoding='utf-8', newline='\n') as stream:
        if hasattr(os, 'fchmod'):
            os.fchmod(stream.fileno(), 0o600)
        stream.write(content)
    path.chmod(0o600)


def clean_environment(values):
    env = dict(os.environ)
    # .env is authoritative for this launcher, unlike a bare Compose invocation.
    for key in set(values) | set(CREDENTIALS) | {
        'COMPOSE_FILE', 'COMPOSE_PROJECT_NAME', 'COMPOSE_PROFILES', 'COMPOSE_ENV_FILES',
        'COMPOSE_DISABLE_ENV_FILE', 'TZ', 'MYSQL_DATABASE',
        'TONGDOU_DASHBOARD_PORT', 'TONGDOU_WS_PORT', 'TONGDOU_HTTP_PORT',
        'TONGDOU_VOICEPRINT_PORT',
        'TONGDOU_VERSION',
        'TONGDOU_IMAGE_NAMESPACE',
    }:
        env.pop(key, None)
    return env


def run(args, env, quiet=False, input_text=None, check=True):
    result = subprocess.run(args, cwd=ROOT, env=env, input=input_text,
                            text=True, encoding='utf-8', errors='replace',
                            stdout=subprocess.PIPE if quiet else None,
                            stderr=subprocess.PIPE if quiet else None)
    if check and result.returncode:
        # Captured SQL/config output can contain secrets. Do not echo it or the command.
        fail('Docker operation failed; no success was recorded. Check service status locally. '
             'Do not share unredacted config or logs.')
    return result


def initialize_env():
    path = ROOT / '.env'
    source = path if path.exists() else ROOT / '.env.example'
    values = read_env(source)
    missing = [key for key in CREDENTIALS
               if values.get(key, '') in ('', 'GENERATED_ON_FIRST_START')]
    env = clean_environment(values)
    run(['docker', 'info'], env, quiet=True)
    project = values.get('COMPOSE_PROJECT_NAME') or 'tongdou-v010'
    if not re.fullmatch(r'[a-z0-9][a-z0-9_-]*', project):
        fail('COMPOSE_PROJECT_NAME must be a lowercase Compose project name.')
    if missing:
        # A failed bare Compose start can leave empty volumes. Inspect those read-only;
        # never replace credentials for an initialized database or voiceprint config.
        names = run(['docker', 'volume', 'ls', '--format', '{{.Name}}'], env, quiet=True).stdout.splitlines()
        protected = []
        if 'MYSQL_ROOT_PASSWORD' in missing:
            protected.append(project + '-mysql-data')
        if 'VOICEPRINT_API_TOKEN' in missing:
            protected.append(project + '-voiceprint-data')
        for name in protected:
            if name not in names:
                continue
            result = run(['docker', 'run', '--rm', '--pull=missing', '--network', 'none',
                          '--entrypoint', 'sh', '--mount',
                          'type=volume,src=' + name + ',dst=/check,readonly', 'mysql:8.0',
                          '-c', 'entries=$(ls -A /check) || exit 1; test -z "$entries"'],
                         env, quiet=True, check=False)
            if result.returncode:
                fail('A credential-bearing volume is nonempty or could not be checked safely. '
                     'Restore the matching .env from the previous installation or backup; '
                     'do not delete volumes or generate a new database password.')
        text = source.read_text(encoding='utf-8-sig')
        for key in missing:
            value = secrets.token_urlsafe(32)
            values[key] = value
            if re.search(r'^[ \t]*' + key + '=', text, flags=re.M):
                text = re.sub(r'^[ \t]*' + key + r'=.*$', key + '=' + value, text, flags=re.M)
            else:
                text += '\n' + key + '=' + value + '\n'
        private_write(path, text, replace=path.exists())
    elif not path.exists():
        fail('.env.example must contain first-start credential markers, not real credentials.')
    if not re.fullmatch(r'[A-Za-z0-9_-]{32,}', values['VOICEPRINT_API_TOKEN']):
        fail('VOICEPRINT_API_TOKEN must have at least 32 letters, digits, underscores or hyphens.')
    path.chmod(0o600)
    return values, project, clean_environment(values)


def sql_value(value):
    encoded = base64.b64encode(value.encode()).decode()
    return "CONVERT(FROM_BASE64('" + encoded + "') USING utf8mb4)"


def warn_resources():
    """Advisory only: image sizes vary, so low resources must not block startup."""
    free_gb = shutil.disk_usage(ROOT).free / (1024 ** 3)
    if free_gb < 8:
        print(f'WARNING: only {free_gb:.1f} GiB free. Allow roughly 8 GiB for image pulls and data; monitor disk use.', flush=True)
    try:
        info = (Path('/proc/meminfo').read_text().splitlines())
        total_kib = int(next(line.split()[1] for line in info if line.startswith('MemTotal:')))
        if total_kib < 4 * 1024 * 1024:
            print(f'WARNING: {total_kib / 1024 / 1024:.1f} GiB RAM. The five services may need more; 4 GiB is a provisional recommendation.', flush=True)
    except (OSError, StopIteration, ValueError):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lan-ip', required=True, help='Server IPv4 address reachable by TongDou')
    parser.add_argument('--configure-only', action='store_true', help='Generate/validate .env without pulling or starting services')
    args = parser.parse_args()
    address = ipaddress.IPv4Address(args.lan_ip)
    if address.is_loopback or address.is_unspecified or address.is_multicast or address.is_link_local:
        fail('Use the server LAN IPv4 address reachable by TongDou.')
    values, project, env = initialize_env()
    version = values.get('TONGDOU_VERSION', '')
    if not re.fullmatch(r'v?[0-9]+\.[0-9]+\.[0-9]+(?:[-.][A-Za-z0-9.-]+)?', version):
        fail('Set TONGDOU_VERSION in .env to a fixed release version, such as v0.1.3.')
    namespace = values.get('TONGDOU_IMAGE_NAMESPACE', 'tongdou')
    if not re.fullmatch(r'[a-z0-9][a-z0-9._/-]*', namespace):
        fail('TONGDOU_IMAGE_NAMESPACE is not a valid image repository prefix.')
    compose = ['docker', 'compose', '--project-directory', str(ROOT), '--env-file', str(ROOT / '.env'),
               '-f', str(ROOT / 'docker-compose.yml'), '-p', project]

    def dc(*args, **kwargs):
        return run(compose + list(args), env, **kwargs)

    config = json.loads(dc('config', '--format', 'json', quiet=True).stdout)
    services = config['services']
    password = values['MYSQL_ROOT_PASSWORD']
    token = values['VOICEPRINT_API_TOKEN']
    if (services['tongdou-db']['environment']['MYSQL_ROOT_PASSWORD'] != password or
            services['tongdou-manager']['environment']['SPRING_DATASOURCE_DRUID_PASSWORD'] != password or
            services['voiceprint-api']['environment']['VOICEPRINT_MYSQL_PASSWORD'] != password or
            services['voiceprint-api']['environment']['VOICEPRINT_API_TOKEN'] != token or
            not services['tongdou-manager']['environment']['VOICEPRINT_API_ENDPOINT'].endswith('key=' + token)):
        fail('Compose credential loading does not match .env.')
    for name, image_name in zip(LOCAL_SERVICES, ('manager', 'server', 'voiceprint')):
        if services[name].get('build') or services[name].get('image') != f'{namespace}/{image_name}:{version}':
            fail('Compose must use the three prebuilt images at TONGDOU_VERSION, without build settings.')
    print('Private .env validated; credentials were not printed.', flush=True)
    if args.configure_only:
        return
    warn_resources()
    ports = {}
    for key, default in [('TONGDOU_DASHBOARD_PORT', 8102), ('TONGDOU_WS_PORT', 8100), ('TONGDOU_HTTP_PORT', 8103)]:
        port = int(values.get(key) or default)
        if not 1 <= port <= 65535:
            fail(f'{key} must be between 1 and 65535.')
        ports[key] = port
    db_name = values.get('MYSQL_DATABASE') or 'xiaozhi_esp32_server'
    if not re.fullmatch(r'[A-Za-z0-9_]+', db_name):
        fail('MYSQL_DATABASE must contain only letters, digits or underscores.')
    print('Pulling published images; this machine will not build images...', flush=True)
    dc('pull')
    dc('up', '-d', '--no-build', '--wait', '--wait-timeout', '900', 'tongdou-db', 'tongdou-redis', 'tongdou-manager')

    def mysql(sql, database=True, check=True):
        # Expand the existing container variable inside the container, not in host argv.
        command = ['exec', '-T', 'tongdou-db', 'sh', '-c',
                   'export MYSQL_PWD="$MYSQL_ROOT_PASSWORD"; exec mysql -uroot -N -B "$@"', 'sh']
        if database:
            command.append(db_name)
        return dc(*command, quiet=True, input_text=sql, check=check)

    secret = ''
    for _ in range(300):
        result = mysql("SELECT param_value FROM sys_params WHERE param_code='server.secret' LIMIT 1;", check=False)
        if result.returncode == 0 and result.stdout.strip() not in ('', 'null', 'NULL'):
            secret = result.stdout.strip()
            break
        time.sleep(3)
    if not secret:
        fail('Dashboard database initialization did not finish within 15 minutes.')
    mysql((ROOT / 'main/xiaozhi-server/mysql-init/01-voiceprint.sql').read_text(), database=False)
    websocket = f"ws://{address}:{ports['TONGDOU_WS_PORT']}/xiaozhi/v1/"
    device_url = f"http://{address}:{ports['TONGDOU_DASHBOARD_PORT']}"
    resource = f"http://{address}:{ports['TONGDOU_HTTP_PORT']}/tongdou/resources"
    endpoint = 'http://voiceprint-api:8005/voiceprint/health?key=' + token
    mysql("UPDATE sys_params SET param_value=" + sql_value(endpoint) + " WHERE param_code='server.voice_print' AND "
          "(param_value IS NULL OR param_value='' OR param_value='null' OR param_value LIKE 'http://voiceprint-api:8005/%');\n"
          "UPDATE sys_params SET param_value=" + sql_value(websocket) + " WHERE param_code='server.websocket';\n"
          "UPDATE sys_params SET param_value=" + sql_value(device_url + '/xiaozhi/ota/') + " WHERE param_code='server.ota';")
    dc('exec', '-T', 'tongdou-redis', 'redis-cli', 'HDEL', 'sys:params',
       'server.voice_print', 'server.websocket', 'server.ota', quiet=True)
    config_path = ROOT / 'data/.config.yaml'
    if not config_path.exists():
        # JSON strings are valid YAML scalars; quote secrets without a YAML dependency.
        private_write(config_path, 'server:\n  ip: 0.0.0.0\n  port: 8000\n  http_port: 8003\n\n'
                      'manager-api:\n  url: http://tongdou-manager:8003/xiaozhi/\n  secret: ' + json.dumps(secret) +
                      '\n\ntongdou_locale_resources:\n  base_url: ' + resource +
                      '\n  directory: config/assets/tongdou_locale_packs\n')
    else:
        print('Preserved data/.config.yaml. Keep its manager secret and resource LAN address consistent with this installation.')
    mcp_path = ROOT / 'data/.mcp_server_settings.json'
    if not mcp_path.exists():
        private_write(mcp_path, '{"mcpServers": {}}\n')
    dc('up', '-d', '--no-build', '--wait', '--wait-timeout', '900')
    # Voice has no healthcheck: --wait alone only proves that its process is running.
    # Probe its actual WebSocket listener from inside its container as well.
    probe = "import socket; s=socket.create_connection(('127.0.0.1',8000),5); s.close()"
    for _ in range(60):
        if dc('exec', '-T', 'tongdou-voice', 'python', '-c', probe, quiet=True, check=False).returncode == 0:
            break
        time.sleep(3)
    else:
        fail('Voice process did not open its port within 3 minutes.')
    with urllib.request.urlopen(f"http://127.0.0.1:{ports['TONGDOU_DASHBOARD_PORT']}/", timeout=10) as response:
        if response.status != 200:
            fail('Dashboard HTTP check failed.')
    dc('ps', '--all')
    print('Services passed startup checks. Configure model providers in the dashboard before testing conversations.')
    print('Dashboard / TongDou Server address: ' + device_url)


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, OSError, ValueError, KeyError) as error:
        # ValueError/KeyError may contain user configuration: avoid leaking it.
        message = str(error) if isinstance(error, RuntimeError) else 'Invalid configuration or unavailable dependency. Check prerequisites and local files.'
        print('ERROR: ' + message, file=sys.stderr)
        sys.exit(1)
