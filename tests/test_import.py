import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]

class ImportTests(unittest.TestCase):
    def test_staged_import_and_repeat_without_donor(self):
        # Synthetic ARM headers exercise the importer; these are not game files.
        with tempfile.TemporaryDirectory() as folder:
            game = Path(folder)
            donors = game / 'gamedata'
            donors.mkdir()
            recipe = json.loads((ROOT / 'portmaster/nfsmw/nfsmw.eapx.json').read_text())
            payloads = {}
            for i, rule in enumerate(recipe['extract']):
                data = bytearray(80 + i)
                if rule['source']['kind'] == 'entry':
                    data[:7] = b'\x7fELF\x01\x01\x01'
                    data[18:20] = (40).to_bytes(2, 'little')
                data[-1] = i
                data = bytes(data)
                digest = hashlib.sha256(data).hexdigest()
                rule['validate']['sha256'] = [digest]
                recipe['validate'][i]['sha256'] = [digest]
                payloads[rule['destination']] = data
            with zipfile.ZipFile(donors/'synthetic.zip', 'w') as archive:
                for rule in recipe['extract']:
                    if rule['source']['kind'] == 'entry':
                        archive.writestr(rule['source']['patterns'][0], payloads[rule['destination']])
                    else:
                        (donors/rule['source']['patterns'][0]).write_bytes(payloads[rule['destination']])
            spec = game/'recipe.json'
            spec.write_text(json.dumps(recipe))
            command = [sys.executable, str(ROOT/'tools/eapx.py'), 'install', '--recipe', str(spec),
                       '--game-dir', str(game), '--no-portmaster', '--tty', 'none']
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for path, data in payloads.items(): self.assertEqual((game/path).read_bytes(), data)
            for path in donors.iterdir(): path.unlink()
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_original_recipe_parses(self):
        import importlib.util
        spec = importlib.util.spec_from_file_location('eapx', ROOT/'tools/eapx.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.Recipe(str(ROOT/'portmaster/nfsmw/nfsmw.eapx.json'))

if __name__ == '__main__': unittest.main()
