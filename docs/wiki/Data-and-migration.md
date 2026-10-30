# Data and migration

## Default location

Settings, research data, runtime files and local checkpoint archives are stored beneath `%LOCALAPPDATA%\labplus`. The executable folder is separate. An absolute `LABPLUS_DATA_DIRECTORY` environment variable overrides the data root for both the companion and native layer; it is useful for isolated previews and tests.

Typical data includes saved preferences, `viewer-runtime/VulkanViewer.log`, research journals and `.full.tsv` files, `attempts/*.json`, motion TSV files, `research-points/*.json` and checkpoint-archive folders. Use the application's log/data controls to locate files rather than assuming a different Windows account's path.

## Migration from the previous product

On first use, LAB Plus copies previous local preferences and research/checkpoint archives into the new data root. It upgrades recognized format tags, retains the originals and never overwrites existing LAB Plus files. Stale runtime DLLs, pending `.request` files and `.tmp` files are excluded. Encrypted checkpoint containers are copied byte for byte.

A `.legacy-import-complete` marker records the import. Previous names remain only in explicit import/registration-cleanup compatibility code and historical attribution. Previous console prefixes are inactive; update personal scripts and bindings to the names in the console reference.

## Runtime identifiers

| Purpose | Identifier |
| --- | --- |
| Application | `labplus.exe` / `labplus.dll` |
| Native library | `labplus-layer.dll` |
| Vulkan layer | `VK_LAYER_LABPLUS_collision` |
| Layer negotiation export | `labplus_negotiate_layer_interface_version` |
| Same-user command pipe | `labplus.commands` |
| Presence event | `Local\labplus.commands.Active` |
| Data-root override | `LABPLUS_DATA_DIRECTORY` |
| Child lighting opt-in | `LABPLUS_LIGHTING_FIX` |

The command protocol validates the selected process and does not forward arbitrary shell or engine commands. Vulkan registration is per user and scoped to the selected executable. A conflicting unrelated Vulkan override is reported rather than overwritten. Exports and checkpoint archives are user data and are not included in the public source or binary release.
