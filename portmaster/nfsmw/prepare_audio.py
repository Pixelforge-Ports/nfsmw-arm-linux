#!/usr/bin/env python3
import os
from pathlib import Path
import shutil
import sys
import zipfile

PREFIX = "published/sounds/"
REQUIRED = (
    "published/sounds/music/loading_01.mp3",
    "published/sounds/ui/ui.fev",
    "published/sounds/ui/ui.fsb",
)


def main():
    game_dir = Path(sys.argv[1]).resolve()
    source = game_dir / "gamefiles" / "main.1003128.com.ea.games.nfs13_row.obb"
    target = game_dir / "gamefiles" / "published" / "sounds"
    marker = game_dir / "gamefiles" / ".nfsmw-audio-source"
    identity = "%d:%d" % (source.stat().st_size, source.stat().st_mtime_ns)
    if marker.is_file() and marker.read_text(encoding="ascii", errors="ignore") == identity and all((game_dir / "gamefiles" / item).is_file() for item in REQUIRED):
        return
    stage = game_dir / "gamefiles" / ".nfsmw-audio-stage"
    shutil.rmtree(stage, ignore_errors=True)
    try:
        with zipfile.ZipFile(source) as archive:
            members = [item for item in archive.infolist() if item.filename.startswith(PREFIX) and not item.is_dir()]
            if not members or not all(name in {item.filename for item in members} for name in REQUIRED):
                raise RuntimeError("required sound files are missing from the OBB")
            print("Preparing NFS audio assets (%d files)..." % len(members))
            for index, item in enumerate(members, 1):
                destination = stage / item.filename
                destination.parent.mkdir(parents=True, exist_ok=True)
                with archive.open(item) as reader, destination.open("wb") as writer:
                    shutil.copyfileobj(reader, writer, 1024 * 1024)
                if index % 25 == 0 or index == len(members):
                    print("NFS audio preparation: %d/%d" % (index, len(members)))
        shutil.rmtree(target, ignore_errors=True)
        target.parent.mkdir(parents=True, exist_ok=True)
        os.replace(stage / "published" / "sounds", target)
        marker.write_text(identity, encoding="ascii")
    finally:
        shutil.rmtree(stage, ignore_errors=True)


if __name__ == "__main__":
    main()
