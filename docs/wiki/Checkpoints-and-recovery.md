# Checkpoints and recovery

## Simple checkpoint library

On **Saves**, optionally enter a name and press **Save**. The tool detects the Steam folder when unambiguous and selects the new capture. Select a library entry and press **Load** to restore it through the supported live workflow.

These are **completed autosave containers**, not arbitrary RAM snapshots. Live loading is narrowly supported for **6.66 Rev 3 / TAG1 / slot 3**, on the same Steam account with Cloud off. Keep Steam running for this live workflow, pause the game first and satisfy all process/map/session/account checks. At 100% loading, press Space if needed and pause again before the next operation.

The workflow authenticates all four containers, pins reviewed hashes, retains originals, writes through Steam storage, verifies disk/cache bytes and requests the engine reload. Partial failures attempt rollback and retain recovery paths. It does not re-sign files for another account or generalize to arbitrary slots and versions.

## Advanced archives

Open **Advanced → Tools and diagnostics** for slot inspection, full-slot archival, import/export folders, archive verification, backup recovery and reviewed offline restore.

A full-slot archive includes the four recognized primary/backup containers and regular slot files. Limits are 256 files, 64 MB per file and 128 MB for the set. Nested directories and reparse points are rejected. Unknown formats are not guessed.

Archive verification combines per-file SHA-256 with authenticated AES-GCM container checks. A checksum alone does not establish that a checkpoint decrypts for the intended account. Archives can contain personal save data; review their contents before sharing them.

## Offline restore

1. Close **both the game and Steam**.
2. Manually disable Steam Cloud and confirm the required state in the tool.
3. Select a recognized same-account archive and an occupied target slot of the matching campaign family.
4. Review the source, destination, file hashes and restore plan.
5. Apply the reviewed restore and retain the before-restore evidence until verified.

The workflow preserves PROFILE, Steam metadata and `remotecache.vdf`; it stages replacements and uses rollback renames. If the process is forcibly terminated, follow the exact retained paths in `restore-plan.json` and `RESTORE_PATHS.txt`. Do not merge partially restored directories or discard the evidence before resolving the interrupted plan.

## Recovering from internal backups

When both backup containers authenticate, the tool can construct a separate recovery archive using their original account and filename additional authenticated data. Both original backups are retained. This is not an account converter or a universal repair tool for damaged saves.

Earlier source acceptance restored earlier/later UAC Atlantica checkpoints on 6.66 Rev 3 / TAG1 / slot 3. The final UI/rebrand was checked with encrypted fixtures; the latest package has not been newly accepted in a live game. See the detailed [source workflow record](https://github.com/IlyaTsyganov/LAB-Plus/blob/main/Native/CollisionViewer/LIVE_PRACTICE_LOCAL.md).
