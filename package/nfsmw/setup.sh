#!/usr/bin/env bash
set -eu
GAMEDIR=${1:-$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)}
exec python3 "$GAMEDIR/eapx.py" install --recipe "$GAMEDIR/nfsmw.eapx.json" --game-dir "$GAMEDIR" --tty "${EAPX_TTY:-/dev/tty0}"
