# Metrics row classification and ordering: local preview 0.8.20

The 0.8.18/0.8.19 audit incorrectly described the two `%s` calls between Vulkan and VRAM as the complete CPU/GPU block. A complete formatter-call audit shows a third, earlier string call on 5.1 and 6.x. CPU text therefore escaped the hardware filter. In addition, the earlier filter could forward an unrecognized trailing string directly, bypassing AMD driver-label capture. That left the driver visible before LAB Plus. The earlier hardware-row interpretation in `hardware-metrics.md` is superseded by this correction.

Verified return RVAs from the actual installed executables:

| Executables | Earlier string | Trailing string 1 | Trailing string 2 |
| --- | --- | --- | --- |
| 6.0, 6.1, 6.2, 6.3, 6.4 | `0x47d939` | `0x47db6b` | `0x47db98` |
| 6.66 | `0x48ec49` | `0x48ee88` | `0x48eeb5` |

On 6.66, the earlier row reads the HUD CPU string. Initialization copies the system processor string and strips leading/trailing whitespace; it is not a device-name call in the later Vulkan block. The full call audit was also run on all 11 currently installed EXEs. No executable was run by that audit.

The filter now examines candidates throughout the verified metrics caller interval. It has no two-call hardware whitelist. Actual hardware labels are matched against the immutable machine-name bank. Arbitrary DESRU text remains visible. AMD driver recognition runs first and independently of hardware matching or Hide state, for both a `%s` argument and a literal replacement format. This prevents hardware matching from bypassing driver handling.

The complete original driver row is deferred until after the LAB indicator and cheat cvars: all eight stack argument slots and the original font scale are retained. The saved row is replayed only when Hide is off. A driver row occupying the final callback also reaches indicator emission before replay. The adapter signature is `LPMET006`; older live adapters require a game restart. The name bank starts at state offset 320 with 14 bounded records; saved driver arguments occupy a separate area inside the existing 4096-byte allocation.

Regression fixtures now reproduce three independent string calls, a CPU call outside the previously selected pair, a driver arriving before GPU, literal driver formats, and a driver occupying the final callback. Both Hide states, indicator-before-driver order, preserved font/printf arguments, custom DESRU text, changed frame-time text and lease shutdown are executed as native x64 calls. Full launcher checks and 398 managed checks with all 11 installed executable layouts passed. Subsequent metrics-only runs do not start any game-like child process.

The full HUD suite initially ran its existing lifecycle fixture, a temporary test executable named `DOOMEternalx64vk.exe`; the retail game executable was never launched. After the user's question about that process, no further lifecycle fixture was run. Live visual confirmation of the new build remains pending. Game installations, saves, Wiki and preferences were not changed.
