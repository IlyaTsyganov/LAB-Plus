# Research timeline

The timeline records sampled route data and research observations. It is separate from named Movement Lab attempts.

## Record a session

1. Open **Research → Timeline** with the selected game process loaded into a level.
2. Press **Start** to record. **Show path** independently controls whether the route is drawn in-game.
3. Add named markers at points of interest.
4. Press **Stop** when finished. Start resumes the session; **New session** starts a new journal while retaining older session files.
5. Search/filter the visible rows and export CSV. Export includes the full journal, not just the filtered or paginated view.

The recording is scoped to the selected process. The journal samples physical body position at about **10 Hz**, retaining a path point after at least **0.05 game units** of movement. The visible line runs from blue to pink. Map changes clear the displayed map route while keeping the journal.

## Gaps and retention

Menus, pauses, unreadable data, gaps over one second and jumps over 50 game units break the line. The tool does not interpolate a reliable route across such gaps. At most 12,000 points are retained for the current-map path and roughly 16,000 rows for the recent interface view. Older journal rows remain in the append-only `.full.tsv` file.

The file is flushed about once per second and on Stop. An abnormal exit can lose the final buffered second. Elapsed time is wall-clock time, not the game's internal timer.

## Interpreting events

Volume discovery and state changes are observations. They do not prove that a trigger fired, a checkpoint was written or an encounter completed. Add a marker describing your experimental action, compare before/after state, and retain exports when investigating a claim.

Console actions are `labplus_recording_start`, `labplus_recording_stop`, `labplus_recording_clear` and `labplus_markers_add <text>`. They control this journal, not the Movement Lab attempt recorder. Basic recording/path drawing does not require meathook; invoking console actions does.
