# Active encounters

Open **Research → Active encounters** to inspect sampled encounter managers and available map-scoped reference names. Search the observations in the companion; enable the optional native HUD when a compatible meathook is loaded.

`labplus_encounters_list` prints available active encounter information through the game integration. `labplus_encounters_show 1` enables the HUD and `0` disables it. The shorter `labplus_encounters` lists with no arguments and accepts `0|1` as a compatibility HUD switch.

An active manager or raw field value is an observation of the sampled engine data. It is not a universal statement that combat is complete, an arena is unlocked or a script has run. Reference names depend on available map metadata. Unavailable reads and stale observations should be resolved before drawing conclusions.

The encounter HUD and its appearance still need in-game acceptance with the rebranded layer after a full restart. Automated fixtures use a recording HUD backend and cannot prove that text is visible in a live game. See [Compatibility](Compatibility) and [Troubleshooting](Troubleshooting).
