#!/bin/sh
set -eu
# Run in the existing cross-compiler container; all output is temporary.
# /src is this repository, /owned contains the supported APK and OBB.
mkdir -p /tmp/fmod-donor
python3 - <<'PY'
from pathlib import Path
import zipfile

root = Path('/tmp/fmod-donor')
with zipfile.ZipFile(next(Path('/owned').glob('*.apk'))) as archive:
    for name in ('libc++_shared.so', 'libfmodex.so'):
        (root / name).write_bytes(archive.read('lib/armeabi-v7a/' + name))
with zipfile.ZipFile(next(Path('/owned').glob('*.obb'))) as archive:
    for name in ('published/sounds/music/loading_01.mp3', 'published/sounds/ui/ui.fsb'):
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(archive.read(name))

# Preserve an explicit baseline without changing production source.
source = Path('/src/runtime/src/compat_bridge.c').read_text()
start = source.index('        size_t stack_size = guest->stack_size;')
end = source.index('        result = pthread_attr_setstacksize(host, stack_size);', start)
end += len('        result = pthread_attr_setstacksize(host, stack_size);')
source = source[:start] + '        result = pthread_attr_setstacksize(host, guest->stack_size);' + source[end:]
(root / 'compat_before.c').write_text(source)
PY
sources=$(find /src/runtime/src -name '*.c' ! -name main.c ! -name compat_bridge.c)
for variant in before after; do
    compat=/src/runtime/src/compat_bridge.c
    expected=0
    if [ "$variant" = before ]; then
        compat=/tmp/fmod-donor/compat_before.c
        expected=33
    fi
    arm-linux-gnueabihf-gcc -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -std=c17 -O0 -g \
        -ffunction-sections -fdata-sections -march=armv7-a -mfpu=neon \
        -mfloat-abi=hard -I/src/runtime/src -o "/tmp/fmod-donor/probe-$variant" \
        /src/tests/fmod_donor_probe.c $sources "$compat" \
        /src/runtime/src/bionic_setjmp.S -Wl,--gc-sections -ldl -lm -pthread
    printf '\nPROBE VARIANT: %s\n' "$variant"
    QEMU_LD_PREFIX=/usr/arm-linux-gnueabihf "/tmp/fmod-donor/probe-$variant" \
        /tmp/fmod-donor /tmp/fmod-donor "$expected"
done
