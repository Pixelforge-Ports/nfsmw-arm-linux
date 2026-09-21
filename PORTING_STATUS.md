# Porting status

Updated: 2026-09-22

## Pixelforge source update (not yet device-verified)

The RG34XX-SP muOS log confirms FMOD output selection fails with result 48
before the EventSystem is created. The verified donor's CPU detector at
`libfmodex.so+0xbfe0c` parses `/proc/cpuinfo`; its output registration checks
bits 2 (NEON) or 3 (VFP) and returns 48 when neither is present. The source
now translates Linux ARM32 `AT_HWCAP` into this bundled detector's bit layout.
Original instructions are checked before patching; unsupported donors fail
instead of receiving an unchecked patch.

The Continue/Buy control changes have been reverted; the existing controls and
quick-A workaround remain unchanged. Only the sound fix is retained.

Race sound effects are now reported working on muOS. The latest log confirms
the sound output and native filesystem initialize successfully, but music and
menu effects still fail when FMOD creates its file thread. The compatibility
bridge now enlarges its 8 KiB stack request to at least 64 KiB for glibc.
No new game runtime or archive was built for this update. Streamed audio needs
a fresh handheld test.
The following milestones and release hashes describe the upstream R36S alpha.

## Current state

The R36S public alpha is playable. It boots the supported Android ARMv7 build,
renders full races, accepts analog and digital controls, saves career progress,
and outputs sound effects. The current release is
[`v0.1.0-alpha`](https://github.com/Detoy/nfsmw-r36s/releases/tag/v0.1.0-alpha).

| Area | Status |
|---|---|
| ARMv7 loader and relocation | Pass on R36S |
| JNI and Android compatibility | Pass for gameplay |
| OpenGL ES 2 rendering | Pass at 640×480 |
| Race controls | Pass |
| Front-end controls | Partial; see car-selection issue |
| Sound effects | Pass |
| Music | Disabled |
| Performance | 48.84 FPS measured average |
| PortMaster packaging | Public alpha |

## Validated technical milestones

- All five bundled ARMv7 modules load on the physical handheld.
- All 56,142 dynamic relocations resolve.
- All guest constructors required for startup complete.
- The compatibility audit covers 84 imported Bionic/Android boundary APIs.
- All 49 imported scalar-float APIs use ARM soft-float thunks.
- The OBB reader indexes all 2,411 entries without expanding the archive.
- GLES2, SDL controller input and SDL/ALSA output run through the R36S Linux
  graphics and audio stack.
- A full high-graphics test session rendered 16,277 frames in 333.265 seconds:
  **48.84 FPS average**, with working sound effects.
- Multiple tutorial and career races have been completed on real hardware.

## Release identity

- Mapper SHA-256:
  `68c2fccb7fb71238bce4a2c266be30cb223855a3a70c84d31b091982aabbf628`
- Mapper GNU build ID: `6e9de3b982ba449f69ac44e5b8804c1e1fbc9bff`
- Alpha archive SHA-256:
  `3f139a3eb68f00c9b9753a074d1a9bb99578061573553db2ebd5488d6ef70fb1`

The release archive contains no APK, OBB, extracted Android library, game save,
or other proprietary game payload.

## Remaining work

### Car-selection Continue / Buy action

The touch-first `car_select_new` screen does not expose its Continue/Buy button
as a MOGA highlight target after the rollout settles. A during the rollout
works; afterward it activates the class filter instead.

The most promising fix is to make synthetic touch input use a view object
accepted by `GameGLSurfaceView_nativeTouchScreenEvent`. Current taps are dropped
with `AndroidInput: Unregistered view calling nativeTouchEvent`.

Approaches already ruled out:

- duplicate D-pad/analog delivery;
- MOGA right-stick focus actions;
- forcing the title's pointer-mode flag;
- forcing controller-presence bytes.

See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) and
[issue #1](https://github.com/Detoy/nfsmw-r36s/issues/1).

### Music and menu audio

The launcher enables the playlist after preparing audio from the owned OBB.
Native FMOD callbacks accept both `published/sounds/` and `sounds/` paths.
They use this donor's error codes (EOF 22, missing file 23) and report EOF on
short reads. ARM callback tests cover both path forms, reads, EOF, seeking
after EOF and missing files. Full music and menu playback still need device
verification.

The new log confirms successful MP3 and UI-bank opens but sound creation
returns internal error 33. In the supported donor, the file-buffer setup
starts `FMOD file thread` with an 8192-byte stack. Its pthread failure is
returned as error 33. Linux rejects that stack size; the bridge now raises
host-allocated stacks to at least 64 KiB and `PTHREAD_STACK_MIN`. Explicit
caller-supplied stacks keep their original address and size.
The focused ARM regression test creates and joins the 8 KiB guest request,
and checks that larger requests, caller-owned stacks and detached attributes
are preserved. It passes with the corrected bridge.

An isolated ARM test loads the owned FMOD library and creates streams from
`loading_01.mp3` and `ui/ui.fsb` with mode `0xa0`. Both return error 33 with
the previous stack handling, and both return success with valid sound handles
after the stack fix. The test uses FMOD's no-sound output, so it verifies sound
creation rather than audible playback or handheld performance.

Diagnostics are capped per launch: twelve successful opens, twelve failed
opens and eight `createSound` results. An open result of zero confirms file
access; a `createSound` result of zero confirms sound creation. Neither alone
proves audible playback. Error 33 in this FMOD version means internal error,
not missing file.

### Wider device testing

Only the R36S running ArkOS has been validated. Other ARMv7 PortMaster devices
may need graphics-driver, controller-map or memory adjustments.
