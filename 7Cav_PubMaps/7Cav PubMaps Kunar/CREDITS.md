# Credits

7Cav PubMaps Kunar is a 7Cav Dev Team release.

## Dependencies, credited

- COE2 and Kex Scenario Core: Kexanone (Kex), APL-SA. The scenario is COE2's, and this mod runs it.
  Source: https://github.com/Kexanone/COE2_AR and https://reforger.armaplatform.com/workshop/60926835F4A7B0CA
- Kunar Province terrain: KIOK, the terrain this scenario sits on and the owner of the navmeshes it
  loads. https://reforger.armaplatform.com/workshop/5C9691EA7FD7A79F

No COE2 file and no terrain file is redistributed here. This addon's world, its layers and its
mission header were written by the 7Cav Dev Team, following COE2's own scenario layout: a sub-scene
over the terrain world, a managers layer, a game mode layer naming the Kex Scenario Core slot config
for that terrain, and a main base. The terrain, its navmeshes, the slot config and the prefabs it
references stay in their own addons and are depended on, not copied.

## What the 7Cav Dev Team wrote

- `worlds/7CavCOE/Kunar/7Cav_COE2_Kunar.ent` and its three layers, L1 to L3
- The mission header, including the 7Cav loading screen.
- The loading screen image itself, taken from 7Cav Assets.

The files are ours; the scenario placement is COE2's. The main base position reproduces the one
COE2's own Kunar scenario uses, carried faithfully rather than re-sited.

## Two upstream references corrected

- The radio manager pair in COE2's layer names the canonical prefab:
  `{B8E09FAB91C4ECCD}Prefabs/Systems/Radio/RadioManager.et`. The GUID was right and the path was
  stale, so this is a repair rather than a change in behaviour.
- A `SCR_BaseTaskManager` entity whose prefab
  (`{F73FF9A9DC8F1B7A}Prefabs/MP/Managers/Tasks/KSC_TaskManager.et`) exists in no installed
  database is not reproduced. It never resolved at runtime, so removing it changes nothing and
  clears an error logged at every world load.

## Licence

No third-party file is redistributed here: the world, its layers and its mission header are written by
the 7Cav Dev Team, and COE2, Kex Scenario Core and the terrain are dependencies rather than copied
content. The scenario placement those files reproduce is COE2's.

Whether reproducing COE2's placement data makes this addon a derivative of COE2 is the unit's call and
not something these notes settle. In the unit repository it sits under the repository licence
(GPL-2.0). The published workshop item carries the Arma Public License Share Alike, which is COE2's own
licence, until the unit rules otherwise:
https://www.bohemia.net/community/licenses/arma-public-license-share-alike

## Compatibility

Compatible with CPO Frontline (JDripstein), the frontier campaign layer for COE2:
https://reforger.armaplatform.com/workshop/61F3A9C24D7B80E5
No FRONTLINE code or content is included in this addon.
