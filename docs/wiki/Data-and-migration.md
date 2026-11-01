# Data and migration

## Locations

The default data root is **%LOCALAPPDATA%\labplus**. The application folder is separate.

| Data | Typical location |
| --- | --- |
| Main preferences | Data root |
| LAB preferences | `hud/settings.json` |
| Native runtime log | `viewer-runtime/VulkanViewer.log` |
| Application log | `logs/labplus.log` beside LABPlus.exe |
| Timeline journals | Application data; export the complete CSV from Timeline |
| Movement attempts | `attempts/*.json` |
| Research points | `research-points/*.json` |
| Checkpoint archives | Separate archive folders beneath the data root |

Use the application's data/log controls where available. Keep the data directory when updating the program.

## Upgrading

Install the complete new program package while retaining your data directory. After a DLL update, fully restart the game through the updated LABPlus.exe.

On first use, migration copies recognized earlier preferences, research data and checkpoint archives into the new root. Originals are retained and existing LAB Plus files are not overwritten. Temporary requests and old runtime DLLs are not imported as current data.

Update personal bindings to `labplus_` command names. Earlier prefixes are not registered.

## Keeping and sharing data

Export a complete timeline CSV, graph JSON, level-comparison pair, attempt or research-point collection from its tool when you need a portable observation. Checkpoint archives have separate authentication and restore rules.

Keep before-restore evidence until a checkpoint restore is verified. Do not merge partial restore directories.

Before attaching logs or exports to an issue, review them for personal paths and account information. Share the relevant excerpt or export, rather than an entire data directory or checkpoint archive by default.
