#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

GAMEDIR="$TMP/game"
XDG_RUNTIME_DIR="$TMP/runtime"
mkdir -p "$GAMEDIR" "$XDG_RUNTIME_DIR" "$TMP/mali" "$TMP/mesa"

touch "$TMP/mali/libEGL.so" "$TMP/mali/libGLESv2.so"
touch "$TMP/mesa/libEGL.so.1" "$TMP/mesa/libGLESv2.so.2"
touch "$TMP/mesa/libEGL_mesa.so.0"

python3 - "$XDG_RUNTIME_DIR/wayland-0" <<'PY'
import socket
import sys

sock = socket.socket(socket.AF_UNIX)
sock.bind(sys.argv[1])
sock.close()
PY

cat > "$GAMEDIR/nfsmw_runtime" <<'SH'
#!/usr/bin/env bash
if [[ "$1" == --probe-gl ]]; then
    printf '%s %s\n' "$2" "$3" >> "$GAMEDIR/probes.log"
    if [[ "${NFSMW_TEST_FAIL_MESA:-0}" == 1 && "$2" == *mesa* ]]; then
        exit 1
    fi
    [[ "$2" == "$NFSMW_TEST_MESA_EGL" && "$3" == "$NFSMW_TEST_MESA_GLES" ]]
    exit $?
fi
exit 2
SH
chmod +x "$GAMEDIR/nfsmw_runtime"

NFSMW_LIBRARY_DIRS="$TMP/mali:$TMP/mesa"
NFSMW_TEST_MESA_EGL="$TMP/mesa/libEGL.so.1"
NFSMW_TEST_MESA_GLES="$TMP/mesa/libGLESv2.so.2"
SDL_VIDEODRIVER=wayland
export GAMEDIR XDG_RUNTIME_DIR NFSMW_LIBRARY_DIRS NFSMW_TEST_MESA_EGL
export NFSMW_TEST_MESA_GLES SDL_VIDEODRIVER
source "$ROOT/portmaster/nfsmw/runtime-env.sh"

if NFSMW_TEST_FAIL_MESA=1 nfsmw_runtime_environment > "$TMP/failure.log" 2>&1; then
    echo "Expected failed Mesa preflight to stop rather than select Mali." >&2
    exit 1
fi
grep -q 'refusing the Mali fallback' "$TMP/failure.log"
if grep -q "$TMP/mali/" "$GAMEDIR/probes.log"; then
    echo "Mali was probed after the Wayland Mesa candidate failed." >&2
    exit 1
fi

: > "$GAMEDIR/probes.log"
nfsmw_runtime_environment > "$TMP/success.log" 2>&1
grep -q 'mode=wayland-mesa' "$TMP/success.log"
[[ "$SDL_VIDEO_EGL_DRIVER" == "$NFSMW_TEST_MESA_EGL" ]]
[[ "$SDL_VIDEO_GL_DRIVER" == "$NFSMW_TEST_MESA_GLES" ]]
if grep -q "$TMP/mali/" "$GAMEDIR/probes.log"; then
    echo "Mali was probed before the Wayland Mesa candidate." >&2
    exit 1
fi
nfsmw_runtime_cleanup
echo "ROCKNIX Wayland Mesa selection checks passed."
