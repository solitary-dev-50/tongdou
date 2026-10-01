# TongDou Server runtime package

This package runs the three published TongDou images. It contains no application source or Docker build files. The publisher must upload all three matching version tags before distributing this package.

## First start

- Linux: follow [LINUX_FIRST_START_EN.md](LINUX_FIRST_START_EN.md).
- Windows: install Docker Desktop, then run `START_TONGDOU_SERVER.bat`. Docker images are downloaded; no local image build is performed. The Windows launcher detects a LAN address automatically.
- Keep `.env` private. The launcher generates database and voiceprint credentials on a fresh install. Preserve `.env` together with your data for upgrades.

The dashboard normally opens at `http://<server-LAN-IP>:8102`. Configure your own speech recognition, language model and speech synthesis providers there. The TongDou device and server must be able to reach each other on the same network.

## Update and data

Back up `.env`, `data/`, `uploadfile/`, and the named Docker volumes. Stop the old containers, extract the new runtime package to a new folder, copy those private files and directories, and retain the same `COMPOSE_PROJECT_NAME`. Set `TONGDOU_VERSION` and `TONGDOU_IMAGE_NAMESPACE` to the published image set, then run the new launcher. It pulls new images and recreates containers while keeping named volumes. Never use `down -v` for an upgrade.

The compatibility default project name is `tongdou-v010`; it is a volume namespace, not a software version. Named volumes include MySQL, Redis, voiceprint data and the voiceprint model cache. The model may download on first use and is reused through the cache volume.

If a credential-bearing volume exists but its matching `.env` credential is missing, restore the old `.env` from backup. The launchers refuse to invent a replacement password for that volume.

The Linux launcher keeps an existing `data/.config.yaml`; check its LAN resource address if the server IP changes. The Windows launcher follows the current Windows configuration workflow. The package includes `STOP_TONGDOU_SERVER` and `STATUS_TONGDOU_SERVER` scripts for both Windows (`.bat`) and Linux (`.sh`). They keep named volumes.

See [RELEASE_NOTES_EN.md](RELEASE_NOTES_EN.md) for the scope and validation limits.
