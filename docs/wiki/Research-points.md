# Research points

Research points are named observations with categories and notes. They are unrelated to the game's upgrade-point currency.

## Capture and organize

Open **Research → Research points** in a loaded level. Capture a point, then give it a name, route/category and notes. Search/edit the library and attach checkpoint-archive links when useful.

A point can retain physical body position, camera eye/aim, map/build, loaded checkpoint label and selected volume identities. Body position and rendered camera position are kept separate.

Use JSON export/import for collections. Imported entries get new local identities, so linked archives may need relinking.

## Return data

**Copy return command** copies numerical `setviewpos x y z yaw pitch` data. It does not execute it. Pasting a supported game command is a separate action and does not restore AI, inventory, velocity, scripts or progress.

Spawn data describes camera eye, yaw/pitch and zero roll rather than a saved actor pose. Historical entity IDs/generations are evidence from capture, not valid live targets after reload.

Points are stored in `research-points/*.json` beneath the [data directory](Data-and-migration). Checkpoint archives remain separate and follow [their own restore rules](Checkpoints-and-recovery).
