# 7Cav PubMaps

COE2 persistent operations scenarios built by the 7Cav Dev Team. Each mod carries one or more maps
and depends on COE2 by Kexanone; the terrain mods named below are still required.

| Mod | Maps covered | Scenario IDs | Depends on | Workshop item |
|---|---|---|---|---|
| 7Cav PubMaps | Everon, Kolguyev | `{F82BF6B4A55177AC}Missions/7Cav_COE2_Everon.conf`, `{1877FE0C164445DB}Missions/7Cav_COE2_Kolguyev.conf` | COE2 | (to be published) |
| 7Cav PubMaps Kunar | Kunar | `{C2EE40E4F8288ECC}Missions/7Cav_COE2_Kunar.conf` | COE2, Kunar Province (KIOK) | (to be published) |
| 7Cav PubMaps Anizay | Anizay | `{E99A7CEA41DACE3C}Missions/7Cav_COE2_Anizay.conf` | COE2, Anizay (Temppa) | (to be published) |
| 7Cav PubMaps Novka | Novka | not authored yet | COE2, Novka | (not started) |

## Notes

- One mod per terrain group so a server loads only what it runs. The Kunar and Anizay mods exist
  because those worlds are authored here rather than taken from COE2's own map addons, so those
  servers do not need to download them; the terrains themselves are still required downloads.
- The scenario IDs above are what a server puts in its `CustomMission` setting.
- Credits and licence for each mod are in its own `CREDITS.md` and `LICENSE.md`, and the same notices
  are repeated in the workshop Description, because those two files are not packed into the addon.

## Conventions

Each mod folder carries a tracked `thumbnail.png` at its root, which is what the repository expects and
what the workshop item image comes from. All three currently use the same 7Cav Maps card as the family
card in `misc/mod_cards/7cav_pubmaps.png`; a per-map card can replace them later. The mods' source
folders ignore `thumbnail.png` because a Workbench build writes one, so the tracked copy is the
curated one.
