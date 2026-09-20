from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which('cc'), 'requires a Linux C compiler')
class NativeBridgeTests(unittest.TestCase):
    def check_program(self, name, arm=False):
        compiler = 'arm-linux-gnueabihf-gcc' if arm else 'cc'
        if arm and (not shutil.which(compiler) or not shutil.which('qemu-arm')):
            self.skipTest('requires ARM cross compiler and qemu-arm')
        with tempfile.TemporaryDirectory() as directory:
            program = Path(directory) / name
            subprocess.run([
                compiler, '-D_GNU_SOURCE', '-std=c17', '-static',
                '-ffunction-sections', '-fdata-sections',
                '-I' + str(ROOT / 'runtime/src'),
                str(ROOT / 'tests' / (name + '.c')),
                '-Wl,--gc-sections', '-o', str(program),
            ], check=True)
            subprocess.run((['qemu-arm'] if arm else []) + [str(program)], check=True)

    def test_output_selection(self):
        self.check_program('fmod_output_test')

    def test_cpu_feature_translation(self):
        self.check_program('fmod_cpu_test')

    def test_text_bounds_and_measurement(self):
        self.check_program('font_metrics_test', arm=True)
