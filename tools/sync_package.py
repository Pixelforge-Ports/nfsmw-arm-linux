from pathlib import Path
import shutil
ROOT=Path(__file__).resolve().parents[1]
def sync():
    data=ROOT/'portmaster/nfsmw'; out=ROOT/'package'; game=out/'nfsmw'
    game.mkdir(parents=True,exist_ok=True)
    for name in ['port.json','README.md','screenshot.png','cover.png','gameinfo.xml']:
        shutil.copyfile(data/name,out/name)
    shutil.copyfile(ROOT/'testing_thread.txt',out/'testing_thread.txt')
    shutil.copyfile(ROOT/'portmaster/Need for Speed Most Wanted.sh',out/'Need for Speed Most Wanted.sh')
    for name in ['setup.sh','runtime-env.sh','nfsmw.ini','nfsmw.eapx.json','README.md']:
        shutil.copyfile(data/name,game/name)
    shutil.copytree(data/'licenses',game/'licenses',dirs_exist_ok=True)
    shutil.copyfile(ROOT/'tools/eapx.py',data/'eapx.py')
    shutil.copyfile(ROOT/'tools/eapx.py',game/'eapx.py')
if __name__=='__main__': sync()
