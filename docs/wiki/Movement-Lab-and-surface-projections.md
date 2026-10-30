# Movement Lab and surface projections

## Named attempts

Open **Research → Movement Lab** to inspect the player's physical body, velocity and support trace. Start a named attempt, perform the route and stop recording. Assign **Success**, **Failure** or leave the outcome unmarked; these labels are user annotations, not automatic gameplay judgments.

Select up to **six** compatible map/build attempts for comparison. Use XY, XZ or YZ projection, choose a reference attempt, scrub elapsed time or follow the latest sample. **Align starts** shifts the plot for comparison; it does not rewrite saved world positions. CSV export retains all recorded samples even if the display decimates a long route.

Recording runs at about **20 Hz**. The native rolling buffer holds 256 rows and is published about every 200 ms. Menu states, recording boundaries, gaps over 250 ms and jumps over 50 game units break routes. Cross-attempt comparison uses the nearest sample within 75 ms and leaves missing intervals unavailable.

## Collider and target measurements

The player collider uses verified convex geometry with Havok rounding. Its pose refreshes each rendered frame using cached geometry; recorded measurements retain the lower sampling rate. Bounds, orientation and reflected body data are validated. Unknown shapes are reported as unavailable or explicitly labelled bounds rather than presented as an exact collider.

An aimed or held target carries entity ID and generation. Origin-to-boundary distance and rounded-body **skin gap** are different quantities. A negative skin-gap estimate is not a penetration-depth measurement and does not prove a trigger fired. Unknown geometry or work exceeding the 50,000 triangle-pair limit withholds the skin-gap value.

## Surface projections

Open **Surface projections** for live, held or recorded-time support data. The view cuts the sampled support plane against XY/XZ/YZ axis planes and displays projected normals, signed tangent angles and gravity-relative slope. Hold preserves an observation; it does not freeze the game.

A flat horizontal plane has no unique XY intersection line. One support plane also cannot establish the edge angle between two adjacent faces. Treat unavailable/coincident cases as geometric limits rather than zero angles.

The optional native numerical HUD uses `labplus_surface_show`; player and attempt visibility use `labplus_player_show` and `labplus_attempts_show`. Recording and outcome assignment belong to the Movement Lab interface. Files include `attempts/*.json`, `motion-<PID>.tsv` and `motion-overlays.tsv` under the local data directory.
