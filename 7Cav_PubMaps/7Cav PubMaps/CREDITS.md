# Credits

7Cav PubMaps is a 7Cav Dev Team release.

## Upstream

- COE2 and Kex Scenario Core: Kexanone (Kex), APL-SA. The scenario is COE2's, and this mod is built
  to run it.
  Source: https://github.com/Kexanone/COE2_AR and https://reforger.armaplatform.com/workshop/60926835F4A7B0CA
- Everon and Kolguyev are base game terrain, Bohemia Interactive's. Their navmeshes are the game's,
  and no base game terrain file is redistributed here.

No COE2 file is redistributed here. The scenario worlds and their layers were written by the 7Cav
Dev Team, following COE2's own scenario layout: a sub-scene over the terrain world, a managers
layer, a game mode layer naming the Kex Scenario Core slot config for that terrain, and a main base.

## What the 7Cav Dev Team wrote

- `worlds/7CavCOE/Eden/7Cav_COE2_Everon.ent` and its three layers, L1 to L3
- `worlds/7CavCOE/Cain/7Cav_COE2_Kolguyev.ent` and its five layers, L1 to L5, including the custom location and custom slot layers
- The two mission headers, including the 7Cav loading screen.
- The loading screen image itself, taken from 7Cav Assets.

The files are ours; the placement data inside them is COE2's. The main base position reproduces the
one COE2's Eden and Cain scenarios use, and on Kolguyev the custom location polygon in
`L4_custom_locations.layer` and the three helipad positions in `L5_custom_slots.layer` reproduce
COE2's Cain setups. That data is what makes the scenarios the same scenarios, so it is carried
faithfully rather than invented, and it is stated here so the provenance is accurate.

Reference forms in the manager layers follow COE2's own Cain layer, which is its cleanest: the
canonical `Prefabs/Systems/Radio/RadioManager.et`, COE2's own music manager prefab, and no task
manager line. Three lines COE2 still ships on other maps are deliberately not reproduced; see the
project notes for the resolver output that found them.

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
