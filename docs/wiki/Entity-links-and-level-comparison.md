# Entity links and level comparison

## Entity links

Open **Research → Entity links**. Follow the volume under the crosshair or search technical/reference names. Select a node to inspect observed incoming/outgoing references and the exact reflected fields that support each edge.

| Input | Action |
| --- | --- |
| Mouse wheel | Zoom around the pointer |
| Right-button drag | Pan |
| Left click | Select a node |
| Fit graph | Fit all neighboring nodes |
| Double-click a related row | Explore that entity |

All distinct neighbors participate in automatic fit. Identical periodic refreshes preserve a manually adjusted view; changing the root or map fits a new graph. Linked-only filters and crosshair following help reduce noise. Export JSON to retain the full sampled graph.

The reader validates reflected metadata, map slots and generations. Missing metadata, stale samples, unreadable fields and scan limits remain visible. An edge is an **observed runtime reference**, not execution order or proof of a causal script dependency. Static map and script-only links are not decoded.

## Level state: before / after

1. Open the level-comparison tool and capture **Before**.
2. Perform one experimental action in the game.
3. Capture **After** in the same process, build and map instance.
4. Review identity, generation, state, transform, geometry and raw encounter-manager changes.
5. Highlight a selected change or all changed volumes, choose Before/After outlines and add notes.
6. Export a JSON pair or the complete changes CSV.

Snapshots include recognized volumes, available encounter-manager fields and the loaded checkpoint name. Missing/unavailable reads remain distinct from confirmed additions/removals. The checkpoint label indicates loaded data; it does not prove a new checkpoint was saved.

Map/process/token mismatches prevent a misleading comparison. At most 256 changed volume outlines are submitted. An optional distance override applies to highlighted comparison geometry. Saved pairs remain useful as data even after the process-local native outline cache is gone.
