#!/usr/bin/env bash

nfsmw_runtime_environment() {
    local library_dirs directory egl gles blob chosen_egl= chosen_gles= graphics_mode=automatic
    local runtime_dir socket wayland_socket= mesa_pass mesa_candidate=0
    library_dirs=${NFSMW_LIBRARY_DIRS:-/usr/local/lib/arm-linux-gnueabihf:/usr/lib/arm-linux-gnueabihf:/usr/lib/arm-linux-gnueabihf/mali:/lib/arm-linux-gnueabihf:/usr/lib32/mali:/usr/lib32:/lib32}
    if [ "${DEVICE_ARCH:-}" = armhf ]; then library_dirs="$library_dirs:/usr/lib:/lib"; fi
    local -a directories
    IFS=: read -r -a directories <<< "$library_dirs"
    for directory in "${directories[@]}"; do
        [ -d "$directory" ] || continue
        export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}$directory"
    done
    if [ -n "${SDL_VIDEO_EGL_DRIVER:-}" ] && [ -n "${SDL_VIDEO_GL_DRIVER:-}" ] &&
        "$GAMEDIR/nfsmw_runtime" --probe-gl "$SDL_VIDEO_EGL_DRIVER" "$SDL_VIDEO_GL_DRIVER"; then
        chosen_egl=$SDL_VIDEO_EGL_DRIVER
        chosen_gles=$SDL_VIDEO_GL_DRIVER
        graphics_mode=manual
    fi

    for runtime_dir in "${XDG_RUNTIME_DIR:-}" "/run/user/$(id -u)" /run/user/0 /var/run/0-runtime-dir; do
        [ -n "$runtime_dir" ] || continue
        for socket in "$runtime_dir"/wayland-*; do
            [ -S "$socket" ] || continue
            wayland_socket=$socket
            break 2
        done
    done

    if [ -n "$wayland_socket" ] && [ -z "$chosen_egl" ]; then
        echo "Graphics: Wayland session ($wayland_socket); trying Mesa EGL/GLES before Mali libraries"
        for mesa_pass in marked versioned unversioned; do
            for directory in "${directories[@]}"; do
                case "$directory" in */mali) continue ;; esac
                if [ "$mesa_pass" = unversioned ]; then
                    egl="$directory/libEGL.so"
                    gles="$directory/libGLESv2.so"
                else
                    egl="$directory/libEGL.so.1"
                    gles="$directory/libGLESv2.so.2"
                fi
                [ -e "$egl" ] && [ -e "$gles" ] || continue
                if [ "$mesa_pass" = marked ]; then
                    [ -e "$directory/libEGL_mesa.so.0" ] || continue
                elif [ "$mesa_pass" = versioned ]; then
                    [ ! -e "$directory/libEGL_mesa.so.0" ] || continue
                fi
                mesa_candidate=1
                echo "Graphics: testing Wayland Mesa EGL=$egl GLES2=$gles"
                if "$GAMEDIR/nfsmw_runtime" --probe-gl "$egl" "$gles"; then
                    chosen_egl=$egl
                    chosen_gles=$gles
                    graphics_mode=wayland-mesa
                    break 2
                fi
            done
        done
        if [ -z "$chosen_egl" ] && [ "$mesa_candidate" -ne 0 ]; then
            echo "Wayland Mesa EGL/GLES candidates failed the window preflight; refusing the Mali fallback."
            return 1
        fi
    fi

    for directory in "${directories[@]}"; do
        [ -z "$chosen_egl" ] || break
        for egl in "$directory/libEGL.so" "$directory/libEGL.so.1"; do
            [ -e "$egl" ] || continue
            for gles in "$directory/libGLESv2.so" "$directory/libGLESv2.so.2"; do
                [ -e "$gles" ] || continue
                if "$GAMEDIR/nfsmw_runtime" --probe-gl "$egl" "$gles"; then
                    chosen_egl=$egl
                    chosen_gles=$gles
                    graphics_mode=probed
                    break 2
                fi
            done
        done
    done
    if [ -z "$chosen_egl" ]; then
        for directory in "${directories[@]}"; do
            for blob in "$directory"/libmali*.so* "$directory"/libMali.so*; do
                [ -e "$blob" ] || continue
                if "$GAMEDIR/nfsmw_runtime" --probe-gl "$blob" "$blob"; then
                    chosen_egl=$blob
                    chosen_gles=$blob
                    graphics_mode=probed
                    break 2
                fi
            done
        done
    fi
    if [ -z "$chosen_egl" ]; then
        echo "No loadable ARM32 EGL/GLES2 pair found in $library_dirs"
        return 1
    fi
    GL_SHIM=$(mktemp -d /tmp/nfsmw-gl.XXXXXX) || return 1
    ln -s "$chosen_egl" "$GL_SHIM/libEGL.so"
    ln -s "$chosen_egl" "$GL_SHIM/libEGL.so.1"
    ln -s "$chosen_gles" "$GL_SHIM/libGLESv2.so"
    ln -s "$chosen_gles" "$GL_SHIM/libGLESv2.so.2"
    export LD_LIBRARY_PATH="$GL_SHIM:$LD_LIBRARY_PATH"
    export SDL_VIDEO_EGL_DRIVER="$chosen_egl" SDL_VIDEO_GL_DRIVER="$chosen_gles"
    echo "Graphics: EGL=$chosen_egl GLES2=$chosen_gles SDL=${SDL_VIDEODRIVER:-auto} mode=$graphics_mode"

    local pipewire_dir= pulse_socket= runtime_dir
    for directory in "${directories[@]}"; do
        if [ -d "$directory/pipewire-0.3" ]; then pipewire_dir="$directory/pipewire-0.3"; break; fi
    done
    for runtime_dir in "${XDG_RUNTIME_DIR:-}" "/run/user/$(id -u)" /run/user/0; do
        [ -n "$runtime_dir" ] && [ -d "$runtime_dir" ] || continue
        export XDG_RUNTIME_DIR="$runtime_dir"; break
    done
    for pulse_socket in "${XDG_RUNTIME_DIR:-}/pulse/native" /run/pulse/native /var/run/pulse/native; do
        [ -S "$pulse_socket" ] && break
    done
    if [ -n "$pipewire_dir" ] || [ -S "$pulse_socket" ]; then
        unset AUDIODEV ALSA_CONFIG_PATH SDL_AUDIO_DEVICE_NAME ALSA_CARD
        for directory in "${directories[@]}"; do
            if [ -d "$directory/spa-0.2" ]; then export SPA_PLUGIN_DIR="$directory/spa-0.2"; break; fi
        done
        [ -z "$pipewire_dir" ] || export PIPEWIRE_MODULE_DIR="$pipewire_dir"
        [ ! -S "$pulse_socket" ] || export PULSE_SERVER="unix:$pulse_socket"
        export SDL_AUDIO_ALSA_SET_BUFFER_SIZE=1
    else
        export AUDIODEV="${AUDIODEV:-plug:dmix}"
    fi
    export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"
    echo "Audio: SDL=$SDL_AUDIODRIVER SPA=${SPA_PLUGIN_DIR:-auto} PipeWire=${PIPEWIRE_MODULE_DIR:-auto} XDG=${XDG_RUNTIME_DIR:-unset}"
}

nfsmw_runtime_cleanup() {
    if [[ "${GL_SHIM:-}" == /tmp/nfsmw-gl.* ]]; then
        rm -f -- "$GL_SHIM/libEGL.so" "$GL_SHIM/libEGL.so.1" "$GL_SHIM/libGLESv2.so" "$GL_SHIM/libGLESv2.so.2"
        rmdir -- "$GL_SHIM" 2>/dev/null || true
    fi
}
