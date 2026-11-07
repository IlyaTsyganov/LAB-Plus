# Finisher Visualizer, v28

Research now has a separate **Finisher Visualizer** tool. Its enable switch,
Glory Kill / Chainsaw mode, animation selection and target catalogue moved
out of Invisible numbers without changing their saved settings keys.

The screen has an English selection summary, separate position-zone and
Glory Kill body-colour legends, and an expandable English guide. The Russian
and English standalone guides are packaged in `help/finisher-guide.ru.txt`
and `help/finisher-guide.en.txt`; the same source texts live in `Assets/help/`.
Only English is displayed inside the application.

Chainsaw no longer builds or draws body-colour meshes. Its position zones
and tracked-enemy box remain. A renderer guard also rejects a cached Glory
Kill body mesh after changing mode. Zombie Tier 1 front 01 / 02 are labelled
as random variants rather than a guaranteed prepared clip.

The finisher debug HUD now has at most six lines: mode, target, selected or
automatic preview, short status, distance/limit when available and a brief
aiming or random-variant hint. Candidate lists, implementation terminology,
geometry dimensions and repeated explanations were removed from this block.
Other HUD features are unchanged.

Main files:

* `UI/FinisherVisualizerForm.cs`, `UI/FinisherGuide.cs`: screen and guide.
* `UI/InvisibleNumbersForm.cs`: remaining health/timer/tech tools.
* `UI/MainForm.*`, `UI/WorkspacePresenter.cs`, `Assets/ui/main.js`: navigation.
* `Assets/ui/help.js`, `Assets/ui/workspace.css`, `Assets/help/`: help and legend.
* `Native/CollisionViewer/src/sync_target_preview.h`: compact HUD, random-front
  label and Chainsaw body-mesh suppression.

Validation passed:

* Release publish, no managed build warnings or errors.
* Native sync-target suite, including 25-family selection replay, 8,961
  positions, 336,888 group/quadrant comparisons and 1,722,066 GK triangle-colour
  checks. Chainsaw body suppression is checked across the reviewed variants
  and captured models/views; stale cached GK meshes are also covered.
* Five related native suites: sync interactions, enemy telemetry, inspector,
  viewer behaviour and performance.
* Launcher/settings fixture suite.
* Actual Research navigation to the new screen; enable/mode/animation
  controls; independent saved GK/Chainsaw choices; enemy changes, pause and
  target loss; English-only guide; both standalone resources; random-front label.
* Invisible numbers browser flow and both windows at 125% / 150% scale.
* Browser screenshots inspected for Research entry, legends and full guide.

The extra desktop-capture check failed with `window capture timed out`.
Built-in WebView interaction checks and captured browser screenshots completed
successfully. The isolated preview was closed; the user's game was not controlled.

Local logs: `build/finisher-v28-sync.log`, `build/finisher-v28-related.log`,
`build/finisher-v28-launcher.log`, `build/finisher-v28-visual.log`,
`build/finisher-v28-numbers.log`, `build/finisher-v28-publish.log`.

Package: `build/playtest-finisher-visualizer-v28/LABPlus.exe`. The existing
lighting module is preserved unchanged. No branch, commit or PR was created.
