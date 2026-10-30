# Research points

Research points are named observations, categories and notes. They are unrelated to the game's inventory or upgrade-point currency.

## Capture and organize

Open **Research → Research points** and capture a point in a loaded level. Give it a name, category/route and notes. Search and edit the library; attach checkpoint-archive links when useful. Export/import JSON to exchange point collections. Imported points receive new local IDs, so archive relationships may need relinking after import.

A point can retain physical body position, rendered camera eye/aim, map/build, loaded checkpoint label, selected volume IDs/generations and notes. Physical actor position is kept separate from the rendered camera. Capture rejects inconsistent moving samples beyond its 0.1-unit tolerance; camera data during menus can still represent the last rendered frame.

## Return and spawn data

**Copy return command** copies numerical `setviewpos x y z yaw pitch` data. It does not execute the command or restore AI, inventory, scripts, velocity or progress. Pasting it into the game console is a separate deliberate action and requires the game's command support.

Spawn information describes camera eye, yaw and pitch with roll zero; it is not a saved actor pose. Historical entity IDs/generations are evidence from the capture and must not be treated as valid live identities after reload.

Points live in `research-points/*.json`. The library supports up to 10,000 entries and does not automatically delete older points. Checkpoint archives occupy separate folders and have their own authentication/restore rules; a research point itself is not a save state.
