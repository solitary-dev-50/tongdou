# Linux first start: prebuilt images

## Requirements

Install Docker Engine and the Docker Compose v2 plugin, plus Python 3.9 or later. Ensure `docker info` succeeds for your user. Buildx and compilers are not needed on this machine. Obtain the runtime ZIP only after its publisher has uploaded the three matching TongDou image tags. MySQL and Redis are pulled from their official image repositories.

Allow roughly 8 GiB of free disk space for downloaded images and data. Four GiB of RAM is a provisional recommendation for running MySQL, Redis, the Java manager, voice server and PyTorch voiceprint service together. The launcher warns below these values but does not block startup. Actual limits need measurement from published images and a real Ubuntu run; a 1.6 GiB machine with no swap may be tight.

## First install

Extract the ZIP into a new directory. Do not extract over an existing installation. From the extracted package root:

```sh
docker info >/dev/null
docker compose version
ip -4 address show scope global
sh ./START_TONGDOU_SERVER.sh --lan-ip 192.168.1.20
```

Replace `192.168.1.20` with this computer's LAN IPv4 address reachable by TongDou. The launcher creates a private `.env` with random credentials if needed, checks the resolved Compose configuration, pulls all images, starts database/cache/manager, performs the existing initialization, starts the remaining services, and checks their status. It performs no Docker image build.

The example `.env.example` contains `GENERATED_ON_FIRST_START` markers, not working passwords. Use the launcher instead of a bare first `docker compose up`. Keep the generated `.env` private and back it up.

## Upgrade

Back up `.env`, `data/`, `uploadfile/` and all named Docker volumes. Stop the old installation without `-v`. Extract the new package to a fresh folder and copy the private files into it. Preserve `COMPOSE_PROJECT_NAME`, then set `TONGDOU_VERSION` and `TONGDOU_IMAGE_NAMESPACE` in `.env` to the published release. Run the same start command. It pulls the new images and recreates containers while reusing the existing volumes. Never run old and new folders against the same volumes at once.

If `.env` or a credential is missing while the matching volume contains data, the launcher stops. Restore the matching old credentials; changing the file does not change the MySQL password stored in the database.

## Status and stop

Run these from the package directory after `.env` exists:

```sh
sh ./STATUS_TONGDOU_SERVER.sh
sh ./STOP_TONGDOU_SERVER.sh
```

For a credential/configuration check without pulling application images or starting services, use `sh ./START_TONGDOU_SERVER.sh --lan-ip 192.168.1.20 --configure-only` with your real LAN address. Empty-volume checks may temporarily pull `mysql:8.0`; they use a read-only mount and do not start MySQL.

The voiceprint model can still download on first use into the `voiceprint-model-cache` volume. Review local container logs if health checks fail, and remove credentials before sharing logs. Do not delete volumes to retry.
