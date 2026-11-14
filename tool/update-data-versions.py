#!/usr/bin/env python3
"""Regenerate the tail of src/_java-data-versions.hpp.

The list in `JavaDataVersions` maps Minecraft Java Edition versions to their
data version, and is used all over the code base as a threshold
(`dataVersion >= (int)JavaDataVersions::Snapshot25w07a`). It is pure data, so
it is generated here instead of being typed by hand.

Version metadata comes from `misode/mcmeta` (`summary/versions/data.json`),
which is generated from the official launcher version manifest plus the
client's own `version.json`.

Usage:

    python tool/update-data-versions.py                # dry run, show the tail
    python tool/update-data-versions.py --write        # rewrite the header
    python tool/update-data-versions.py --write --to 26.4-snapshot-8
    python tool/update-data-versions.py --write --from 25w41a

`--from` names the first mcmeta version to regenerate; every entry before it is
kept exactly as it is, so old (sometimes historically wrong) values are not
silently rewritten. `--to` names the last version to include.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
HEADER = os.path.join(ROOT, "src", "_java-data-versions.hpp")
CACHE = os.path.join(HERE, "versions-data.json")
SOURCE = "https://raw.githubusercontent.com/misode/mcmeta/summary/versions/data.json"

# By default only regenerate from the first version this project needs on top of
# the previously supported 1.21.10 (data version 4556).
DEFAULT_FROM = "25w41a"

ENTRY_RE = re.compile(r"^\s*(\w+)\s*=\s*(\d+)\s*,?\s*$")


def load_versions(offline: bool) -> list[dict]:
    """Return the mcmeta version list, newest first, as published."""
    if not offline:
        try:
            with urllib.request.urlopen(SOURCE, timeout=30) as res:
                data = res.read()
            with open(CACHE, "wb") as fp:
                fp.write(data)
            return json.loads(data)
        except Exception as exc:  # noqa: BLE001 - fall back to the cache
            print(f"warning: could not fetch {SOURCE} ({exc}); using cache", file=sys.stderr)
    with open(CACHE, "rb") as fp:
        return json.load(fp)


def is_java_release_stream(version: dict) -> bool:
    """Keep release versions and weekly/`year.` snapshots; drop the rest."""
    if version.get("type") not in {"release", "snapshot", "old_beta", "old_alpha"}:
        return False
    vid = version["id"]
    if vid.startswith(("a", "b", "c", "inf", "rd", "pre", "rc", "1.RV", "2.0", "3D")):
        return False
    return bool(re.match(r"^(\d+w\d+[a-z]|\d+\.\d+|\d+\.\d+-)", vid))


def cpp_name(vid: str) -> str:
    """Turn an mcmeta version id into this project's identifier style."""
    # 25w41a -> Snapshot25w41a
    if re.fullmatch(r"\d+w\d+[a-z]", vid):
        return "Snapshot" + vid

    # 26.1-snapshot-3 -> Snapshot26_1s3
    m = re.fullmatch(r"(\d+(?:\.\d+)+)-snapshot-(\d+)", vid)
    if m:
        return f"Snapshot{m.group(1).replace('.', '_')}s{m.group(2)}"

    # 26.1-pre-2 -> PreRelease26_1pre2
    m = re.fullmatch(r"(\d+(?:\.\d+)+)-pre-?(\d+)", vid)
    if m:
        return f"PreRelease{m.group(1).replace('.', '_')}pre{m.group(2)}"

    # 26.1-rc-2 -> ReleaseCandidate26_1rc2
    m = re.fullmatch(r"(\d+(?:\.\d+)+)-rc-?(\d+)", vid)
    if m:
        return f"ReleaseCandidate{m.group(1).replace('.', '_')}rc{m.group(2)}"

    # 26.1.2 -> Release26_1_2
    if re.fullmatch(r"\d+\.\d+(\.\d+)*", vid):
        return "Release" + vid.replace(".", "_")

    raise ValueError(f"unrecognised version id {vid!r}")


def build_tail(versions: list[dict], first: str, last: str | None) -> list[tuple[str, int]]:
    ordered = sorted(versions, key=lambda v: v["data_version"])
    ids = [v["id"] for v in ordered]
    if first not in ids:
        raise SystemExit(f"error: {first!r} is not a known version id")
    start = ids.index(first)
    stop = len(ordered) if last is None else ids.index(last) + 1
    if first not in ids or stop < start:
        raise SystemExit("error: bad --from/--to range")

    tail: list[tuple[str, int]] = []
    seen: dict[int, str] = {}
    for version in ordered[start:stop]:
        if not is_java_release_stream(version):
            continue
        name = cpp_name(version["id"])
        value = version["data_version"]
        # A couple of historical versions share a data version (e.g. 1.21.6-rc1);
        # the enum keeps only the first so values stay unambiguous.
        if value in seen:
            print(f"note: skipping {version['id']} ({value}), same as {seen[value]}", file=sys.stderr)
            continue
        seen[value] = version["id"]
        tail.append((name, value))
    return tail


def render(path: str, first: str, tail: list[tuple[str, int]]) -> str:
    with open(path, "r", encoding="utf-8", newline="") as fp:
        raw = fp.read()
    newline = "\r\n" if "\r\n" in raw else "\n"
    lines = raw.split(newline)

    first_name = cpp_name(first)
    first_value = tail[0][1]

    prefix: list[str] = []
    kept: list[str] = []
    suffix: list[str] = []
    state = "prefix"
    for line in lines:
        if state == "suffix":
            suffix.append(line)
            continue

        match = ENTRY_RE.match(line)
        if state == "prefix":
            if match and (match.group(1) == first_name or int(match.group(2)) >= first_value):
                # Everything from here on is regenerated.
                state = "cut"
            else:
                prefix.append(line)
            continue

        # state == "cut": skip entries that are being regenerated, and resume
        # copying at the first line that is not one of them.
        if not match:
            state = "suffix"
            suffix.append(line)

    if state == "prefix":
        raise SystemExit(f"error: {first!r} not found in {path}; nothing to splice from")

    body = [f"  {name} = {value}," for name, value in kept + tail]
    return newline.join(prefix + body + suffix)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--from", dest="first", default=DEFAULT_FROM)
    parser.add_argument("--to", dest="last", default=None)
    parser.add_argument("--write", action="store_true", help="rewrite the header in place")
    parser.add_argument("--offline", action="store_true", help="use the cached version list")
    args = parser.parse_args()

    versions = load_versions(args.offline)
    tail = build_tail(versions, args.first, args.last)
    if not tail:
        raise SystemExit("error: nothing to write")

    out = render(HEADER, args.first, tail)
    if args.write:
        with open(HEADER, "w", encoding="utf-8", newline="") as fp:
            fp.write(out)
        print(f"wrote {len(tail)} versions to {HEADER}")
    else:
        print("\n".join(f"  {name} = {value}," for name, value in tail))
        print(f"\n({len(tail)} versions; pass --write to update {HEADER})", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
