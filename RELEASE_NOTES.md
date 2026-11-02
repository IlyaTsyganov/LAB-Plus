# LAB Plus 1.0

Updated Windows x64 release of LAB Plus with the latest fixes and stability improvements.

Default settings: Collision Viewer is off; all three Geometry categories and all trigger types are enabled; view distance is 40 metres.

Extract the entire archive into one folder and run **LABPlus.exe**. Requires **.NET 10 Desktop Runtime x64** and **Microsoft Edge WebView2 Evergreen Runtime**. Close the previous LAB Plus instance and restart the game through LAB Plus after updating.

The release includes **SHA256SUMS** for archive verification.

---

# LAB Plus 0.8.21

This release includes the patch compatibility and fixes developed since 0.8.12.

## Patch support and timers

- Adds audited observation, health, timer and BATS profiles for all 22 recognized executable builds: 1.0, 1.1, 2.0, 2.1, 3.0, 3.1, 4.0, 4.1, 5.0, 5.1, 6.0, 6.1, 6.2, 6.3, 6.4, 6.66, and 6.66 Rev 1, Rev 1.1, Rev 2, Rev 2.2, Rev 3 and Rev 3.2.
- Corrects Praetor Token, ordinary and Golden Gore Nest, and other sync-interaction countdowns. Player lock follows the actual player-release animation event instead of the longer NPC clip. Pause, ownership release and unsupported event metadata remain guarded.
- Adapts health component layouts, game clocks and BATS calculation checks per executable. Updates managed player/HUD/velocity layouts and the missing base 6.66 metrics profile.

## Launch ownership and HUD

- Enables LAB Plus only for the exact game process launched through LAB Plus. An external Steam shortcut no longer acquires its HUD or native layers; saving settings no longer registers a persistent implicit Vulkan layer.
- Extends **Hide AMD driver line** to CPU and GPU labels from any manufacturer, while preserving unrelated DESRU text.
- Places LAB Plus indicators and changed cheat cvars above the original AMD driver row when that row is visible. Preserves its original font and formatting arguments.

## Install

Extract the entire Windows x64 ZIP into one folder and run **LABPlus.exe**. Requires **.NET 10 Desktop Runtime x64** and **Microsoft Edge WebView2 Evergreen Runtime**. Keep the selected executable's required outbound firewall block. Close the previous companion and fully restart the game through LAB Plus after replacing this build's native DLLs or HUD adapter. Meathook is not bundled.

The release includes `SHA256SUMS` for the program ZIP.

## Validation and scope

All 23 native regression suites and launcher checks passed locally. The latest patch checks passed 239 managed assertions across the 12 currently installed executable layouts; earlier patch audits cover the other listed builds. Retail executable audits inspect code and reflection as data without executing the games. The release workflow rebuilds the native layers and launcher, runs the public fixtures, and checks the exact 29-file runtime ZIP before publication.

Per-patch data audits and synthetic fixtures establish adapter compatibility; they do not replace live gameplay and visual acceptance for every feature on every patch. Existing feature-specific restrictions, including early NVIDIA Lighting Fix and checkpoint loading, still apply. See `docs/validation/` for the audit scope and remaining live checks.


# LAB Plus 0.8.10

- On game 1.1, the verified Praetor Suit Point interaction now shows Player lock through the authored release event in the player's model timeline. The continuing NPC clip no longer determines this countdown.
- Read the event's identity, frame and playback rate from current game data. The demonstrated pickup releases at about 3.133 seconds while the full NPC clip lasts 6.633 seconds; these are observations, not hardcoded durations.
- Hide the lock countdown after the player's sync binding clears. Preserve current-clip timers for other interactions and earlier supported profiles.
- Guard the game-time domain and bound model, leaf and animation-event scanning.
- Add detailed continuation context and a checked Windows build/release workflow.

# LAB Plus 0.8.9 (local)

- Adapt the scaled game clock, timer lifetime override, BATS calculation and GK stagger source to the audited 1.1 executable.
- Read non-melee interaction syncs from the active interactable component, alongside player/melee/mechanic references. Display separate current-clip countdowns for independent active syncs, deduplicate shared references and keep Mantle separate.
- Stop copying a glory-kill countdown into the generic interaction row merely because the player's body interaction flag is set.
- Keep unsupported animation clocks, loops and ambiguous clip timelines unavailable; the current-clip countdown is not presented as the duration of an entire scripted sequence.

Validation: 21 native suites, 24 isolated GPU cases, managed launcher/settings/protocol checks and the managed UI build passed. A combined read-only production capture on the running 1.1 game resolved all metadata, returned three living enemies, nine timer rows and ordinary BATS inactive; the companion parser accepted the actual native snapshot. Exact 1.1 executable/process code and source layouts were audited. Active BATS, equipment cooldowns, powerups and non-melee syncs were covered by fixtures; new positive in-game demonstrations of those actions on 1.1 remain pending. The optional interaction watch recorded idle only.

# LAB Plus 0.8.8 (local)

- Add separate countdowns beside Sticky Bomb for each missing charge, in restoration order. During an empty-magazine full reload, all missing charges share its actual deadline.
- Display verified maximum durations beside countdowns, including the stagger glory-kill window. Unknown total durations stay unspecified.
- Keep checking BATS through ordinary melee using the actual previous weapon and current animation rate; a real rate reset immediately clears the active state.
- Render the research Debug HUD across the full viewport, with one font size, wrapping and automatic columns. Remove fixed enemy/timer/encounter row truncation and preserve all Volume inspector and Support plane labels while no target/contact is available.
- Use EternalUi2Regular for enemy HP, GK and the research Debug HUD, with an embedded antialiased glyph atlas and one quad per glyph.

Validation: 20 native suites, 24 isolated GPU cases and managed launcher/settings/protocol checks passed; the managed UI build passed. GPU readback verifies full-viewport text below the former 480 boundary, hidden/console states, font-atlas HP/GK rendering and depth occlusion. A read-only 1.0 inspection confirmed Sticky maximum durations and the actual Meathook/cvar visibility ABI. Melee BATS transitions were checked with fixtures; a new live punch demonstration and visual acceptance of the combined build remain pending after a full game restart.

# LAB Plus 0.8.7 (local)

- Expand BATS detection using each weapon's configured normal raise speed, including its upgrade and clamp settings. Positive game demonstrations cover Ballista and Rocket Launcher in 1.0; unresolved animation resource cases stay unavailable.
- Activate Fast DBlade at the prepared Arbalest hit, including before selecting Destroyer Blade and while another weapon is held. Ordinary mastered Arbalest recharge and a completed ordinary mod switch are excluded. Consumption and player/map/weapon identity changes clear the state.
- Raise enemy panels, add solid green/yellow/red health fill and current/max HP numerals, and show a golden countdown during the actual stagger glory-kill window.
- Include living AI outside active encounter lists in world HP panels while preserving exact encounter membership in the table.
- Check real item/mod ownership before showing equipment readiness. Uncollected equipment is Not acquired; unknown ownership stays unavailable.

Validation: 19 native suites, 21 isolated GPU cases and managed launcher/settings/protocol checks passed. GPU readback covers complete health-panel fill/text and scene-depth occlusion. Read-only game 1.0 demonstrations confirmed ordinary and prepared Arbalest hits, Fast DBlade consumption, Rocket Launcher BATS, collected/uncollected equipment and the actual Imp stagger deadline. The assembled new build still requires a full game restart and visual in-game acceptance; isolated rendering tests are not that acceptance.

# LAB Plus 0.8.6 (local)

- Add Research / Invisible numbers with living enemies grouped by active encounter, current/max HP and optional world HP bars that follow validated live enemy positions.
- Add equipment and powerup countdowns, separate Sticky Bomb next-charge/full-reload timers, and current sync-animation clip countdowns. Unsupported sources are marked unavailable.
- Add observed BATS and Fast DBlade states. BATS initially supports Ballista in game 1.0; Fast DBlade reads and clears with the game's actual one-shot charge override.
- Add labplus_enemies, labplus_enemy_hp_bars, labplus_timers and labplus_techs (0/1). New HUD blocks share the existing DebugHUD text scale and only sample enabled features or an open tool window.

Validation: 18 native suites, 19 isolated GPU cases, launcher/settings/protocol checks and the managed UI build passed. The actual 1.0 native snapshot was accepted by the companion parser. User demonstrations confirmed BATS activation/reset/repeat, Fast DBlade preparation/consumption, and both Sticky Bomb recharge modes. Current live enemy capture has no active encounters; combat HP-bar observation and a positive live sync-clip observation remain pending.

# LAB Plus 0.8.5 (local)

- Rename the launcher to `LABPlus.exe`.
- Keep Help focused on controls and practical instructions.
- Simplify the missing-firewall-rule message as requested.
- Display actual noclip, notarget and god player states alongside changed cheat cvars, including argument-free engine toggles.
- Stack Volume inspector, Support plane and Active encounters at the upper left without gaps from hidden blocks.
- Add `labplus_debughud_scale <0.5–3>` (default 1; no argument queries the value). Text size and stacked spacing scale together and persist between runs.

Validation: 310 HUD checks with player-state layouts from four real EXEs, managed firewall/scale/persistence checks, 14 native suites and 19 isolated GPU cases passed. Layout fixtures cover all visibility combinations, four scales and both callback registration orders.

# LAB Plus 0.8.4 (local)

- Correct 5.1 cvar filtering with a verified catalog of menu/profile/graphics-preset variables and automatic renderer state. These values remain hidden even when loaded or changed after the startup baseline. Runtime modified flags are not used to classify menu ownership.
- Keep true debug/cheat variables, including infinite ammo, timescale and difficulty-specific damage controls, eligible for display.
- Reduce cvar text to 75% of the indicator font size, with matching page capacity and unchanged MR/metrics sizing. Fully restart the game to load the updated native metrics adapter.
- Read-only inspection of the user's running 5.1 process reproduced 29 false rows; the corrected backend returned zero. Regression coverage uses those actual names and flags, late preference loading and subsequent menu changes.

Validation: 286 HUD checks including executed smaller-font callbacks, font restoration, all observed false names, and metadata for four real executables. The collision DLL is unchanged from the verified 0.8.3 build.

# LAB Plus 0.8.3 (local)

- Use the original DESRU-Enhanced cheat-cvar session display filter: exclude initial nondefault preferences, including their later changes, and retain the baseline on companion reconnection to the same running game.
- Correct native appearance mapping: World collision uses World color/opacity, Triggers use Trigger color/opacity, and Invisible walls use Wall color/opacity. Preserve existing custom preferences.

Validation: 238 HUD checks with four real executable layouts, all 13 native suites, 19 GPU cases, and focused browser checks with different colors/opacities for every category passed.

# LAB Plus 0.8.2 (local)

- Keep only M (Q/E + Modbot input fix) and R (red-screen opacity other than 100%) in the LAB Plus indicator.
- Show nondefault cheat-protected cvars and current values directly below that line, using factory defaults from the game's registry. Long lists use numbered six-second pages. Monitoring is read-only and independent of cheat-session startup values.
- Require an effective outbound block for the exact executable in both launch paths, including with all optional layers disabled; recheck immediately before starting the game. Missing rules, disabled filtering services and restricted coverage refuse launch.
- Release the Vulkan tracking mutex before the driver's presentation wait, allowing other rendering threads to keep recording commands.
- Cache immutable world-geometry transforms with a bounded 32 MiB cache, preserving geometry, colors and draw order. A Fortress-sized CPU fixture reduced warm preparation from approximately 2.52–2.59 ms to 2.05–2.10 ms; this is not an in-game FPS measurement.

Validation: 232 HUD checks with metadata from four real executables, managed launcher/firewall fixtures, all 13 native suites across 22 profile fixtures and 19 isolated Vulkan GPU readback cases passed. The GPU tests include concurrent/exclusive queue presentation, depth occlusion and console clipping.

Restart DOOM Eternal through this version after updating to load the new native layer and metrics adapter.

# LAB Plus 0.8.1 (local)

- Restore joint collision/lighting discovery: the child Vulkan search includes every owned manifest.
- Lighting opts in only for audited 1.0, 1.1 and 2.0 executables on NVIDIA; AMD, Intel, unknown GPUs and other versions remain off.
- Embed the full-color red/gold UI emblem in all seven EXE icon sizes.
- Match the LAB folder button, add the header separator, place Types beneath its trigger switch, and start child-window focus in content.
- Remove references to the unimplemented save-state project.
- Cover actual Vulkan loader composition, 22 profile fixtures, 19 isolated GPU cases, HUD functions and all tool windows.

# LAB Plus 0.8.0 — Slayer Workspace

First LAB Plus release with the approved red/gold Plus emblem and fully English Slayer Workspace interface.

- Full product and runtime rebrand: `labplus.exe`, `labplus-layer.dll`, `labplus_` commands/cvars, Vulkan/IPC identifiers and local storage.
- Unified local HTML/CSS interface across the four main pages and research/save tools. Original Eternal fonts, gradient, spacing and colors are packaged with the application.
- Removed the former native widget layouts. Browser hosts preload assets and reveal each window only after its first prepared frame, preventing old-interface flashes.
- Retained collision, filters, timeline, Movement Lab, surface projections, entity links, level comparison, encounter observations, research points and checkpoint tools.
- Detailed English Wiki, installation guide, complete console reference and source/build documentation.

## Download

Extract all files from `labplus-0.8.0-win-x64.zip` and run `labplus.exe`. Requires Windows x64, .NET 10 Desktop Runtime x64 and Microsoft Edge WebView2 Evergreen Runtime. `SHA256SUMS.txt` contains the archive checksum. Compatible meathook is optional for native HUD/console integration and is not bundled.

## Validation and limits

Managed launcher checks, encrypted checkpoint fixtures, headless screen scenarios and packaged WebView checks passed, including child windows and first-frame readiness. The native DLL is unchanged from the previously verified local rebrand build: eleven native suites, 22 profile fixtures and 38 isolated GPU cases. Those native/GPU tests were not rerun for the final UI-only change. No new live-game acceptance was performed for this package.

Native cvar creation is audited for 6.66 Rev 3 only and still requires acceptance after restarting with this layer. Live checkpoint loading remains 6.66 Rev 3 / TAG1 / slot 3, same account, Cloud off. See the Wiki for all compatibility boundaries.

## Historical source development record

# LAB Plus — local Slayer Workspace revision

Local revision, 2026-10-31. Run `publish/labplus.exe` with .NET 10 Desktop Runtime x64 and Microsoft Edge WebView2 Runtime. This revision has not been published to GitHub.

The approved HTML/CSS renders directly in the desktop shell, with its gradient, original fonts, exact main-window geometry and selected red/gold emblem. The old native widget tree has been removed: screen controllers hold plain data and actions, and each native window contains only WebView2. Shared, preloaded browser hosts prepare a complete first frame before reveal, eliminating the old-interface flash. Application tools and confirmations share this theme; OS file pickers remain native. The game layer is byte-identical to the previously verified local rebrand build.

Visual verification covers four main pages, eleven child windows, populated research/movement/graph fixtures and 125%/150% browser zoom. Separate managed model checks also pass. Live game acceptance was not repeated for this presentation change.

## Inherited 0.8 functional baseline — research tools and practice checkpoints

Released 2026-10-27. Requires Windows x64 and the .NET 10 Desktop Runtime (x64).
The following functionality and acceptance notes are inherited from upstream 0.8. The current local package is `artifacts/labplus-local.zip`; extract the full folder and run `labplus.exe`.

This release includes the accumulated entity graph, trigger-folder filters, Movement lab, support projections, before/after snapshots, research points, simple practice-save library and firewall launch gate described below. A separately installed compatible meathook is required for native HUD and reflection features; it is not bundled.

Live checkpoint archiving and repeated early/late restoration passed in-game acceptance without game/Steam restarts on **6.66 Rev 3 / TAG1 / slot 3 / UAC Atlantica**. The user also confirmed the simplified companion starts without errors. Cloud must be off globally or for the game; mixing Steam accounts remains blocked. Archives preserve completed checkpoint files. Other builds/slots use the advanced offline workflow, whose closed-process guards remain intact.

Launch from LAB Plus requires DESRU's enabled outbound block for the exact selected game EXE. Existing Windows rules are only read. When upgrading from public 0.7, restart the game once through the new build to load its native layer. When upgrading from the already tested live-restore/simple-saves development build, only the companion needs restarting: the native DLL is unchanged.

Validation: eleven native fixture suites, managed parser/settings/save/firewall checks, WinForms scenario/minimum-layout checks and 19 isolated Vulkan GPU cases passed. Fixture coverage across 22 collision-viewer profiles does not broaden the narrow live-restore support or prove HUD visibility and every research feature on every build.

## Changes since 0.7

- Simplify **Saves** to an automatically detected Steam folder, optional name, searchable library and **Save / Load** buttons. New captures are selected automatically; live loading needs no intermediate dialog. Technical offline/recovery tools remain under **Advanced…**. Existing archives and all native account/Cloud/pause/writer checks are preserved. Live support remains 6.66 Rev 3 / TAG1 slot 3.
- Require an effective Windows Firewall outbound block for the exact selected executable before launch from LAB Plus. Disabled/inbound/allow/partial rules, missing active-profile coverage and disabled firewall profiles reject launch with a reason. The check does not change Windows settings or restrict manual launches outside LAB Plus.

- Add live archival and **Restore archive + reload…** for same-account native live-copy archives on 6.66 Rev 3 / TAG1 slot 3. Pin reviewed source/target hashes, authenticate independently through Windows CNG, preserve all four originals, write through Steam storage and verify disk/cache bytes before engine loading. Roll partial writes back; retain recovery paths on failure. Extend the companion wait and explain pressing Space at 100% loading. Native archival and repeated restoration between earlier/later checkpoints passed retail acceptance without restarting game/Steam; other slots remained unchanged.

- Fix the live experiment falsely reporting Cloud enabled when Steam Cloud is globally disabled but the per-game preference remains enabled. Query both account and app flags, report their values, and permit operations when either setting disables synchronization; unknown states still block. Add regression coverage for all combinations.


- Add **Research points & practice saves…** (proposal item 7): persistent points with map/profile, physical position, rendered eye/aim/matrix, loaded checkpoint, selected volumes, category/route notes and practice-archive links. Copy return/spawn data, edit/search and import/export points. Capture never executes a teleport or save command.
- Add full Steam checkpoint-slot archives with per-file SHA-256 and AES-GCM checks, including both internal backups. Prepare a separate same-account archive from a valid backup pair when primary containers fail/miss. Guard imports/restores against account mismatch, guard campaign-family/occupied-slot/Cloud/process compatibility, stage replacement and preserve current-slot/profile/Steam evidence with failure rollback. No owner changes, automatic profile/index deletion or arbitrary live-state restoration. See [instructions and validation](Native/CollisionViewer/RESEARCH_POINTS_LOCAL.md).
- Add **Level state: before / after…** (proposal item 6): on-demand recognized-volume and encounter-manager snapshots, loaded spawn-info checkpoint name, identity/state/transform/bounds/fingerprint comparisons, selected/all captured outlines, JSON pairs/notes and complete CSV export. Preserve the distinction between removed identities and unavailable data; reject different processes/profiles/map instances. Saved fields remain readable without the native outline cache. See [usage and validation](Native/CollisionViewer/LEVEL_STATE_LOCAL.md).
- Move the optional support-angle HUD from y=500 to a compact block at `(360,300)` and add callback/submission/source diagnostics for missing text. Keep the companion running and reload the layer with a full game restart; live visibility remains subject to in-game acceptance.
- Refresh the player collider pose every presented frame while reusing verified background geometry; keep recording/target measurements at 20 Hz. Add **Surface projections…** with XY/XZ/YZ plane cuts, signed tangent angles, live/held/recorded sources and a `labplus_surface_show` numerical HUD. Distinguish coincident cuts and edge/face geometry from the single observed support plane. Report frame-reader CPU cost in the native log.

- Add **Movement lab…** (proposal item 5): validated convex player core with Havok rounding, body bounds/velocity/support trace, 20 Hz target sampling, named persistent attempts, manual success/failure annotations, up to six world-space path overlays, XY/XZ/YZ plots and elapsed-time comparison. Show origin-to-boundary distance and rounded-hull skin gap for an aimed/held volume; export complete attempts as CSV. Invalid geometry falls back to explicitly labelled bounds; sampling gaps remain gaps. New native visibility cvars: `labplus_player_show` / `labplus_attempts_show`.

- Show all distinct incoming/outgoing neighbors in **Entity links…**, replacing the four-per-side display limit with automatic fit. Add pointer-centred wheel zoom, RMB drag, LMB node selection, full-name hover hints and **Fit graph**. Periodic updates retain manual zoom/position; new roots and maps fit automatically.

- Add hierarchical path filters beneath each trigger class in **Trigger types…**. The bundled catalogue contains 24 class/folder groups from 107 trigger definitions in the installed 6.66 Rev 3 resources, including lootdrop, weapons, ai, stat_event and encounter paths. Live discoveries extend it. Match both instance names and entity definitions so map-renamed objects keep their resource subtype; flat names have no artificial directory entry. Choices are class-scoped, persistent and apply live to surfaces, outlines, history snapshots and picking. Parent switches retain child preferences.

- Fix repeated `Unknown command 'set'` messages: create owned `labplus_*` variables with the audited native engine constructor/register API, synchronize through the native setter and stop permanently on failed registration/ownership checks. Native creation is guarded for 6.66 Rev 3; unsupported versions produce one diagnostic and retain GUI/action-command features. The resolver passed read-only validation on the running retail game; executing the new native calls still needs acceptance after restarting with this build.
- Fix **Entity metadata unavailable**: accept empty superclass names for root/value types and follow the public field-name terminator. Validate resolved managed pointers against map slots instead of equating different ID domains; omit internal self pointers. A read-only capture of UAC Atlantica produced 10,107 entities / 1,694 links / 97 owner links and passed companion parsing.

- Add **Entity links…**: a navigable graph of observed runtime entity references, crosshair following, technical/reference-name search, incoming/outgoing field evidence, name copying and full JSON export.
- Resolve current-game layouts through meathook reflection; validate map slots, generations and stable resolved managed pointers. Missing metadata, read errors, stale data and scan limits remain visible. Capture runs while the window requests it.
- Links describe sampled references, not script execution or confirmed causal dependencies. Script-only and static map links are not decoded. See [scope, controls and validation](Native/CollisionViewer/ENTITY_GRAPH_LOCAL.md).
- Automated coverage includes the new graph suite on all 22 profile fixtures, existing native suites, managed parser/control checks and 19 isolated GPU cases. The graph still needs in-game acceptance with this layer.

# LAB Plus 0.7

Adds tools for studying DOOM Eternal volumes, routes and encounter managers.

- **Volume inspector:** aim at a trigger or invisible wall to highlight its exact geometry and show its name, class, map ID/generation, collision state and role in the game's native debug HUD. Confirmed encounter names are taken from 188 map-scoped mappings in the community reference.
- **Volume changes:** optionally show disabled volumes, flash observed state changes and retain removed/unreadable geometry as dashed snapshots for up to 15 seconds.
- **Trigger filters:** a searchable checklist controls visibility of each exact runtime trigger class and remembers choices.
- **Research timeline and player path:** manually start/stop recording, display the recorded route, add markers and export the full journal to CSV. Path visibility and recording are independent.
- **Active encounters:** a live window updates about every 250 ms with reference/technical names, state, script counters and entity IDs. An optional native HUD shows up to five active managers.
- **Console controls:** real `labplus_*` cvars represent settings, such as `labplus_encounters_show 0/1` and `labplus_path_show 0/1`. Commands perform actions: `labplus_recording_start`, `labplus_recording_stop`, `labplus_encounters_list`. Use `labplus_commands_list` for the complete reference. Controls apply only while the matching LAB Plus companion is running.
- Launching the game from LAB Plus always adds `+com_skipIntroVideo 1`. The companion also reports **Update pending** when the running game has an older viewer DLL.

Download **labplus-0.7.zip**, extract it and run **LAB Plus.exe**. Requires Windows x64 and the **.NET 10 Desktop Runtime (x64)**. The archive includes matching source code and licenses; **SHA256SUMS.txt** contains its checksum. A separately installed, compatible **meathook** is required for native HUD text, encounter metadata and console controls; it is not bundled.

Completely exit the game and launch it through the updated LAB Plus with Collision Viewer enabled to load the new layer. Enable **Volume inspector (meathook)** for the inspector, or open **Active encounters…** and select **Show in-game HUD**. `labplus_encounters_show 1` enables that HUD; `0` hides it; entering the cvar without a value queries it.

Validation: all six native suites, 22 profile fixtures, managed launcher/settings/parser checks, UI layout/control checks and 19 isolated Vulkan GPU readback cases passed. Read-only production inspection on **6.66 Rev 3 / UAC Atlantica** validated player position, 36 encounter managers, reference-name matching and the existing engine command/cvar-system ABI. The inspector HUD visibility fix was confirmed by the user on that game build.

Research features are experimental. The newly added encounter HUD, route recording and native cvar creation still require in-game acceptance after loading this layer; fixture coverage is not a claim that meathook supports every collision-viewer profile. Manager activity means a nonzero reflected `thinkFlags`, not proof that combat started or completed. Unknown fields and unmatched reference names remain explicit rather than being inferred.
