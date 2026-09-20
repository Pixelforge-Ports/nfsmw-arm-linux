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
Sound effects are not yet confirmed fixed on muOS; device testing is required.

## Music

Music is disabled because the original Android decoder
fails and retries continuously on Linux, causing a large performance loss.

## Experimental cursor

Select toggles a development cursor, but taps are rejected by the game. It is
not a functional control method in this alpha.

When reporting a problem, include your handheld, firmware, release version,
exact reproduction steps and the relevant part of `ports/nfsmw/logs/nfsmw.log`.
Do not upload game files, extracted libraries, assets, audio, or saves.
