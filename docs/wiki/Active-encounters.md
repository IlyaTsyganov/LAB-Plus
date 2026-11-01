# Active encounters

Open **Research → Active encounters** to inspect sampled managers, technical names, available map-scoped reference names, states and script counters. Search the table to find a manager.

An active observation reflects the manager's current thinking state. It does not by itself prove that combat has started, finished, unlocked an exit or executed a script. Missing/stale data is distinct from a confirmed inactive manager.

## In-game display

With the matching layer and compatible meathook loaded, enable the encounter HUD in the tool or use:

```text
labplus_encounters 1
```

Use 0 to hide it. `labplus_encounters_list` prints observed active managers; `labplus_encounters` without arguments also lists them.

The shared Research HUD places Encounters beneath Volume inspector and Support plane, moving upward when earlier blocks are hidden. It uses the full viewport with a common text size, wrapping and columns.

```text
labplus_debughud_scale 1.25
```

Scale is 0.5–3. Menus/pause, console visibility and stale data can hide the display.

## Enemy lists

The encounter tool describes **managers**. For living enemies grouped by their actual active manager, current/max HP and world HP/GK panels, open [Invisible numbers](Invisible-numbers). An enemy outside an active manager list can still receive a world HP panel without being assigned to a made-up encounter.

Unknown reference names remain technical names. See [Entity links and level comparison](Entity-links-and-level-comparison) for observed references and before/after data, and [Compatibility](Compatibility) for prerequisites.
