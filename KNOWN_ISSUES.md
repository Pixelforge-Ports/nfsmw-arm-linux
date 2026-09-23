# Known issues

## Pre-race car selection and purchase

After the car-selection animation settles, A changes the car class instead of
activating the visible Continue or Buy action.

### Workaround

Press B to leave the screen, A to re-enter, then A again immediately—before the
animation settles. The later modifications screen works normally.

### Developer notes

The screen exposes `mogaHighlightCar` and `mogaHighlightClass`, but not a MOGA
highlight for `btn_car_action_large`. Synthetic taps currently reach the JNI
entry point but are rejected because the supplied view is not registered.
Making touch injection use the title's registered view is the leading fix.

Technical discussion is tracked in
[issue #1](https://github.com/Detoy/nfsmw-r36s/issues/1).

## Sound effects

The RG34XX-SP muOS log shows successful SDL/ALSA startup followed by FMOD
error 48 and a null EventSystem. The bundled CPU detector parses Android-era
`/proc/cpuinfo`; the updated source uses the ARM32 process's Linux HWCAP flags.
The patch checks the donor instructions before changing the detector.
Race sound effects are now reported working on muOS. Menu effects use the
streaming path described below and still need a new device test.

## Music

The latest muOS log shows successful music and menu-bank file opens followed
by FMOD internal error 33. The bundled FMOD file thread requests an 8 KiB
stack, which is too small for glibc. The compatibility bridge now gives
host-allocated threads at least 64 KiB (or the host minimum, if larger),
without enlarging caller-supplied memory. Music and menu playback with this
change still need confirmation on the handheld.

The launcher now enables the Android soundtrack by default. If music needs to
be disabled for a specific device test, set `NFSMW_SILENT_AUDIO=1` before
launching and include `logs/nfsmw.log` in the report.

## Experimental menu mouse controls

Menus now start in mouse mode. The D-pad and left stick move the cursor, A sends
a tap, B sends Back, L1/R1 drag horizontally, L2/R2 drag vertically, and the
right stick pans by sending a drag gesture. Select switches to driving
controls; in driving mode, left/right D-pad also steer. Mouse mode automatically
switches to normal gamepad controls after 15 seconds without button, stick, or
trigger activity. Any input restarts the timer. Start switches to mouse mode
when pausing and back to driving when resuming after Select has enabled driving
mode.

The runtime does not yet have a verified screen-state signal, so the race start
and finish transitions are not switched automatically. Synthetic touch events
also need confirmation on the handheld; if the game rejects a tap, use Select
to switch modes and the existing gamepad menu controls.

When reporting a problem, include your handheld, firmware, release version,
exact reproduction steps and the relevant part of `ports/nfsmw/logs/nfsmw.log`.
Do not upload game files, extracted libraries, assets, audio, or saves.
