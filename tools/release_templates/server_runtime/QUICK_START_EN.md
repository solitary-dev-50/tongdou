# TongDou Server quick start

1. Install Docker Engine with Compose v2 (Linux) or Docker Desktop (Windows).
2. Extract the small runtime ZIP and keep its files together.
3. Confirm `.env.example` names the published `TONGDOU_VERSION` and `TONGDOU_IMAGE_NAMESPACE`. The launcher creates private credentials in `.env`.
4. Linux: run `sh ./START_TONGDOU_SERVER.sh --lan-ip <server-LAN-IPv4>`. Windows: double-click `START_TONGDOU_SERVER.bat`.
5. Open the printed dashboard address, configure your service providers, then set the same server address on TongDou.

The launcher downloads versioned images and starts containers. It does not build images locally. For upgrades and status commands, read `LINUX_FIRST_START_EN.md` or `README.md`.
