"""Extract pinned official Qt SDK archives using the existing 7-Zip utility.

Qt 6.12 uses architecture-specific repository directories that aqt 3.3 does
not resolve. This small installer only handles this project's three SDKs.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import shutil
import subprocess
import os
from fetch_assets import ROOT, download


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", required=True, choices=["windows-amd64", "macos-arm64", "linux-amd64"])
    parser.add_argument("--output", type=Path, default=ROOT / ".deps" / "qt")
    args = parser.parse_args()
    sevenzip = shutil.which("7z") or shutil.which("7zz") or shutil.which("7za")
    if not sevenzip and os.name == "nt":
        candidate = Path(os.environ["ProgramFiles"]) / "7-Zip" / "7z.exe"
        if candidate.is_file():
            sevenzip = str(candidate)
    if not sevenzip:
        raise RuntimeError("Install 7-Zip / p7zip before installing the build SDK")
    lock = json.loads((ROOT / "dependencies.lock.json").read_text())
    assets = lock["qt"]["sdk"][args.platform]
    cache = ROOT / ".deps" / "downloads" / args.platform

    def fetch(asset):
        path = cache / asset["name"]
        download(asset["url"], path, asset["sha256"])
        print("Verified:", asset["name"], flush=True)
        return path

    args.output.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=3) as executor:
        for archive in executor.map(fetch, assets):
            subprocess.run([sevenzip, "x", "-y", str(archive), f"-o{args.output.resolve()}"], check=True, stdout=subprocess.DEVNULL)
    platform_folder = {"windows-amd64": "mingw_64", "macos-arm64": "macos", "linux-amd64": "gcc_64"}[args.platform]
    prefix = args.output.resolve()
    if not (prefix / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").is_file():
        prefix = prefix / lock["qt"]["version"] / platform_folder
    if not (prefix / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").is_file():
        raise RuntimeError(f"Missing SDK configuration: {prefix}")
    (prefix / "bin" / "qt.conf").write_text("[Paths]\nPrefix=..\n")
    if args.platform == "linux-amd64":
        # The official ICU archive is flat; Qt's shared libraries need it in lib/.
        for library in prefix.glob("libicu*.so*"):
            shutil.copy2(library, prefix / "lib" / library.name)
    if args.platform == "windows-amd64":
        # Runtime archives use a flat layout, while deployment tools expect bin/.
        for library in prefix.glob("*.dll"):
            shutil.copy2(library, prefix / "bin" / library.name)
        compiler = lock["build"]["mingw_asset"]
        archive = cache / compiler["name"]
        download(compiler["url"], archive, compiler["sha256"])
        subprocess.run([sevenzip, "x", "-y", str(archive), f"-o{(ROOT / '.deps' / 'compiler').resolve()}"], check=True, stdout=subprocess.DEVNULL)
        compiler_root = ROOT / ".deps" / "compiler" / "mingw64"
        for library in compiler_root.joinpath("bin").glob("*.dll"):
            if library.name in {"libstdc++-6.dll", "libgcc_s_seh-1.dll", "libwinpthread-1.dll"}:
                shutil.copy2(library, prefix / "bin" / library.name)
    print("QT_ROOT=" + str(prefix), flush=True)


if __name__ == "__main__":
    main()
