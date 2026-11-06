# Local validation: collision contents, equipment and movement

Changes are local only. No PR, remote branch, release, or installed-game files were created or changed.

## Geometry

`Projectile collision` is an independent, default-off Geometry switch. It has its own color and fill opacity; default purple is `#cc59ff` with the existing 9% fill. Existing categories, colors, visibility and inspector appearance retain their settings.

The new category requires `CONTENTS_SHOTCLIP` (0x80), excludes SOLID / PLAYERCLIP (0x1 / 0x8), and rejects NOCOLLISION (0x40000000). It does not infer blockers from entity names, trigger inheritance, volumes, `CONTENTS_PROJECTILE` (0x800), or arbitrary non-player contents. SHOTCLIP is present in both retail `MASK_SHOT` (0x103085) and `MASK_PROJECTILE` (0x503085). This is the dedicated shotclip category, not a promise that every weapon-specific trace uses these default masks.

Static shapes use the validated engine-owned idHavokShape contents table, including per-primitive tags in compressed meshes. Unknown/missing shot tags are omitted. A separate background decode pass has a separate geometry cache. Dynamic idDynamicEntity shotclip uses physics contents, active body and in-world status; disabled histories retain their original category.

Offline enum-table checks covered 16 distinct local PE identities: 1.0, 1.1, 3.0, 3.1, 5.1, 6.1, 6.2, 6.3, 6.4, 6.66, 6.66 Rev 1, Rev 1.1, Rev 2, Rev 2.2, Rev 3, and Rev 3.2 (plus duplicate copies). All had the same seven audited contents/mask values. Native synthetic scene tests cover all 22 profile adapters. These are not gameplay collision tests. Builds 2.0, 2.1, 4.0, 4.1, 5.0 and 6.0 were not available for this enum-table check.

## Flame Belch

The input-attempt field `flameBelchNextUseTime` remains the anchor for validating the game clock, but no longer extends the equipment cooldown. The actual recharge/reuse records supply remaining time, and equipmentRechargeDuration supplies the maximum. A rejected input with a future attempt gate must not start/reset this timer. This is covered by a memory fixture; successful/rejected Flame Belch behavior has not yet been replayed with the changed layer in game.

## Weapon-switch state

The old BATS row becomes `Weapon switch`; legacy active snapshots display `BATS`. A lower-than-configured bring-up rate is kept as `slower`, rather than disappearing as inactive. This also avoids calling a not-yet-applied Hot Swapper multiplier Inverse BATS just because the rate is lower.

Plasma's Inverse BATS classifier is separate from raise-speed comparison. The user reproduced an interval with a primary-fire animation but no normal shot/ammo consumption, and clarified that any source weapon with two available mods can cause it. The read-only 1.1 trace had primary mode 0, fireModeInhibited=[0,0], retained mod-change, fire animation/pending fire, an aborted shot, and no isWeaponFiring flag, followed by real firing. The classifier follows those reflected flags and rejects ordinary dry-fire. It does not restrict the source weapon. This supports the observed flag signature, but the updated overlay itself has not been installed/replayed in game. No weapon timing or Hot Swapper upgrade is modified.

## Jump windows

Status: the two estimated jump-window timers described below were removed at the user's request on 2026-11-05. The following records their earlier investigation and is not a description of current timer output.

A read-only capture of the user's retail 1.1 process, without the Air Control rune, observed jumpCount=2 and then doubleJumpAirEnglishScale changing from 1 to 0. airControlEnableTime stayed zero. The visible behavior is not an Air Control rune cooldown.

The audited 1.0 SetJumpParms branch compares highestFallPosition.z - player origin.z + 0.3048 against playerFalling.fatalDamageDistance. Once the jump-boots height condition fails and the alternate double-jump path is off, it zeroes the jump parameters. In the observed 1.1 level, fatalDamageDistance was 39.0144 metres. Therefore, a fixed stopwatch measured from either jump would be misleading.

Two separate rows estimate time to this spatial cutoff: `Second jump loss (estimate)` after the first jump and `Air steering loss (estimate)` after the second. They use current world origin, current playerPState_t velocity (the base Havok body's velocity is zero in this case), highest fall position, and gravity. Ascending estimates account for a future apex. Dashes, grapples, launchers, collisions and other movement overrides invalidate a constant free-fall assumption, so these are explicitly estimates, with no invented maximum time.

The new estimates are enabled only on the audited 1.0/1.1 profiles. Other versions return unavailable for these two rows while keeping all existing timers. Rule auditing/controlled movement tests on other patches remain outstanding. Synthetic fixtures cover phase changes, landing, zero steering, invalid gravity, apex prediction and unsupported-profile rejection.

## Checkpoint timeout

The launcher previously omitted the native module when all native overlay features were off. Saves always need that module, so every LAB Plus direct game launch now configures it. The regression test checks the all-overlays-off launch and keeps the firewall gate and final firewall recheck. This fixes a concrete missing-bridge path, but cannot establish the cause of a third-party user's timeout from a screenshot. No actual checkpoint archive/restore was attempted during this task.

During the first live observation the user reported a black screen and closed the game through Task Manager. That process had no LAB Plus collision module loaded; prepared changes were not installed. A subsequent manual launch allowed the movement observation. The initial black-screen cause was not established.

## Validation

Managed launcher suite and app build passed. Native deterministic suites and targeted final checks are recorded locally under build/. Retail EXEs, process traces, local filesystem paths and private audit artifacts remain untracked in build/. No release or online workflow was requested.

## In-game validation, 2026-11-04

The local Debug app launched DOOM Eternal 1.1 with `labplus-layer-0d52858d6461d38d.dll`; its identity matches the built module. Viewer and shot-only visibility were enabled. On `game/sp/e1m1_intro/e1m1_intro`, the Vulkan log reports initialized rendering, a completed runtime scene, and continuing geometry draws. The shot-only setting is independent of disabled world collision. A read-only capture of the native numbers stream observed both jump rows becoming active with changing values and returning to inactive. The capture did not contain a fall to the zero cutoff, so timing accuracy at loss is still unconfirmed.

A live checkpoint `status` request returned `ready` and identified the loaded checkpoint. This confirms communication in this launch; no checkpoint archive or restore was performed and the request reported no save-file reads or changes.

Flame Belch, Ice Bomb and Frag were `not_acquired` in the loaded campaign state. Their gameplay cooldowns cannot be checked in that state. The updated Inverse BATS classifier was not yet reproduced during this launch. Visual confirmation of purple geometry, actual bullet/projectile impacts, HUD readability and fall-cutoff timing remain pending user observation. Window screenshots could not be captured by the computer-use service; accessibility text confirmed that the new Geometry row and color/opacity controls exist.

### User-confirmed observations and follow-up

The user confirmed in 1.1 that the game works, purple geometry is visible, both jump indicators appear and count down toward the observed losses. This is user-observed gameplay evidence; actual weapon impacts against individual shotclip surfaces were not separately verified. The user also identified a missing case: leaving a ledge without jumping can exhaust the aerial-jump window. The zero-jump airborne phase now keeps the second-jump-loss estimate active; grounded zero-jump state stays inactive. Synthetic fixtures cover ledge fall, cutoff and landing. The running game's previous DLL does not include that correction yet.

Movement Lab now has independent `Coordinates (debug HUD)` and `Angles (debug HUD)` switches, both default off. The shared debug HUD displays XYZ from the player's current physical world origin and Pitch/Yaw/Roll from the player's physics view angles, in degrees. Values are read and refreshed on the game-frame callback, independently of Invisible Numbers and geometry visibility. Loading/menu/console/app shutdown hides the rows; unreadable or nonfinite position/angle values are omitted independently. Existing debug HUD blocks retain their own rows and layout.

The pose HUD uses reflected layouts rather than fixed per-patch offsets. Tests cover signed values, axis order, individual switches, invalid-angle isolation, removed-player rejection, shared HUD layout cleanup, settings persistence and Viewer-disabled activation. App build and relevant native/managed suites pass. The new local playtest build is under `build/playtest-next`; its pose and ledge changes still require a fresh game launch to verify visually. Nothing was pushed to GitHub or released.

### Support-plane stability and jump maxima

Movement Lab labels are now exactly `Show coordinates in game` and `Show angles in game`. Support Plane preserves its six-row block while enabled during gameplay if the current player sample is missing, invalid, future-dated or at least 250 ms old. Values become `-`; old normals/angles are not held. Freshness transitions bypass the normal 100 ms formatting throttle so stale data is cleared immediately. Explicit disable and gameplay/console visibility gates still remove the block. Tests include repeated fresh/missing alternation with a real lower HUD block and verify that its vertical position stays fixed.

Both jump/fall indicators now publish an estimated maximum alongside remaining time. Total estimated duration uses the engine's phase start (lastJumpTime, lastDoubleJumpTime, or highestFallPositionTime for a ledge fall) and physics gameTime. It remains an estimate rather than a fixed patch-wide constant. After crossing the spatial cutoff, the calculation solves backwards to the crossing instead of growing with time spent below the cutoff. Invalid clocks or impossible trajectories leave maximum unknown. The existing timer formatter displays remaining / maximum in both the game HUD and the companion.

Relevant movement, shared HUD layout/rendering, game-timer and managed launcher tests pass. The app builds without errors. The updated local build is `build/playtest-hud-update`; it has not replaced the module already loaded by the user's running game. No remote changes or release were made.

### Local projectile-overlap experiment, 2026-11-05

Added a separate default-off `Hide parts inside world collision (experimental)` setting beneath Projectile collision. World collision does not need to be visible for subtraction. Both should be enabled for visual comparison of the boundaries. Color, opacity, fill/outlines and all existing geometry switches remain independent.

Subtraction uses only native convex World collision leaves and additionally validates closed edges, consistent winding, nondegenerate planes and convexity. Compressed triangle surfaces, open meshes and dynamic/live entities do not supply subtraction volumes. AABBs only find possible intersections. Triangle polygons and original wire segments are clipped against inset convex planes; coincident boundaries remain conservatively visible. A replacement is retained only if neither triangle nor line count increases. Original immutable meshes remain available for instant A/B comparison and fallback. No gameplay collision, contents, masks or weapon logic is changed.

Preparation runs lazily on the existing scene worker and is cached for the level. Its soft limits are two seconds, 80 million work units, 4,096 cached convex shapes and 32 MiB of replacement vertex data. Unprocessed, ambiguous or unhelpfully split meshes keep their originals. Work starts with the view when first enabled, then nearby geometry. This is a bounded initial-view experiment, not complete map-wide CSG or continuously updating coverage as the camera moves. There can be transient background CPU/memory cost while preparing it. Level generations cancel publication of obsolete results, and large old replacements are retired on the worker. The existing bounded transformed-vertex cache supports both original and reduced meshes.

Deterministic tests cover analytic partial triangle area and exterior segment lengths, complete containment, coincident boundaries, open/nonconvex rejection, a rotated AABB false positive, reflected nonuniform transforms, cancellation, nearby-first budget fallback, raw/reduced selection, vertex-cache use and dynamic-entity isolation. Existing renderer concurrency/cache and 22-profile synthetic viewer suites pass. Managed build, settings transport/persistence suite, JavaScript syntax and diff whitespace checks pass. Only game 1.1 has been exercised live for this experiment; those synthetic profile checks are not live compatibility tests.

The initial arbitrary-order pass reached its work cap before this camera's geometry and showed no visible-set reduction. A separate observer opened only PROCESS_VM_READ / PROCESS_QUERY_LIMITED_INFORMATION and replayed the captured camera and static physics scene. Completing the nearby geometry reduced 38,892 triangles to 27,893 and 56,850 wire segments to 39,661. This exposed the scheduling limitation and motivated processing the initial view first. The whole-map focused replay with the production budget retained the same visible reduction even though distant processing remained limited.

The updated module SHA-256 was 3737007F9AC43B8FCC3145EDF3596824F3B9777EDC55EC5010527C1F4A6B6176, verified against the loaded game module after a user-controlled restart. The live scene was DOOM Eternal 1.1, game/sp/e1m1_intro/e1m1_intro, camera approximately (254.932, -803.553, -170.528), Filled, 40 m, swapchain 2560 x 1440. World visibility was off. Other settings were held constant; seven visible triggers contributed 84 extra triangles/edges to both modes, and no invisible walls were visible. Two raw/reduced pairs lasted 18 seconds per phase. Only the last two full five-second windows of each phase are used below, excluding transitions and cold caches.

| Live measurement | Original | Reduced |
| --- | ---: | ---: |
| Static projectile triangles | 38,892 | 27,893 (-28.3%) |
| Projectile wire segments | 56,850 | 39,661 (-30.2%) |
| Mean CPU geometry preparation | 1.575 ms | 1.143 ms (-27.4%) |
| Presentation-call rate | 197.31/s | 220.71/s (+11.9%) |

The rate is a presentation/overlay-call FPS proxy, not a GPU timestamp or independent frame-time capture. No GPU stage timings or other scene/patch performance gains are claimed. The full-map background pass took about 2,000 ms, retained about 0.9 MiB of replacement vertices, and hit its soft time limit; the visible initial region was processed. Original collision behavior and actual projectile impacts were not altered or retested by this rendering experiment.

The measurement helper changed only the native TrimShotCollision INI entry and restored its initial off value in a finally block. User-requested visual boundary inspection with both World collision and Projectile collision is pending. Local detailed A/B logs, read-only replay results and summary JSON are ignored under build/. The current runnable build remains build/playtest-projectile-trim. No PR, push or release was performed.

Final review additionally requires positively verified SOLID / PLAYERCLIP contents before a native convex leaf can supply a subtraction volume. Legacy unknown-contents geometry remains visible in World collision, but cannot hide projectile surfaces. Regression tests explicitly cover unknown / SOLID / PLAYERCLIP cases. The final local module SHA-256 is 6215F423271EFA1824F9BC81BF8DF25FBBAF103E17855CF4BDC8526D948810E5. It is copied into the runnable local build and will load at the next game launch; the preceding measured in-game module remains loaded in the current process. No new live FPS measurement is attributed to this final guard.

A final read-only replay of the stricter contents guard succeeded on the same 1.1 map after the user moved to another location. This establishes successful preparation there, not a repeated fixed-camera FPS benchmark or other-patch compatibility. Detailed output remains local under build/projectile-strict-contents-replay.log. Visual inspection with both world and projectile categories was requested; the user enabled both for that inspection. No visual correctness confirmation has been received yet.

### Follow-up: incomplete trimming and native HUD priority, 2026-11-05

The user identified surviving Projectile collision inside visible World collision and clarified that World collision itself looked correct. A read-only capture preserved the full static scene and the chosen camera for repeatable local analysis; no live projectile entities were visible there. The private scene dump and images remain ignored under build/ and are not distributable artifacts.

The observation was confirmed. In the selected view, 772 static projectile instances contained 51,691 triangles and 70,243 segments. The complete conservative solver initially accepted changes to only 38 instances. Some actual ray-hit points inside valid subtraction volumes survived because clipping increased primitive count and the entire original mesh was retained. Many other nearby world meshes failed strict convex support-plane validation; measured errors ranged from tiny rounding differences to more than a metre, so loosening a single tolerance would not be a safe general fix. Open, nonconvex, unverified and overly complex sources still do not supply subtraction solids. Their appearance in World collision remains unchanged.

Two concrete implementation problems were corrected:

- An entirely disjoint triangle could be partitioned by individual infinite face planes even when it had no positive-area intersection with the volume. Those unnecessary fragments caused avoidable growth/complexity fallbacks. The intersection is now checked first, preserving the exact original triangulation when disjoint. In the captured view, accepted changes increased from 38 to 43 instances; the reduced totals improved from 51,109/68,888 to 51,001/68,623 triangles/segments. This is a modest local coverage improvement, not full CSG.
- The old bounded result stopped permanently after its initial pass. The worker now continues unfinished geometry when it enters the current view, preserving previously completed reductions in immutable snapshots. It uses the world BVH to find pending visible work, retains the time/work/memory limits and prevents a repeatedly interrupted mesh from starving the remainder. The original world's warm transformed positions survive publication of new clipping batches. Reused or foreign scene state cannot resume an earlier world's result.

The updated continuation was observed live on 1.1 as the user moved through the level: completed reductions accumulated across regions. Further conservative fallbacks remain intentional: no verified usable solid, excessive subdivision, increased drawing complexity, resource limits, and dynamic entities. Diagnostic reason codes now distinguish them. The setting does not promise to subtract every overlap.

The user also requested that projectile geometry leave native health, ammunition and crosshair UI readable. Investigation found that all viewer categories share a presentation-time pass, after the game's native HUD. Native render-pass tracing and paired GPU captures identified the actual full-resolution GUI target. A later capture, after loading, showed health/ammo/crosshair/messages on a transparent background at both 2560x1440 and 2544x1361. The first loading capture was opaque black and was rejected as evidence of the GUI contents. No guessed widget rectangles or fixed HUD coordinates were introduced.

An experimental native-GUI alpha mask was implemented and passed synthetic and GPU checks, but the user subsequently canceled the HUD-overlap changes before visual acceptance. That experiment, its capture hooks, queue-routing changes, shader changes and dedicated tests have been removed. The existing geometry rendering path is restored; earlier approved LAB HUD timers, player pose and support-plane stability changes remain. The old HUD-mask module (SHA-256 00A66733C6D7A5C34369C62B04DA34530CC9DDF6646EC33D568393618EEEFB70) may remain in the running game until it is restarted. It is not the current deliverable.

### Clipping-only follow-up after HUD cancellation

The remaining support-plane failures were traced to normals reconstructed from decoded triangles. A read-only 1.1 capture verified the native polytope plane hkRelArray at +0x50 against its face/corner tables. The builder now reads those actual planes for positively verified native convex player blockers, validates bounded non-overlapping arrays, face/plane correspondence, finite unit normals, interior center and vertex support, then transforms planes by the inverse transpose. Unsupported or invalid layouts retain the previous strict mesh-derived fallback. Classification still requires real known SOLID / PLAYERCLIP contents; neither triggers nor generic volumes become subtraction solids. World collision rendering is unchanged.

Fill and wire reductions are now accepted independently. Subdivision failure or primitive-count growth restores only the affected channel. Equal-count shortening is retained. Neither triangle count nor segment count may grow, and resource/candidate/fragment limits still preserve the original geometry when necessary.

In the saved problem view, a completed nearby replay changed 687 of 772 projectile instances (543 partial, 144 removed). Original 51,691 triangles / 70,243 segments became 35,610 / 34,872, approximately 31% / 50% fewer. Twenty instances retained both channels because of growth, two exceeded candidate limits, two exceeded fragment limits, and 61 had no accepted reduction. This is a captured-scene geometry result, not a new live FPS benchmark or proof that every overlap is removed. The native-plane layout has live evidence only for 1.1; other profiles require game validation and use the guarded fallback when the certificate fails.

The clipping-only production DLL SHA-256 is 724DEC8A33783684E23E3586164A599880B7EE68A6225EDC0E461F3815C06ABF. Native clipping tests (including array validation, transformed planes and independent channel fallback), 22-profile synthetic viewer tests and concurrency/cache tests pass. All 24 original Vulkan GPU regression scenarios pass after the HUD rollback. JavaScript syntax and whitespace checks pass. Visual acceptance and a fresh FPS comparison of this module remain pending. The runnable local build is build/playtest-projectile-trim. No PR, push or release was performed.

### Live clipping-only verification, 2026-11-05

After the user-controlled restart, the loaded module in process 19636 matched SHA-256 724DEC8A33783684E23E3586164A599880B7EE68A6225EDC0E461F3815C06ABF. The HUD experiment is absent from this module. DOOM Eternal 1.1 loaded the same e1m1_intro level. World collision and Projectile collision were both visible, Filled mode, 40 m, 2560x1440 swapchain. The user held the camera still at approximately (71.780, -465.616, -222.115).

Two raw/trim pairs were measured for 18 seconds per phase, changing only TrimShotCollision through the existing native setting. The original enabled value was restored in a finally block and confirmed after completion. Only the last two complete five-second telemetry windows from each phase were retained. Geometry counts and logged camera coordinates remained constant within each mode; four trigger instances contributed the same 72 triangles/segments in both modes. World collision also remained visible throughout.

| Metric | Original projectile geometry | Trimmed projectile geometry |
| --- | ---: | ---: |
| Visible projectile triangles | 46,027 | 32,596 (-29.2%) |
| Visible projectile wire segments | 62,797 | 32,289 (-48.6%) |
| Mean CPU geometry preparation | 2.461 ms | 1.873 ms (-23.9%) |
| Presentation/overlay-call rate | 211.69/s | 249.71/s (+18.0%) |

The rate is an FPS proxy from the existing renderer telemetry, not GPU timestamps or an independent frame-time capture. The near-250/s result does not establish whether a limiter was reached. No general FPS improvement or compatibility with other game patches is inferred. Private phase logs and the summary remain ignored under build/projectile-live-ab-nativeplanes.log and build/projectile-live-native-summary.json.

After comparison, the user inspected the problem area with both categories visible and confirmed: fewer unwanted surfaces, with the exterior protruding projectile portions still present. This establishes user-observed visual acceptance for that area, not complete map-wide overlap removal. Actual weapon impacts, all camera positions and other game versions were not revalidated. The setting remains experimental and default-off; nothing was pushed, published or released.

### Compact optimization control

At the user's request, the optimization now appears as a small `Trim overlap` checkbox in Projectile collision's color/fill-opacity row, replacing the separate long switch row. Its hover text is: `Hides projectile collision parts inside world collision and may slightly improve FPS. Experimental.` Help uses the same name. The existing trimShots setting, persisted state, master enable gate and native implementation are unchanged.

An isolated WebView preview verified the three supported window sizes at 100%, 125% and 150% zoom, including actual rendered captures. The checkbox stays in the appearance row in those nine cases without horizontal page overflow. Clicking it still persists off/on, and the Viewer master disables it. App build (zero warnings/errors), JavaScript syntax and whitespace checks pass. UI assets are copied to build/playtest-projectile-trim; the already-open companion needs to reopen to display the new markup. The running game does not need a restart for this UI-only change.

Outstanding gameplay acceptance is separate from completed implementation: Flame Belch success/rejected-input/max-duration behavior; final Weapon switch overlay states for BATS, inverse BATS and slower raise/Hot Swapper loading cases; final movement HUD poses/support-plane transitions; an actual checkpoint save and load. Checkpoint communication alone has been verified. The plasma engine signature has live evidence, but that does not replace checking all updated overlay cases. Only 1.1 has been exercised live during this work. Other game versions and weapon impacts against individual projectile-collision surfaces remain unverified. Jump/ledge estimates were subsequently removed at the user's request, as recorded below.

### Removal of jump estimates and fall-distance answer

The user rejected the estimated time windows and requested their removal. `Second jump loss (estimate)` and `Air steering loss (estimate)` are no longer collected or emitted, so neither the companion nor the game's timer HUD receives those rows or their maxima. The isolated jump-estimation header and its dedicated prediction fixtures were removed. Equipment timers, weapon-switch states, coordinate/angle HUD and support-plane changes remain.

A fresh read-only observation of the running retail 1.1 process again returned fatalDamageDistance = 39.014400482177734. The previously audited height predicate compares the saved highestFallPosition.z minus current player Z plus 0.3048 against that parameter. Its nominal boundary is therefore approximately 38.7096 m of downward vertical separation from the saved upper fall point. This is not horizontal/path distance or a universal distance measured from the second-jump button press. In the investigated ordinary jump/fall path this is the common cutoff associated with the aerial-jump and post-double-jump steering losses. The parameter is read from the active player declaration; other configurations or patches must not be assigned this number without verification. No new controlled fall/impact experiment was performed in this removal step.

The remaining native game-timer suite and shared HUD suite pass, and the production native module builds successfully. The updated module is copied to build/playtest-projectile-trim. The running game retains its previous DLL until the next game restart through LAB Plus. No replacement feature was added, and no remote changes were made.

### CSC, Flame Belch, Saves setup and Steam startup test build

The user confirmed that checkpoint saving works and requested clearer setup guidance. The earlier missing-bridge path remains guarded, but no new archive/restore repair is claimed. The agreed Saves mock is now implemented: a three-condition list (save folder, effective Steam Cloud state, loaded game checkpoint), followed immediately by the folder selector, then archive controls. It shows each missing action explicitly. The folder is empty without a saved preference, is never automatically discovered/selected, and an explicitly selected path persists across launches. Missing/unavailable folders, Steam account mismatches, Cloud ON/unknown and unloaded slots still block live operations. The native operation performs its existing checks again before touching save data.

Cloud state comes from the existing read-only native status reply, not an acknowledgment checkbox. Both account-wide Cloud OFF and per-game Cloud OFF are accepted, matching the existing native rule. Ambiguous/missing/invalid flags stay unknown. The folder tooltip shows the usual Program Files (x86)/Steam/userdata/<SteamID>/782330/remote location, while noting that Steam can be installed elsewhere. The Cloud tooltip contains numbered Steam > Settings > Cloud instructions, legacy toggle wording, and the per-game alternative. The instructions were checked against [Valve's Steam Cloud FAQ](https://help.steampowered.com/en/faqs/view/68d2-35ab-09a9-7678). The program does not modify Steam Cloud settings itself.

The Flame Belch maximum now has a validated fallback to the active player declaration's flameBelchCooldownTimeMS when the equipment recharge-duration field is zero. A fresh read-only 1.1 observation found zero recharge duration with an equipmentReuseTime deadline; the configured cooldown was 25,000 ms. The production capture now publishes maximum 25 seconds in that state, without hardcoding 25. Remaining time still follows the actual equipment recharge/reuse records. Fixtures cover a rejected-input deadline while ready and during cooldown, reuse-only maximum, and invalid/too-short configuration. The original reported button-reset problem was not newly reproduced in game, and the updated HUD's successful/rejected-use sequence still needs live acceptance.

CSC is a separate third speedrun row; existing BATS rate and inverse-BATS classifiers were left as they stood at the user's request. The supplied [Combo Swap Chain document](https://docs.google.com/document/d/1HgY4yINnSgNxMJtGELndQcOPYNI8VCUbUA_wXdhkI54/edit) was read through its public text export. During the user's 1.1 demonstration, a read-only observer captured a retained modChangeAnimPlaying flag across weapon handoffs; the user confirmed plasma and chaingun firing animations without actual shots. An ordinary same-weapon mod change also sets that flag transiently, so that flag alone does not activate CSC.

The tracker confirms a handoff retaining the flag or the specific coherent ghost-fire signature, then follows the flag until it clears. It validates reflected property getters and held-weapon generations, rejects incoherent reads, and resets on identity changes or sample gaps. A replay through the production tracker, sampled at approximately the worker's 100 ms cadence, found 22 active chain intervals in the recorded demonstration; ordinary mod-only control intervals remained inactive. A separate production read-only capture returned CSC inactive after the chain, Flame Belch maximum 25, and three technique rows accepted by the managed reader. No new module was injected for either observation. Rune/Mod Bot starts without later observable handoff/ghost-fire, all supported patches, and the new in-game CSC HUD are not declared validated.

Launch game now starts Steam if it is absent and asynchronously waits for a current-session Steam process with the matching active-client PID and a nonzero active user. An already-ready client is reused. Steam receives no game-only Vulkan/LAB Plus environment and no game-launch command; DOOM Eternal is still started directly with its own environment. The executable identity and firewall protection are checked again after the Steam wait. Timeout/sign-in failures stop game launch with a message, and closing LAB Plus cancels the pending launch. Existing Steam readiness was checked read-only; a real cold start was not tested by closing the user's running Steam/game.

Validation passed: native timer and technique suites, the managed launcher/save/protocol/Steam suite, and isolated Saves browser workflows covering empty first run, preference reopening, Cloud ON/unknown/OFF, tooltips, archive operations, categories and 125/150% scaling. The approved layout was visually checked after correcting UTF-8 label corruption caught by the UI workflow. Local generated build helpers are excluded from the app's source compilation.

The Debug app was built with zero warnings and zero errors at build/playtest-csc-saves-steam/LABPlus.exe and opened for testing (process 22520 at startup). Its native DLL SHA-256 matches the compiled source module: 30C992BCC7332D90DD032C127855516CA4505E94C6ADD36FD90B654FD36E3964. The existing game process was left running and therefore retains its old native module until a user-controlled restart. This is a local test build; no PR, push, release or publication was performed.

### Prelaunch Cloud status and save-folder feedback

Steam Cloud preferences are now read before launching the game and refreshed from Steam's local VDF settings. The successful UI state says only `Steam Cloud: disabled.` Completed prerequisites use a gold check mark; incomplete ones use an orange attention mark. The existing native confirmation still gates live archive/restore operations. Prelaunch settings do not bypass that check. The effective global OFF preference was verified read-only on this installation with the game absent; per-game overrides and malformed/ambiguous settings have fixture coverage, without changing the user's Steam settings.

The save-folder validator requires an existing absolute non-linked path ending in `Steam/userdata/<positive numeric userdata ID>/782330/remote`, with arbitrary installation prefix and userdata ID. The folder picker now always displays and remembers the selected folder, including an unrelated folder. Validation happens separately: invalid choices clear previous runtime readiness and archive selection, retain disabled Save/Load actions, and show the correction directly in the `Save folder` prerequisite. Its `?` and hover hint explain the expected path; the question mark disappears for a valid folder. The separate field label no longer duplicates this marker.

The isolated real-browser workflow passed after selecting a valid ready source, changing to an unrelated folder, checking the displayed input value and inline hint, verifying that a programmatic save emitted no live request, reopening with the invalid choice retained, and correcting the source again. The existing synthetic archive/load/category/DLC workflow and 125/150% scaling also pass. Evidence: build/saves-folder-feedback-qa-user.log and build/saves-folder-feedback-qa-user/saves-invalid-folder.png. The initial restricted-session WebView attempt failed to create its window; the successful run used the normal desktop session with isolated fictional save data. No real game saves were read or modified by these tests.

The updated Debug app built with zero warnings/errors and was opened at build/playtest-save-folder-feedback/LABPlus.exe (process 12976 at startup), closing the previous companion normally. No native game changes were made for this correction. No PR, push or release was performed.


### Wall dash deferred; continuing Flame Belch / CSC

At the user's explicit request, the Wall dash experiment was removed from active UI, settings, INI transport, native metadata, classification, render path and worker. Dedicated sources/tests and integration snapshots were verified into build/deferred/wall-dash-v6.zip; research notes and private captures remain local. See docs/deferred-wall-dash.md. Shared strict native-plane certification for Projectile collision trimming remains; it was not reverted with the experiment.

Ready build: build/playtest-without-wall-dash/LABPlus.exe. Native source/package SHA-256 DC8B36D30ACA4ACA1CB1C370AFE674F93D79A5EA0300F1F379D1BD99C4528E1D. Added the previously omitted separately built lighting DLL to the complete test package; its source/package hashes also match. Native geometry/timer/technique/HUD/movement and managed launcher/save/Steam fixtures pass. Managed build: zero warnings/errors. Native: existing unused techNear warning only. Complete isolated WebView workflow and visual inspection pass after correcting package completeness; source JavaScript checks pass. Logs: build/without-wall-dash-*.log, build/playtest-without-wall-dash-build.log. No new in-game behavior validation or all-patch gameplay claim.

Flame Belch still follows actual equipment recharge/reuse rather than the rejected-input next-use gate, with the configured declaration maximum fallback. CSC remains implemented independently. Next is user-controlled gameplay acceptance of Flame Belch successful use, repeated presses during cooldown and displayed maximum, then CSC HUD. BATS is explicitly out of scope and unchanged; jump estimates remain removed and the HUD overlap experiment stays canceled. User confirmed actual saving works; Saves folder/Cloud guidance and prelaunch checks remain implemented. No PR, push, release or automatic game/Steam shutdown/restart.


### Flame Belch gameplay acceptance, 2026-11-05

The user restarted through playtest-without-wall-dash and confirmed both requested checks in game: the maximum duration is visible, and repeated Flame Belch button presses during the actual cooldown do not reset the displayed countdown. This is acceptance for the tested retail 1.1 session, not every upgrade configuration or supported patch. CSC in-game status is the next separate check.


### CSC: derived weapon types caused unavailable (2026-11-05)

The user reported that CSC Heavy Cannon -> Plasma Gun displayed unavailable while Plasma played firing animations without ammo use or actual shots. A bounded capture of the old loaded module exported 2,886 distinct snapshots (2,358 live), including 1,409 unavailable CSC rows, 151 active rows and 23 active intervals. Two transient file-read sharing errors belong to the external watcher; they do not explain the native unavailable rows. Recording has ended. Evidence: build/csc-hud-check-4576.jsonl and summary JSON.

Read-only inspection in retail 1.1 identified the held plasma instance as idPlasmaRifleCharged with declaration weapon/player/plasma_rifle. Its reflected parent is idWeapon. The old reader required the exact native type name idWeapon and therefore rejected legitimate subclasses. The same inventory contains idChaingun, idDoubleBarrelShotgun, idBFG and idChainsaw, also reflected as idWeapon subclasses. The old running module still exports unavailable while holding the same plasma instance.

Only the CSC reader is changed: it follows verified reflected parent metadata to idWeapon, with complete/nonzero layouts, monotone base sizes, a 32-step bound and cycle rejection. Common weapon layouts are prefetched to reduce first-swap metadata warmup; these hints are not an acceptance whitelist. Actual property-getter checks, weapon generation/identity coherence, flag coherence, chain acquisition/termination and stale-gap reset remain. Failure details now distinguish missing layouts, weapon identity/type and changing snapshots. Existing BATS/Inverse BATS and Fast DBlade functions were not edited. Wall dash remains deferred and absent. Flame Belch gameplay acceptance is retained.

Regression checks cover real weapon type names, multi-level inheritance, unrelated/missing/empty types, incompatible/oversized layouts, parent cycles and an incomplete base. Native technique, timer, shared HUD and 22-profile synthetic Viewer suites pass (build/csc-weapon-types-native-build.log); only the existing unused techNear warning remains. An external read-only capture through the newly compiled production reader accepts the current held Plasma Gun and returns CSC inactive instead of unavailable (build/csc-weapon-types-live-probe.log). This resting/paused-state capture is not a new injected-module chain/HUD acceptance test. Other game patches are not declared tested live.

Ready local build: build/playtest-csc-weapon-types/LABPlus.exe. Source/packaged native SHA-256 10B700386302D010B571455AD9DA446A6FFD81B149D4C519713E0653933A9D65. Managed build zero warnings/errors. Both collision and lighting libraries are included. Next: user-controlled restart through this build, confirm the new module, then record Heavy Cannon -> Plasma CSC and the ordinary mod-swap negative control. No PR/push/release or automatic game/Steam restart.


### CSC gameplay acceptance after weapon inheritance fix, 2026-11-05

The user restarted retail 1.1 through playtest-csc-weapon-types (game PID11308, companion PID8360). The loaded native module was verified by full SHA-256 10B700386302D010B571455AD9DA446A6FFD81B149D4C519713E0653933A9D65 before the test. They performed the requested ordinary mod-swap control and Heavy Cannon -> Plasma CSC/ghost-fire/chain-break checks, and replied that everything worked correctly. This is user visual acceptance for the demonstrated case, not all CSC starts, weapons, timings or patches.

The bounded 300-second exported-HUD capture ended cleanly and its session is closed; no observer remains running for this check. Saved data: build/csc-weapon-types-hud-11308.jsonl and summary JSON. It contains 2,925 distinct live snapshots: 2,400 inactive, 525 active, 39 active intervals, zero native unavailable rows, finishing inactive. The external file reader encountered 34 transient sharing/read errors while observing producer file replacement; those attempts were not counted as native unavailable samples. The status log has no independent per-input/weapon labels, so all 39 intervals are not individually claimed to correspond to labeled successful attempts.

Flame Belch displayed maximum/repeated-button cooldown behavior and this CSC Heavy Cannon/Plasma unavailable fix are now accepted in the tested 1.1 session. No additional code changes after acceptance. BATS remains unchanged; Wall dash remains deferred. Current runnable local build is build/playtest-csc-weapon-types/LABPlus.exe. No PR, push or release.
