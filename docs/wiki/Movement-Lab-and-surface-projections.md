# Movement Lab and surface projections

## Record and compare attempts

Open **Research → Movement Lab**. Start a named attempt, perform the route and stop recording. Mark it Success, Failure or unmarked; these are your annotations, not automatic gameplay judgments.

Compare up to **six** compatible map/build attempts. Choose XY/XZ/YZ projection, a reference attempt, elapsed-time scrubbing or follow-latest. **Align starts** changes the comparison view without rewriting saved world positions.

CSV exports retain all recorded samples even when the plot reduces detail for a long route. This recorder is separate from the general [Research timeline](Research-timeline).

## Collider and targets

The player collider display follows the current pose while recorded movement uses sampled observations. Unknown geometry stays unavailable or is clearly labelled as bounds.

An aimed/held target carries identity and generation. **Origin-to-boundary distance** and **skin gap** measure different things. A negative skin-gap estimate is not penetration depth and does not prove that a trigger fired.

Menus, recording boundaries, unreadable data, timing gaps and large jumps break routes rather than filling them with invented movement. A missing comparison sample remains unavailable.

## Surface projections

Open **Surface projections** to inspect live, held or recorded-time support data. XY/XZ/YZ cuts show projected normals, signed tangent angles and gravity-relative slope.

**Hold** retains an observation; it does not freeze the game. A horizontal plane has no unique XY intersection line, and one support plane cannot establish the edge angle between two adjacent faces.

## In-game displays

Use the UI or supported actions:

```text
labplus_player 1
labplus_surface 1
labplus_attempts 1
labplus_debughud_scale 1.25
```

Support plane shares the Research HUD's common font/scale and moves upward when the Inspector block is hidden. Attempt recording/outcome controls remain in Movement Lab.

Saved attempts are under `attempts/*.json` in the [data directory](Data-and-migration). See [Compatibility](Compatibility) for metadata and HUD prerequisites.
