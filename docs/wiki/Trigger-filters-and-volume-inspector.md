# Trigger filters and volume inspector

## Types

On Viewer, use **Types** beneath the trigger visibility switch to open the Trigger types window.

A top-level switch controls an exact runtime class. Nested folders refine instance names and entity-definition paths inside that class. The catalogue includes packaged entries and live discoveries.

A disabled parent overrides descendants while preserving their choices. Re-enable the parent to restore those choices. Entries may remain in the catalogue when absent from the current map.

Filters apply to drawing, history and picking. The command `labplus_trigger_set <exact-class> 0|1` changes one recognized class; use the window for hierarchical folder filters.

## Crosshair inspector

Enable **Volume inspector** and aim at a recognized trigger or invisible wall. The selected geometry uses its own highlight appearance, separate from the World/Trigger/Wall colors.

The HUD shows the available identity, class, name, collision state and role. It uses the same full-viewport Research HUD font and layout as Support plane, Encounters and Invisible numbers:

```text
labplus_inspector 1
labplus_debughud_scale 1.25
```

A compatible meathook is needed for the metadata/HUD. Picking and text availability are distinct; missing text is not evidence that the volume is absent.

The Inspector retains its labelled rows while no target is selected. Hidden blocks do not leave fixed gaps above the other Research blocks.

## History and interpretation

Disabled volumes can be shown as pale outlines. Observed changes can flash, and history retains labelled removed/unreadable geometry for up to 15 seconds.

A vanished shape can reflect an unreadable sample, transition or changed identity. It does not alone prove deletion or script execution. Historical geometry and recycled IDs/generations are not current live entities.

Use [Entity links](Entity-links-and-level-comparison) for sampled references and [Research points](Research-points) for annotated observations. See [Collision viewer](Collision-viewer) for appearance, distance and rendering controls.
