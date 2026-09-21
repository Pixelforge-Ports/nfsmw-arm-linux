#!/usr/bin/env bash
# PORTMASTER: nfsmw.zip, Need for Speed Most Wanted.sh

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
XDG_DATA_HOME=${XDG_DATA_HOME:-${HOME:-/tmp}/.local/share}
export PORT_32BIT=Y
controlfolder=
for candidate in /mnt/mmc/MUOS/PortMaster /opt/system/Tools/PortMaster /opt/tools/PortMaster \
    "$XDG_DATA_HOME/PortMaster" "$SCRIPT_DIR/PortMaster" /roms/ports/PortMaster; do
    if [ -f "$candidate/control.txt" ]; then
        controlfolder=$candidate
        # shellcheck disable=SC1090
        source "$controlfolder/control.txt"
        if [ -n "${CFW_NAME:-}" ] && [ -f "$controlfolder/mod_${CFW_NAME}.txt" ]; then
            # shellcheck disable=SC1090
            source "$controlfolder/mod_${CFW_NAME}.txt"
        fi
        declare -F get_controls >/dev/null 2>&1 && get_controls
        break
    fi
done

GAMEDIR="$SCRIPT_DIR/nfsmw"
if [ ! -d "$GAMEDIR" ]; then
    for candidate in "${directory:+/${directory#/}/ports/nfsmw}" \
        /roms/ports/nfsmw /sdcard/ports/nfsmw /mnt/mmc/ports/nfsmw; do
        [ -n "$candidate" ] && [ -d "$candidate" ] && { GAMEDIR=$candidate; break; }
    done
fi
cd "$GAMEDIR" || exit 1
mkdir -p logs files cache Android/data/com.ea.games.nfs13_row || exit 1
LOG="$GAMEDIR/logs/nfsmw.log"
[ -f "$LOG" ] && mv -f -- "$LOG" "$LOG.1" 2>/dev/null || true
exec >>"$LOG" 2>&1

export NFSMW_RESOLUTION="${NFSMW_RESOLUTION:-auto}"
if [ -f "$GAMEDIR/resolution.txt" ]; then
    export NFSMW_RESOLUTION="$(tr -d '\r\n' < "$GAMEDIR/resolution.txt")"
fi
export EAPX_TTY=/dev/tty0
if [ ! -f "$GAMEDIR/.eapx-nfsmw-data.json" ] && command -v pm_message >/dev/null 2>&1; then
    pm_message "Need for Speed Most Wanted: preparing game data. Keep the device powered on."
    [ -p /dev/shm/portmaster/pm_input ] && export EAPX_TTY=none
fi
SETUP="$GAMEDIR/setup.sh"
chmod +x "$SETUP" "$GAMEDIR/nfsmw_runtime" 2>/dev/null || true
if ! "$SETUP" "$GAMEDIR"; then
    echo "NFS Most Wanted setup failed; see gamedata/README.txt"
    sleep 8
    command -v pm_finish >/dev/null 2>&1 && pm_finish
    exit 1
fi

if python3 "$GAMEDIR/prepare_audio.py" "$GAMEDIR"; then
    export NFSMW_AUDIO_ROOT="$GAMEDIR/gamefiles"
    export NFSMW_SILENT_AUDIO=${NFSMW_SILENT_AUDIO:-0}
else
    echo "NFS audio preparation failed; music is disabled for this launch"
    export NFSMW_SILENT_AUDIO=1
fi

source "$GAMEDIR/runtime-env.sh"
if ! nfsmw_runtime_environment; then
    command -v pm_finish >/dev/null 2>&1 && pm_finish
    exit 1
fi

export SDL_GAMECONTROLLERCONFIG=${sdl_controllerconfig:-${SDL_GAMECONTROLLERCONFIG:-}}
export SDL_NO_SIGNAL_HANDLERS=1
export NFSMW_RUN_CONSTRUCTORS=1 NFSMW_RUN_JNI=1 NFSMW_RUN_GAME=1
export NFSMW_TEST_FRAMES=0
export NFSMW_PERFORMANCE_SCORE=${NFSMW_PERFORMANCE_SCORE:-20}
export NFSMW_AUDIO_OUTPUT=${NFSMW_AUDIO_OUTPUT:-1}
export NFSMW_OBB_PATH="$GAMEDIR/gamefiles/main.1003128.com.ea.games.nfs13_row.obb"

command -v pm_platform_helper >/dev/null 2>&1 && \
    pm_platform_helper "$GAMEDIR/nfsmw_runtime"
mapper_pid=
if [ -n "${GPTOKEYB2:-}" ]; then
    $GPTOKEYB2 "nfsmw_runtime" -c "$GAMEDIR/nfsmw.ini" &
    mapper_pid=$!
fi
"$GAMEDIR/nfsmw_runtime" "$GAMEDIR/gamefiles/android-libs"
result=$?
[ -z "$mapper_pid" ] || kill "$mapper_pid" 2>/dev/null || true
nfsmw_runtime_cleanup
echo "exit_code=$result"
sync
command -v pm_finish >/dev/null 2>&1 && pm_finish
exit "$result"
