"""Fetch pinned release assets. Only build machines need Python."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import tarfile
import urllib.request
import zipfile
import time

ROOT = Path(__file__).resolve().parents[1]


def download(url, path, sha256):
    if path.exists() and hashlib.sha256(path.read_bytes()).hexdigest() == sha256:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(url, headers={"User-Agent": "AutoSplatoon-build"})
    for attempt in range(3):
        try:
            with urllib.request.urlopen(request, timeout=120) as response:
                data = response.read()
            break
        except (OSError, TimeoutError):
            if attempt == 2:
                raise
            time.sleep(attempt + 1)
    if hashlib.sha256(data).hexdigest() != sha256:
        raise RuntimeError(f"Checksum mismatch: {url}")
    path.write_bytes(data)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", choices=["windows-amd64", "macos-arm64", "linux-amd64"], required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "assets")
    args = parser.parse_args()
    lock = json.loads((ROOT / "dependencies.lock.json").read_text())
    asset = lock["esptool"]["assets"][args.platform]
    archive = args.output / asset["name"]
    download(asset["url"], archive, asset["sha256"])
    unpacked = args.output / "unpacked"
    unpacked.mkdir(parents=True, exist_ok=True)
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(unpacked)
    else:
        with tarfile.open(archive) as bundle:
            bundle.extractall(unpacked, filter="data")
    executable = "esptool.exe" if args.platform.startswith("windows") else "esptool"
    source = next(unpacked.rglob(executable))
    target = args.output / "tools" / executable
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)
    target.chmod(0o755)
    # The upstream binaries are standalone; no Python runtime is required.
    print(target)


if __name__ == "__main__":
    main()
