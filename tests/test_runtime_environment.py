import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class RuntimeEnvironmentTests(unittest.TestCase):
    def check_environment(self, mode):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            game = root/'game'; game.mkdir()
            rejected = root/'rejected'; rejected.mkdir()
            libs = root/'lib32'; libs.mkdir()
            for directory in [rejected, libs]:
                for name in ['libEGL.so', 'libGLESv2.so']: (directory/name).touch()
            if mode in ('blob', 'missing'):
                for path in libs.iterdir(): path.unlink()
            if mode == 'blob': (libs/'libmali.so.0').touch()
            (libs/'pipewire-0.3').mkdir(); (libs/'spa-0.2').mkdir()
            runtime=game/'nfsmw_runtime'
            runtime.write_text('''#!/bin/bash
[ "$1" = --probe-gl ] || exit 2
case "$2:$3" in *rejected*) exit 1;; esac
[ -f "$2" ] && [ -f "$3" ]
''')
            runtime.chmod(0o755)
            env = dict(os.environ, GAMEDIR=str(game), NFSMW_LIBRARY_DIRS=f'{rejected}:{libs}',
                       DEVICE_ARCH='aarch64', SDL_VIDEODRIVER='firmware-choice',
                       AUDIODEV='wrong-device', ALSA_CONFIG_PATH='/wrong/config')
            for key in ['SDL_VIDEO_EGL_DRIVER','SDL_VIDEO_GL_DRIVER','LD_LIBRARY_PATH',
                        'SPA_PLUGIN_DIR','PIPEWIRE_MODULE_DIR','SDL_AUDIODRIVER']:
                env.pop(key, None)
            command='''set -eu
source "$1"
trap nfsmw_runtime_cleanup EXIT
nfsmw_runtime_environment
test -L "$GL_SHIM/libGLESv2.so.2"
test "${AUDIODEV:-}" = ""
test "${ALSA_CONFIG_PATH:-}" = ""
printf 'EGL=%s\nGLES=%s\nSPA=%s\nPW=%s\nVIDEO=%s\n' "$SDL_VIDEO_EGL_DRIVER" "$SDL_VIDEO_GL_DRIVER" "$SPA_PLUGIN_DIR" "$PIPEWIRE_MODULE_DIR" "$SDL_VIDEODRIVER"
'''
            result=subprocess.run(['bash','-c',command,'test',str(ROOT/'portmaster/nfsmw/runtime-env.sh')],
                                  env=env,capture_output=True,text=True)
            if mode == 'missing':
                self.assertNotEqual(result.returncode,0)
                self.assertIn('No loadable ARM32',result.stdout)
            else:
                self.assertEqual(result.returncode,0,result.stdout+result.stderr)
                name='libmali.so.0' if mode=='blob' else 'libEGL.so'
                self.assertIn('EGL='+str(libs/name),result.stdout)
                self.assertIn('SPA='+str(libs/'spa-0.2'),result.stdout)
                self.assertIn('PW='+str(libs/'pipewire-0.3'),result.stdout)
                self.assertIn('VIDEO=firmware-choice',result.stdout)

    def test_split_wrappers_and_pipewire(self): self.check_environment('wrappers')
    def test_blob_fallback(self): self.check_environment('blob')
    def test_no_usable_graphics(self): self.check_environment('missing')
