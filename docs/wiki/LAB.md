# LAB

The **LAB** tab brings the ordinary LAB features into one place. It works separately from the optional Research meathook integration. Select the matching game executable, launch through LAB Plus and keep the companion running.

## Speedometers

Configure three readings independently:

| Reading | Meaning |
| --- | --- |
| Current | Current horizontal speed |
| Landing | Horizontal speed at landing, including the brief impact peak |
| Vertical | Vertical speed; optionally show direction as positive/up and negative/down |

Each has its own visibility, position, size and RGB color. The master switch controls whether speedometers are drawn while preserving secondary choices. Check positions after changing game resolution.

A starting console configuration is:

```text
labplus_speedometer_show 1
labplus_speedometer_showlanding 1
labplus_speedometer_showvertical 1
labplus_speedometer_verticalsign 1
labplus_speedometer_size 2 current
labplus_speedometer_color 255 255 255 current
```

Enter a LAB command without arguments to query its value and help. The [console reference](Console-commands-and-cvars) lists exact ranges and target syntax.

## Input and display fixes

- **Q/E + Modbot input protection:** enables the input fix; its availability or standby status is shown separately.
- **Low-health red-screen opacity:** 100 is the original opacity; lower values reduce both supported red layers.
- **AMD driver-line hiding:** hides the driver's line in the supported metrics display.
- **Lighting Fix:** eligible only on NVIDIA with verified game 1.0, 1.1 or 2.0. It remains off on AMD, Intel, unknown GPUs and other game versions.

## The MR indicator

Only two letters are used:

- **M:** the Q/E + Modbot fix preference is enabled.
- **R:** red-screen opacity differs from 100%.

Other LAB features can work without adding letters. Read the LAB status when checking whether a selected feature is available on the current process.

## Changed cheat cvars and player cheats

Eligible nondefault cheat cvars and their values appear beneath the LAB Plus indicator. Menu/profile settings such as sound volume, ordinary game difficulty and graphics preferences are excluded, including when changed later on game 5.1. True debug and gameplay controls remain eligible.

The display uses the DESRU-Enhanced session filtering behavior: initial nondefault preferences at attachment are kept out of that game session's list, even if changed again. Reopening LAB Plus against the same running game preserves that session. Other eligible entries disappear when restored to their factory defaults. Long lists page automatically.

The list shows current values, including eligible changes made by the game itself.

Active **noclip**, **notarget** and **god** are read from the player independently of cvar filtering. The game's existing argument-free toggle commands are supported; LAB Plus does not replace them with new cheat commands.

For missing readings or stale cvar text after an update, see [Troubleshooting](Troubleshooting).
