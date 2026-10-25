# Sandbox map status

Status of every 7Cav Maps sandbox as of **25 October 2026**, game version 1.8. Update the table
whenever a map is tested; a row without a test date is a map nobody has confirmed.

## What "working" means

A map is **working** once a test on a server running its `TCAdmin/` list confirms all of:

1. It loads and players can spawn with the preset kits.
2. Radio voice works between two players on the same frequency.
3. A Cav AN/PRC-77 (backpack or FILBE) deploys and players can spawn on it.
4. A player can save a kit at an arsenal and spawn with it from the deploy menu.
5. A command truck spawn point works.

**Repo checks** are what can be confirmed without a server, and they pass on every map below:
mission header present, world file present, 7Cav sandbox game mode, a radio manager in the world
layer, and a standard mod list in `TCAdmin/`. Passing them means nothing in our own files stops
the map; it does not prove the terrain or the session works.

## Status

| Map | Scenario | Terrain mod (built for game) | Last seen loading | Status |
|---|---|---|---|---|
| Eden (Everon) | `SB_Eden` | base game | 18 Oct 2026, client | loads; full test pending |
| Faircroft Islands | `SB_FaircroftIslands` | Faircroft Islands (1.8) | 25 Oct 2026, client | loads; full test pending |
| Bergamo | `SB_Bergamo` | Bergamo byHeine WIP (1.8) | 25 Oct 2026, server | loaded but no radio worked; radio manager added since, retest |
| Anizay | `SB_Anizay` | Anizay (1.8) | 27 Sep 2026, client | loads; full test pending |
| Kunar | `SB_Kunar` | KunarProvince (1.7) | 2 Sep 2026, client | loads; full test pending |
| AlHadra | `SB_AlHadra` | AL Hadra (1.7) | 25 Aug 2026, client | loads; full test pending |
| Worthy Islands | `SB_WorthyIslands` | Worthy Islands (1.8) | 23 Aug 2026, client | loads; full test pending |
| Ruha | `SB_Ruha` | Ruha (1.8) | 25 Jul 2026, client (before 1.8) | untested on 1.8 |
| Arland | `SB_Arland` | base game | none found | untested |
| Kolguyev | `SB_Kolguyev` | base game | none found | untested |
| Fallujah | `SB_Fallujah` | Fallujah byHeine (1.8) | none found | untested; MEI added to its list since |
| Serhiivka | `SB_Serhiivka` | WCS_Serhiivka (1.8) | none found | untested |
| Mussalo | `SB_Mussalo` | Mussalo (1.8) | none found | untested; new mod list |
| Zimnitrita | `SB_Zimnitrita` | Zimnitrita (1.8) | none found | untested |
| Gogland | `SB_Gogland` | Gogland (1.7) | none found | untested |
| GulfCoast Islands | `SB_GulfCoastIslands` | Gulfcoast Islands (1.7) | none found | untested |
| Glenwood | `SB_Glenwood` | Glenwood (1.7) | none found | untested |
| Novka | `SB_Novka` | Novka (1.7) | none found | untested |
| Takistan | `SB_Takistan` | Takistan (1.7) | none found | untested |
| Hamland | `SB_Hamland` | Hamland (1.6) | none found | untested |
| Denali | `SB_Denali` | Denali Base (1.4) | none found | untested |
| Woodbury | `SB_Woodbury` | Woodbury (1.4) | none found | untested; world parent `Worlds/unnamed.ent` not yet matched to the terrain |
| Backwoods NC | `SB_BackwoodsNC` | North Carolina Back Country (1.4) | none found | untested |

"Last seen loading" comes from the console logs available on 25 October 2026 (one client's logs
and the Bergamo server log), so a map missing here may well have run elsewhere. "Built for game"
is the game version the terrain was last published against on the workshop. No terrain is
blocked, private or marked obsolete. Terrains built on 1.4 to 1.6 have not been republished for
two or more game versions; they are the most likely to fail on load, so test them first.

Fort Tharen (`Training_FtTharen`) is out of scope for this pass and is not tracked here.

## Testing a map

1. On the test server, apply its list: `python "Map JSON files/library/build_library.py" --apply
   config.json SB_<Map>`, upload, restart from the panel.
2. Run the five checks above with two players.
3. Keep the server `console.log`. A failure is far quicker to diagnose with it.
4. Record the result and date in the table.

A note on one log line: `World doesn't contain RadioManagerEntity to support any
BaseRadioComponent` appears at load on every map, including ones whose radios work. It fires
when a terrain's own radio tower loads before our world layer's radio manager, so it is not on
its own evidence that a map's radios are broken.
