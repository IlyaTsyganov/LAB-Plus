# Checkpoints and recovery

## Checkpoint library

On **Saves**, enter an optional name and press **Save** to add a supported completed checkpoint to the library. Select an entry and use **Load** only when the tool's live checks permit it.

Library entries contain the game's **completed autosave containers**. Supported live loading is limited to **6.66 Rev 3 / TAG1 / slot 3**, on the same Steam account with Steam Cloud off. Keep Steam running, pause the game before the operation and satisfy the displayed process, account and session checks.

After loading reaches 100%, press Space if the game requests it and pause again before another operation. A library entry does not broaden live loading to another game build, campaign or slot.

## Advanced archives

**Advanced → Tools and diagnostics** provides slot inspection, full-slot archives, import/export folders, archive verification, internal-backup recovery and reviewed offline restore.

Verify an archive before restoring it. A file checksum alone does not establish that its checkpoint containers authenticate for the intended account. Preserve original files and review source, target and campaign family.

Archives may contain personal save data. Review them before sharing.

## Offline restore

1. Disable Steam Cloud for the game.
2. Fully close **both DOOM Eternal and Steam**.
3. Select a recognized same-account archive and a supported occupied target slot of the matching campaign family.
4. Review the source, destination, hashes and restore plan.
5. Apply that reviewed plan and retain the before-restore evidence until verified.

The workflow preserves profile/Steam metadata and retains recovery paths. Do not merge partial directories after an interrupted operation.

If interrupted, preserve the originals and follow the retained `restore-plan.json` and `RESTORE_PATHS.txt`. Include relevant diagnostics when asking for help rather than discarding recovery evidence.

## Internal backups

When both supported backup containers authenticate, recovery can create a separate archive while retaining the original backups. It does not convert accounts or guarantee repair of arbitrary damaged/unknown formats.

See [Compatibility](Compatibility) for scope and [Data and migration](Data-and-migration) for storage.
