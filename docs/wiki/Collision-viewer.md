# Collision viewer

Open **Viewer** to control the geometry overlay. The overlay displays sampled physics geometry over the game; it does not edit collision or make walls passable.

| Control | Meaning | Initial value |
| --- | --- | --- |
| Collision Viewer | Enable the scoped layer/viewer | Off |
| World collision | Player-blocking world collision | On |
| Triggers | Recognized trigger volumes, subject to class/path filters | On |
| Invisible walls | Recognized invisible-wall volumes | On |
| Draw style | Filled surfaces or outlines | Filled |
| View distance | Rendering distance, 20–200 metres | 40 m |
| Volume inspector | Pick/highlight a volume under the crosshair | Off |
| Disabled volumes | Pale outlines for observed disabled volumes | Off |
| Volume history | Recent removed/unreadable geometry snapshots | Off |

Saved preferences replace these initial values on subsequent launches. Geometry groups are independent: turning off triggers does not turn off world collision. Exact trigger-class and nested path switches are under **Trigger types**.

## Rendering behavior

All profiles use player-blocking contents filtering, frustum/distance culling and depth occlusion. Menu/pause states hide the overlay and console clipping avoids drawing through the open game console. Large distances or many visible volumes can reduce performance; start at 40 m and increase only as needed.

The layer handles a separate graphics/presentation queue when the Vulkan device requires it. An unknown profile, unreadable camera or invalid geometry can withhold drawing. An enabled checkbox does not prove the loaded process has the layer: check the runtime log and restart the game after enabling or replacing it.

## Typical workflow

1. Enable the viewer before launch and load a level.
2. Temporarily disable world collision to focus on triggers and walls.
3. Reduce distance to isolate the immediate area.
4. Use outlines to see volume boundaries, then the inspector for identity and metadata.
5. Use Research tools to record an observation rather than infer script execution from color alone.

Console equivalents and exact ranges are in [Console commands and cvars](Console-commands-and-cvars).
