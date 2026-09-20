#!/usr/bin/env bash
set -euo pipefail
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
cd "$ROOT"
python3 tools/check_glibc.py runtime/build/nfsmw_mapper
python3 tools/sync_package.py
python3 tools/package_port.py
