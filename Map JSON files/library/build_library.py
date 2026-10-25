#!/usr/bin/env python3
"""Build the server mod lists from library/mods.json.

    python build_library.py                            write every output
    python build_library.py --check                    exit 1 if an output is stale or orphaned
    python build_library.py --diff config.json SB_Kunar
                                                       compare a server config.json with the standard

mods.json is the only file anyone edits. Each list is its stack's core, then the
preset's terrain mods, then the stack's last mods (7Cav Maps for sandboxes), minus
any modIds the preset names under "omit". It writes TCAdmin/<scenario>.json: the
scenarioId and mods keys of a TCAdmin config.json, applied to the server by hand.

Scenario IDs are read from the mission .conf.meta files in this repository, so a
renamed mission is picked up and a deleted one fails the build.

Exit codes: 0 ok, 1 stale or orphaned outputs (--check) or drift (--diff), 2 bad input.
Stdlib only.
"""
import argparse
import glob
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
PRESET_DIR = os.path.dirname(HERE)
REPO = os.path.dirname(PRESET_DIR)
TCADMIN_DIR = os.path.join(PRESET_DIR, 'TCAdmin')
MOD_ID = re.compile(r'^[0-9A-F]{16}$')
META_NAME = re.compile(r'Name\s+"(\{[0-9A-F]{16}\}[^"]*missions/([^"/]+)\.conf)"', re.IGNORECASE)
PRESET_KEYS = ('preset', 'stack', 'scenarios', 'terrain')


def fail(message):
    print('error: ' + message, file=sys.stderr)
    sys.exit(2)


def mission_ids():
    """Map mission conf basename -> set of scenario IDs found under that name in the repo."""
    found = {}
    for meta in glob.glob(os.path.join(REPO, '**', '*.conf.meta'), recursive=True):
        with open(meta, encoding='utf-8-sig') as fh:
            m = META_NAME.search(fh.read())
        if m:
            found.setdefault(m.group(2), set()).add(m.group(1))
    return found


def resolve(ids, preset_name, base):
    """A referenced mission must exist exactly once; unreferenced duplicates are not our business."""
    matches = ids.get(base, set())
    if not matches:
        fail(f'{preset_name}: no mission named {base}.conf in the repo')
    if len(matches) > 1:
        fail(f'{preset_name}: {base}.conf exists more than once: {", ".join(sorted(matches))}')
    return next(iter(matches))


def mod_list(stack, preset):
    mods = stack['core'] + preset['terrain'] + stack['last']
    omit = preset.get('omit', [])
    if not isinstance(omit, list):
        fail(f"{preset['preset']}: omit must be a list of modIds")
    unknown = set(omit) - {m.get('modId') for m in mods}
    if unknown:
        fail(f"{preset['preset']}: omit names mods not in its list: {', '.join(sorted(unknown))}")
    mods = [m for m in mods if m.get('modId') not in omit]
    seen = set()
    for m in mods:
        if not MOD_ID.match(m.get('modId', '')):
            fail(f"{preset['preset']}: bad modId {m.get('modId')!r} ({m.get('name')})")
        if not m.get('name'):
            fail(f"{preset['preset']}: modId {m['modId']} has no name")
        if m['modId'] in seen:
            fail(f"{preset['preset']}: {m['name']} ({m['modId']}) is listed twice")
        seen.add(m['modId'])
    # An empty version string means the server takes the latest workshop version.
    return [{'modId': m['modId'], 'name': m['name'], 'version': ''} for m in mods]


def render_tcadmin(scenario_id, mods):
    # One mod per line, so the file scans and pastes easily; still valid JSON.
    rows = ',\n'.join('        ' + json.dumps(m) for m in mods)
    return f'{{\n    "scenarioId": {json.dumps(scenario_id)},\n    "mods": [\n{rows}\n    ]\n}}\n'


def load_library():
    with open(os.path.join(HERE, 'mods.json'), encoding='utf-8-sig') as fh:
        try:
            lib = json.load(fh)
        except json.JSONDecodeError as exc:
            fail(f'library/mods.json is not valid JSON: {exc}')
    for i, preset in enumerate(lib.get('presets', [])):
        missing = [k for k in PRESET_KEYS if k not in preset]
        if missing:
            fail(f"preset #{i} ({preset.get('preset', '?')}) is missing {', '.join(missing)}")
        if preset['stack'] not in lib.get('stacks', {}):
            fail(f"{preset['preset']}: unknown stack {preset['stack']!r}")
    return lib


def build():
    """Return (files, scenarios): output path -> text, and TCAdmin name -> (scenarioId, mods)."""
    lib = load_library()
    ids = mission_ids()
    files, scenarios = {}, {}

    def add(path, text, owner):
        if path in files:
            fail(f'{owner} would overwrite {os.path.relpath(path, PRESET_DIR)}, already built by another preset')
        files[path] = text

    for preset in lib['presets']:
        stack = lib['stacks'][preset['stack']]
        mods = mod_list(stack, preset)
        for base in preset['scenarios']:
            scenario_id = resolve(ids, preset['preset'], base)
            short = stack.get('file_prefix', '') + (base[len('7Cav_'):] if base.startswith('7Cav_') else base)
            add(os.path.join(TCADMIN_DIR, short + '.json'), render_tcadmin(scenario_id, mods), preset['preset'])
            scenarios[short] = (scenario_id, mods)
    return files, scenarios


def diff(config_path, name, scenarios):
    """Report how a server config.json differs from the standard list. Reads only scenarioId and mods."""
    name = name[:-5] if name.endswith('.json') else name
    if name not in scenarios:
        fail(f'unknown scenario {name!r}; pick one of: {", ".join(sorted(scenarios))}')
    try:
        with open(config_path, encoding='utf-8-sig') as fh:
            config = json.load(fh)
    except (OSError, json.JSONDecodeError) as exc:
        fail(f'cannot read {config_path}: {exc}')
    game = config.get('game') if isinstance(config, dict) else None
    have = game.get('mods') if isinstance(game, dict) else None
    if not isinstance(have, list) or not all(isinstance(m, dict) for m in have):
        fail(f'{config_path} has no game.mods list; is this a server config.json?')
    want_id, want = scenarios[name]
    have_ids = [m.get('modId') for m in have]
    want_ids = [m['modId'] for m in want]
    names = {m.get('modId'): m.get('name', '?') for m in have + want}
    problems = []
    if game.get('scenarioId') != want_id:
        problems.append(f'scenarioId is {game.get("scenarioId")!r}, standard is {want_id!r}')
    problems += [f'missing: {names[i]} ({i})' for i in want_ids if i not in have_ids]
    problems += [f'extra:   {names[i]} ({i})' for i in have_ids if i not in want_ids]
    problems += [f'listed {have_ids.count(i)} times: {names[i]} ({i})' for i in sorted(set(have_ids))
                 if have_ids.count(i) > 1]
    problems += [f'pinned:  {m.get("name")} ({m.get("modId")}) to {m["version"]}' for m in have if m.get('version')]
    if len(set(have_ids)) == len(have_ids) and [i for i in have_ids if i in want_ids] != [i for i in want_ids if i in have_ids]:
        problems.append('order differs from the standard')
    print(f'{config_path} against {name}')
    for p in problems or ['matches the standard']:
        print('  ' + p)
    return 1 if problems else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument('--check', action='store_true', help='report stale or orphaned outputs, write nothing')
    mode.add_argument('--diff', nargs=2, metavar=('CONFIG', 'SCENARIO'),
                      help='compare a server config.json with the standard list, change nothing')
    args = ap.parse_args()
    files, scenarios = build()
    if args.diff:
        return diff(args.diff[0], args.diff[1], scenarios)

    stale = []
    for path, text in sorted(files.items()):
        current = None
        if os.path.exists(path):
            with open(path, encoding='utf-8-sig') as fh:
                current = fh.read()
        if current == text:
            continue
        stale.append(path)
        if not args.check:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, 'w', encoding='utf-8', newline='\n') as fh:
                fh.write(text)

    orphans = [p for p in glob.glob(os.path.join(TCADMIN_DIR, '*.json')) if p not in files]
    for path in sorted(orphans):
        print('orphan (not built from mods.json, delete it or add a preset): ' + os.path.relpath(path, PRESET_DIR))
    for path in stale:
        print(('stale: ' if args.check else 'wrote: ') + os.path.relpath(path, PRESET_DIR))
    print(f'{len(files)} files, {len(stale)} {"stale" if args.check else "changed"}, {len(orphans)} orphaned')
    return 1 if (args.check and (stale or orphans)) else 0


if __name__ == '__main__':
    sys.exit(main())
