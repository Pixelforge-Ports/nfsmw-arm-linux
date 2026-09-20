from pathlib import Path
import zipfile
ROOT=Path(__file__).resolve().parents[1]
data=ROOT/'portmaster/nfsmw'; package=ROOT/'package'
out=ROOT/'portmaster/dist/nfsmw.zip';out.parent.mkdir(exist_ok=True)
entries=[(package/'Need for Speed Most Wanted.sh','Need for Speed Most Wanted.sh'),
         (ROOT/'runtime/build/nfsmw_mapper','nfsmw/nfsmw_runtime')]
for name in ['setup.sh','runtime-env.sh','nfsmw.ini','nfsmw.eapx.json','eapx.py','README.md','port.json','gameinfo.xml','screenshot.png','cover.png']:
    entries.append((data/name,'nfsmw/'+name))
for path in sorted((data/'licenses').iterdir()): entries.append((path,'nfsmw/licenses/'+path.name))
entries.append((data/'gamedata/README.txt','nfsmw/gamedata/README.txt'))
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as z:
    for path,arc in entries:
        info=zipfile.ZipInfo(arc);info.create_system=3
        info.external_attr=(0o100755 if arc.endswith(('.sh','nfsmw_runtime')) else 0o100644)<<16
        info.compress_type=zipfile.ZIP_DEFLATED
        z.writestr(info,path.read_bytes())
print('Built',out)
