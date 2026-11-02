# Checkpoint file workflow — 2026-11-02

Base: `389e17d862765628fca4dddad96a5c45fb8407e9`.

This records the initial offline phase. The subsequent [1.1 live acceptance](checkpoint-live-1.1.md) supersedes the live-operation limits below.

## Root cause

This was an explicit prototype restriction, rather than an observed 1.1 encryption failure. `PracticeSaveForm` filtered its list using `PracticeSaveSource.SupportsLive` and sent every request to `DLC1-AUTOSAVE5`. The managed protocol, restore staging and native runtime independently reject other slots. The native runtime and loader use audited 6.66 Rev 3 addresses and permit only TAG1. Existing tests explicitly expected main-campaign responses to be rejected, so they passed before the change.

## Change and usage

The main checkpoint window now defaults to the existing authenticated file archive/restore implementation. This path makes no engine calls and has no patch-specific addresses. Advanced selects any of the nine canonical campaign slots and persists that choice. Close both the game and Steam, Save the latest completed autosave, select an archive, then Load. Load presents the target/recovery plan and asks for confirmation that Cloud is disabled. Start the game and load the selected slot normally. Archives from the main campaign are visible; the list remains scoped to account and selected slot. All four containers are retained, original slot and recovery evidence are preserved, and PROFILE/Steam metadata are not replaced.

The previous in-game experiment is selectable separately and retains its original checks. An unsupported runtime now directs the user to the file workflow.

## Validation performed

- Initial launcher suite passed unchanged, reproducing that the tests enforced the restriction.
- Launcher suite with `CampaignSaveChecks` passed: all nine slots, byte-exact capture/restore of primary and backup ciphertext, retained originals, unchanged other slots/PROFILE/Steam metadata. Existing account/Cloud/process/concurrent-writer/tampering/race/rollback cases also pass.
- InspectorPreview passed, including default offline main-campaign Save, list visibility, enabled Load, saved preferences and no native request; existing live protocol UI fixtures still pass.
- Focused real WebView2 test passed: `dotnet run --project Tests/VisualPreview/VisualPreview.csproj -c Release -- build/save-visual-qa --saves-only`. Browser clicks Save, Load and review OK, verifies restored ciphertext, renders settings and checks 125%/150% scale. Screenshots are under `build/save-visual-qa`.
- All 24 native fixture executables passed. HUD suite passed 343 checks.
- Native layer and lighting DLL built; managed Release build succeeded without warnings. Local framework-dependent package contains the expected 30 runtime files.
- Full VisualPreview was attempted separately and failed in its existing `CompactChecks` combined Vulkan-launch manifest assertion before reaching save windows. The focused save scenario passes. WebView2 requires the normal user desktop session here; the isolated runner initially reported an invalid window handle.

All save tests used synthetic encrypted containers in isolated directories. User game saves were not changed. No game was launched, GitHub changes pushed, tag created or release published.

## Remaining limits

This provides a patch-independent **offline file workflow**, not in-game reload adapters for all 22 supported executable profiles. Live operations remain limited to 6.66 Rev 3 / TAG1 / slot 3. Live main-campaign loading on patch 1.1 and other patches is still unimplemented. The local EXE inventory includes 1.0, 1.1, 2.0, 2.1, 3.0, 3.1, 4.0, 4.1, 5.0, 5.1, 6.66 Rev 3 and the current installation; no runtime save/restore acceptance was performed on them. Synthetic tests establish file/transaction correctness, not gameplay acceptance on all patches. Use the same patch that created the archive: this feature does not convert game save formats across patches. Captures contain the latest completed autosave, not arbitrary current RAM state.
