"""Check source strings and the English Qt Linguist catalog."""

import argparse
from collections import Counter
from pathlib import Path
import re
import shutil
import subprocess
from tempfile import TemporaryDirectory
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "translations/autosplatoon_en.ts"
PLACEHOLDERS = re.compile(r"%L?(?:[1-9]\d*|n)")


def messages(path):
    for context in ET.parse(path).getroot().findall("context"):
        for message in context.findall("message"):
            yield (context.findtext("name"), message.findtext("source")), message.find("translation")


def check(expected):
    actual = set()
    errors = []
    for key, translation in messages(CATALOG):
        if key in actual:
            errors.append(f"Duplicate: {key!r}")
        actual.add(key)
        if translation is None or translation.get("type") or not (translation.text or "").strip():
            errors.append(f"Unfinished: {key!r}")
        elif Counter(PLACEHOLDERS.findall(key[1])) != Counter(PLACEHOLDERS.findall(translation.text)):
            errors.append(f"Placeholder mismatch: {key!r}")
    errors += [f"Missing: {key!r}" for key in sorted(expected - actual)]
    errors += [f"Obsolete: {key!r}" for key in sorted(actual - expected)]
    if errors:
        raise SystemExit("\n".join(errors))
    print(f"Verified {len(expected)} translated source messages and their placeholders")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--extract", type=Path, help="Write a source-only TS file for review")
    parser.add_argument("--extract-only", action="store_true", help="Skip comparing the committed catalog")
    args = parser.parse_args()
    lupdate = shutil.which("lupdate")
    if lupdate is None:
        raise SystemExit("lupdate not found. Add the pinned Qt SDK's bin directory to PATH.")
    with TemporaryDirectory() as directory:
        source = Path(directory) / "source.ts"
        try:
            subprocess.run([lupdate, "-warnings-are-errors", "-no-recursive", "-extensions", "cpp,h", "-locations", "none",
                            "-sort-messages", "-source-language", "zh_CN", "-target-language", "en",
                            "AutoSplatoon", "tools/generate_examples.cpp", "-ts", source], cwd=ROOT, check=True)
        except (OSError, subprocess.CalledProcessError) as error:
            raise SystemExit(f"Qt lupdate failed. Check the SDK and its runtime libraries: {error}") from error
        if args.extract:
            args.extract.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, args.extract)
        if not args.extract_only:
            check({key for key, _ in messages(source)})
