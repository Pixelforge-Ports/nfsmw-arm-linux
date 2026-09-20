from pathlib import Path
import runpy
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]

class GlibcGateTests(unittest.TestCase):
    def run_gate(self, version, machine='ARM'):
        with tempfile.NamedTemporaryFile() as binary:
            with patch('sys.argv', ['check_glibc.py', binary.name]), patch(
                'subprocess.check_output', side_effect=[
                    'Machine: ' + machine, 'Name: GLIBC_2.4\nName: GLIBC_' + version
                ]):
                runpy.run_path(str(ROOT/'tools/check_glibc.py'), run_name='__main__')

    def test_accepts_baseline(self): self.run_gate('2.31')

    def test_rejects_reported_device_failure(self):
        with self.assertRaisesRegex(SystemExit, 'exceeds 2.31'): self.run_gate('2.38')

    def test_rejects_wrong_architecture(self):
        with self.assertRaisesRegex(SystemExit, 'ARM32'): self.run_gate('2.31', 'AArch64')
