"""Offscreen GUI/drawing invariance check; does not certify physical monitors."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("executable", type=Path)
parser.add_argument("--output", type=Path, default=Path("build/qa"))
args = parser.parse_args()
if os.name == "nt":
    ctypes.windll.kernel32.SetErrorMode(3)  # Tests must never spawn loader error dialogs.
environment = os.environ.copy()
for variable in ["LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QTDIR"]:
    environment.pop(variable, None)
if os.name != "nt":
    environment["PATH"] = "/usr/bin:/bin"
if os.name == "nt":
    system_root = os.environ["SystemRoot"]
    environment["PATH"] = system_root + "/System32"
    environment["QT_QPA_FONTDIR"] = system_root + "/Fonts"
hashes = set()
results = []
for width, height in [(1920, 1080), (2560, 1440), (3840, 2160)]:
    for scale in [1, 1.25, 1.5, 2, 2.5, 3]:
        directory = (args.output / f"{width}x{height}-{scale}").resolve()
        directory.mkdir(parents=True, exist_ok=True)
        config = directory / "screen.json"
        config.write_text(json.dumps({"screens": [{"name": "test", "x": 0, "y": 0,
            "width": width, "height": height, "logicalDpi": 96, "logicalBaseDpi": 96, "dpr": 1}]}))
        # QPA separates options with ':', so Windows drive letters need a relative path.
        configuration_path = os.path.relpath(config).replace("\\", "/")
        environment.update(QT_QPA_PLATFORM="offscreen:configfile=" + configuration_path, QT_SCALE_FACTOR=str(scale),
                           AUTOSPLATOON_SMOKE_OUTPUT=str(directory))
        subprocess.run([str(args.executable.resolve()), "--smoke-test"], env=environment, check=True, timeout=30)
        hashes.add(hashlib.sha256((directory / "output.png").read_bytes()).hexdigest())
        metadata = json.loads((directory / "window.json").read_text())
        assert metadata["width"] <= metadata["screenWidth"], metadata
        assert metadata["height"] <= metadata["screenHeight"], metadata
        results.append({"resolution": [width, height], "scale": scale, **metadata})
assert len(hashes) == 1, "DPI changed output pixels"
(args.output / "gui-results.json").write_text(json.dumps({"output_sha256": hashes.pop(), "cases": results}, indent=2))
print(f"Passed {len(results)} offscreen display cases; output identical; development PATH removed on Windows")
