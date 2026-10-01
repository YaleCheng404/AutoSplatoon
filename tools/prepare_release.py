"""Require all four native packages before publishing a tagged release."""
import hashlib
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
directory, tag = Path(sys.argv[1]), sys.argv[2]
version = re.search(r"project\(AutoSplatoon VERSION ([\d.]+)", (root / "CMakeLists.txt").read_text())[1]
if tag != "v" + version:
    raise SystemExit(f"Tag {tag} does not match project version {version}")
if not (root / "docs" / "release-notes" / (tag + ".md")).is_file():
    raise SystemExit("Missing release notes")
packages = []
for pattern in [f"AutoSplatoon-{version}-Windows-*.zip", f"AutoSplatoon-{version}-Darwin-*.dmg",
                "AutoSplatoon-linux-x64.AppImage", "AutoSplatoon-linux-x64.tar.gz"]:
    matches = list(directory.glob(pattern))
    if len(matches) != 1 or matches[0].stat().st_size == 0:
        raise SystemExit(f"Expected one nonempty package: {pattern}")
    packages.extend(matches)
checksums = "".join(f"{hashlib.sha256(file.read_bytes()).hexdigest()}  {file.name}\n"
                    for file in sorted(packages))
(directory / "SHA256SUMS.txt").write_text(checksums)
print("Release complete: Windows x64, macOS ARM64, Linux AppImage and directory archive")
