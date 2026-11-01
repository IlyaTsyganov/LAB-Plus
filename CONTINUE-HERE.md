# LAB Plus — контекст продолжения

**2026-11-01. LAB Plus 0.8.10: local implementation and checks complete.** Remote code/release/Wiki publication is blocked by the current GitHub connection; the exact status and remaining steps are recorded at the end of this file.

Repo: [IlyaTsyganov/LAB-Plus](https://github.com/IlyaTsyganov/LAB-Plus); workspace `C:\workspace\LAB-Plus`, ниже repo-relative paths. Не путать с отдельным LAB. Запрошены code push и полный ZIP в Releases после Praetor; публикацией занимается root.

## 1. Обязательные требования

- EXE ровно **LABPlus.exe**, assembly/deps/runtimeconfig — `LABPlus`; бренд — LAB Plus. Наши команды начинаются `labplus_`, не `lab_`.
- Одна вкладка **LAB** со всеми функциями обычного LAB. UI/Help английские; Help содержит управление, а не внутренние детали вроде процента уменьшения шрифта.
- Только индикаторы **M** (Q/E + Modbot fix) и **R** (красный экран не100%). Остальные функции продолжают работать без своих букв.
- **Не исключать отдельно `viewInfo_viewAngleimportanceChainsaw`**: оставить в общем cvar-фильтре. Engine-side изменение не повод для специального blacklist.
- Меню/громкость/обычная сложность/графические настройки не выводятся как cheat cvars. Настоящие debug/AI/damage controls остаются допустимыми.
- Lighting Fix фактически разрешён только для audited **1.0/1.1/2.0 на NVIDIA**. AMD, Intel, unknown GPU и другие EXE — off до загрузки и в runtime.
- Запуск требует эффективного outbound firewall block **точного выбранного EXE**. LAB Plus проверяет, не создаёт/включает правила.
- Не возвращать подавление перекрывающейся/поглощённой collision geometry.
- Не рекламировать незавершённый battle/save-state проект. Реализованная checkpoint library использует completed autosaves и отдельные guards.
- Телеметрические пробы читают память, не симулируют ввод и не меняют бой. Демонстрации выполняет человек по согласованию; уже доказанное действие не повторять ради лога.
- В результате давать проверенные ссылки на локальные EXE/папку/ZIP и настоящий Release, а не ссылку на диалог.

## 2. Архитектура

Windows x64, .NET10 WinForms/WebView2, C++17 Vulkan layer. Нужны .NET10 Desktop Runtime x64 и WebView2 Evergreen; извлекать весь framework-dependent ZIP.

| Область | Исходники |
| --- | --- |
| UI/host | `UI/MainForm*`, `WorkspaceWindow`, `WorkspacePresenter`, `UI/State`; packaged `Assets/ui`; одна WebView2 на окно |
| Обычный LAB | `Hud/Runtime/GameSession.cs`, `Settings.cs`, `ConsoleControls.cs`; `Hud/Overlay/NativeGameLayout.cs` и адаптеры HUD/input/tint/metrics |
| Launch/policy | `Game/GameLauncher.cs`, `Hud/Runtime/GameLauncher.cs`, `VulkanLayerRegistration.cs`, `GameFirewall.cs`, `LightingFixPolicy.cs` |
| Preferences | `Configuration/LabPlusSettings.cs`; отдельные LAB preferences в `Hud/Runtime/Settings.cs` |
| Viewer | `Native/CollisionViewer/src/vulkan_layer.cpp`, `depth_*`, `live_walls.h`, `runtime_scene.h` |
| Appearance/performance | `volume_style.h`, `volume_appearance.h`, `world_vertex_cache.h` |
| Identity/reflection | `native_profile.h`, `entity_metadata.h`, `entity_graph.h`, `meathook_api.h`, `game_memory.h` |
| Research HUD | `debug_hud_stack.h`, `debug_hud_render.h`, `meathook_hud.h`, `active_encounters.h`, `movement_lab.h` |
| Invisible numbers | `invisible_numbers*.h`, `enemy_telemetry.h`, `game_timers.h`, `sync_animation_timers.h`, `speedrun_techs.h` |
| HP/font | `enemy_health_bars.h`, `eternal_ui2_font*.h`, `vulkan_font_atlas.h` |
| Protocol/actions | `Game/InvisibleNumbersSnapshot.cs`, `LabCommands.cs`, `LabCommandServer.cs` |
| Checkpoints | `practice_live.h`, `practice_transaction.h` и managed archive/restore/backup workflow |

Data: `%LOCALAPPDATA%\labplus`; абсолютный override `LABPLUS_DATA_DIRECTORY`. LAB preferences отдельно в `hud`, override `LABPLUS_HUD_DATA_DIRECTORY`. Migration копирует старые данные, не портит originals/существующие настройки. `viewer.ini` перечитывается native примерно250ms. Логи: `logs/labplus.log` рядом с EXE; `viewer-runtime/VulkanViewer.log` в data directory.

Research использует уже загруженный compatible meathook (`XINPUT1_3.dll`/`mh_api_get`), который **не bundled**. Только game-frame callback вызывает его API; worker копирует данные, Vulkan thread читает immutable snapshots. Module pinning исключает unload под callback. Первый enable/обновление DLL требует **полного restart игры через новый LAB Plus**.

IPC same-user: pipe `labplus.commands`, event `Local\labplus.commands.Active`, PID выбранной игры, UTF-8 tab-separated request ≤2000bytes. Только allowlisted actions, не arbitrary shell/engine text.

## 3. История 0.8.1–0.8.9

| Версия | Результат |
| --- | --- |
| .1 | Joint collision/lighting manifests; early-NVIDIA policy; цветная EXE иконка во всех7размерах; folder вместо Browse; separator под drag bar; Types под trigger toggle как кнопка; focus child window в content; удаление неподдержанного save-state текста |
| .2 | Только MR, changed cvars, обязательный firewall; unlock tracking mutex перед driver Present wait; bounded immutable world cache |
| .3 | DESRU session baseline/filter с reconnect persistence; правильные цвета/opacity трёх категорий |
| .4 | Verified menu/profile/preset exclusion5.1; cvar font75%; устранены29реально воспроизведённых ложных строк |
| .5 | `LABPlus.exe`, краткий Help, точный firewall text, actual noclip/notarget/god flags; stack Inspector→Support→Encounters; debughud scale |
| .6 | Invisible numbers: grouped enemies/HP, bars, equipment/powerup/Sticky/current-sync timers, BATS/Fast DBlade; четыре action-команды и sampling по enabled features |
| .7 | Generic BATS rate calculation и Rocket positive; Fast DBlade с prepared hit; higher filled HP/numerals/GK; bars standalone AI; actual ownership |
| .8 | Missing Sticky charges queue/maxima; BATS через ordinary melee; full-viewport HUD/общий font/wrapping/columns без старого truncation; EternalUi2Regular atlas |
| .9 | Audited1.1 clocks/lifetime/BATS/GK; interactable-owned sync; независимые clip timers, без копирования GK в interaction; mantle отдельно |
| .10 | Praetor player-model release-event countdown; см. раздел8 |

`RELEASE_NOTES.md` содержит границы validation. Последний отчёт .9: **21native suites,24isolated GPU cases**, managed launcher/settings/protocol и UI build passed. Это не результаты будущей .10 и не acceptance всех действий в игре.

## 4. LAB, MR, cvars и launch

LAB adapters: три speedometers current/landing/vertical, position/size/RGB, signed vertical, low-HP tint, Q/E+Modbot, driver-line hiding и conditional lighting; независимы от Research meathook. Availability не равна preference.

Cvars рисуются под MR. `noclip`/`notarget`/`god` читаются из реальных packed player flags **после** cvar filtering, поэтому argument-free toggles и уже включённые состояния видны. Исходные игровые команды не переименовываются.

`ProtectedCvarMonitor` читает registry/resetString/type flags; comparison типизирован, integer не сводится к float. Sampler ~500ms вне render thread; stale/error rows скрываются. `CheatCvarSession.DisplayChanges` повторяет DESRU-Enhanced: initial nondefault имена исключаются на эту сессию даже после дальнейших изменений. Baseline `LPCVAR01` в game-shared allocation переживает reconnect companion; pointer slot `NativeSpeedometerCode.StateSize-8`.

На5.1 flag bits **не различают** menu preferences и настоящие cheats. `CvarDisplayPolicy.cs` — точные имена из SettingsDataSource/quality/profile bindings, не широкий prefix-ban. `r_umbraAccurateOcclusionThreshold` исключён как автоматическое renderer state. Настройки сложности из меню исключены; difficulty-specific damage/debug остаются. Evidence: `docs/validation/menu-cvar-references-5.1*`. Chainsaw targeting специального исключения не имеет.

Metrics magic `LPMET003`: `001/002` требуют restart, не hot rewrite. Cvar scale .75, MR/последующие metrics сохраняют размер. Immutable text banks освобождаются только после native acknowledgement; updates coalesce, paused/minimized игра не растит remote queue. Длинные списки меняют страницы раз в6s.

Firewall gate действует в **обеих** ветках, даже без optional layers, с повторной проверкой прямо перед Start. Нужны BFE/MpsSvc, active enabled profiles и unrestricted effective exact-path outbound block; restricted interfaces/addresses/service/identity не считать полным block. Точное сообщение missing rule:

```text
Launch blocked: enable firewall rule for the selected DOOM Eternal version(e.g. via DESRU).
```

Lighting: NVIDIA vendor0x10de + verified1.0/1.1/2.0; AMD0x1002/Intel/unknown/другие версии off. `VulkanLayerRegistration` передаёт child-scoped search **всех** owned manifests и явно collision layer: раньше lighting manifest скрывал viewer. Не исправлять это глобальным вмешательством в чужие layers.

## 5. Collision, UI и Debug HUD

Collision читает геометрию, не меняет физику. Независимые world/triggers/walls,20–200m, outlines/fill; exact-class/path filters, inspector, disabled/history, research timeline, Movement Lab, entity links и level comparison сохранены.

**Appearance slots ≠ mesh kinds:** slots World0/Trigger1/Wall2/Inspector3, mesh kinds Trigger0/World1/Wall2. Использовать explicit mapping `volume_style.h`. Defaults: cyan `#26ccff`, green `#40f21f`, orange `#ff730d`, inspector gold `#ffd91a`. Custom preferences не сбрасывать; tests различают RGB и zero/partial/full opacity.

Performance: commit depth boundary под lock, unlock **до** реального QueuePresent wait; после unlock не обращаться к незащищённому tracked state. Test проверяет record/submit пока Present blocked. Immutable transformed world cache:32MiB LRU, owner/control-block reuse invalidation; live transforms/colors/opacity свежие. Synthetic Fortress CPU preparation≈2.52–2.59→2.05–2.10ms — **не игровой FPS**. Overlap suppression не возвращать.

HUD stack: Inspector→Support→Encounters→Enemies→Timers→Techs, без пустот отключённых блоков. Inspector/Support сохраняют подписи без target/contact. Общий scale0.5–3. На1.0 engine persistent rows нижеY480 отбрасываются; метод игры не патчится. Stack публикует complete immutable generations, Vulkan layout использует весь viewport, wrapping/columns; persistent rows при handover удаляются, fallback остаётся. Старые caps «5encounters/фиксированные enemy rows/88chars» не возвращать. Visibility следует `com_debugHUD`, menu/console и freshness.

Font — `Assets/fonts/EternalUi2Regular.ttf`, baked18px ASCII R8 atlas256², generated source. Один quad/glyph; coverage×alpha ровно один раз, straight-alpha blend и world-depth occlusion. Runtime не требует Pillow/FreeType. Bake verify: `Native/CollisionViewer/tools/bake_eternal_ui2_font.py --verify`. CPU preview/benchmark — не screenshot/FPS игры.

## 6. Invisible numbers

Worker ~100ms только при open request или enabled group. Bits: enemies1/timers2/techs4/bars8; окно запрашивает1|2|4, bars отдельно. Callback копирует ≤4metadata types/frame. Не держать полный sampling при выключенных функциях.

Atomic `invisible-numbers-PID.tsv`: magic `#LABPLUS_NUMBERS_V1`,9header cells, E enemy/T timer/S tech. Parser: ≤2MB/8193lines/1024chars cell, PID/freshness/finite/duplicate validation; T accepts optional maximum. States: ready/cooldown/active/inactive/not_acquired/unavailable. Unknown — `?`/nullable, native max=-1; не подменять unavailable нулём.

**Enemies/HP.** E-table — actual manager spawn/spirit/adopted lists при thinkFlags≠0, не proximity assignment и не доказательство начала/окончания боя. Map/slot/id/generation/removed и managed reference stability обязательны. **spawnId и objectNumber — разные numbering domains.** HP из HITPOINTS, не shield; ancestry checked, component stride88на1.0/1.1,96на5.1.

`barEnemies` отдельно содержит живых map AI вне encounter тоже; membership E-table не выдумывается. Fresh world bounds/death/generation проверяются на render, stale>250ms не рисуется; zero dormant bounds не дают позицию. Higher panels: solid green→yellow→red, current/max numerals. При пропаже bars сначала проверить switch/restart: реальная жалоба оказалась выключенным toggle.

**GK.** Audited1.0/1.1/5.1 getter pairs → `aiPainState_t.currentPainReaction/finishStaggerTime/staggerLength`. Правильно **16 OR17**, не17-only: actual blinking Imp был16 с5sdeadline; recovery-код принимает оба. Flinch/falter/stun исключены. Remaining=(deadline−scaled game clock)/999960, pause сохраняет значение. В native tests есть реальные raw regression pairs. Старый watch ошибочно писал0 из-за predicate; recalculated значения явно derived, не притворяться исправным старым output. См. `stagger-window-1.0.md`, `stagger-window-1.1.md`, `stagger-deadline-layouts.json` в `docs/validation`. Не добавлять guessed hidden-byte filter:1.0 соседний byte был hasFinishedEntranceAnim.

**Equipment ownership.** Initialized counters не acquisition. `playerProps.validEquipmentList` сопоставляется с usable inventory item, count>0, owner=current player; flame допускает обе стороны declaration. Confirmed absence=Not acquired, unreadable=unavailable. Fresh intro реально имел counters без equipment inventory.

**Clocks/equipment.** Reflected equipment items:3на1.0/1.1,4на5.1; charge/recharge/reuse deadlines +flame next-use. Caller/getter instructions, map identity и scaled domain1 проверяются на1.0/1.1/5.1. Max — stored rechargeDuration; до первого recharge может быть неизвестен. Не вставлять fixed20/30s.

**Powerups.** Active status effects с declaration isPowerUp, actual timeLeft. Полярность важна: **inActive==0 означает active**. Max precedence: positive statusEffect_lifetimeSec → instance override → declaration lifetime. Negative/unbounded/unsupported не становятся guessed timers.

**Sticky Bomb.** Resource `weapon/player/shotgun_secondary_pop_rockets`; нужны owned usable weapon и grant exact declaration активным upgrade modifier (`WMT_WEAPON_DECL_WEAPON=40` проверен1.0/1.1/5.1). Cached selection/mastery bool недостаточны; bypass-ledger unlock может дать unavailable. Capacity/rates берутся из heat/mod overrides.

Partial heat cooling возвращает charges по очереди; empty-magazine overheatedDelay — все вместе. `sticky_charge_1..N` — **очередь missing charges**, не независимые часы гранат. Full reload даёт всем один deadline. Maxima из current delay/rate/per-mode config, не sampled high-water mark. Actual upgraded1.0: heat.2/.4/.6 partial; heat1+1999920ticks и общий2sreload пяти charges. Это loadout-specific, не constants для всех версий.

## 7. BATS/Fast DBlade/sync

BATS сравнивает actual bringUpAnimRate с audited normal configuration. Supported zero desired bring-up duration, mode0, no active status effects:

```text
normal = bringUpRateScale / max(max(dexterityWeaponChangeSpeed, FLT_EPSILON), weaponSwitchClampScale)
```

Calculation/helpers/constants проверяются на1.0/1.1; mode scales должны согласовываться. Nonzero desired duration требует nominal clip resource, пока unavailable. Не использовать rate==5/weapon-name gate/историю клавиш. Positive1.0 Ballista и Rocket:1.66667→5 при норме1.66667. Во время ordinary melee допустим verified meleeD5PrevWeapon, но всегда читается **current rate**; настоящий reset немедленно снимает BATS. Новый live punch .8 не подтверждён, fixtures есть. На1.1 подтверждён normal-state/code audit; positive BATS — отдельная acceptance.

Fast DBlade активируется на **prepared Arbalest hit**, до выбора Destroyer Blade, остаётся при другом оружии до расхода actual one-shot override. Ordinary mastered hit тоже временно даёт chargeGameTimeOnce999960, поэтому одного >0 недостаточно. Нужен decoded modChangeAnimPlaying +hit provenance; completed ordinary mod-switch — negative control. State привязан к map/world/player/weapon/token/secondary-shot timestamp; consumption/new provenance/identity change очищают. Пропущенный до observation prepared hit не выдумывать. weaponModMastered не надёжен как ownership/mastery. Actual controlled traces:14433samples replay,0mismatches. Источник `docs/validation/speedrun-techs-1.0.md`.

Sync sources: syncMaster, syncMelee.currentSyncEntity, playerMechanicSync.syncAnimEntities; на1.1 ещё **lastUsedInteractable→interaction.activeSyncEntity** с owner/activator/current player/alive/working checks. Historical pointer не активность. Независимые syncs дают отдельные rows, shared refs deduplicate; melee=sync_animation, interaction=interaction, other=sync_other_N. **Не копировать GK timer в interaction только по interactAnimIsPlaying.** Mantle отдельно по ledge-grab start/end.

Current MD6 leaf/framecount−1/fps/start/rates/scales дают **current-clip** remaining/max, не всю будущую scripted sequence. Loops/local clocks/overrides/ambiguous trees unavailable. Recheck tokens/lists/owner/clip/tree/rates после чтения. .9 actual1.1 idle probe подтверждает source/ownership, optional watch видел idle — не positive pickup/button proof. См. `docs/validation/sync-interactions-1.1.md`.

## 8. Praetor 0.8.10 - player control lock

The fix is implemented and locally verified. The user repeated a Praetor Suit Point pickup on game 1.1 and held forward until a free step. The old current-clip timer used the continuing NPC/master animation (200 frames, 30 FPS, 6.633333 seconds). Player control returns earlier.

The authoritative source is the player model timeline: `idSyncEntity.animatorSyncEntity.modelAnimStates`, paired with `idDeclAnimWeb.models[].model`, the model's `idDeclMD6.animEvents`, and its active clamped MD6 leaf. The player model declaration contains the authored `ae_releasePlayerFromSync` event at frame 94. The registry number is discovered by name through the guarded engine event lookup; neither event number 691, frame 94, model index 1 nor 3.133 seconds is a production duration constant.

Recorded inputs: leaf start 22000876 ticks, FPS 30, event frame 94, rate/scales 1, ticks-per-second 999960. Calculated release deadline 25134084; actual dispatch was sampled at 25136123; the next observed restored abilities/binding state was 25148326 (about 14 ms after the authored deadline). `savedSyncEntity` clears and abilities restore to 4095 while the NPC sync keeps working. The countdown is suppressed after release.

Do not confuse the NPC's event 198 (`ae_disableClip`) or 223 (`ae_hideMesh`) at the same frame with player release. Event 691 was the actual registered release number in the recorded process. These numerical IDs are evidence, not constants to reuse without the registry check.

Scope: control-lock mode is enabled only for the demonstrated game 1.1 Praetor asset under `/mechanics/interact/preator_suit_token/`. Other 1.1 interactions and older supported profiles preserve the explicitly named current-clip display. The release-event lookup getter itself is audited on 1.0/1.1/5.1, but that does not extend the validated control-lock feature to those other cases.

The helper rejects unknown time domains (only 999960), fractional starts, local animation clocks, loops/time overrides, ambiguous release schedules, disabled/flagged events, changed leaves/resources/lists and stale player bindings. One shared bounded work counter includes model nodes, animation-info entries and event entries. No observed stopwatch or fixed correction is added. Maximum duration is derived from the same authored release timeline.

`sync_animation_timers.h` owns `syncReleaseEventNumber`, `syncReleaseDeadline`, `captureSyncPlayerControlLock` and `captureSyncAnimationTimers`. The production caller passes module base and profile from `game_timers.h`. `SyncInteractionTests` replays the recorded start/pre-release/released clocks through the production reader, with the NPC still working; it also checks registry identity/opcode guards, wrong IDs, foreign TPS, total work limits and preservation of ordinary interactions.

Evidence: `docs/validation/player-control-lock-1.1.md` and `docs/validation/praetor-lock-1.1.md`. Detailed local recordings are ignored under `build/praetor-lock-1.1-research`; a fresh checkout may not contain them. The read-only pickup trace is live evidence; the captured-data replay is a regression fixture, not a second live integration demonstration. All finite samplers have ended.

Current local verification: 21 native suites, 24 GPU cases, launcher/settings/protocol checks, 294 HUD fixture checks and the managed UI build passed. Runtime packaging and remote publication have separate verification records; see the publication status below.

## 9. Команды и compatibility

LAB без аргумента query/help; targets current/landing/vertical.

| Команда | Аргументы |
| --- | --- |
| labplus_speedometer_show | `0\|1 [target]` |
| labplus_speedometer_showlanding | `0\|1` |
| labplus_speedometer_showvertical | `0\|1` |
| labplus_speedometer_verticalsign | `0\|1` |
| labplus_speedometer_x_pos / labplus_speedometer_y_pos | `<pixels> [target]` |
| labplus_speedometer_size | `<0.1..10> [target]` |
| labplus_speedometer_color | `<R> <G> <B> [target]`,0..255 |
| labplus_lowhptintopacity | `<0..100>` |
| labplus_debughud_scale | `<0.5..3>`, noarg query |

Research short actions0/1: `labplus_viewer`, `labplus_triggers`, `labplus_collision`, `labplus_walls`, `labplus_inspector`, `labplus_disabled`, `labplus_history`, `labplus_encounters`, `labplus_path`, `labplus_player`, `labplus_surface`, `labplus_attempts`, `labplus_fill`. `labplus_distance 20..200`. Invisible switches independent/default off: `labplus_enemies 0|1`, `labplus_enemy_hp_bars 0|1`, `labplus_timers 0|1`, `labplus_techs 0|1`.

Actions: `labplus_commands_list`/`labplus_help`, `labplus_encounters_list`, `labplus_recording_start`/`labplus_recording_stop`/`labplus_recording_clear`, `labplus_markers_add <text>`, `labplus_trigger_set <class> 0|1`. Aliases: `labplus_record 1|0`, `labplus_clear`, `labplus_mark`, `labplus_trigger`. `labplus_encounters` noarg=list. Timeline recording не Movement Lab attempt recording.

Persistent cvars: `labplus_viewer_show`, `labplus_triggers_show`, `labplus_collision_show`, `labplus_walls_show`, `labplus_inspector_show`, `labplus_disabled_volumes_show`, `labplus_volume_history_show`, `labplus_path_show`, `labplus_player_show`, `labplus_surface_show`, `labplus_attempts_show`, `labplus_encounters_show`, `labplus_viewer_distance`, `labplus_viewer_fill`. Construction/register/set audited **только6.66Rev3**. Это не запрещает работающий short-action bridge/UI на других версиях. Не создавать неподдержанные variables произвольным set.

| Подсистема | Граница |
| --- | --- |
| Collision | 22audited profiles1.0..6.66Rev3.2; не22проверенные gameplay timers |
| LAB adapters | Независимые validated layouts; HUD tests использовали4real EXE |
| Equipment/animation clocks,GK | Audited1.0/1.1/5.1; code mismatch/другие clocks unavailable |
| BATS | Audited normal formula1.0/1.1; actual positive Ballista/Rocket1.0 |
| Fast DBlade | Actual1.0 positive/negative controls;1.1 getter/layout/fixture/normal-state proof |
| Native cvar creation | 6.66Rev3 +compatible meathook |
| Live checkpoint reload | 6.66Rev3/TAG1/slot3, same-account, Cloud off, runtime/file checks |

Полный список22 collision profiles — в `native_profile.h`; поддержка их геометрии не расширяет автоматически gameplay timer audit.

## 10. Проверка/сборка/публикация

`build-local.ps1` собирает native+lighting, выполняет native/managed/UI checks, publish и package. `-SkipTests` — только после подходящей проверки той же версии. .NET10 SDK +MSVC Build Tools либо LLVM-MinGW; Vulkan-Headers bundled, WebView2 pinned в csproj.

```powershell
./build-local.ps1
# Или -NativeCompiler C:\tools\llvm-mingw\bin\clang++.exe
cmd /c Native\CollisionViewer\build-viewer.cmd
Get-ChildItem Native\CollisionViewer\bin -Filter '*Tests.exe' | ForEach-Object {
    & $_.FullName
    if ($LASTEXITCODE -ne 0) { throw "Native suite failed: $($_.Name)" }
}
dotnet run --project Tests/Launcher/LauncherTests.csproj -c Release --no-restore
dotnet run --project Tests/Hud/Hud.Tests.csproj -c Release --no-restore
dotnet run --project Tests/InspectorPreview/InspectorPreview.csproj -c Release -- build/screenshots/application.png
dotnet run --project Tests/VisualPreview/VisualPreview.csproj -c Release -- build/visual-qa
./Native/CollisionViewer/tests/run-gpu-tests.ps1 -RuntimeDirectory C:\workspace\LAB-Plus\build\gpu-final
```

No-restore требует restored dependencies. `LAB_LAYOUT_TEST_PATHS` — реальные absolute EXE paths через `|` (использовались1.0/1.1/5.1/6.66). Game EXE в git не включать. `LABPlus.exe --preview` — isolated UI; VisualPreview имеет `--main-only`.

Последняя .9:21native suites/24GPUcases; точный suite manifest — native build script. Ключевые: EnemyTelemetry/GameTimer/SpeedrunTech/SyncInteraction/HealthBarRender/DebugHudRender/Performance. GPU:font/HP/HUD,visibility/console,depth/split queues. После Praetor число может измениться; старое19 не текущий результат.

Fixture/GPU/browser/read-only snapshot/human demo — разные уровни proof. .9 actual1.1 capture: metadata resolved,3enemies,9timers, BATS inactive, parser accepted; positive cooldown/powerup/button/BATS1.1 не подтверждены. UI build не live visual acceptance.

Read-only probes используют VM_READ|QUERY_LIMITED_INFORMATION и свежие PID/base. Broad Process.Handle/GUI job могут быть ограничены runner; не путать с product bug и не обходить tool rejection.

Паковать в **fresh raw directory**, особенно после смены casing AssemblyName. После проверенной сборки:
```powershell
dotnet publish labplus.csproj -c Release --no-restore --self-contained false -o build/raw-publish-0.8.10
./package-local.ps1 -Source build/raw-publish-0.8.10
```
Номер здесь текущая цель, не доказательство готовности. ZIP должен содержать LABPlus EXE/DLL/deps/runtimeconfig, WebView2 DLLs/x64loader, collision/lighting DLL, Assets/fonts/UI и licenses. Не один EXE. Package script проверяет manifest/не портит logs. Не hardcode старые29files после font/UI additions.

Перед commit: review diff без reset накопленных изменений; diff --check (Windows cr-at-eol), согласованные version/notes/help, EXE casing, native hashes raw/final/ZIP и licenses. Build/bin/obj/publish/artifacts/recordings игнорируются; generated font source, tests, validation docs — tracked. Secrets, личные данные, game binaries/dumps/settings/logs/probes не публиковать.

После Praetor root выполняет запрошенные code push и Release ZIP upload, проверяет asset и записывает реальные tag/commit/URL/hash. Notes отражают конечное поведение и реальные проверки. Сохранить GPL/history и attribution DESRU-Enhanced/Vulkan/lighting/WebView2/fonts.

Implementation and combined local validation are complete. Durable technical records: `docs/validation`; user documentation: `docs/wiki`. Publication results are recorded below.


## Publication workflow

`.github/release-request.json` requests version 0.8.10. The release workflow checks out the exact triggering main commit, installs a stable .NET 10 SDK, builds native DLLs, runs native/launcher/HUD fixtures, validates the x64 EXE and 29-file runtime package, then creates a new tag and a draft release with ZIP/checksum assets. It publishes only after verifying that draft. Existing release/tag values are not silently overwritten. Local GPU/live acceptance remains separate from CI.

The workspace's local Git metadata can remain on the original snapshot while source changes are uploaded through the connected GitHub API. Do not mistake a dirty local status for missing remote publication; check the remote main/tag and the result recorded below. Never reset the user's worktree to hide those changes.

### Publication status

**Not published remotely.** On 2026-11-01, the connected GitHub API rejected the first blob write with HTTP 403, `Resource not accessible by integration`. Its accessible-installations list was empty. Direct Git HTTPS from the execution environment also failed to connect. Main was verified still at `33df02e01a0f6d3ab6321750cab0d15d3ee66f98`; no source branch, tag, release asset or remote Wiki page was changed by this publication attempt. This is a connection capability limitation, not a request for the user to approve the same work again.

Local ZIP: `labplus-0.8.10-win-x64.zip`, SHA-256 `CC3997C1C5A5CBADBD154B840F42E2F86BFA35FE9910DDF86AA0A47B81C6EA61`. Native layer SHA-256: `92782B91FF16298A8783CA88833F78F84767B1B22BCDF0D4A2513C22E7B645A1`. Runtime package has 29 verified files; `LABPlus.exe` FileVersion is `0.8.10.0`. Full local verification is recorded in `build/qa-0.8.10/package-verification.json` (not a tracked runtime file).

The Wiki has been refreshed locally for 0.8.10: 19 pages, including LAB and Invisible numbers. Internal links and documented command names were checked. The legacy developer page is now a short source/release pointer and is absent from the Sidebar. The user's final Wiki direction is to keep only practical user information: installation, features, commands, meaningful compatibility limits and troubleshooting. Developer details, fixture counts and implementation notes belong in this continuation context and `docs/validation`, outside the Wiki.

Resume after a GitHub connection with repository contents/workflow write access becomes available: inspect the current remote main, publish the prepared source as a normal fast-forward, monitor the release workflow and Wiki sync, verify the tag/ZIP digest and actual Wiki HEAD, then update this status with observed URLs and SHAs. The built-in Actions token's ability to push the Wiki remains unverified; a denied Wiki job must not be reported as success. Do not overwrite an existing release or force-push a changed remote branch.
