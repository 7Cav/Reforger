# Server mod lists

One mod list per scenario, ready for a TCAdmin server's `config.json`.

| File prefix | Server type |
|---|---|
| `SB_` | 7Cav Game Master sandbox (7Cav Maps) |
| `Training_` | Fort Tharen training (BTC, S7) |
| `CE_` | 7Cav Conflict Escalation |

Eden, Arland and Kolguyev run on the base game terrains, so their lists are identical apart from
the scenario. A mod's `"version": ""` means the server takes the latest workshop version.

## Bringing a map up on TCAdmin

**Option A, the tool (no hand editing):**

1. Download the server's `config.json` from the TCAdmin File Manager.
2. Run `python "Map JSON files/library/build_library.py" --apply config.json SB_Kunar`
   (the name is any file in `TCAdmin/`, without `.json`).
3. Read what it prints. If it warns about `missionHeader`, see the note below.
4. Upload the file back and restart from the panel or RCON.

The tool changes `game.scenarioId` and `game.mods` and nothing else. It never prints passwords
or the admin list.

**Option B, by hand in the panel's editor:**

1. Open `TCAdmin/<scenario>.json`.
2. In the server's `config.json`, inside `"game"`, replace the value of `"scenarioId"` with the
   one from the file (keep the comma after it).
3. Replace the whole `"mods"` array, from its `[` to its matching `]`, with the array from the
   file. Do not paste the file's outer `{ }` braces.
4. Save, then restart from the panel or RCON.

Either way, an in-game `#restart` does not reload the server config, so the change will not take
effect until a panel or RCON restart.

**`missionHeader` note.** `game.gameProperties.missionHeader` overrides settings of the scenario
the server runs. Values left over from a different scenario (a COE2 header's `SystemsConfig` and
`m_sCOE_*` keys on a 7Cav sandbox, for example) do not belong to the new one, and a wrong
`SystemsConfig` can stop the scenario's systems from loading. When changing scenario, remove the
header entries that belong to the old one.

## The older launcher presets

The `<Map>.json` files in this folder are launcher presets (Arma Reforger launcher, Mods, Import
preset). They are written from the same source, so they always match the TCAdmin lists.

## Changing a mod list

Never edit the generated files by hand. Edit `library/mods.json`, then run:

    python "Map JSON files/library/build_library.py"

and commit `mods.json` together with every file it rewrote. `--check` writes nothing and exits 1
if any output is stale, or if a file in this folder or `TCAdmin/` is not built from `mods.json`.
Run it before opening a pull request.

`mods.json` holds:

- `stacks`: the mods every server of a type loads. `sandbox` has `core` (frameworks, weapons,
  vehicles, ACE, quality of life), `cav` (the five 7Cav content mods) and `last` (7Cav Maps).
  `conflict_escalation` is the CE list.
- `presets`: one entry per map. `content` holds map-specific gameplay mods (Middle East
  Insurgents, M1 Abrams), `terrain` holds the terrain and its asset packs. `scenarios` names the
  mission confs the list serves.

Every list is built in this order: core, preset content, 7Cav mods, preset terrain, last. Adding
a mod for every sandbox server means adding it once to `stacks.sandbox.core`.

Scenario IDs are read from the mission `.conf.meta` files in this repository, never typed in, so
a renamed mission is picked up on the next build and a deleted one fails it.

## Not covered yet

These scenarios exist in the repository but have no mod list:

- **Mussalo** (`7Cav_SB_Mussalo.conf`): its terrain mod is not recorded anywhere here.
- **7Cav PubMaps** (the COE2 scenarios): waiting on the public server's confirmed list.
- **7CavPVE** (`7Cav_Linear_Cain.conf`, `7Cav_Ruha_PvE.conf`) and **Cav Island**
  (`Island_Rebuild_GM.conf`).
