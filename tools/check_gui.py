"""Offscreen GUI/drawing invariance check; does not certify physical monitors."""
import argparse
import ctypes
import hashlib
import json
from itertools import product
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("executable", type=Path)
parser.add_argument("--output", type=Path, default=Path("build/qa"))
args = parser.parse_args()
environment = os.environ.copy()
for variable in ["LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QTDIR"]:
    environment.pop(variable, None)
if os.name == "nt":
    ctypes.windll.kernel32.SetErrorMode(3)  # Suppress loader error dialogs during tests.
    system_root = os.environ["SystemRoot"]
    environment["PATH"] = system_root + "/System32"
    environment["QT_QPA_FONTDIR"] = system_root + "/Fonts"
else:
    environment["PATH"] = "/usr/bin:/bin"
hashes = set()
plan_hashes = set()
results = []
labels = {"en": "Export PNG", "zh_CN": "导出 PNG"}
for language in labels:
    settings = (args.output / language / "settings").resolve()
    ini = settings / "AutoSplatoon" / "AutoSplatoon.ini"
    ini.parent.mkdir(parents=True, exist_ok=True)
    ini.write_text(f"[ui]\nlanguage={language}\n", encoding="utf-8")
    resolutions = [(1920, 1080), (2560, 1440), (3840, 2160)]
    for (width, height), scale in product(resolutions, [1, 1.25, 1.5, 2, 2.5, 3]):
        directory = (args.output / language / f"{width}x{height}-{scale}").resolve()
        directory.mkdir(parents=True, exist_ok=True)
        config = directory / "screen.json"
        config.write_text(json.dumps({"screens": [{"name": "test", "x": 0, "y": 0,
            "width": width, "height": height, "logicalDpi": 96, "logicalBaseDpi": 96, "dpr": 1}]}))
        # QPA separates options with ':', so Windows drive letters need a relative path.
        configuration_path = os.path.relpath(config).replace("\\", "/")
        environment.update(QT_QPA_PLATFORM="offscreen:configfile=" + configuration_path, QT_SCALE_FACTOR=str(scale),
                           AUTOSPLATOON_SMOKE_OUTPUT=str(directory), AUTOSPLATOON_SMOKE_SETTINGS=str(settings))
        subprocess.run([str(args.executable.resolve()), "--smoke-test"], env=environment, check=True, timeout=30)
        hashes.add(hashlib.sha256((directory / "output.png").read_bytes()).hexdigest())
        metadata = json.loads((directory / "window.json").read_text(encoding="utf-8"))
        assert metadata["language"] == language and metadata["exportLabel"] == labels[language], metadata
        assert metadata["width"] <= metadata["screenWidth"], metadata
        assert metadata["height"] <= metadata["screenHeight"], metadata
        plan_hashes.add(metadata["planHash"])
        results.append({"resolution": [width, height], "scale": scale, **metadata})
assert len(hashes) == 1, "Language or DPI changed output pixels"
assert len(plan_hashes) == 1, "Language or DPI changed the drawing plan"
(args.output / "gui-results.json").write_text(json.dumps({
    "output_sha256": hashes.pop(), "plan_sha256": plan_hashes.pop(), "cases": results,
}, indent=2), encoding="utf-8")
print(f"Passed {len(results)} offscreen display cases with identical output and drawing plans")
