#!/usr/bin/env python3

import argparse
import glob
import json
import os
import sys

UNSUPPORTED_PATHS = {
    "Video@@Renderer",
}

BOOL_ENUMS = {
    "Video@@Frame limit": {False: "Off", True: "Auto"},
}

NULL_ENUMS = {
    "Input/Output@@Mouse": "Null",
    "Input/Output@@Move": "Null",
    "Input/Output@@Keyboard": "Null",
    "Input/Output@@Camera": "Null",
    "Audio@@Microphone Type": "Null",
}


def flatten(node, prefix=""):
    out = {}
    for key, value in node.items():
        path = f"{prefix}@@{key}" if prefix else key
        if isinstance(value, dict):
            out.update(flatten(value, path))
        else:
            out[path] = value
    return out


def normalize(path, value):
    if path in UNSUPPORTED_PATHS:
        return None
    if value is None:
        replacement = NULL_ENUMS.get(path)
        return replacement if replacement is not None else None
    if isinstance(value, bool) and path in BOOL_ENUMS:
        return BOOL_ENUMS[path][value]
    if isinstance(value, list):
        entries = [str(item) for item in value if str(item)]
        return entries or None
    return value


def build(source_dir):
    configs = []
    seen = {}
    games = {}

    for path in sorted(glob.glob(os.path.join(source_dir, "*.json"))):
        record = json.load(open(path, encoding="utf-8"))
        game_id = record.get("game_id")
        if not game_id:
            continue

        merged = (record.get("configuration") or {}).get("merged_config")
        settings = {}
        if merged:
            for key, value in flatten(merged).items():
                normalized = normalize(key, value)
                if normalized is not None:
                    settings[key] = normalized

        if not settings:
            games[game_id] = -1
            continue

        fingerprint = json.dumps(settings, sort_keys=True, separators=(",", ":"))
        index = seen.get(fingerprint)
        if index is None:
            index = len(configs)
            seen[fingerprint] = index
            configs.append(settings)
        games[game_id] = index

    return {"version": 1, "configs": configs, "games": games}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    if not os.path.isdir(args.source):
        print(f"missing source directory: {args.source}", file=sys.stderr)
        return 1

    payload = build(args.source)
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        json.dump(payload, handle, sort_keys=True, separators=(",", ":"))
        handle.write("\n")

    listed = len(payload["games"])
    tuned = sum(1 for index in payload["games"].values() if index >= 0)
    print(
        f"{listed} title ids, {tuned} with tuned settings, "
        f"{len(payload['configs'])} distinct configs, "
        f"{os.path.getsize(args.out)} bytes"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
