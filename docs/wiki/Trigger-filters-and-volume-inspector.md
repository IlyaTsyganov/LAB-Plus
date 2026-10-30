# Trigger filters and volume inspector

## Trigger types

Open **Viewer → Trigger types**. Filters are persistent and class scoped. A top-level switch controls an exact trigger class; nested folder switches refine instance names and entity-definition paths within that class. The bundled resource catalogue is extended by live discoveries.

A disabled parent overrides its descendants while preserving their individual choices. Re-enabling the parent restores those choices. Entries can remain in the catalogue when absent from the current map. Flat names do not receive invented folder levels. Matching both instance names and definitions helps retain a resource subtype after a map renames an instance.

Filters apply to surfaces, outlines, history and picking. Unknown or invalid class names are rejected by the command interface. `labplus_trigger_set <class> 0|1` changes **one exact class**; use the interface for hierarchical path filters.

## Crosshair inspector

Enable **Volume inspector**, point at a recognized trigger or wall and inspect the highlighted geometry. A compatible meathook supplies native HUD text for entity identity and available fields. Geometry/picking and HUD availability have separate prerequisites; missing text is not evidence that a volume is absent.

Observed identity includes the entity ID and generation where available. A recycled ID or a different map instance must not be treated as the same live object. Use **Entity links** to explore resolved references and **Research points** to retain an annotated observation.

## Disabled volumes and history

Advanced options can show disabled volumes as pale outlines. State changes can flash. Volume history retains labelled snapshots of removed or unreadable geometry for up to **15 seconds**.

These are sampled observations. A vanished shape may represent an unreadable sample, a map transition or an identity change. It does not by itself prove deletion, trigger activation or execution of a script. Historical geometry is not a currently valid live entity.
