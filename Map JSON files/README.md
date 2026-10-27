# Server mod lists

One mod list per scenario in `TCAdmin/`: `SB_` for the 7Cav Maps sandboxes, `COE2_` for the TACR
public COE2 servers (7Cav PubMaps), `Training_` for Fort Tharen, `CE_` for Conflict Escalation.
Every sandbox runs the same standard stack (81 mods, same order) plus only its terrain mods, with
7Cav Maps last; Fort Tharen keeps its earlier list. The COE2 lists share their own smaller stack
(42 mods: the TACR server's own list plus Enhanced Radio). `"version": ""` means the latest
workshop version. Which sandbox maps are confirmed working is tracked in [MAPS.md](MAPS.md).

## Bringing a map up on TCAdmin

1. Open `TCAdmin/<scenario>.json`.
2. In the server's `config.json`, inside `"game"`, replace the `"scenarioId"` value and the whole
   `"mods": [ ... ]` array with the ones from the file. Leave everything else alone.
3. Check `game.gameProperties.missionHeader`. COE2 servers need theirs (`SystemsConfig`, the
   `m_sCOE_*` faction keys) and keep it when moving between COE2 maps. When a server moves to a
   sandbox, remove those entries: a wrong `SystemsConfig` can stop the scenario's systems loading.
4. Restart from the panel or RCON. An in-game `#restart` does not reload the config.

To see how far a live server has drifted, download its `config.json` and run
`python "Map JSON files/library/build_library.py" --diff config.json SB_Kunar`. It lists missing,
extra, duplicated and pinned mods, order and scenario, and changes nothing. Mods a listed mod
depends on (M157 for 7Cav Assets, for example) are downloaded by the server itself and need no
listing.

## Changing a mod list

Edit `library/mods.json`, never the generated files, then run
`python "Map JSON files/library/build_library.py"` and commit everything it rewrote. `--check`
writes nothing and fails on stale or stray files; run it before a pull request.

In `mods.json`, a stack's `core` is what every server of that type loads and `last` loads after
the terrain. Each preset names its stack, the mission confs it serves (`scenarios`, whose IDs are
read from the mission metas), its `terrain` mods, and optionally `omit`, stack mods to leave out.
To add a mod to every sandbox, add it once to `stacks.sandbox.core`.

## Not covered yet

7CavPVE (`7Cav_Linear_Cain`, `7Cav_Ruha_PvE`) and Cav Island (`Island_Rebuild_GM`) have
scenarios but no mod list.
