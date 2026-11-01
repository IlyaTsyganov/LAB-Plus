# Entity links and level comparison

## Entity links

Open **Research → Entity links**. Follow the crosshair volume or search technical/reference names. A selected node shows observed incoming/outgoing references and the fields supporting each edge.

| Input | Action |
| --- | --- |
| Mouse wheel | Zoom around the pointer |
| Right-button drag | Pan |
| Left click | Select |
| Fit graph | Fit the neighboring nodes |
| Double-click a related row | Explore that entity |

Identical refreshes preserve a manually adjusted view. A new root or map can fit a new graph. Use linked-only filtering to reduce noise and export JSON to retain the complete observation.

An edge is an **observed runtime reference**, not execution order or proof that a script caused another action. Static map/script-only links are not decoded. Missing metadata, stale reads and scan limits remain explicit.

## Before / after

1. Open level comparison and capture **Before**.
2. Perform one deliberate action in the game.
3. Capture **After** in the same supported process/build/map instance.
4. Review identity, generation, state, transform, geometry and encounter-manager differences.
5. Highlight selected/all changed volumes, choose Before/After outlines, add notes and export JSON/CSV.

A loaded checkpoint label is an observation of loaded data, not proof that a new checkpoint was saved.

Map/process/identity mismatches prevent a misleading comparison. Missing reads are distinct from confirmed additions/removals. Highlight geometry is bounded, so use the complete exported differences when reviewing a large change set.

Historical node identities and cached outlines must not be treated as current live entities after reloading a map.
