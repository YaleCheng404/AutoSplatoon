"""Check source strings and the English Qt Linguist catalog."""

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "translations/autosplatoon_en.ts"
TOKENS = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|::|\S')
PLACEHOLDERS = re.compile(r"%L?(?:[1-9]\d*|n)")


def extract():
    messages = defaultdict(set)
    sources = sorted((ROOT / "AutoSplatoon").glob("*.[ch]*")) + [ROOT / "tools/generate_examples.cpp"]
    for path in sources:
        header = path.with_suffix(".h")
        context = None
        if header.exists():
            match = re.search(r"class\s+(\w+)\s*:[^{]+\{\s*Q_OBJECT", header.read_text(encoding="utf-8"))
            if match:
                context = match[1]
        tokens = [token for token in TOKENS.findall(path.read_text(encoding="utf-8"))
                  if not token.startswith(("//", "/*"))]
        for index, token in enumerate(tokens):
            if tokens[index + 1:index + 2] != ["("]:
                continue
            qualified = index >= 2 and tokens[index - 1] == "::"
            start = index + 2
            if token == "tr":
                message_context = tokens[index - 2] if qualified else context
            elif token == "translate" and qualified and tokens[index - 2] == "QCoreApplication":
                message_context = json.loads(tokens[start])
                start += 2
            else:
                continue
            end = start
            while end < len(tokens) and tokens[end].startswith('"'):
                end += 1
            if not message_context or start == end or tokens[end] != ")":
                raise ValueError(f"{path.name}: expected a literal translation with a Qt context")
            messages[message_context].add("".join(json.loads(t) for t in tokens[start:end]))
    return dict(sorted(messages.items()))


def write_extracted(messages, path):
    root = ET.Element("TS", version="2.1", language="en", sourcelanguage="zh_CN")
    for name, sources in messages.items():
        context = ET.SubElement(root, "context")
        ET.SubElement(context, "name").text = name
        for source in sorted(sources):
            message = ET.SubElement(context, "message")
            ET.SubElement(message, "source").text = source
            ET.SubElement(message, "translation", type="unfinished")
    ET.indent(root, space="    ")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n'
                    + ET.tostring(root, encoding="unicode") + "\n", encoding="utf-8")


def check(messages):
    expected = {(name, source) for name, sources in messages.items() for source in sources}
    actual = set()
    errors = []
    for context in ET.parse(CATALOG).getroot().findall("context"):
        name = context.findtext("name")
        for message in context.findall("message"):
            source = message.findtext("source")
            translation = message.find("translation")
            key = (name, source)
            if key in actual:
                errors.append(f"Duplicate: {key!r}")
            actual.add(key)
            if translation is None or translation.get("type") or not (translation.text or "").strip():
                errors.append(f"Unfinished: {key!r}")
            elif Counter(PLACEHOLDERS.findall(source)) != Counter(PLACEHOLDERS.findall(translation.text)):
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
    messages = extract()
    if args.extract:
        write_extracted(messages, args.extract)
    if not args.extract_only:
        check(messages)
