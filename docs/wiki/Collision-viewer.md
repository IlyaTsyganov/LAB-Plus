# Collision viewer

Use **Viewer** to show sampled physics geometry over the game. The viewer does not edit collision or make walls passable.

| Control | Purpose | Fresh value |
| --- | --- | --- |
| Collision Viewer | Viewer master | Off |
| World collision | Player-blocking world geometry | On |
| Triggers | Recognized trigger volumes, subject to filters | On |
| Invisible walls | Recognized wall volumes | On |
| Style | Filled surfaces or outlines | Filled |
| Distance | 20–200 metres | 40 m |
| Volume inspector | Pick/highlight a volume under the crosshair | Off |
| Disabled volumes | Show observed disabled volumes | Off |
| Volume history | Retain recent historical geometry | Off |

Saved preferences replace fresh values on later runs.

## Appearance and filters

World collision, Triggers and Invisible walls have **separate colors and fill opacity**. Inspector highlighting also has its own appearance. Custom preferences are retained.

The **Types** button beneath the trigger visibility switch opens exact trigger-class and nested path filters. Turning off one geometry group does not disable the other groups.

Use [Trigger filters and volume inspector](Trigger-filters-and-volume-inspector) for identification, disabled outlines and history.

## First use

1. Select the correct executable and enable the viewer before launch.
2. Launch through LAB Plus and load a level.
3. Start at 40 m.
4. Hide world collision temporarily to focus on triggers/walls.
5. Compare filled surfaces with outlines and aim at a volume with Inspector enabled.

A first enable or DLL update requires a **full game restart**. The checkbox alone does not prove the current game has loaded that layer.

## Visibility and performance

Geometry uses distance/frustum filtering and scene-depth occlusion. Menus/pause hide the overlay; the open console clips it. Unreadable camera/geometry or an unrecognized executable can withhold drawing.

A larger radius or dense scene can cost more rendering work. Reduce distance or hide unnecessary groups when investigating performance. Internal geometry caching preserves the geometry rather than suppressing overlapping surfaces; its isolated CPU benchmark is not an FPS guarantee.

HP/GK panels are controlled separately in [Invisible numbers](Invisible-numbers), including when the collision geometry display is off. See [Compatibility](Compatibility) and [Troubleshooting](Troubleshooting).
