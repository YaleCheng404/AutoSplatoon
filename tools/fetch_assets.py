"""Fetch pinned release assets. Only build machines need Python."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from tempfile import TemporaryDirectory
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def file_sha256(path):
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def download(url, path, sha256):
    if path.is_file() and file_sha256(path) == sha256:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(url, headers={"User-Agent": "AutoSplatoon-build"})
    with TemporaryDirectory(dir=path.parent) as staging:
        pending = Path(staging) / path.name
        for attempt in range(3):
            try:
                with urllib.request.urlopen(request, timeout=120) as response, pending.open("wb") as file:
                    shutil.copyfileobj(response, file)
                break
            except OSError:
                if attempt == 2:
                    raise
                time.sleep(attempt + 1)
        if file_sha256(pending) != sha256:
            raise RuntimeError(f"Checksum mismatch: {url}")
        pending.replace(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", choices=["windows-amd64", "macos-arm64", "linux-amd64"], required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "assets")
    args = parser.parse_args()
    lock = json.loads((ROOT / "dependencies.lock.json").read_text())
    asset = lock["esptool"]["assets"][args.platform]
    archive = args.output / asset["name"]
    download(asset["url"], archive, asset["sha256"])
    executable = "esptool.exe" if args.platform.startswith("windows") else "esptool"
    target = args.output / "tools" / executable
    target.parent.mkdir(parents=True, exist_ok=True)
    with TemporaryDirectory(dir=args.output) as staging:
        shutil.unpack_archive(archive, staging)
        source = next(Path(staging).rglob(executable))
        source.chmod(0o755)
        source.replace(target)
    print(target)


if __name__ == "__main__":
    main()
