# Need for Speed: Most Wanted for R36S

An experimental compatibility port of the 2012 Android release of *Need for
Speed: Most Wanted* for the R36S and ArkOS.

> **Public alpha:** races are playable with working controls and sound effects.
> One pre-race car-selection action still requires a timing workaround. See
> [Known issues](KNOWN_ISSUES.md) before installing.

This repository contains only independently written compatibility code and
PortMaster packaging. It does not contain the game, an APK, an OBB, extracted
Android libraries, or other Electronic Arts assets.

## What works

- 640×480 OpenGL ES 2 rendering on the Mali-G31
- High visual settings at an observed 48.84 FPS average
- Smooth analog steering and D-pad menu navigation
- Race, map, garage and modification-screen controls
- Sound effects through the FMOD/OpenSL-to-SDL audio bridge
- Career saves and multiple completed races
- Clean exit back to PortMaster

The launcher enables the game soundtrack and menu audio by default. Set
`NFSMW_SILENT_AUDIO=1` only when testing without the game's music player.

### Audio and text troubleshooting

On RG34XX SP with muOS, a successful SDL audio preflight can still be followed
by `System_setOutput` and `EventSystem::init` failures inside FMOD. The runtime
now retries a rejected explicit output selection with FMOD's automatic mode.
The hook covers both the C API and the C++ method imported by this game build.
This fallback still needs handheld verification. Check `logs/nfsmw.log` for
`G8-FMOD` selection results and `G8-AUDIOTRACK` mixer startup when reporting
remaining silence. The launcher now reports the platform's music player as
inactive so the game can start its own playlist.

The fallback bitmap font now uses matching measurement, drawing and baseline
bounds to prevent clipping. It remains a simple block font. Audio fallback and
font-bound regression checks can be run with `python3 -m unittest discover -s
tests -v` on Linux; font checks require `arm-linux-gnueabihf-gcc` and `qemu-arm`.

## Installation

1. Download
   [`nfsmw-r36s-v0.1.0-alpha.zip`](https://github.com/Detoy/nfsmw-r36s/releases/tag/v0.1.0-alpha).
2. Install it through PortMaster, or extract it at the root of the ROMs card.
3. Copy your legally obtained APK and OBB to `ports/nfsmw/gamedata/`.
4. Launch the port. First-run setup verifies the files, extracts the five
   required ARMv7 libraries, and prepares the OBB sound files needed by FMOD.

### Supported game version

| Item | Value |
|---|---|
| Package | `com.ea.games.nfs13_row` |
| Version | `1.3.128` (`1003128`) |
| APK SHA-256 | `bfbe9d08165b8e976924e94879b40ac6575108d5b92521ca837175c0b291c7c7` |
| OBB filename | `main.1003128.com.ea.games.nfs13_row.obb` |
| OBB SHA-256 | `66dd4e695e698929f789e7c825eabe3ba5a50ed2ce28b628c96e5dbc008043a1` |

Files from other releases and similarly named repacks are not compatible.

## Controls

| Control | Action |
|---|---|
| Left stick | Move the menu cursor; steer while driving |
| D-pad | Move the menu cursor; steer while driving |
| Right stick | Drag-scroll menus and pan the city map |
| A | Click in mouse mode; existing game action while driving |
| B | Back |
| L1 / R1 | Scroll the menu or map horizontally; existing driving actions while driving |
| L2 / R2 | Scroll the menu or map vertically |
| Select | Switch between mouse-menu and driving controls |
| Start | Pause while driving; switch back to driving from a pause menu |
| Select + Start | Exit to PortMaster |

The cursor starts visible for menus. If mouse mode receives no button, stick,
or trigger input for 15 seconds, it automatically hides and normal gamepad
controls take over. Any input restarts the timer. Select remains the manual
mode switch; Start opens mouse controls on pause, and Start again resumes
driving. Race-finish screens may require Select to return to mouse mode. A and
scrolling use synthetic touch events; this control path still needs handheld
verification.

On the pre-race car-selection or purchase screen, use the workaround described
in [KNOWN_ISSUES.md](KNOWN_ISSUES.md). The following modifications screen works
normally with D-pad and A.

## Development

The runtime maps the original Android ARMv7 libraries, provides the required
Bionic and JNI compatibility surface, translates the Android soft-float ABI,
and hosts graphics, controller and audio output through Linux/SDL.

FMOD reads menu effects and music from `gamefiles/published/sounds/`. These
files are prepared from the owned OBB on first launch and reused afterwards.
The compatibility bridge enlarges small Android thread-stack requests to fit
Linux: FMOD's 8 KiB file-thread stack otherwise prevents streamed music and
menu sounds from being created even when the audio files open successfully.

To prepare a private local test directory from your own game files:

```sh
tools/extract_nfsmw.sh \
  /path/to/your-game.apk \
  /path/to/main.1003128.com.ea.games.nfs13_row.obb \
  gamefiles
```

Build the hard-float ARMv7 runtime with:

```sh
make -C runtime
```

Create the clean PortMaster archive with:

```sh
portmaster/build_port.sh
```

Further technical details are in [PORTING_STATUS.md](PORTING_STATUS.md) and
[research/abi-contract.md](research/abi-contract.md).

## Contributing

Reports and focused patches are welcome. The main controller blocker is
tracked in [issue #1](https://github.com/Detoy/nfsmw-r36s/issues/1). Please read
[CONTRIBUTING.md](CONTRIBUTING.md) before submitting logs or code.

## License and game ownership

The compatibility code and packaging are available under the [MIT License](LICENSE).
The license does not cover *Need for Speed*, its code, data, artwork, audio, or
trademarks. Those remain the property of their respective owners.

## Pixelforge handheld adaptation

Based on [Detoy's port](https://github.com/Detoy/nfsmw-r36s), with display, setup and packaging adaptations by Pixelforge ports (Ronax). eapx by EapRules provides the first-launch stages and percentage display. Original MIT notices are retained; the separately distributed eapx tool carries its own GPL licence.

Native display size is detected automatically. `ports/nfsmw/resolution.txt` accepts `auto` or `WIDTHxHEIGHT`; manual render sizes are stretched onto the panel. Target sizes include 640x480, 720x480, 720x720, 1024x768 and 1280x720. New sizes and muOS support require handheld testing.

## Build on Windows

Install Docker Desktop with the WSL2 Linux engine and leave it running. From PowerShell in this repository:

```powershell
docker build -t nfsmw-build -f Dockerfile.build .
docker run --rm --mount "type=bind,source=$($PWD.Path),target=/src" -w /src nfsmw-build bash -lc "make -C runtime clean && make -C runtime CROSS=arm-linux-gnueabihf- -j2 && bash portmaster/build_port.sh"
```

Output: `portmaster/dist/nfsmw.zip`. No APK, OBB or extracted game files are needed to build.

The build uses a Debian Bullseye snapshot and rejects a runtime requiring glibc newer than 2.31. A clean rebuild prevents objects from a newer toolchain being reused. This addresses newer-toolchain glibc requirements without replacing the handheld's libc. The RG34XX-SP muOS report confirms the old `nfsmw_runtime` requires `GLIBC_2.38` from `/lib32/libm.so.6` and `/lib32/libc.so.6`. Replace the runtime with the new build; do not replace the firmware libraries. Firmware must provide ARM32 support and its own SDL2/EGL/GLES libraries.

`package/` contains the website metadata and redistributable files. Run `python tools/sync_package.py` to refresh it. The ZIP includes no purchased game content. See `package/README.md` for installation, progress and display settings. These changes have not been tested on a handheld.

## One-command Windows build

With Docker Desktop running in Linux-container mode, run from this source folder:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

This script runs the Docker image build, compilation and packaging steps above. Add `-NoCache` to refresh the build environment. No game files are required.

## muOS graphics and audio setup

The latest source replaces the bundled FMOD library's old `/proc/cpuinfo`
feature detector with Linux ARM32 `AT_HWCAP` detection. The reported
`setOutput ... result=48` followed by `EventSystem is null` happens before
sound banks load; changing volume or ALSA settings cannot repair that failure.
The patch checks the donor instructions before applying and does not claim
CPU features absent from the kernel. It enables FMOD sound output and the
game's own soundtrack. After rebuilding, look for
`G8-FMOD CPU HWCAP`, a successful `setOutput` result and an active
`G8-AUDIOTRACK` mixer in `logs/nfsmw.log`. Device verification is still required.

The launcher probes the firmware's ARM32 EGL/GLES pair, including `/usr/lib32` and `/lib32`, before starting the game. It uses the firmware's video driver selection and sets the 32-bit PipeWire/SPA module directories where present. The graphics startup check accepts the actual display size instead of requiring 640x480.

The reported RG34XX-SP log confirms glibc compatibility and successful game-data import; the graphics/audio startup changes still need a device test. Rebuild and update the launcher, runtime and `runtime-env.sh` together. Keep `gamefiles/`, `files/`, and `.eapx-nfsmw-data.json` when updating so validated data and saves are retained.
