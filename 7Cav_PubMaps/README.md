# 7Cav PubMaps

COE2 persistent operations scenarios built by the 7Cav Dev Team. Each mod carries one or more maps
and depends on COE2 by Kexanone; the terrain mods named below are still required.

| Mod | Maps covered | Scenario IDs | Depends on | Workshop item |
|---|---|---|---|---|
| 7Cav PubMaps | Everon, Kolguyev | `{23027A8302DB3141}Missions/7Cav_COE2_Everon.conf`, `{47F6D7364F506C1E}Missions/7Cav_COE2_Kolguyev.conf` | COE2 | (to be published) |
| 7Cav PubMaps Kunar | Kunar | `{253697A390FBEAC2}Missions/7Cav_COE2_Kunar.conf` | COE2, Kunar Province (KIOK) | (to be published) |
| 7Cav PubMaps Anizay | Anizay | `{742D69423787D10D}Missions/7Cav_COE2_Anizay.conf` | COE2, Anizay (Temppa) | (to be published) |
| 7Cav PubMaps Novka | Novka | not authored yet | COE2, Novka | (not started) |

## Notes

- One mod per terrain group so a server loads only what it runs. The Kunar and Anizay mods exist
  because those worlds are authored here rather than taken from COE2's own map addons, so those
  servers do not need to download them; the terrains themselves are still required downloads.
- The scenario IDs above are what a server puts in its `CustomMission` setting.
- Credits and licence for each mod are in its own `CREDITS.md` and `LICENSE.md`, and the same notices
  are repeated in the workshop Description, because those two files are not packed into the addon.
