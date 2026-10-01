#!/bin/sh
# Bootstrap the extracted server package on Linux; no executable ZIP bit required.
set -eu
cd -- "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec python3 scripts/start_tongdou_server.py "$@"
