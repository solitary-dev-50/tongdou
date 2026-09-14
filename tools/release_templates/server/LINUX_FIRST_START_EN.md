# TongDou Server v0.1.2: Debian / Linux first start

The latest companion server is v0.1.2. Firmware v0.1.3 does not replace it.
Use the complete extracted server ZIP plus the Linux startup fix if you have the
original v0.1.2 ZIP. The original ZIP only contained Windows initialization scripts.

## Prerequisites

- Docker Engine running Linux containers, with the Buildx plugin and Docker Compose v2.20 or later
  (the `docker compose` plugin; not the old `docker-compose` command).
- Python 3.9 or later. On Debian: `sudo apt-get update` then
  `sudo apt-get install python3`. Install Docker using
  <https://docs.docker.com/engine/install/debian/> if needed.
- Your current user must be able to run `docker info` successfully.
- Internet access and sufficient disk space for source builds and the voiceprint
  model. Base images, npm, Maven, Python packages, and ModelScope downloads are
  still required. The voiceprint base is pinned at `ghcr.nju.edu.cn`; the voice
  Dockerfile uses Aliyun package mirrors. A network failure there is a separate
  build/download problem, not a request to log in to `tongdou/*`.
- The server IPv4 address reachable from TongDou, with ports 8100, 8102 and 8103
  reachable on your LAN. Port 8105 is localhost-only. Choose the LAN address, not
  a Docker bridge, loopback, or VPN address.

## Apply the fix to the original v0.1.2 folder

Obtain `TongDou_Server_v0.1.2_Linux_Startup_Fix.zip` from the project owner and save
it to your Downloads folder. This is a deployment-only patch. It contains
`docker-compose.yml`, `START_TONGDOU_SERVER.sh`, `scripts/start_tongdou_server.py`
and this guide; no credentials or persistent data. No full server re-download
is necessary. Extract these files over the original complete v0.1.2 folder.

```sh
cd "$HOME/tongdou/server/TongDou_Server_v0.1.2_Early_Tester"
# Preserve the old deployment description, without changing containers or data.
cp -p docker-compose.yml "docker-compose.yml.before-linux-fix.$(date +%Y%m%d-%H%M%S)"
python3 -m zipfile -e "$HOME/Downloads/TongDou_Server_v0.1.2_Linux_Startup_Fix.zip" .
```

The patch changes four files. It does not overwrite `.env`, `data/`, `uploadfile/`
or volumes. Its ZIP digest is supplied alongside the patch. The original
`SHA256SUMS.txt` describes the original release and will no longer match the
patched Compose file; preserve it as original evidence, not as a patched manifest.

## First start (also safe to retry after an interrupted first build)

```sh
cd "$HOME/tongdou/server/TongDou_Server_v0.1.2_Early_Tester"
docker info >/dev/null
docker compose version
ip -4 address show scope global
printf 'Enter the server LAN IPv4 address shown above: '
read -r TONGDOU_LAN_IP
sh ./START_TONGDOU_SERVER.sh --lan-ip "$TONGDOU_LAN_IP"
```

No `chmod` step is needed when using `sh`. The launcher resolves all paths from
its own location, creates private `.env` credentials with cryptographic random
values, validates Compose's resolved settings without printing them, explicitly
builds the three TongDou images, starts MySQL/Redis/dashboard, waits for the
dashboard database secret, initializes the voiceprint schema, records LAN URLs,
creates the voice configuration, then starts/checks the remaining services.
Windows users keep using `START_TONGDOU_SERVER.bat`.

For configuration checks only, use the same command with `--configure-only`.
It requires a reachable Docker daemon to check for existing volumes. If a
credential-bearing volume already exists, a temporary MySQL-image container
checks that it is completely empty through a read-only mount (the MySQL server
is not started). This may download `mysql:8.0`; no application services start.

Do not use bare `docker compose up` for first initialization. Copying
`.env.example` to `.env` alone is also insufficient: `GENERATED_ON_FIRST_START`
is a marker, not a password/token. Compose does not execute either launcher.

The launcher makes package-root `.env` authoritative: exported settings with
the same names are removed from its child environment. It uses explicit
`--project-directory`, `--env-file`, `-f` and `-p` options. Bare Compose commands
instead allow shell variables to override `.env`. This package does not use
container `env_file`; its `environment` entries receive values after Compose
interpolation. A container `env_file` would not supply `${...}` interpolation.

Keep `.env` private and preserve it with your backups. Use one literal `KEY=value`
per line; simple quoted values and full-line comments are accepted. The Linux
launcher rejects duplicate keys, interpolation, escapes and inline comments.
Configure optional weather/search provider credentials through the dashboard;
the Linux launcher does not import the Windows-only optional shared-tool settings.

## Existing data and retries

- Existing valid credentials are preserved. Empty volumes left by an unsuccessful
  first attempt are checked read-only and can be reused. If credentials are
  missing/markers but the corresponding database/voiceprint data volume is nonempty
  (or cannot be safely inspected), startup stops. Restore the matching `.env` from
  your original installation/backup. Do not rotate a database
  password in `.env` and expect MySQL's existing database password to change.
- Keep `COMPOSE_PROJECT_NAME` unchanged for existing data. The compatibility
  default `tongdou-v010` is a volume namespace, not the server version. Use a
  different name and different ports only for an intentionally separate install.
- Existing `data/.config.yaml` and advanced tool settings are preserved. If you
  previously copied the YAML example manually, its marker secret is invalid:
  back it up, then move only that placeholder configuration out of the way and
  rerun the launcher. Never move/delete a real working configuration blindly.
  For an existing server with a changed IP, update its resource URL in this file.
- The launcher only refreshes package-owned voiceprint endpoints; custom
  endpoints remain unchanged. Voiceprint schema creation is non-destructive.
- The patch is intended for the standard v0.1.2 Compose file. If you customized
  it, merge the three `pull_policy: never` entries and the required credential
  substitutions instead of overwriting your deployment changes.

## Check and stop

```sh
docker compose --env-file .env -f docker-compose.yml ps --all
docker compose --env-file .env -f docker-compose.yml stop
```

The three `tongdou/*:v0.1.2` names are local build output tags. `pull_policy: never`
prevents registry requests for those tags; the launcher explicitly runs `build`
before `up --no-build`. MySQL, Redis and Dockerfile base images still download.
Do not run `down -v` or volume deletion commands when retaining data.

Successful startup means the configured health checks passed, the voice port
opened and the dashboard returned HTTP 200. It does not prove external speech,
language-model or memory providers work. Configure those in the dashboard, then
test a device conversation separately. On failure, keep the final exit status
and `ps --all` result. Review logs locally and redact secrets before sharing;
do not publish `.env` or the full output of `docker compose config`.
