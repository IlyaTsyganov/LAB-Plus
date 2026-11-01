# Research timeline

The timeline records sampled route data and observations. It is separate from Movement Lab's named attempts.

## Record

1. Open **Research → Timeline** in a loaded level.
2. Press **Start**.
3. Add named markers for deliberate actions or points of interest.
4. Press **Stop** when finished.
5. Search/filter the view and export CSV.

Start resumes the current journal. **New session** starts a new journal while retaining older files. **Show path** controls in-game route drawing independently of recording.

The route uses physical body position. Map changes clear the displayed map route while retaining the journal. Export includes the complete journal rather than only filtered/paginated rows.

## Gaps and interpretation

Pause/menu states, unreadable data, large jumps and timing gaps break the route. Missing intervals are not interpolated evidence. Abnormal exit can lose the final buffered portion.

Elapsed journal time is wall-clock time, not the game's internal cooldown clock.

Volume discovery and state changes do not prove that a trigger fired, a checkpoint was written or an encounter finished. Pair a marker for your action with [before/after comparison](Entity-links-and-level-comparison) when investigating a causal claim.

## Commands

```text
labplus_recording_start
labplus_markers_add before entering arena
labplus_recording_stop
labplus_recording_clear
```

These control this journal, not Movement Lab attempts. Basic recording/path drawing and invoking its console actions have different prerequisites; see [Compatibility](Compatibility).

Storage and full-journal files are described in [Data and migration](Data-and-migration).
