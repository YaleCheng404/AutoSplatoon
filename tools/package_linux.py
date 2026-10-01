"""Use linuxdeploy's Qt plugin to produce a self-contained Linux AppDir."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
from fetch_assets import ROOT, download

parser = argparse.ArgumentParser()
parser.add_argument("--build", type=Path, default=ROOT / "build" / "release")
parser.add_argument("--qt", type=Path, required=True)
args = parser.parse_args()
build = args.build.resolve()
appdir = build / "AutoSplatoon.AppDir"
binary = appdir / "usr" / "bin"
binary.mkdir(parents=True, exist_ok=True)
shutil.copy2(build / "AutoSplatoon", binary / "AutoSplatoon")
shutil.copytree(build / "assets" / "tools", binary / "tools", dirs_exist_ok=True)
(binary / "translations").mkdir(exist_ok=True)
shutil.copy2(args.qt / "translations" / "qtbase_zh_CN.qm", binary / "translations" / "qtbase_zh_CN.qm")
for folder in ["firmware", "licenses", "docs"]:
    shutil.copytree(ROOT / folder, appdir / "usr" / "share" / "autosplatoon" / folder, dirs_exist_ok=True)
shutil.copy2(ROOT / "LICENSE", appdir / "usr" / "share" / "autosplatoon" / "LICENSE")
shutil.copy2(ROOT / "README.md", appdir / "usr" / "share" / "autosplatoon" / "README.md")
shutil.copy2(ROOT / "dependencies.lock.json", appdir / "usr" / "share" / "autosplatoon" / "dependencies.lock.json")
shutil.copytree(args.qt / "sbom", appdir / "usr" / "share" / "autosplatoon" / "licenses" / "Qt-SBOM", dirs_exist_ok=True)
lock = json.loads((ROOT / "dependencies.lock.json").read_text())
for asset in lock["appimage"].values():
    path = build / asset["name"]
    download(asset["url"], path, asset["sha256"])
    path.chmod(0o755)
environment = os.environ.copy()
platform_plugins = [args.qt / "plugins/platforms/libqoffscreen.so",
                    *sorted((args.qt / "plugins/platforms").glob("libqwayland*.so"))]
if len(platform_plugins) < 2 or not all(plugin.is_file() for plugin in platform_plugins):
    raise RuntimeError("Missing offscreen / Wayland SDK platform plugins")
environment.update({"APPIMAGE_EXTRACT_AND_RUN": "1", "QMAKE": str(args.qt.resolve() / "bin" / "qmake"),
                    "EXTRA_QT_MODULES": "svg;waylandcompositor",
                    "EXTRA_PLATFORM_PLUGINS": ";".join(plugin.name for plugin in platform_plugins),
                    "OUTPUT": str(build / "AutoSplatoon-linux-x64.AppImage")})
environment["PATH"] = str(build) + os.pathsep + str(args.qt.resolve() / "bin") + os.pathsep + environment["PATH"]
# Wayland plugins may depend on QtWayland libraries; the Qt plugin resolves them.
icon = build / "autosplatoon.svg"
shutil.copy2(ROOT / "AutoSplatoon" / "app.svg", icon)
subprocess.run([str(build / "linuxdeploy-x86_64.AppImage"), "--appdir", str(appdir), "--executable", str(binary / "AutoSplatoon"),
                "--desktop-file", str(ROOT / "tools" / "AutoSplatoon.desktop"), "--icon-file", str(icon),
                "--plugin", "qt", "--output", "appimage"], env=environment, check=True)
with tarfile.open(build / "AutoSplatoon-linux-x64.tar.gz", "w:gz") as archive:
    archive.add(appdir, arcname="AutoSplatoon")
