"""Check a staged package, without Qt or a Python runtime installed in it."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import os
import ctypes

parser = argparse.ArgumentParser()
parser.add_argument("directory", type=Path)
parser.add_argument("--platform", required=True, choices=["windows-amd64", "macos-arm64", "linux-amd64"])
args = parser.parse_args()
if os.name == "nt":
    ctypes.windll.kernel32.SetErrorMode(3)
root = args.directory.resolve()
if args.platform == "macos-arm64":
    executable = root / "AutoSplatoon.app" / "Contents" / "MacOS" / "AutoSplatoon"
    resources = executable.parents[1] / "Resources"
elif args.platform == "linux-amd64":
    executable = root / "usr" / "bin" / "AutoSplatoon"
    resources = root / "usr" / "share" / "autosplatoon"
else:
    executable = root / "AutoSplatoon.exe"
    resources = root
lock = json.loads((resources / "dependencies.lock.json").read_text())
firmware = resources / "firmware" / "PRO-UART0.bin"
assert hashlib.sha256(firmware.read_bytes()).hexdigest() == lock["firmware"]["sha256"]
assert executable.is_file()
tool = (resources if args.platform == "macos-arm64" else executable.parent) / "tools" / ("esptool.exe" if args.platform == "windows-amd64" else "esptool")
result = subprocess.run([str(tool), "version"], capture_output=True, text=True, check=True, timeout=30)
assert lock["esptool"]["version"] in result.stdout
print("Verified firmware, executable and bundled esptool " + lock["esptool"]["version"])
