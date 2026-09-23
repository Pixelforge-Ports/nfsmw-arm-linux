#!/bin/bash

XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
export PORT_32BIT=Y

if [ -d "/opt/system/Tools/PortMaster/" ]; then
  controlfolder="/opt/system/Tools/PortMaster"
elif [ -d "/opt/tools/PortMaster/" ]; then
  controlfolder="/opt/tools/PortMaster"
elif [ -d "$XDG_DATA_HOME/PortMaster/" ]; then
  controlfolder="$XDG_DATA_HOME/PortMaster"
else
  controlfolder="/roms/ports/PortMaster"
fi

source "$controlfolder/control.txt"
get_controls

GAMEDIR="/$directory/ports/nfsmw"
BINARY="nfsmw_runtime"

mkdir -p "$GAMEDIR/logs" "$GAMEDIR/files" "$GAMEDIR/cache" \
  "$GAMEDIR/Android/data/com.ea.games.nfs13_row" || exit 1
cd "$GAMEDIR" || exit 1
exec > >(tee "$GAMEDIR/logs/nfsmw.log") 2>&1

export NFSMW_RESOLUTION="${NFSMW_RESOLUTION:-auto}"
if [ -f "$GAMEDIR/resolution.txt" ]; then
  export NFSMW_RESOLUTION="$(tr -d '\r\n' < "$GAMEDIR/resolution.txt")"
fi

export EAPX_TTY=/dev/tty0
if [ ! -f "$GAMEDIR/.eapx-nfsmw-data.json" ] && command -v pm_message >/dev/null 2>&1; then
  pm_message "Need for Speed Most Wanted: preparing game data. Keep the device powered on."
  [ -p /dev/shm/portmaster/pm_input ] && export EAPX_TTY=none
fi

chmod +x "$GAMEDIR/setup.sh" "$GAMEDIR/$BINARY" 2>/dev/null || true
if ! "$GAMEDIR/setup.sh" "$GAMEDIR"; then
  echo "NFS Most Wanted setup failed; see gamedata/README.txt"
  sleep 8
  pm_finish
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
  pm_finish
  exit 1
fi

export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
export SDL_NO_SIGNAL_HANDLERS=1
export NFSMW_RUN_CONSTRUCTORS=1 NFSMW_RUN_JNI=1 NFSMW_RUN_GAME=1
export NFSMW_TEST_FRAMES=0
export NFSMW_PERFORMANCE_SCORE=${NFSMW_PERFORMANCE_SCORE:-20}
export NFSMW_AUDIO_OUTPUT=${NFSMW_AUDIO_OUTPUT:-1}
export NFSMW_OBB_PATH="$GAMEDIR/gamefiles/main.1003128.com.ea.games.nfs13_row.obb"

pm_platform_helper "$GAMEDIR/$BINARY"
$GPTOKEYB2 "$BINARY" -c "$GAMEDIR/nfsmw.ini" &
mapper_pid=$!
"$GAMEDIR/$BINARY" "$GAMEDIR/gamefiles/android-libs"
result=$?
kill "$mapper_pid" 2>/dev/null || true
nfsmw_runtime_cleanup
echo "exit_code=$result"
sync
pm_finish
exit "$result"
