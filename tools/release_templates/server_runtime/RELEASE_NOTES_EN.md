# Prebuilt-image delivery change

Candidate version: `v0.1.3-rc.1`. The release uses `ghcr.io/solitary-dev-50/server`, `ghcr.io/solitary-dev-50/manager`, and `ghcr.io/solitary-dev-50/voiceprint`, all at the same candidate tag.

The runtime package now uses three published, matching version tags: `server`, `manager` and `voiceprint`. The Compose file has no `build` entry. The user launcher pulls images before starting and keeps the current staged database and voice configuration initialization.

The package excludes frontend, Java and Python application source as well as Docker build inputs. The small voiceprint initialization SQL remains for first installation. The voiceprint startup program and language packs are inside their respective final images; model downloads still use the persistent cache volume.

Named volumes, service ports, environment, dependency order and existing health checks remain. The Linux launcher retains its read-only check that stops credential regeneration when a data-bearing volume is present. The Windows launcher now refuses automatic credential generation when the matching named volume already exists.

Real image builds and an isolated Docker Desktop startup on Windows passed. MySQL, Redis, Manager, Server, and Voiceprint were checked through actual queries, HTTP endpoints, or a WebSocket handshake. Voiceprint first-time model download, warm-up, and health checks passed. The required local speech-detection and speech-recognition models are included in the Server image.

This remains a release candidate, not a stable release. Fresh Ubuntu installation from the remote registry, upgrades with existing volumes, low-memory environments, and complete voice conversations with a physical TongDou still require external testing. Windows batch-launcher end-to-end testing and model-cache reuse after a dedicated restart were not included in the local acceptance. Configure your own service-provider credentials; no shared keys are included.
