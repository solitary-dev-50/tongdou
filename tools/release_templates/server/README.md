# TongDou Server v0.1.2 Early Tester

Companion server for TongDou application firmware v0.1.2. This package follows the v0.1.0 source-build layout: Windows launchers, scripts, Docker build files, `main/`, `docs/`, empty persistent-data locations and usage guides. Docker Desktop builds the current voice server, manager and local voiceprint service from this package; MySQL and Redis are separate containers.

## First start

Debian / Linux: follow [LINUX_FIRST_START_EN.md](LINUX_FIRST_START_EN.md) and run
`sh ./START_TONGDOU_SERVER.sh --lan-ip <server-LAN-IPv4>`. Do not use bare
`docker compose up` for first initialization. The Windows steps below remain supported.

1. Install and start Docker Desktop. Keep the complete extracted folder together.
2. Run `START_TONGDOU_SERVER.bat`. First start downloads dependencies and builds images; the voiceprint model is downloaded into a persistent cache when needed.
3. The launcher creates a package-root `.env` with random database and voiceprint credentials. Keep it private and preserve it for updates.
4. Open the printed dashboard (normally `http://localhost:8102`), register/login, create an agent, and supply your own speech recognition, language model and speech synthesis configuration.
5. Enter the launcher's device Server address on TongDou, normally `http://<server-PC-LAN-IP>:8102`, and bind its activation code.
6. For memory, configure PowerMem and voiceprints as below. Existing agents keep their selected models.

The PC and TongDou must be on the same network. Host ports: 8102 (dashboard/device configuration), 8100 (voice), 8103 (language resources), localhost-only 8105 (voiceprint diagnostics). Full extraction and internet access for first-time dependencies are required. No private provider keys, user databases, recordings, remembered facts or enrolled voiceprints are shipped.

## Memory setup and limits

The backend uses PowerMem 1.1.7 on Python 3.11. In the dashboard select PowerMem, supply memory-generation and embedding API credentials, enable user profiles, choose SQLite, set `database_path` to `data/powermem_dev.db`, and use collection `memories`. The background queue uses the same database. Set the embedding dimensions for your model.

Enable `memory_llm_disable_thinking` only for models whose OpenAI-compatible interface accepts `thinking.type`; use false for others. This controls background memory generation, not normal conversation.

The launcher supplies the voiceprint service and dashboard endpoint. Enrol each speaker through the dashboard. Ordinary memory uses trusted speaker ID plus device ID; profiles are per speaker. Unknown speakers do not read or write long-term memory. Conversation history and long-term memory are different.

Memory saves run after a conversation. Model failures can delay them; spoken acknowledgement does not prove saving has finished. Backend persistence and identity logic have automated checks. Cross-session recall, corrections and multiple speakers remain follow-up tests; this is not advertised as fully verified multi-user behaviour.

## Updating an existing v0.1.0 installation

Stop the old package and back up `.env`, the entire `data/`, `uploadfile/` and database volumes. Extract v0.1.2 to a new folder, then copy your own `.env`, `data/` and `uploadfile/` into it. Keep `COMPOSE_PROJECT_NAME` unchanged so MySQL/Redis volumes are reused. The compatibility default remains `tongdou-v010`; this is a data-volume name, not the software version. New images are tagged v0.1.2.

The launcher preserves credentials and adds a voiceprint token if missing. It initializes the voiceprint schema without clearing data. Custom voiceprint endpoints are preserved. Never run old and new packages against the same ports or data volumes simultaneously.

## Persistent data and stopping

- `data/`: voice configuration, memory database, queue and `.replay.sqlite` plans/receipts, advanced tool settings.
- `uploadfile/`: dashboard uploads.
- `<COMPOSE_PROJECT_NAME>-mysql-data`: accounts, agents, settings and voiceprint database.
- `<COMPOSE_PROJECT_NAME>-redis-data`: cache.
- `<COMPOSE_PROJECT_NAME>-voiceprint-data` and `-voiceprint-model-cache`: voiceprint configuration and model cache.

Stop before a consistent backup; keep all memory databases, plans and receipts together. `STOP_TONGDOU_SERVER.bat` stops containers without deleting volumes. `STATUS_TONGDOU_SERVER.bat` shows status. Do not delete persistent volumes when retaining data.

## Firmware and test scope

The firmware ZIP includes a full USB image and an application-only remote-update file. Initial migration from v0.1.0 is via USB. This server package does not automatically publish an update image to existing devices. Updates require compatible layouts and version/size/hash checks. No production-level OTA power-loss stress validation is claimed.

The project owner confirmed physical IP/console queries. Memory follow-up testing remains open. See Features & Usage, Quick Start, `docs/Early_Tester_Guide.md` and `RELEASE_NOTES_CN.md`.

Source provenance is in `SOURCE_MANIFEST.json`; file hashes are in `SHA256SUMS.txt`. First-start credentials are generated locally and must not enter the archive.

The packaged full dashboard has not yet aligned its update response with the recovery image metadata. Use USB for this delivery. The standalone Python update catalog and the full dashboard are different paths; the included OTA file does not imply dashboard OTA is verified.
