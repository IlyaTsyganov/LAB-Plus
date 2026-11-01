# Launch ownership: local preview 0.8.17

An external launch of the selected game (including a Steam non-Steam shortcut) previously enabled LAB Plus. Two independent paths allowed it: settings changes registered an implicit Vulkan override for the executable path, and the managed HUD admitted any game started after the companion.

The main launcher now configures explicit collision and lighting layers only in the child process environment. Settings changes never register layers. On normal application startup, old LAB Plus registry entries are retired; unrelated layer registrations are preserved. Existing versioned runtime DLLs remain available to already running processes.

The managed session requires a launch record matching PID, process creation time and executable path, regardless of startup order. It polls for a late launch record to handle detection racing process creation. A loaded lighting DLL is no longer treated as ownership evidence. External games receive no HUD adapters, including the former in-game restart notice; the restart status remains in the application. Native features additionally require the child-only `LABPLUS_LAUNCHED=1` marker and a live companion.

Validation uses isolated non-game child processes and a real Vulkan instance. Launcher tests verify that explicit-layer configuration preserves other layers, deduplicates paths and leaves the parent environment unchanged. HUD fixtures cover external launches before and after the companion, delayed ownership publication, process exit, PID reuse and companion restart. Native lifetime fixtures reject a missing/disabled launch marker even with a live companion. The real Vulkan loader probe verifies absence of both LAB Plus layers for the external case and successful loading of collision plus lighting for the opted-in child.

All launcher checks, 297 managed HUD checks and all 23 native regression suites passed. The real Vulkan loader accepted both the negative external case and the positive collision/lighting case. Production layer and launcher builds succeeded.

The running retail game and its installation were not modified or restarted during this fix. To test 0.8.17, close the game and the older companion first. Open the new companion, verify a Steam launch stays inactive, then close the game and launch it through LAB Plus. Patch support added through 0.8.16 is retained. Wiki and release request files remain unchanged.
