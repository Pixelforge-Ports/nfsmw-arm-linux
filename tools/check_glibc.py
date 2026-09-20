"""Reject a runtime requiring glibc newer than the handheld build baseline."""
import re, subprocess, sys
from pathlib import Path
p=Path(sys.argv[1])
if not p.is_file(): raise SystemExit("Missing runtime: " + str(p))
header=subprocess.check_output(["arm-linux-gnueabihf-readelf","-h",str(p)],text=True)
if not re.search(r"Machine:\s+ARM\b",header): raise SystemExit("Runtime must be ARM32")
output=subprocess.check_output(["arm-linux-gnueabihf-readelf","--version-info",str(p)],text=True)
versions={tuple(map(int,v.split('.'))) for v in re.findall(r"GLIBC_([0-9]+(?:\.[0-9]+)+)",output)}
if not versions: raise SystemExit("No glibc version information found")
highest=max(versions)
print("Runtime requires GLIBC_"+'.'.join(map(str,highest)))
if highest > (2,31): raise SystemExit("Rebuild using Dockerfile.build: glibc requirement exceeds 2.31")
