#!/bin/sh
set -eu
cd -- "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec python3 scripts/control_tongdou_server.py stop
