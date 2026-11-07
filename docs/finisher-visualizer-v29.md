# Finisher Visualizer v29: 5.1 playtest and rune notice

This is a local intermediate playtest. Finisher preview is enabled for the
audited **1.1 and 5.1** executable identities. Support for every other LAB Plus
patch and the two DLC enemy rosters remains unfinished; this package does not
claim that coverage. No branch, commit or PR was created.

## Changes

* `Native/CollisionViewer/src/sync_patch_layout.h`: scoped descriptors for the
  changed 5.1 fields, virtual slots, game/view manager and cvar locations.
  Reflection and immutable instruction guards remain mandatory. Nested scopes
  restore the previous patch and do not leak into the 1.1 fixtures.
* `sync_target_preview.h`, `sync_authored_preview.h`, `sync_range_reader.h`,
  `sync_player_context.h`, `sync_nearest_choice.h`: use the selected descriptor
  to read health, shot spheres, focus groups, native assigned animations,
  blueprint poses, facing regions, priorities and range limits. The 5.1
  `canAwardBloodpunchCharge` flag is a reward flag, not an eligibility condition;
  the shifted `requiredMomentum` flag is checked separately. Native player
  input still uses priority, then nearest animation start, with first-on-tie
  ordering. Equipment and special-state finishers are excluded from ordinary
  authored priority predictions.
* `sync_target_preview.h`: Debug HUD now explains unsupported patches, missing
  preview data and missing targets instead of silently clearing the feature.
  Existing compact successful-preview lines remain.
* `UI/FinisherGuide.cs`, `UI/FinisherVisualizerForm.cs`,
  `Assets/help/finisher-guide.en.txt`, `.ru.txt`, `Assets/ui/workspace.css`:
  visible English Seek and Destroy limitation and matching standalone guides.
  The in-app guide remains English only. Chainsaw body colouring remains off.

## Verification

The local 5.1 EXE has SHA256
`93da12b964438b8bcabeb8fb12ff4050cbc6042f723b00baa89544482292594c`.
`Tools/PatchAudit/verify_finisher_profile.py` verifies its identity and eight
production instruction guards without exporting game bytes.

`Native/CollisionViewer/tests/finisher_live_probe.cpp` opens the game with
query/read access only. It verifies production reflection, code, world, camera,
model, assigned-declaration, range and authored-geometry readers. Its optional
`--replay-aim` changes an observer-local camera only; it neither controls the
game nor executes a finisher. In the running 5.1 scene it read:

* Carcass: 63 shot spheres, 15 ordinary animation rows, two assigned blueprints,
  13 complete GK regions with known priorities, and native range limits.
* Cinematic Revenant: 54 shot spheres, eight ordinary rows, two assigned
  blueprints and six complete regions with known priorities.
* Carcass observer-camera replay: ready target, valid aim, range limits and
  ordinary nearest-choice guard. This is a reader test, not a recorded 5.1
  finisher execution or a visual in-game acceptance test.

Regression checks include the full finisher suite (all 25 reviewed base
families), five related native suites, launcher checks, and WebView Finisher
Visualizer controls, mode isolation, lifecycle, English help, rune notice and
125%/150% layouts. Logs are under `build/finisher-v29-*.log`; UI screenshots
are under `build/qa-finisher-v29/`.

## Remaining patch and DLC coverage

The two provided game directories contain 16 unique supported executable
versions. EXEs for 2.0, 2.1, 4.0, 4.1, 5.0 and 6.0 have not been located in the
completed inventories. Finding a file or matching its identity is not proof
that the finisher readers work on it; the other found versions also need
their own descriptors and checks.

Armored Baron, Cursed Prowler and the other DLC actors still need actual
assigned-declaration/model checks in DLC scenes. The reader follows an enemy's
own assigned declarations and blueprint definitions; it does not substitute
base animations by a model-name guess. New actors without finisher declarations
must not receive invented zones, and GK/Chainsaw availability must be checked
independently. The base-game scene above does not establish DLC coverage.

For tomorrow's in-game check: launch the v29 package, use patch 5.1, unequip
Seek and Destroy, enable Research → Finisher Visualizer, and check Carcass,
Zombie and Soldier from front, sides and rear in both modes. Check that
Debug HUD shows the feature status and that Chainsaw has no body colouring.
