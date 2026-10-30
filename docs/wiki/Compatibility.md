# Compatibility

## Collision-viewer profiles

The reader selects one of **22 audited executable profiles** using PE timestamp, image size and a code fingerprint. Unknown layouts withhold rendering rather than guess addresses. Selecting an executable in Settings alone does not establish support.

| Family | Recognized versions |
| --- | --- |
| Release | 1.0, 1.1 |
| Updates 2–3 | 2.0, 2.1, 3.0, 3.1 |
| Updates 4–5 | 4.0, 4.1, 5.0, 5.1 |
| Update 6 | 6.0, 6.1, 6.2, 6.3, 6.4 |
| 6.66 | 6.66, Rev 1, Rev 1.1, Rev 2, Rev 2.2, Rev 3, Rev 3.2 |

The native definitions are in [`native_profile.h`](https://github.com/IlyaTsyganov/LAB-Plus/blob/main/Native/CollisionViewer/src/native_profile.h). Profile-specific validation records are in the same source directory. Maps and DLC geometry are discovered from the loaded map and physics state rather than a fixed list of map filenames.

## Feature boundaries

| Feature | Boundary |
| --- | --- |
| Collision, triggers, walls and path drawing | Recognized executable and valid runtime camera/physics reads |
| Native HUD text and console actions | Compatible, already-loaded meathook and validated engine callbacks |
| Native cvar creation | Constructor/register/set ABI audited for **6.66 Rev 3 only** |
| Live checkpoint Save/Load | **6.66 Rev 3 / TAG1 / slot 3**, same Steam account, Cloud off and all live checks passing |
| Offline checkpoint tools | Recognized authenticated containers, same-account and campaign-family/target-slot checks |
| Lighting correction | NVIDIA, versions **1.0, 1.1, 2.0**, launched through LAB Plus |

Meathook is not included and compatibility is not implied for every collision-viewer profile. Native cvar creation is withheld on other versions; the interface and independently available action commands remain usable when their own prerequisites pass.

## Lighting correction

For the three supported early versions, launching through LAB Plus opts the child process into the indirect-barrier correction adapted from [DoomEternal-v1.1--RTXFix](https://github.com/billymooreq/DoomEternal-v1.1--RTXFix). It works even with Collision Viewer disabled. Keep Steam running. The child receives `SteamAppId=782330` and `LABPLUS_LIGHTING_FIX=1`; LAB Plus does not create `steam_appid.txt` or change system environment variables. Other versions and non-NVIDIA devices are excluded.

## Verification limits

The retained native binary passed eleven native fixture suites across 22 profiles and 38 isolated GPU cases (19 normal plus 19 lighting cases). The managed launcher, encrypted checkpoint fixtures, headless screen scenarios and packaged WebView interface were checked for this UI migration. The native DLL is byte-identical to the prior verified local build; those native/GPU tests were not rerun for the final UI-only change.

The newly rebranded layer, encounter HUD and corrected native cvar creation still require in-game acceptance after a full restart. Earlier live checkpoint acceptance on 6.66 Rev 3 / TAG1 slot 3 does not expand that feature to other slots or builds. Elevated launches and every GPU/driver combination have not been verified.
