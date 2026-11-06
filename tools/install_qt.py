"""Install the pinned Qt SDK, compiler, CMake and 7-Zip."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
from tempfile import TemporaryDirectory
from fetch_assets import ROOT, download


def install_cmake(cmake, platform, cache):
    cmake_root = ROOT / ".deps" / "cmake" / cmake["version"]
    if not cmake_root.is_dir():
        asset = cmake["assets"][platform]
        archive = cache / asset["name"]
        download(asset["url"], archive, asset["sha256"])
        cmake_root.parent.mkdir(parents=True, exist_ok=True)
        with TemporaryDirectory(dir=cmake_root.parent) as staging:
            shutil.unpack_archive(archive, staging)
            unpacked = archive.name.removesuffix(".zip").removesuffix(".tar.gz")
            (Path(staging) / unpacked).rename(cmake_root)
    cmake_bin = cmake_root / ("CMake.app/Contents/bin" if platform == "macos-arm64" else "bin")
    suffix = ".exe" if platform == "windows-amd64" else ""
    for tool in ("cmake", "ctest", "cpack"):
        version = subprocess.check_output([cmake_bin / (tool + suffix), "--version"], text=True).splitlines()[0]
        if version != f"{tool} version {cmake['version']}":
            raise RuntimeError(f"Unexpected build tool version in {cmake_bin}: {version}")
    return cmake_bin


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", required=True, choices=["windows-amd64", "macos-arm64", "linux-amd64"])
    parser.add_argument("--output", type=Path, default=ROOT / ".deps" / "qt")
    args = parser.parse_args()
    lock = json.loads((ROOT / "dependencies.lock.json").read_text())
    assets = lock["qt"]["sdk"][args.platform]
    cache = ROOT / ".deps" / "downloads" / args.platform
    cmake_bin = install_cmake(lock["build"]["cmake"], args.platform, cache)

    extractor = lock["build"]["sevenzip"]["assets"][args.platform]
    archive = cache / extractor["name"]
    download(extractor["url"], archive, extractor["sha256"])
    if args.platform == "windows-amd64":
        sevenzip = archive.resolve()
    else:
        directory = ROOT / ".deps" / "7zip"
        directory.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as bundle:
            bundle.extractall(directory, filter="data")
        sevenzip = directory / "7zz"
        sevenzip.chmod(0o755)
    sevenzip = str(sevenzip)

    def fetch(asset):
        path = cache / asset["name"]
        download(asset["url"], path, asset["sha256"])
        print("Verified:", asset["name"], flush=True)
        return path

    args.output.mkdir(parents=True, exist_ok=True)
    with ThreadPoolExecutor(max_workers=3) as executor:
        for archive in executor.map(fetch, assets):
            # Verified Qt SDKs contain framework / .so symlinks pointing to other symlinks.
            links = ["-snld20"] if args.platform != "windows-amd64" else []
            subprocess.run([sevenzip, "x", "-y", *links, str(archive), f"-o{args.output.resolve()}"], check=True, stdout=subprocess.DEVNULL)
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
    print("QT_ROOT=" + prefix.as_posix(), flush=True)
    print("CMAKE_BIN=" + cmake_bin.as_posix(), flush=True)


if __name__ == "__main__":
    main()
