#!/usr/bin/env python3
"""Build the server mod lists from library/mods.json.

    python build_library.py                      write every output
    python build_library.py --check              exit 1 if any output is stale or orphaned
    python build_library.py --apply config.json SB_Kunar
                                                 set scenarioId and mods in a server config.json
    python build_library.py --diff config.json SB_Kunar
                                                 report how a server config.json differs from the standard

mods.json is the only file anyone edits. It holds the shared mod stacks and, per
preset, the extra content and terrain mods that map needs. This script writes two
things from it, so the copies can never drift apart:

  TCAdmin/<scenario>.json   scenarioId plus the mods array, the two keys a TCAdmin
                            server's config.json carries in its "game" block
  <preset>.json             the launcher preset files this folder already held

Load order inside every list is: stack core, preset content, 7Cav mods, preset
terrain, then the stack's "last" mods (7Cav Maps for the sandbox stack).

Scenario IDs are not typed by hand. They are read from the mission .conf.meta
files in this repository, so a renamed or re-GUIDed mission is picked up on the
next build, and a mission that no longer exists fails the build.

--apply edits a config.json you downloaded from the panel: it replaces
game.scenarioId and game.mods, leaves every other value as it was, prints nothing
from the file, and warns if gameProperties.missionHeader is set (those values
belong to the previous scenario). Upload the result and restart from the panel.

A preset may list modIds under "omit" to leave stack mods out of that one list.

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
PRESET_KEYS = ('preset', 'stack', 'scenarios', 'content', 'terrain')


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
    mods = stack['core'] + preset['content'] + stack['cav'] + preset['terrain'] + stack['last']
    omit = preset.get('omit', [])
    if not isinstance(omit, list):
        fail(f"{preset['preset']}: omit must be a list of modIds")
    omit = set(omit)
    unknown = omit - {m.get('modId') for m in mods}
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
    return json.dumps({'scenarioId': scenario_id, 'mods': mods}, indent=4) + '\n'


def render_launcher(name, mods):
    # No trailing newline: matches the launcher's own exports, which this folder has always held.
    body = {'preset_name': name, 'preset_version': 2, 'preset_experimental_branch': False, 'mods': mods}
    return json.dumps(body, separators=(',', ':'))


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
        add(os.path.join(PRESET_DIR, preset['preset'] + '.json'),
            render_launcher(preset.get('preset_name', preset['preset']), mods), preset['preset'])
        for base in preset['scenarios']:
            scenario_id = resolve(ids, preset['preset'], base)
            short = stack.get('file_prefix', '') + (base[len('7Cav_'):] if base.startswith('7Cav_') else base)
            add(os.path.join(TCADMIN_DIR, short + '.json'), render_tcadmin(scenario_id, mods), preset['preset'])
            scenarios[short] = (scenario_id, mods)
    return files, scenarios


def load_config(config_path):
    """Return (config, game) from a server config.json, or exit 2 with a clear message."""
    try:
        with open(config_path, encoding='utf-8-sig') as fh:
            config = json.load(fh)
    except OSError as exc:
        fail(f'cannot read {config_path}: {exc.strerror or exc}')
    except json.JSONDecodeError as exc:
        fail(f'{config_path} is not valid JSON: {exc}')
    game = config.get('game') if isinstance(config, dict) else None
    if not isinstance(game, dict):
        fail(f'{config_path} has no "game" block; is this a server config.json?')
    mods = game.get('mods', [])
    if not isinstance(mods, list) or not all(isinstance(m, dict) for m in mods):
        fail(f'{config_path}: game.mods is not a list of mod entries')
    return config, game


def apply(config_path, name, scenarios):
    if name.endswith('.json'):
        name = name[:-5]
    if name not in scenarios:
        fail(f'unknown scenario {name!r}; pick one of: {", ".join(sorted(scenarios))}')
    config, game = load_config(config_path)
    scenario_id, mods = scenarios[name]
    old_count = len(game.get('mods', []))
    game['scenarioId'] = scenario_id
    game['mods'] = mods
    with open(config_path, 'w', encoding='utf-8', newline='\n') as fh:
        json.dump(config, fh, indent=4)
        fh.write('\n')
    print(f'{config_path}: scenario {scenario_id}, mods {old_count} -> {len(mods)}')
    header = (game.get('gameProperties') or {}).get('missionHeader')
    if header:
        print('warning: gameProperties.missionHeader is set (' + ', '.join(sorted(header)) + ').')
        print('         Those values were written for the previous scenario. Remove any that do not')
        print('         belong to the new one, SystemsConfig above all, before uploading.')
    print('Upload it through the panel, then restart from the panel or RCON (not in-game #restart).')
    return 0


def diff(config_path, name, scenarios):
    """Report how a server config.json differs from the standard list. Reads only scenarioId and mods."""
    if name.endswith('.json'):
        name = name[:-5]
    if name not in scenarios:
        fail(f'unknown scenario {name!r}; pick one of: {", ".join(sorted(scenarios))}')
    _, game = load_config(config_path)
    want_id, want = scenarios[name]
    have = game.get('mods', [])
    have_ids = [m.get('modId') for m in have]
    want_ids = [m['modId'] for m in want]
    names = {m.get('modId'): m.get('name', '?') for m in have + want}
    missing = [i for i in want_ids if i not in have_ids]
    extra = [i for i in have_ids if i not in want_ids]
    pinned = [m for m in have if m.get('version')]
    dupes = sorted({i for i in have_ids if have_ids.count(i) > 1})
    common_have = [i for i in have_ids if i in want_ids]
    common_want = [i for i in want_ids if i in have_ids]
    print(f'{config_path} against {name}')
    if game.get('scenarioId') != want_id:
        print(f'  scenarioId differs: server has {game.get("scenarioId")!r}, standard is {want_id!r}')
    for i in missing:
        print(f'  missing: {names[i]} ({i})')
    for i in extra:
        print(f'  extra:   {names[i]} ({i})')
    for m in pinned:
        print(f'  pinned:  {m.get("name")} ({m["modId"]}) to version {m["version"]}, standard takes latest')
    for i in dupes:
        print(f'  listed {have_ids.count(i)} times: {names[i]} ({i})')
    reorder = not dupes and common_have != common_want
    if reorder:
        print('  order differs from the standard')
    scenario_off = game.get('scenarioId') != want_id
    clean = not (missing or extra or pinned or dupes or reorder or scenario_off)
    if clean:
        print('  matches the standard')
    else:
        parts = [f'{len(missing)} missing', f'{len(extra)} extra']
        parts += [f'{len(pinned)} pinned'] if pinned else []
        parts += [f'{len(dupes)} duplicated'] if dupes else []
        parts += ['order differs'] if reorder else []
        parts += ['wrong scenario'] if scenario_off else []
        print('  ' + ', '.join(parts))
    return 0 if clean else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument('--check', action='store_true', help='report stale or orphaned outputs, write nothing')
    mode.add_argument('--apply', nargs=2, metavar=('CONFIG', 'SCENARIO'),
                      help='set game.scenarioId and game.mods in a downloaded server config.json')
    mode.add_argument('--diff', nargs=2, metavar=('CONFIG', 'SCENARIO'),
                      help='compare a server config.json with the standard list, change nothing')
    args = ap.parse_args()
    files, scenarios = build()
    if args.apply:
        return apply(args.apply[0], args.apply[1], scenarios)
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

    orphans = [p for p in glob.glob(os.path.join(PRESET_DIR, '*.json')) + glob.glob(os.path.join(TCADMIN_DIR, '*.json'))
               if p not in files]
    for path in sorted(orphans):
        print('orphan (not built from mods.json, delete it or add a preset): ' + os.path.relpath(path, PRESET_DIR))
    for path in stale:
        print(('stale: ' if args.check else 'wrote: ') + os.path.relpath(path, PRESET_DIR))
    print(f'{len(files)} files, {len(stale)} {"stale" if args.check else "changed"}, {len(orphans)} orphaned')
    return 1 if (args.check and (stale or orphans)) else 0


if __name__ == '__main__':
    sys.exit(main())
