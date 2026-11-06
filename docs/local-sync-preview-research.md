# Предварительный выбор Glory Kill / Chainsaw: исследование 1.1

Статус на 6 ноября 2026: локальная экспериментальная реализация
и исследование локального EXE с демонстрациями пользователя. Работа по
другим патчам отложена. Приведённые ниже адреса — RVA именно retail 1.1
(PE timestamp `5ec7deca`, image size `1d551000`).

## Текущая сборка: Zombie tier 1, руки солдата и нижние квадранты (v19)

Локальный путь: `build/playtest-sync-zombie-soldier-v19/LABPlus.exe`.
Нужен полный ручной перезапуск игры через эту сборку. Только retail 1.1.

Разрешённая запись параметров длилась ровно 60 секунд: 4919 samples,
0 ошибок чтения. Восемь подтверждённых player-owned исполнений Soldier
Blaster: три `kill_front_lower`, четыре `kill_front_leftarm`, одно
`kill_front_rightarm`. Прицеливание в нижнюю часть действительно связано
с нижним роликом; отсутствие обязательной named leg group этого не отменяет.
Новая запись после этих 60 секунд не запускалась. Отдельный разрешённый
срез Zombie tier 1 Hell подтвердил собственные назначения и шаблоны.

Исправлены три независимые причины:

* Zombie tier 1 назначены T1 и T3 declarations для одного `kill_front_rightarm`.
  Aim одинаковый, но yaw и priority различаются. Правила больше не теряются
  при дедупликации по имени: каждый spatial branch сохраняется, его конкуренты
  вычитаются отдельно, результаты объединяются без двойной заливки.
  Несовместимые aim rules по-прежнему не считаются поддержанными.
* У T1 Chainsaw `cut_front_01` и `cut_front_02` принадлежат одному declaration
  (`chosenBy=LEASTUSED`), а текущий interaction содержит только одну альтернативу.
  Отсутствующая альтернатива не скрывает back/активный front. Её собственная
  зона не выдумывается из позы соседнего ролика; независимый неизвестный
  конкурент по-прежнему блокирует утверждение об уникальной зоне.
* Standing reference для руки учитывает текущий допустимый экранный квадрант.
  У правой руки солдата нижний прицел исключает upper-only конкурента;
  верхняя общая область остаётся вырезанной. Plane geometry привязана к врагу.
  Для левой руки учтён фактически наблюдаемый AI priority bonus +0.25:
  `bbf4a8` проверяет AI requirement list, `bbf652..bbf663` добавляет бонус.
  Native early-success ветки могут обойти бонус, поэтому один лишь authored
  nonempty list не даёт права обещать этот приоритет. Без eligible runtime
  подтверждения он остаётся неизвестным и обрабатывается консервативно.

Нижние screen tiles теперь клипуются к near plane и viewport, как и маски
частей тела: крупный хитбокс за краем камеры больше не отменяет видимую
нижнюю половину. Named leg/foot groups имеют отдельные подписи; там, где
native требует только квадрант, не вводится фиктивное условие конкретной ноги.

Контрольные данные: `tests/fixtures/sync-zombie-soldier-authoring-11.tsv`
с actual declarations и frame-zero matrices четырёх T1 шаблонов и солдата.
Проверяются обе native ветки одного ролика, их union, неизвестные конкуренты,
неактивная CS альтернатива, нижний/верхний прицел правой руки, приоритет левой.
Дополнительно 640 views actual lower-quadrant rules (8 сторон × 2 дистанции)
имеют непустую подсветку во всех 640 видимых случаях; у Blaster и Shield
Soldier по четыре lower states. Предыдущие 672 anatomical cases сохранены.
Это offline проверки actual frozen poses; новая DLL ещё не получила проверки
игрового рендера и всех запусков анимаций после ручного перезапуска.

Проверки сборки: шесть native regression suites, финальный focused suite и
DLL, Launcher tests, browser events при 125%/150%, managed build без ошибок
и предупреждений, Help JavaScript syntax и Git whitespace прошли. Последний
одноразовый production-reader срез принял layout 1.1, но текущей цели под
прицелом уже не было: это не считается визуальной проверкой исправления.
Контрольные схемы нижних квадрантов сохранены локально в
`build/qa-sync-legs-v19/`; оба масштаба проверены по actual native vertices.

Код предназначен для ветки `gk-visualizer`; релиз и теги не создаются.

## Предыдущая сборка: части тела со всех сторон, фактические модели (v18)

Готова `build/playtest-sync-limbs-v18/LABPlus.exe`; v17 сохранена.
Требуется ручной полный запуск игры через v18. Только retail 1.1.

Пользователь заспавнил всех доступных врагов. Два bounded one-shot read-only
среза (второй дополнил native physics aim bounds) получили 29 живых actors,
22 семейства назначенных sync rules, 28 поддержанных shot-sphere моделей.
Все восемь семейств с body-dependent GK представлены настоящими моделями:
12 вариантов, включая hell/scientist zombie, shield soldier и Cyber Mancubus.
Захват: `build/sync-limbs-v18-all-aim-bounds-once.tsv/.log`; неизменный fixture
`Native/CollisionViewer/tests/fixtures/sync-limbs-all-models-11.tsv` содержит
реальные poses, shot spheres, native MD6 type-0 groups, physics aim bounds
и правила, назначенные каждому врагу. Identity и world descriptor сверены.
Никакого управления, engine calls, записи видео или observation loop.

Исправления:
* coherent lookup без группы (`nullptr` из 5d7200) — known no-group blocker,
  а не причина отменить все анатомические маски. Negative/invalid joint ID
  сопоставляется как native 16-bit ID; неизвестная/меняющаяся MD6 collection
  по-прежнему скрывает guidance. Helper без группы не превращает broad
  all-mapped-groups правила в фиктивный выбор отдельной конечности.
* projected AABB клипуется по рёбрам к near depth .051 (native overlay shader
  использует .05). Часть box за камерой больше не отменяет видимую руку/голову.
  Полностью behind-eye сферы не блокируют forward rays; near-gap sphere
  сохраняется как conservative blocker. Camera-facing marker остаётся
  проекционной маской на wire-box, отдельные hurt spheres в игре не рисуются.
* сетка концентрируется на projected bounds требуемых групп, 8..36 на 8..48,
  вместо растра всего крупного демона. При пустом результате исходная сетка
  проверяется один раз для тонких открытых краёв. Оба пути проходят один
  и тот же convex coverage / occlusion proof; outline без внутренней сетки.
* при проверке мелких ячеек обнаружена численная ошибка far-cap normal,
  вычисляемого из трёх почти совпадающих точек. Теперь используется известная
  camera-depth normal, а side-plane cross products считаются в double.
  Это исправлено до упаковки; независимые ray checks больше не находят
  ошибочно окрашенную соседнюю группу.

`sync_limb_multiview_checks.h` проверяет все реальные body-specific назначения
из fixture в 8 направлениях вокруг native facing axis и на двух дистанциях.
672 cases, 672 known masks; все 349 явно видимых core samples имеют непустую
маску. Каждый interior ray четырёх точек каждой нарисованной треугольной
ячейки независимо проверяется по первому пересечению с настоящими spheres
и native physics-centre quadrant; всего 386352 проверок лучей.
Это offline viewpoints над настоящим frozen pose, не управление камерой
игры и не проигрывание всех GK клипов. No-part/all-group назначения всех
captured actors отдельно проверены на отсутствие выдуманной анатомии.

Сравнение ТОГО ЖЕ fixture со скопированным v17 renderer:
`build/sync-limbs-v18-before-after.tsv/.log`: 120 прежних unknown проекций,
89 восстановленных пустых масок; ни один ранее непустой случай не стал
пустым. 113 оставшихся пустых случаев относятся к occlusion, viewport
или native aim restrictions, а не пропавшим known maps. Сравнение использует
те же источники/позы/virtual cameras, а не разные игровые моменты.

Контрольные схемы actual fill vertices просмотрены:
`build/qa-sync-limbs-v18/limbs-all-sides.png` и
`build/qa-sync-limbs-v18/limbs-all-sides-close.png`. Цвета схем объединяют
разные выбранные GK для сравнения; в продукте выбранная часть остаётся
жёлтой. Это не GPU screenshot игры и не рендер textured meshes.

Предел проверки: Buff Pod, Tentacle, Gladiator отсутствуют среди spawned
actors; их загруженные authored rules из каталога не требуют body groups,
и общий тест всех 25 семейств/224 entries сохраняется. Для Doom Hunter
native sphere reader в этом срезе вернул unsupported shape; его шесть
назначенных GK не требуют конкретной части тела. Это не подтверждение
общей поддержки его hitbox/standing geometry. Не выдавать отсутствие
анатомического ограничения у остальных врагов за ошибку подсветки.
В игре новая DLL ещё не загружалась; game PID8128/base7ff621b70000 и LAB19324
v17 — лишь наблюдавшиеся при срезе значения, позже перепроверять.

Проверки v18: все шесть native suite PASS, включая actual multi-view
и прежние automatic/retention cases; DLL собрана после последних product
изменений. Launcher PASS; managed 0 warnings / 0 errors; изолированное окно
125%/150% PASS, снимок 150% просмотрен. Help JS syntax, git diff whitespace
и новые native fixture/header whitespace PASS. Исходные/упакованные DLL
сверены; native SHA256 `7306A499262E6AEA2AC3148C769BCE13BFC7376BA4153B9C58FA314F94603D37`.
Manifest: `build/sync-limbs-v18-build-manifest.json`.

## Предыдущая сборка: автоматический вход в зоны и удержание цели (v17)

`build/playtest-sync-auto-retain-v17/LABPlus.exe`. Сборка v16 сохранена.
Для проверки требуется ручной полный запуск игры через v17. Только retail 1.1.

Automatic сначала проверяет зоны, содержащие позицию игрока, включая
янтарную полосу между origin-exclusive и collider-clearance областями.
Прежняя зона больше не удерживается только потому, что её всё ещё можно
нарисовать. В пересечениях предпочтение отдаётся текущему наведению,
затем полному clearance, затем прежнему выбору при равных условиях.
Вне всех зон сохраняется доступный прежний ориентир или выбирается
ближайший доступный. Ручной выбор анимации не меняется.

Вошедший в позиционную зону конечности или квадранта игрок может увидеть
её до правильного наведения: подсветка объясняет требуемую область.
Это conditional reference; native aim/focus всё ещё обязательны для
подтверждения. Общие позиции разных групп тела не вырезаются. Для
квадрантного правила без обязательной группы известный текущий native
focus продолжает отсекать неподходящих анатомических конкурентов.

После отвода камеры сохраняется последняя цель, полученная настоящим
пересечением прицела с её shot spheres, пока игрок находится в радиусе
выбранного действия. Проверяется 3D расстояние от native viewPosition
до native view-target point, обновлённого в текущей системе координат
врага. Применяются текущие GK/Chainsaw caps и предел дальности отображения.
Заново читаются player origin, pose, shot bounds, состояние жизни,
hostile targeting bits и назначенные правила. Новая цель под прицелом
имеет приоритет. Выход за радиус, смерть, friendly state, переиспользование
entity generation, смена карты или отключение/скрытие gameplay очищают
удержание. Простое возвращение в радиус после очистки не захватывает
врага автоматически: сначала нужно снова навести прицел.

Удержанная область использует последние осмысленные требования наведения
как reference и текущую систему координат врага. При утрате временного
CS selector pool последний проверенный рисунок его стартовых поз может
сохраняться в enemy-local frame, с явной надписью last aimed poses.
Эти данные подставляются ТОЛЬКО в reference diagram: они не заполняют
nativeChoices и не подтверждают текущий native selection. При отведённой
камере подтверждение всегда янтарное, никогда не зелёное. После возврата
прицела требуется свежая native проверка. World-space отображение естественно
обрезается камерой, если сам враг/объём уже вне её поля зрения.

Новые проверки: переход front→back→front при остающейся drawable старой
зоне; вход в янтарную полосу; обе CS distance areas и пустая random band;
перекрывающиеся конечности/квадранты и смена focus; сохранение frame mesh
при отведённой камере; native reader lifecycle с настоящей цепочкой чтения
ranges/pose в синтетической памяти, включая новую цель, выход/повторный вход,
смерть, hostile/friendly, generation, map и clear. Сохранённый Zombie fixture
вне всех exclusive областей продолжает выбирать полезный ground fallback.
Это проверки кода и геометрии, не запись/исполнение в реальной игре.

Проверки v17: шесть native suite PASS; Launcher PASS; managed build
0 warnings / 0 errors; изолированное окно 125%/150% PASS и снимок 150%
просмотрен; Help JS syntax и whitespace PASS. Исходные и упакованные DLL
совпадают. Manifest: `build/sync-auto-retain-v17-build-manifest.json`.
Native SHA256: `ED78B323FA28DDEF35B2B96F0AE6AEEB40A545102D28CF79EC601CAEE0F3F6F0`.
Новая DLL ещё не проверена в реальном игровом запуске.

## Предыдущая сборка: конечности, конкуренты и список до GK state (v16)

Готова `build/playtest-sync-limb-v16/LABPlus.exe`; состояние сборки и
проверок зафиксировано в `build/sync-limb-v16-build-manifest.json`. Сборка v15 сохранена. Для проверки нужен ручной полный запуск
игры через v16. Только retail 1.1.

При ручном выборе анимации с обязательным наведением на часть тела теперь
одновременно показываются жёлтая область наведения и позиционный объём.
Последний предполагает, что игрок наведётся на требуемую часть, поэтому
не исчезает при текущем наведении на живот или голову. Текущий native focus
по-прежнему проверяется отдельно; неверный focus не даёт зелёного подтверждения.
Automatic выбирает только варианты, подходящие текущему наведению.

Конкурент вырезается с полным собственным стартовым смещением, дальностью,
высотой и ориентацией. Поэтому близкий вариант оставляет отверстие в зоне
дальнего. Непересекающиеся требования к частям тела или квадрантам позволяют
сохранить общую позицию. Проверенные более низкие обычные приоритеты не
вытесняют более высокий вариант; неизвестные остаются конкурентами.
Для обычного Generic подтверждены копирование default priority, пустой
requiredEquipt через native sentinel, отсутствие Berserk/Crucible/Momentum
и specific validator. Default не подменяет runtime выбор распила.

Живой Zombie tier 3: read-only однократный снимок и внешний production reader
дали 13 GK-правил, текущий focus `gut` и непустой conditional standing volume
для `kill_front_rightarm`. Native default/runtime приоритет руки равен 2,
общего chest — 0. Данные и замороженная фактическая поза сохранены в
`build/sync-limb-v16-zombie-*` и тестовом `sync-zombie-rightarm-11.tsv`.
Это проверка считывания/геометрии; новый DLL в игре ещё не проверен.

Найдена причина `Unavailable on target` до GK state: список зависел от
временных `player.syncMelee.possibleSyncInteractions`. У живого Tyrant до
оглушения этот список был пуст, но собственный `aiConstants.syncMelee`
уже содержал назначенные `syncInteractionData` и `syncMeleeEntityDefs`.
В сохранённом снимке именно этого Tyrant назначены три обычных GK:
front_upper, above_front, back_upper; остальные четыре записи — специальные.
Это не все шесть записей семейства из общего отчёта: каталог не является
списком вариантов, назначенных каждому конкретному экземпляру.

Новый общий read-only путь читает именно собственные назначения цели и
строго сверяет полные источники/состояния/наведение с проверенным каталогом.
Позы ищутся среди idSyncEntity текущего мира только по native entityDef,
назначенным этому врагу. Узел определяется по subweb+state; используются
его frame-0 attach-матрицы, не направления из имён front/back. Если несколько
подходящих шаблонов дают разные области, геометрия остаётся неизвестной.
Одинаковая подтверждённая геометрия допустима. Изменение назначенного списка
не восполняется старыми данными target cache. Исполнение, топливо и GK state
по-прежнему определяются текущими native кандидатами.

Проверка до GK state охватывает все 25 семейств/224 записи из отчёта на
синтетических кэшированных матрицах и сохранённой реальной структуре полей.
Отдельно проверяется полный capture с пустым native GK-пулом у Tyrant,
чужой entityDef, разные first/third person старты, невалидный кэш позы и
отсутствие назначения правила конкретному врагу. Это не игровые исполнения
всех клипов. Сохранённый world snapshot подтверждает наличие loaded шаблонов
Archvile и Tyrant, но полные матрицы Archvile в новом пути не были захвачены
до закрытия игры. Финальное отображение этих врагов требует проверки v16
в следующем пользовательском запуске.

Проверки v16: шесть native suite PASS; финальные focused tests и DLL build
PASS после последних изменений; Launcher PASS; managed build 0 warnings /
0 errors; изолированное окно 125%/150% PASS, снимок 150% просмотрен.
Help JS syntax и whitespace PASS. Native SHA256: `21F3102C285B92DD01859F603CA183247924C6B962C5153F30D220C512A32DCA`.
Lighting без изменений, исходные и упакованные DLL сверены.

## Предыдущая сборка: отображение по каждому набору правил (v15)

Готова `build/playtest-sync-display-v15/LABPlus.exe`. Полный restart через
новую сборку обязателен: native/managed протокол дополнен H-строками с
названием набора правил и подсказкой наведения выбранной анимации.

Встроен проверяемый каталог 224 пар «исходное описание / состояние» из
отчёта (25 групп; 188 GK-записей и 36 CS-записей). `Tools/audit-sync-rules.py
--header ...` генерирует `sync_enemy_catalog.h` из того же сохранённого
read-only источника, с SHA256. Runtime сверяет полный resource name, state,
action, квадранты и группы с native данными. Изменённые/неизвестные правила
не заимствуют подсветку у соседнего врага. Алиасы моделей автоматически
не приравниваются: используется привязка действительного interaction к
текущей цели и реально прочитанному исходному описанию.

Aim metadata отделены от пространственных transforms: вариант остаётся
в списке и получает свою подсветку/подсказку, даже когда standing diagram
для него недоступна. Требования сохранены в отдельном target/blueprint
cache с identity/generation/level guards. Имена состояний не уникальны:
найдены разные исходники для Doom Hunter kill_front_upper и Pain Elemental
kill_back_lower. Несовместимые требования одного state дают unknown;
разные дистанции/позы не схлопываются в одну ложную пространственную область.

Каждый вариант в настройках получает собственную подсказку: голова,
рука, корпус/смешанная область, настоящие квадранты, отсутствие специальной
части или unavailable. Подсказка меняется сразу при выборе и очищается при
смене цели/действия. Mesh cache учитывает target generation, action/state,
камеру, bounds, pose и текущие joint-group mappings. Нельзя перенести head
mask от Imp на одноимённый front_head Hell Knight. Общий допуск всех групп
остаётся без anatomical patch; отсутствующая нужная часть явно отмечается.

### Согласованная запись Hell Knight и исправление полосы clearance

Пользователь явно разрешил 60 секунд read-only параметров. Запись ровно
60,0 с, 5107 samples, 1973 samples с interaction pool, один отвергнутый
переходный read (`bounded object`), ~22 MB. Видео/ввод/вызовы функций игры
не применялись; recording process завершён, handle закрыт. После фильтра
сброшенных victim refs подтверждены 8 исполнений: 6 front, 2 back. Во всех
восьми фактический player-owned clip входит в рассчитанный набор выбора
последнего согласованного pre-action sample за 15–78 мс до начала.

У трёх front-исполнений два варианта оставались в random band. У других
трёх front уже был единственным по native distance band: разницы дистанций
.548703/.474309/1.047375 при tolerance .381. Но дополнительный preview
буфер .381+2*horizontalFootprintRadius исключал эти позиции из заливки.
То есть часть наблюдаемого «вне зоны» действительно вызвана чрезмерно
узким отображением относительно native origin-based правила, не только RNG.
В UI во время записи был Automatic (SyncChainsawAnimation пустой), а не
фиксированный front. Старый resolved auto name этим логгером не записывался;
точную видимую GPU-картинку каждого нажатия лог не восстанавливает.

Теперь отдельно строятся origin-exclusive и collider-cleared reference
areas. Их разность окрашивается янтарным: выбранная origin-позиция входит
в уникальную область, но полный player-hitbox запас не помещается. Random
overlap исключён уже из origin area и никогда не закрашивается полосой.
Caps цветной полосы плюс caps core и внешние стены origin mesh дают одну
залитую фигуру. Рисуется только внешний origin contour с прежними двумя
uprights; внутренних стен/веера/дублированных маркеров clearance не добавлено.
GK использует тот же механизм; native actual aim/body-facing/candidates
по-прежнему проверяются отдельно от enemy-anchored reference.

CS distance-gap область является выпуклым пересечением balls/halfspaces/
SOC. При отсутствии holes и wide facing конструкционный partition заменён
convex hull только его доказанно допустимых вершин. Это сохраняет native
predicate и убирает дорогую разность сотен треугольников; hull запрещён
для областей с competitors/holes/nonconvex facing. Clearance band последней
offline проверки ~.317 ms, guide+sector ~.832 ms (не измерения game FPS).

Проверки: rendering path всех 224 записей/25 групп на СИНТЕТИЧЕСКИХ позах;
реальная frozen pose текущего ai/heavy/hellknight (35 shot spheres), отдельно
front_head/front_lower и оба CS; три записанных sole-front позиции теперь
в янтарной полосе, три ambiguous позиции остаются пустыми. Проверены
внутренние точки новых caps, отсутствие random leakage/дублированного
контура, source drift, одинаковый state у разных врагов, смена action/
generation/camera/pose, unknown и incomplete pools. Полный 6-suite regression
PASS; после финального convex/caps изменения focused + DLL PASS, старые
hole/anchor/clearance predicates сохранены. Managed build 0 warnings/errors,
Launcher protocol old P3/P4 и новые H/subset guards PASS; отдельный браузер
125/150 PASS, финальный screenshot150 с Hell Knight hint просмотрен.
Native geometry QA из текущих вершин просмотрена, не игровой screenshot.

Live read-only reader на текущей игре PID17536 / LAB4448v14 принял новый
layout и все 9 Hell Knight source bindings. В одном снимке current choice
set — только cut_front_01, оба body rules — AllCurrentGroups. Новая DLL v15
в игре ещё не загружалась; все клипы остальных 24 групп не проигрывались.
Полная гарантированная activation map с late checks/fallbacks не заявляется.

## Предыдущая сборка: классификация правил и текущий набор выбора (v14)

Готова `build/playtest-sync-enemy-audit-v14/LABPlus.exe`. Отчёт по каждому
из 25 наборов описаний — `docs/sync-enemy-audit-11.md`, исходные условия —
`docs/sync-enemy-audit-11.json`. Репродуцируемый offline export из сохранённого
read-only аудита — `Tools/audit-sync-rules.py`; 188 kill-состояний (включая
боссовые), 36 cut-состояний. Наследование моделей и отсутствующие алиасы
не предполагаются. Это не 224 проверенных исполнения в игре.

Подтверждён важный дефект трактовки v13: непустой damageGroupNames не
обязательно означает выбор конкретной конечности. У обоих CS Hell Knight
11 одинаковых групп. Новый `syncBodyAimKind` сверяет их с ВСЕМИ текущими
согласованно сопоставленными shot-группами экземпляра. Если допускаются
все, анатомическая маска не рисуется, ограничение квадранта сохраняется,
native focus по-прежнему проверяется отдельно. Неизвестное соответствие
не объявляется общим разрешением. У обычных GK Hell Knight нет требований
групп: front_head требует top quadrants, а не head joint group.

Read-only snapshot текущего `ai/heavy/hellknight` PID24500 показал оба CS
valid1/pending0, priority1.25. Player origin(-3.72072434,-85.1501389,20.6000042),
front adjusted position(-2.95662451,-82.4016724,20.6000023), back
(-4.99406385,-82.40168,20.6000042). Дистанции2.852703/3.029096, разница.176392.
Параметр dp_gloryKillMaxAllowableDistanceForRandom отдельно прочитан и
проверен по имени: .381000012. Это объясняет разрешение front вне уникальной
зоны, но не является записью исполнения из пользовательского сообщения.
Затем exact production reader новой версии принял layout, вернул оба CS
в native possible set, а обе aim-правила классифицировал AllCurrentGroups.

Новый `nativeChoices` фиксирует приоритет/adjusted position для готовых
ordinary GK и CS. `syncNativeSelection` работает только при согласованном
pool, известных проверенных selector-позах/transform branch, отсутствии
pending и полном покрытии eligible names. Сначала max priority, затем
native distance band; near-boundary membership консервативно расширено
на .0001. Недостаточные/особые данные дают unknown, не фиктивного победителя.
HUD Engine now отображает этот ТЕКУЩИЙ набор отдельно от области с запасом
под игрока. Это не предсказание будущих поз/поздних проверок.

Залитая область остаётся внутренней консервативной частью, исключающей
конкурентов и случайную полосу. Новое пояснение явно говорит: вне неё
анимация всё ещё может выпасть. Native origin-based activation не заменяется
контактом капсулы. HUD также отдельно выводит authored start XY max и
диапазон разницы foot height; это не дистанция до поверхности врага.

Проверки: 6 native suites PASS, затем final focused PASS с полной новой
metadata fixture 25/188/36 и eight focus-dependent families. Реальный
Hell Knight selector snapshot даёт оба CS, тест priority-before-distance,
missing competitor, pending/unknown, GK selection, full-body suppression.
Production one-shot и новый UI125/150 PASS, финальный screenshot150 просмотрен.
Старый UI-test текстовых пояснений обновлён под новое значение подсказки;
первый UI-прогон падал на старой ожидаемой строке, финальный проходит.
Managed build 0 warnings/errors, Launcher tests PASS, help JS syntax PASS.
Новая DLL внутри игры не загружалась: нужен пользовательский полный restart.

## Предыдущая сборка: области прицеливания на хитбоксе (v13)

Готова `build/playtest-sync-aim-parts-v13/LABPlus.exe`. Требуется полный
перезапуск игры через новую сборку. Квадратный каркас врага сохранён.
Когда выбранная анимация требует head/arm/leg/torso-группу, прямо внутри
его экранной проекции рисуются жёлтые полупрозрачные области прицеливания.
При наличии дополнительного требования квадранта показывается пересечение.
Общий квадрант под анатомической маской не закрашивается. Для вариантов без
требования части тела остаётся прежняя подсветка квадрантов.

Источник — `havokShapeData_t.jointIndex` (+3a, short; layout проверяется),
согласованная текущая геометрия shot-spheres и target.jointAnimator->decl->
jointGroupCollection/type0. Используется первая группа, содержащая joint,
как в native lookup 5d7200. Название material jointGroup не подменяет этот
поиск. Списки, владельцы, названия и joint IDs перепроверяются на согласованность.
Ни имена анимаций, ни условное деление тела на половины не определяют маску.

`sync_aim_parts.h` строит консервативную маску в пределах проекции shot-box:
четыре угловых луча каждой ячейки должны пересечь одну и ту же уменьшенную
сферу требуемой группы на общем параметре. Выпуклость гарантирует покрытие
всей ячейки. Сферы остальных групп должны отделяться плоскостью от всей
пирамиды между камерой и ячейкой до попадания. Поэтому закрытая корпусом
рука не подсвечивается насквозь. При неопределённости ячейка исключается.
Рисуется только заливка и внешний контур оставшихся ячеек, без сетки,
отдельных анатомических сфер и внутренних диагоналей. Бюджет ограничен
39 x 51 ячейкой, максимум 128 shot-spheres. На границе разрешённых и
запрещённых квадрантов оставлена численная защитная полоса.

Маска обновляется по текущей позе. Это визуальная подсказка, а не замена
native focus: cached contact/history/fallback и условия доступности
позиционной зоны работают отдельно по правилам v11/v12. Подсветка может
помочь перевести прицел в нужную часть, пока позиционная зона скрыта из-за
неверного focus. HUD сообщает, если нужная часть закрыта, слишком мала
для уверенной подсветки либо её данные недоступны. Алгоритм общий для GK
и Chainsaw, если декларация действительно содержит такое требование.

Одноразовая read-only проверка production reader в игре 1.1: PID25476,
base7ff79f3d0000, zombie id16796822, 35 текущих shot-spheres. Layout и native
маппинг приняты; получены непустые маски головы, chest/gut и обеих рук.
Данные сохранены в `tests/fixtures/sync-aim-parts-zombie-11.tsv`; это
реальная поза и native group mapping, а не синтетические матрицы v11.
Новая отрисовка внутри игры и фактическое исполнение каждого клипа не
проверялись; игра не перезапускалась и ввод не генерировался.

Проверки: шесть native regression suites PASS (22 профиля Viewer —
синтетические), финальные focused tests и DLL PASS после добавления
защитной полосы. Плотные независимые лучи по треугольникам реальной позы
проверяют первое попадание в разрешённую группу. Дополнительно проверены
голова/рука/нога/корпус, движение части, перекрытие корпусом, отсутствующая
группа, неизвестный model, перевод координат и сохранение старых квадрантов.
Managed build: 0 warnings/errors; Launcher tests PASS; отдельное браузерное
окно 125%/150% PASS, скриншот 150% просмотрен. Геометрическая QA-картинка
из вершин production renderer просмотрена; это не игровой скриншот.
JS syntax и whitespace checks PASS. Source/package native и lighting DLL
совпадают; хэши в `build/sync-aim-parts-v13-build-manifest.json`.

## Предыдущая сборка: автоматический выбор доступной зоны (v12)

Пользователь повторил просьбу сделать зоны остальных врагов и уточнил:
у zombie tier 3 GK-зоны не появляются. Проверена именно запущенная v11:
игра PID15092/base7ff79f3d0000, LAB23320 из playtest-sync-demons-v11.
Выбор GK в пользовательских settings.json был пустым (автоматический),
action0. Файлы настроек только прочитаны, не изменялись.

Production reader прочитал реального zombie tier 3 id16796871: 45 shot
spheres,13GKregions, state=ready, range6/10, footprintknown1,
nativefocusknown1/gut. Значит основной сбой этого случая — не отсутствие
правил для демона. Старый default выбирал regions.front по алфавиту:
kill_above_back. Его minZ .031 при footZ==targetZ20.6000004 даёт пустое
сечение. Другие варианты имели реальные области: back_head1piece,
front_chest2,left_upper14,right_upper7. Доказательство в
build/sync-remaining-v12-initial.tsv и sync-remaining-v12-choices.tsv.
У Caco обычный первый вариант не aerial, поэтому проблема маскировалась.

Исправлено общим SyncAutomaticSelection, без подстановки Caco-углов и
без ослабления height/aim/exclusive/collider gates. Автоматический режим
проверяет nonempty reference guide И drawable mesh. При первом выборе
предпочитает область с текущей позицией внутри, иначе выбирает по
расстоянию до допустимых центров частей. Выбор сохраняется, пока область
доступна, отдельно для GK/CS и по level/entity address/slot/id/generation.
При смене высоты/aim eligibility/цели заново выбирает доступную область.
Полный поиск выполняется в HUD update, не в presentation-rate renderer.
Ручная анимация сохраняется даже при пустой/неподходящей области; никакой
тихой замены. Если подходящих уникальных областей нет, остаётся явное
сообщение, а не выдуманная fallback-зона.

Configured selectedAnimation ОСТАЁТСЯ пустым для automatic; новое поле
Frame.automaticAnimation содержит эффективное имя. Это принципиально:
render freshness guard сравнивает selectedAnimation с настройкой пользователя.
Нельзя записывать результат auto поверх configured key, иначе draw guard
скроет весь кадр. syncSelectedAnimation используется единообразно для
volume, HUD, aim box, feedback и mesh. Raw-reader fallback больше не
начинает с airborne, но production HUD всегда вызывает resolver.
Привязка сектора к врагу, все v11 габариты/цвета/условия и две вертикали
сохраняются. Auto selection не управляет выбором анимации в самой игре.

В native numbers catalog P-строка теперь имеет optional четвёртую ячейку
с automatic choice. Новый managed parser читает старые P3 и новые P4,
проверяет key и принадлежность A-списку. UI показывает Automatic: front
chest и т.п., вместо ложного First available: above back. При ручной
анимации, которой нет у новой цели, UI подсказывает выбрать Automatic
или другой вариант. Публикация не меняет сохранённую настройку blank/
manual. Старый v11-клиент не обязан понимать новый P4; новый пакет нужен
целиком с полным рестартом игры, не отдельно заменённый DLL.

Новый bounded read-only one-shot production diagnostic на текущем зомби
выбрал AUTOMATIC=kill_front_chest: renderer построил84fill triangles,
61line segments (включая frame). build/sync-auto-v12-zombie-live.tsv.
Это проверка production данных/генерации вершин в отдельном read-only
процессе, НЕ наблюдение новой DLL на GPU и НЕ запуск/запись GK-клипа.
Время6.483ms включает начальный resolve и запись fixture, НЕ FPS и
не чистый reader benchmark. Снимок с настоящими transforms/aim/ranges/
footprint сохранён в tracked fixtures/sync-zombie-tier3-11.tsv, без
угаданных synthetic directions. На нём тест воспроизводит старую пустую
above_back, требует automatic front_chest с fill+outline, проверяет
sticky при перемещении, reselection на lower aim, сброс target identity,
разделение action, ручной aerial/missing выбор и отсутствие fallback
при неизвестном range.

Проверки:6 native regression suites PASS, включая обновлённый настоящий
atomic numbers protocol writer;22 Viewer профиля только SYNTHETIC.
Launcher managed tests PASS с P3/P4/invalid-auto/subset/culture checks.
Actual isolated browser UI125/150% PASS, включая actual Automatic:
left upper при другом первом имени в списке, без перезаписи настройки;
150% screenshot просмотрен. Managed build0warnings0errors, JSsyntax и
whitespace с cr-at-eol PASS. Все старые math/clearance/recorded overlap/
anchor tests сохранены. Логи build/sync-auto-v12-regression.log,
sync-auto-v12-launcher-tests.log,sync-auto-v12-ui-qa.log.

Готово build/playtest-sync-auto-v12/LABPlus.exe, обе source/package DLL
сверены по hash в sync-auto-v12-build-manifest.json. Полный ручной рестарт
через v12. Новый in-game визуал/финальные клипы/FPS ещё не наблюдались.
Никаких game calls/input/writes/injection/restart, новых длительных
observers/записи или выключения ПК не делалось. Остальные неизвестные
centric/scripted/focus-fallback ограничения v11 не выдаются за решённые.
Исходники перед правкой: build/sync-auto-v12-baseline.

## Предыдущая пробная сборка: другие демоны и габариты игрока (v11)

Пользователь принял привязку v10 и попросил распространить зоны на других
демонов, а также учитывать хитбокс игрока при окраске границ. Привёл случай:
стоя сзади перед границей, получил передний распил при голубой зоне.
Конкретный новый распил не записывался; НЕ объявляем его причиной касание
капсулы. Уже исследованный QUICK_CHAINSAW не привязывает имена front/back
к полуплоскостям. Его start-pose/priority selector остаётся условным.

Через bounded read-only resource pool (owner-pointer и имена сверены) в
текущей 1.1 прочитано 338 sync declarations, из них 315 playervsai записей
25 типов. Параметры обычных target-centric GK без berserk: 192 записи,
24 типа; таблица фактических параметров сохранена в
Native/CollisionViewer/tests/fixtures/sync-demon-authoring-11.tsv.
Это НЕ 192 проверенных в бою клипа. Матрицы в тесте таблицы синтетические;
правила и имена реальные. Типы включают arachnotron, archvile, baron,
buffpod, cacodemon, carcass, doomhunter, dreadknight, gargoyle, gladiator,
hellknight, imp, mancubus_fire, marauder, painelemental, pinky, prowler,
revenant, soldier_blaster, tyrant, whiplash, zombie_maykr, zombie_tier1,
zombie_tier3. Tentacle quick Chainsaw остаётся общим data-driven путём.
Особые instigator-centric / scripted варианты не выдаются за точные
фиксированные области и при неизвестных правилах скрываются.

Расширен общий parser/geometry без таблицы секторов по имени демона:
* pitch45 для плоских authored attach-матриц и upright текущей позы;
  обе frame0 матрицы дополнительно проверяются на отсутствие pitch/roll.
  Неподдержанные меньшие ограничения остаются unknown.
* minZ теперь настоящий нижний порог ABS(bodyZ-targetZ), с корректной
  ветвью на текущей высоте ног; никакой догадки above/back по имени.
* native cos(yawDeltaMax) симметричен знаку: поддержан отрицательный yaw
  из реального buffpod; full half-yaw180 означает круг. Широкие half-yaw
  95/100 поддержаны через вычитание противоположного клина; конкуренты
  шире полукруга вычитаются как union двух выпуклых частей. Внутреннего
  искусственного разреза/стенок в выбранном секторе нет.
* авторские радиусы до4096 (в реальных данных512) разрешены, но реальная
  внешняя область по-прежнему ограничена native camera/lunge caps.
* исправлен readSyncRangeLimits: отсутствие CS-строки более не выключает
  GK тяжёлого демона. Native normal lunge начинает с aim+19c и optionally
  берёт ПЕРВУЮ matching monster row; отсутствие override сохраняет базу.
  CS без проверенной строки диапазона не получает выдуманный fallback.

Добавлен sync_player_context.h. Player footprint берётся из physics
bodyBoundsLocal c8 / bodyBoundsWorld e0 с origin/axis/map/type/shapeIndex,
проверкой преобразования 8 углов и повторным чтением. Это консервативная
горизонтальная проекция фактических bounds; высота активации остаётся
native-проверкой origin/ног. Нельзя заявлять, что движок переключает
анимацию от касания капсулой: геометрический запас — дополнительное
консервативное требование предпросмотра по просьбе пользователя.
Selected halfspaces erode на support(player box), competing constraints
expand; balls/cylinders используют conservative box half-diagonal и сдвиг
центра. CS distance-choice band увеличен на 2*horizontal half-diagonal,
что следует из Lipschitz границы разности двух расстояний. Не только
центр, а весь horizontal box должен поместиться в уникальную область.
Неизвестные bounds скрывают цветную область. Изменение текущего shape/
стойки учитывается без выдуманного постоянного radius.

Реализован native joint focus для разных вариантов головы/рук/корпуса:
1) default mode sync_UseFocusHistory=0, sync_semiRandom=0: только native
   cached idPlayer.focusTracker.focusTrace.tr.c при type4, совпавшем
   spawnId цели и валидном modelFeature. joint — trmFeature (+15c60).
2) explicitly enabled history mode: строгая копия выбора native winner
   из jointFocusHistory (+7768), bounded player_focusMaxHistory.
Для обоих путей target.jointAnimator(+1a20)->decl(idDeclMD6)->
jointGroupCollection(+e0), type0, ПЕРВАЯ группа с нужным jointId, как в
5d7200. Native name сравнивается с required damageGroupNames. Проверены
reflection fields/retail profile, bytes 5d721d, имена/значения cvars,
coherence owner/list/contact/history/mode. Первая shot sphere НЕ заменяет
joint focus. Если native использует fallback closest-joint search или
semiRandom, результат unknown и зависимая область скрывается. Когда
native focus известен, несовместимые focus-конкуренты не вычитаются.

Голубая заливка удалена. Теперь GREY = только reference; AMBER = граница
хитбокса/проверки pending/условный CS selector; RED = actual native reject
или другой native выбор; GREEN = полный footprint fit и sole observed
candidate snapshot. Для green дополнительно строится body-clearance
actual guide, а не только reference/point-origin. CS с несколькими
current-pose вариантами по-прежнему не получает обещание green за одно
геометрическое расстояние. Tone один раз считается на fresh draw frame
и используется заливкой и контуром. Near-boundary amber определяется по
внешнему mesh; серый не выдаёт положение за готовый запуск анимации.
HUD показывает ширину/глубину игрока и легенду. Фиксация на враге, заливка
по высоте его рамки, только внешние контуры +2 arc uprights сохранены.

Проверки: 5 native suites PASS (22 Viewer профиля СИНТЕТИЧЕСКИЕ), затем
финальные focused tests + DLL PASS. Новые тесты: 192 real metadata cases /
24 типа; широкий/full/negative yaw; absolute minZ оба знака; focus mismatch
и исключение arm-конкурента при head; native cached contact ownership и
first matching group, history winner, unsupported fallback; настоящие
physics bounds и смена shape/несогласованные bounds; point-inside при
footprint-crossing не green; CS centre-only случай исключён body margin;
base lunge fallback и first matching monster override. Старые recorded
Back/Right и anchored gaze-invariance tests PASS. Managed0warnings0errors;
real isolated UI125/150% PASS,150% screenshot inspected; JSsyntax/diff
whitespace с cr-at-eol PASS. Offline example guide+mesh~2.776ms, НЕ FPS.

Read-only diagnostics текущего процесса22592/base7ff79f3d0000: новый
production layout accepted; прямой production footprint reader known1,
min(-.395,-.394997),max(.395,.394997). Во время двух одноразовых reticle
проб цель не была под прицелом (targetFrames0): это НЕ live-проверка новых
областей/анимаций. Сохранён native cached Imp blueprint: 28 frame0 caches,
all valid/flat; payload build/sync-demons-v11-imp-pose-cache.json.
Сопоставление declaration.animWeb с cache дало 0 строк; причина не
подтверждена. В другой временной диагностике после сохранения cache возник
Python variable shadow. Не заявляем полную production-проверку Imp. Новых gameplay observers/записи, ввода/движковых вызовов/
инъекций/рестарта не было; все handles/одноразовые процессы закрыты.

Сборка build/playtest-sync-demons-v11/LABPlus.exe; source/package hashes
обеих DLL проверены в build/sync-demons-v11-build-manifest.json. Нужен
полный пользовательский перезапуск. Все демоны в игре по отдельности,
фактические новые клипы и FPS пока не проверены. Основные логи:
build/sync-demons-v11-final-native.log, sync-demons-v11-regression.log,
sync-demons-v11-ui-qa.log. Исходники до правки — sync-demons-v11-baseline.

## Предыдущая пробная сборка: привязка к врагу (v10)

Пользователь проверил v9 и сообщил, что при движении прицела границы
и вся зона смещаются относительно врага. Причина подтверждена исходниками:
actual aimOrigin вычислялся из ближайшего попадания reticle-ray по shot-
сферам минус eyeOffset; modelViewOffset дополнительно вращал yaw-границы.
Это исходная модель ТЕКУЩЕГО взгляда, а не жёсткий опорный сектор врага.

Добавлена ЯВНО ОТДЕЛЬНАЯ опорная геометрия: syncTargetReferenceLimits /
syncBuildReferenceGuide. Её apex — facing.origin, углы — authored facing
в native осях врага, modelViewOffset для reference равен 0; lateral eye
XY offset обнулён. Предположение reference: тело игрока смотрит на origin
врага. Сохранены текущая высота ног/вертикальная eyeOffset, native диапазоны,
height/start/lunge, aim-quadrant eligibility, маски авторинга и вырезы
конкурентов ПРИ ЭТОМ ПРЕДПОЛОЖЕНИИ. Reference — не обещание той же
эксклюзивности для любого фактического направления взгляда. При смене
стойки/высоты, native правил или разрешённого aim-квадранта область может
измениться/скрыться; это не screen-space заморозка.

Actual aimOrigin / modelViewOffset не переписываются, исходный native
volume остаётся прежним. Перед подтверждением syncGuideFeedback проверяет
действительный выбранный volume, текущих native конкурентов, camera range,
coherent/pending кандидатные флаги. Неподходящий взгляд или actual overlap
не даёт GREEN, даже если игрок стоит внутри фиксированного reference
и кандидатная публикация пока содержит только выбранное имя. Текст
явно разделяет опорный сектор и текущую проверку взгляда; camera distance
выводится настоящая, distance intervals помечены как reference. Для GK
не строится второй полный actual exclusive mesh: достаточно native
selected/competitor volumes; для CS current-pose selector guide сохранён.
Native CS позы/приоритеты по-прежнему условны и могут меняться при реальном
изменении состояния selector, их не замораживают искусственно.

Высота заливки по hit-box, отсутствие внутреннего веера, контур и две
вертикали на третях внешней дуги v9 сохранены. HUD, подсказки и справка
обновлены. Никакого изменения игрового выбора/форсирования анимаций.

Проверки: 5 native suites PASS, 22 Viewer профиля только синтетические;
финальные focused tests/DLL PASS; managed 0 warnings/errors; настоящее
изолированное окно настроек 125/150% PASS, скриншот 150% просмотрен.
Тест сравнивает ВСЕ вершины fill+outline побитно при 12 разных aim points,
model/view offsets, incoming yaw и lateral camera offsets: reference
одинаков. Отдельные проверки translation/rotation врага, сохранности raw
sample, actual wrong-facing, actual competing overlap при reference-only
эксклюзивности, actual camera range 10.50 > 9.00 при reference внутри,
а также CS reference при неизменных selector poses. Записанная раньше
ошибочная Back/Right точка по-прежнему исключена и из нового reference.
Новые focused tests: build/sync-anchor-v10-final-native.log;
DLL: build/sync-anchor-v10-final-package-native.log;
regression: build/sync-anchor-v10-regression.log;
UI: build/sync-anchor-v10-ui-qa.log. JS syntax / diff whitespace с
cr-at-eol PASS. Offline CPU пример guide+mesh ~2.46 мс, не игровой FPS.

Готово: build/playtest-sync-anchor-v10/LABPlus.exe. Source/package обеих
DLL сверены, hashes в build/sync-anchor-v10-build-manifest.json.
Источник перед правкой: build/sync-anchor-v10-baseline. Пользователю
нужен полный перезапуск игры через v10. Новый визуал в игре ещё не
проверен; последний read-only inventory: игра PID2084, LAB22760 v9.
В этом ходе не было игровых вызовов/ввода/записи/рестарта/инъекций,
новых наблюдателей или повторного выключения ПК. Local only, strict1.1.

## Предыдущая пробная сборка: залитый объёмный сектор (v9)

Пользователь уточнил рисунком: нужны только границы сектора и цветная
заливка по всей высоте текущей рамки врага, а не веер лучей. Затем явно
попросил ещё две вертикальные линии по внешнему контуру между границами.
Реализовано в sync_sector_geometry.h: готовое исключительное XY-сечение
обрезается по прямоугольнику shot-AABB и вытягивается от shotBounds[2]
до shotBounds[5]. Полупрозрачные верх, низ и внешние боковые стенки;
контур по периметру, вертикали на настоящих углах, без швов разбиения
и без частокола на каждом отсчёте дуги. Две добавочные вертикали делят
внешнюю дугу по длине на три части. Выбор дуги: связная гладкая выпуклая
цепочка внешнего контура; прямые стороны и контуры отверстий исключены.
При нескольких дугах предпочтение внешней цепочке по длине и удалению
от рамки. Статический пример клина проверяет обе позиции +/-15 градусов
внутри дуги +/-45 градусов и полную высоту этих двух вертикалей.

Геометрия сохраняет вырезы конкурентов и пустоты между областями.
Общие и частично совпавшие внутренние рёбра выпуклого разбиения удаляются
через ограниченный пространственный индекс; боковые стенки и линии
внутри объёма не рисуются. Бензопила сохраняет условный current-pose
расчёт v8. Предел высоты отрисовки — визуальное требование пользователя,
а НЕ утверждение о native-высоте активации: XY-условия оцениваются на
текущей высоте ног, что прямо указано в HUD и справке. Золотой указатель
из геометрии убран; расстояние до подходящей точки остаётся текстом.

Проверки: 5 native regression suites PASS (22 профиля Viewer синтетические),
последующие focused tests с обеими вертикалями + финальный DLL PASS;
managed 0 warnings/errors; изолированное реальное окно настроек 125/150%
PASS, скриншот 150% просмотрен. Тесты проверяют точную высоту AABB,
сохранность вырезов/случайной полосы Chainsaw, отсутствие внутренних
линий и стенок, отсутствие заполнения внутри рамки, ровно две добавочные
вертикали на 1/3 и 2/3 внешней дуги. Native-меш примера выгружен в
build/sync-sector-v9-mesh.tsv, QA-рисунок из этих вершин просмотрен:
build/sync-sector-v9-geometry-qa.png. Это НЕ скриншот новой отрисовки
в игре. Средний offline CPU current-pose guide+mesh ~2.48 мс, не FPS.
Работа/размер меша ограничены; расширение лимита вычитания до 4096
касается только новых display-сечений, прежний лимит GK 256 сохранён.
Общий mesh используется одновременно для заливки и контура кадра.

Сборка build/playtest-sync-sector-v9/LABPlus.exe, обе DLL сверены с
исходными; hashes в build/sync-sector-v9-build-manifest.json. Нужен
пользовательский полный перезапуск игры через новую сборку. Новых
наблюдателей, записи, игрового ввода/вызовов/инъекций или рестарта не было.
Внутриигровой визуал, итоговые клипы и FPS пока не подтверждены.

## Предыдущая пробная сборка: лучи от рамки и отбор бензопилы (v8)

Вместо отдельного заполненного сектора теперь рисуются лучи, связанные
с поверхностью shot-AABB врага. Тусклый соединитель ведёт от рамки до
первого подходящего интервала на высоте ног. Цветными рисуются только
оставшиеся интервалы; разрывы и случайные пересечения не закрашиваются.
Каркас врага и условная подсветка native-квадрантов сохранены.

Подпись ANY заменена на `Aim quadrants: no aim-quadrant restriction`.
Это отсутствие ограничения ТОЛЬКО по focalScreenQuadrants. Требование
joint-focus history выводится отдельно и по-прежнему не подменяется
первым sphere-hit. У исследованного какодемона оба распила имеют ANY;
это не доказывает, что наведение определяет Front/Back.

Исправлена прежняя причина пустой зоны Chainsaw: общий диапазон
бензопилы больше не вычитается сам из себя, когда известны данные
последующего selector. Аудит функции 18d2600 показывает отбор по
runtime `idSyncAttackInteraction.syncPriority` (+4c), затем расстояние
от native origin игрока до `adjustedParticipantPosition` (+84).
Это НЕ `syncPos` (+54), НЕ declaration default priority и НЕ имя распила.
Среди максимальных равных приоритетов native сохраняет случайную полосу
расстояний: d < min(d) + dp_gloryKillMaxAllowableDistanceForRandom.
Ссылка cvar 6189a40, имя проверяется; текущее значение 0.381 м.
Приоритет какодемона runtime 1.25, хотя default priority равен 0.

Предпросмотр Chainsaw явно УСЛОВНЫЙ ПО ТЕКУЩИМ NATIVE-ПОЗАМ И ПРИОРИТЕТАМ.
Он не утверждает их неизменность при перемещении игрока и не заменяет
проверки допуска. Поддержан native origin без interpolation-end adjustment:
1178330 -> physics origin; 1173d70 добавляет поправку при blueprint+1caa.
При установленном adjustToInterpolationEndPosition предпросмотр скрывается.
Проверяются reflection layouts, retail 1.1 и байты потребителя приоритетов,
поз и полосы. Динамические позы не восстанавливаются из authoring-кэша;
неполные/несогласованные данные скрывают область. Владельцы и поколения
сверяются; свежая отрисовка скрывает snapshot при сдвиге игрока более
0.05 м, цели более 0.01 м либо изменении компоненты оси более 0.001.

Геометрия при равном приоритете — внутренняя аппроксимация условий
|P-B| > |P-A| + tolerance для каждого конкурента. На треугольнике
аффинная интерполяция нормы даёт верхнюю оценку |P-A|, поэтому clipping
не создаёт ложного уникального участка. Дополнительный зазор учитывает
точность координат. При более высоком конкурирующем приоритете область
пуста; более низкий не исключает её. Зелёный статус не обещается только
из-за уникальности этого условного distance-choice snapshot.

Проверки: полный набор 5 native suites PASS (22 профиля Viewer синтетические),
финальные focused tests и DLL PASS; managed build 0 warnings/errors;
изолированное настоящее окно настроек 125/150% PASS, скриншот 150% просмотрен.
Математические тесты проверяют случайную полосу, равные/различные приоритеты,
совпавшие позы, неизвестные данные, все рёбра и выборку >1000 точек,
начала соединителей на рамке и отсутствие пересечений в цветных лучах.
Локальный геометрический QA-рисунок построен из реальных вершин теста
и просмотрен. Это не проверка визуала внутри игры.

Однократный read-only production reader: PID 11884, реальная base
7ff79f3d0000, layout accepted, living Caco id 16796769; оба runtime
CS pose known, приоритеты 1.25, tolerance .381; для cut_front получено
665 оставшихся частей. `build/sync-rays-v8-live-once.tsv`. Никаких
вызовов движка, записи в игру, ввода, инъекций, перезапуска или нового
длительного наблюдателя. Сначала отклонён старый base 7ff781d60000;
после чтения фактического MainModule base диагностика прошла.
Offline CPU: GK ~0.806 мс, CS guide+rays ~1.457 мс; это НЕ игровой FPS.

Сборка: `build/playtest-sync-rays-v8/LABPlus.exe`; hashes в
`build/sync-rays-v8-build-manifest.json`. Нужен пользовательский полный
перезапуск игры через новую сборку. В текущей игре всё ещё загружена v7.
Новый визуал и итоговые клипы в игре пока не подтверждены пользователем.
Выключение ПК, запрошенное для прошлой v7, повторно не планировалось.

## Предыдущая пробная сборка: рамка врага и правила наведения (v7)


Пользователь попросил вернуть квадратный хитбокс, добавить зоны бензопилы
и учесть выбор GK по горизонтальной/вертикальной области наведения.
Для бензопилы отдельно выбрал ТОЛЬКО области без пересечений, даже если
для конкретного врага таких областей не остаётся. Привёл наблюдение:
стоя сзади и наведясь на переднюю часть, можно получить передний распил.
Наблюдение не объявляется доказательством причинного влияния точки прицела.

Возвращена одна каркасная прямоугольная рамка вокруг выбранного живого
врага: AABB объединения фактических shot-сфер, используемых внутри поиска
цели. Анатомические сферы не рисуются; рамка не превращает пустоты между
сферами в действительные попадания. На рамке подсвечиваются области
наведения ТОЛЬКО при наличии соответствующего native-требования выбранной
анимации. Имена upper/lower/front/back не используются для угадывания правил.

Офлайн-аудит EXE 1.1 подтвердил playerRequirements.focalScreenQuadrants
по decl+0x310. Consumer 16bb2b2 читает маску, 16bb2c4 вызывает 16ce6b0,
16bb2c9 проверяет пересечение битов. Маска 0 пропускает проверку; enum
TOP_LEFT=1, TOP_RIGHT=2, BOTTOM_LEFT=4, BOTTOM_RIGHT=8, ANY=0x7fffffff.
16ce6b0 получает GetPhysics через virtual+0xe28, затем GetAbsBounds через
physics virtual+0x38, проецирует СЕРЕДИНУ абсолютных физических bounds
через viewMatrix+0x21370 и projectionMatrix+0x212f0. Сравнения с серединой
экрана включительные, поэтому на осях могут совпадать несколько битов.
Это не средняя точка экранного прямоугольника shot bounds и не деление
на анатомические части. Подсветка и направляющие проходят через правильный
спроецированный physics-centre, даже если он отличается от центра рамки.

Внутренний reader читает и проверяет инструкции getter'ов (mov physics
из entity+0x1d0; lea bounds physics+0xe0), map, повторное чтение bounds
и владельца. Функции движка не вызываются. Применяется та же GPU combined
матрица, соответствие native двухматричной проекции сверено однократно
read-only. Небольшая численная полоса около центральных линий трактуется
как неоднозначная: выбранная маска должна принимать все возможные стороны,
а конкурент достаточно одну. Это защита точности, не новое правило игры.

GK-позиции теперь условны относительно ТЕКУЩЕЙ ОБЛАСТИ НАВЕДЕНИЯ.
Если выбранный вариант не соответствует ей, позиционный контур скрыт,
а HUD просит навести на нужную подсвеченную область. Конкуренты, которые
отвергаются своей native aim-маской, не вычитаются. Остальные вычитаются
по прежним native ограничениям. При движении нужно сохранять указанную
область наведения; старые гарантии итогового клипа не добавлены.

QUICK_CHAINSAW по-прежнему НЕ наследует GK-направления и его position
validation: каждый CS-вариант хранит собственные native aim-маску и
QUICK-признак. Когда наведение оставляет один поддержанный вариант,
отображается его диапазон без придуманных front/back полусфер. Когда
варианты совпадают по условиям, область исключительного положения пуста.
Неподдержанные не-QUICK правила не подменяются QUICK-диапазоном.

Одноразовое чтение работающей 1.1 (PID 19624) подтвердило на какодемоне:
cut_front и cut_back оба имеют ANY=0x7fffffff, пустые damageGroupNames
и syncValid=true. Это не позиционный уникальный разделитель и не
подтверждение, что передний распил вызван наведением на переднюю часть.
Сохранены `build/sync-aim-live-rules-v7.json`,
`build/sync-aim-physics-getters-live-v7.json`,
`build/sync-aim-quadrant-full-v7.log`,
`build/sync-aim-player-validation-v7.log`,
`build/sync-aim-projection-helper-v7.log` и schema-v7.json.
Это ограниченные read-only снимки, не новая игровая запись/наблюдатель.

Отдельно выявлены damageGroupNames и consumer jointFocusHistory
(player+0x7768): они не равны первой sphere-группе текущего луча. Выбранные
анимации, требующие этой ещё не воспроизведённой истории фокуса, помечаются
недоступными для позиционного прогноза. Такой конкурент консервативно
остаётся возможным при совпадении aim-маски. Никаких выдуманных правил
головы/рук/ног для них не добавлено. Полная поддержка всех focus-условий
и всех врагов в игре не заявлена.

Проверки: чтение масок и отдельного QUICK-флага, четверти и общие оси,
неизвестные биты, guarded physics bounds, рамка и перепроекция подсветки
от отличающегося physics-centre, отсутствие искусственного деления ANY,
фильтрация конкурента по наведению, скрытие при неверном/неизвестном
наведении, двусторонний диапазон CS без GK-секторов, исключение общей
оси CS и недоступных joint-focus/non-QUICK правил прошли. Сохранились
проверки v6 и общий HUD/таймеры/техники/22 синтетических профиля Viewer.
UI 125%/150% в отдельном браузере прошёл; UI150% просмотрен.
Финальная .NET-сборка 0 предупреждений, 0 ошибок; packaged DLL сверены.
Native сборка проходит с прежними unused reference-helper warnings.

Выход `build/playtest-sync-aim-v7/LABPlus.exe`, manifest
`build/sync-aim-v7-build-manifest.json`. Native SHA256:
`797F48EB886450C1840521D3F81F90DF43CD1915E5C9D363EA2CF07745EB0F6B`.
Новую рамку/подсветку/контуры v7 в игре ещё не проверяли. Игра не
перезапускалась, управление и память не менялись; активных наблюдателей нет.
Только локально, retail 1.1; другие функции сохранены, без PR/push/release.

## Предыдущая пробная сборка: только области без пересечений (v6)

Пользователь явно попросил исключить позиции, допускающие Back upper и
боковой вариант одновременно. Теперь показано горизонтальное сечение
`область выбранного GK − объединение областей всех остальных известных GK`
на текущей высоте physics origin игрока. При смене высоты сечение меняется.
Для всех вариантов учитываются прежние native ограничения направления,
стартового XY, высоты, camera range и условного body-lunge. Вычитание не
сводится к углам: если боковой вариант уже вне своей дальности/высоты,
соответствующие позиции выбранного GK могут остаться видимыми.

Новая `sync_exclusive_geometry.h` пересекает native объёмы с плоскостью Z,
вычитает объединение выпуклых контуров и сохраняет отдельные части. Видимая
заливка целиком находится в выбранном объёме и вне конкурирующих объёмов.
Обводка строится по настоящей внешней границе и границам отверстий, без
внутренних линий разбиения. Общая оболочка полного исходного объёма убрана:
она снова показывала бы запрещённые перекрытия. Высота остаётся в расчёте,
но показан именно текущий горизонтальный срез, не проекция на рельеф.

Кривые консервативно аппроксимируются 96-угольниками: выбранные круги
вписаны, исключаемые описаны; добавлен зазор от численной границы около
2–3 мм в координатах демонстрации (масштабируется по точности float).
Это численная защита, не новое правило движка. Она не оставляет ложных
узких полос внутри конкурирующего круга. Неподдержанный конкурент,
неполные authoring rules, превышение лимита работы/геометрии или ошибка
проверки float-вершин скрывают область целиком, без возврата к исходному
полноразмерному объёму. Для построения конкурирующих ограничений не нужен
эвристический interior seed старого объёмного mesh: неудача seed не может
стать основанием тихо пропустить конкурентную область.

Золотая линия ведёт к точке в ОСТАВШЕЙСЯ области на высоте ног, проверенной
по выбранному объёму и всем конкурирующим. При полном перекрытии или
отсутствии сечения этой высоты нет ни области, ни точки перемещения.
Линия остаётся прямой и не обходит препятствия. Диапазон на текущей стороне
подхода также вычитает конкурентов аналитически и может содержать несколько
интервалов: запрещённые промежутки не заполняются общей min/max-парой.
Если два CS-варианта имеют одинаковый известный диапазон, область
исключительного положения для одного из них пустая; искусственное
разделение front/back не добавлено.

Результат по сохранённому случаю victim 16796944: точка Back + Right
исключена, Back-only точка позади врага сохранена, подсказка ведёт из
перекрытия в оставшийся контур. Regression проверяет вершины и внутренние
точки треугольников против всех исходных native объёмов, обводку,
несколько отверстий и раздельные интервалы, повторяющиеся исключения,
криволинейное отверстие, высоту конкурента, полное перекрытие, неизвестные
правила и детерминированность. Контрольная иллюстрация из реальных
координат `build/sync-exclusive-v6-fixture.png` просмотрена; это схема
офлайн-данных, не скриншот игры. Один расчёт этого случая занял около
0,67–0,71 мс в офлайн CPU-проверке, не замер игрового FPS. HUD и renderer
используют готовый immutable результат; две отрисовочные проходки не
пересчитывают вычитание повторно. Общие native регрессии и отдельный UI
125%/150% проверены; снимок UI150% просмотрен.

Локальный выход `build/playtest-sync-exclusive-v6/LABPlus.exe`.
Native SHA256:
`E9BFFD8B7047651EA513E62D7094EA4A348C929E2AE0F9722E8A4A4CFF4A0225`.
Native и Lighting DLL в сборке сверены с исходными файлами; manifest
`build/sync-exclusive-v6-build-manifest.json`. Финальная .NET-сборка:
0 предупреждений, 0 ошибок. Native сборка проходит с предупреждениями о
неиспользуемых прежних reference-отрисовщиках; они не вызываются продуктом.
V6 ещё не загружена и не проверена визуально в игре. Это исключение
известных геометрических альтернатив, не подмена финального selector:
состояние врага, препятствия, поздние проверки и актуальность наведения
по-прежнему влияют. Игра не перезапускалась, новых записей не запускалось.
Работа только локально, текущая retail 1.1; BATS и другие функции сохранены.

## Предыдущая пробная сборка: исправление ложного зелёного статуса (v5)

Пользователь сообщил: выбрал какодемону Back upper, вошёл в зелёную область,
но выполнился боковой GK. По его явной просьбе проведена новая read-only
демонстрация на игре PID 9576, подтверждён retail 1.1. Загруженный модуль
`labplus-layer-c820a7c5cc0a92c1.dll` сверён по SHA256 с v4; до записи прочитан
выбор `SyncGloryKillAnimation=kill_back_upper` из реального viewer.ini.
Игра не перезапускалась, её ввод и память не изменялись.

Запись остановлена сразу после сообщения «закончил». Оба наблюдателя
завершились и закрыли handles: Python — 106,89 с, 9 243 снимка, 0 ошибок;
production-reader — 1 085 снимков, из них 436 с целью. Второй наблюдатель
подключился позже первого. Это измерения внешнего чтения, не FPS.
Сырые файлы: `build/sync-v4-backupper-9576-20261106.jsonl` и
`build/sync-v4-geometry-9576-20261106.tsv`; сводка с проверкой victim spawnId
и actual clip: `build/sync-v4-backupper-9576-summary.json`.

По активным INSTIGATOR/TARGET и actual resource clip подтверждены пять GK:
Right upper, Back upper, Left upper, Right upper, Right upper. Перед каждым
наблюдались `syncValid` одновременно для заднего и соответствующего бокового
варианта; ожидание трассировки периодически менялось, поэтому это не снимок
точного кадра потребления ввода. Для последнего GK, victim spawnId 16796944,
есть прямое совпадение production-reader: выбран Back upper, `V4_GREEN=1`,
`GEOMETRY_HERE=kill_back_upper` и `kill_right_upper`; в последнем таком снимке
также оба GK числятся готовыми кандидатами. Фактический clip — Right upper.
Это подтверждает ошибку представления неоднозначной позиции зелёным;
не доказывает, что именно RNG, а не другой этап финального отбора решил исход.
В новых демонстрациях выбранные старты также были ближе к позиции игрока,
но body-distance до adjusted position не следует объявлять полной копией
всех аргументов нативного selector без дополнительной проверки.

v5 не меняет геометрические границы движка. Цвет отражает статус ТЕКУЩЕЙ
позиции: внутри выбранной геометрии жёлтый при любом известном перекрытии,
неподтверждённом/несогласованном списке, ожидании трассировки, нескольких
кандидатах или частичных правилах. Подпись явно называет конкурирующие
геометрические варианты. Зелёный допускается только без известного
перекрытия и при единственном наблюдавшемся подтверждённом кандидате,
совпадающем с выбором; даже он обозначает снимок, а не гарантию нажатия.
Бензопила с несколькими именами общего диапазона остаётся неоднозначной.
Прочие поздние проверки и ограничения исследования сохраняются.

В unit regression внесена реальная поза и авторские начала Back/Right из
этой записи. Позиция по-прежнему удовлетворяет исходной геометрии Back,
но теперь возвращает жёлтый статус с Right upper, и цвет вершин проверен.
Проверены также другой кандидат, pending GK отдельно от Chainsaw,
несогласованное чтение, частичные правила и единственный выбранный кандидат.
Целевые и общие native регрессии прошли; Viewer 22 профиля — синтетические.
Отдельный браузерный UI 125%/150% прошёл. Финальная .NET-сборка:
0 предупреждений, 0 ошибок. Синтаксис help JS и whitespace проверены.

Выход `build/playtest-sync-guide-v5/LABPlus.exe`, manifest
`build/sync-guide-v5-build-manifest.json`. Native SHA256:
`35B05FA51B14C2A50196C06D784E8F75F254CEDFD10996DEF67D507891E375F6`.
Обе packaged DLL сверены с исходными файлами. V5 в игру не загружалась,
новый цвет в игре ещё не проверен. Нет точного прогноза победителя или
замера FPS. Действующий модуль игры остался v4; перезапуск — вручную
пользователем. Всё локально; старые сборки сохранены, без PR/push/release.

## Предыдущая пробная сборка: одна выбранная анимация (v4)

После отзыва о непонятном смысле v3 пользователь согласовал пробу:
выбрать одну анимацию и увидеть, где для неё стоять. Локальный выход:
`build/playtest-sync-guide-v4/LABPlus.exe`. Визуальное принятие v3 или v4
не подразумевается; новая отрисовка v4 ещё не проверена в игре.

В `Research → Invisible numbers` рядом с `Sync preview` добавлен `Animation`.
Список приходит от одного живого врага под прицелом через фоновый writer
Invisible numbers; файлового I/O в игровом callback не добавлено.
Для GK перечисляются поддержанные авторские правила, для Chainsaw —
прочитанные имена взаимодействий. Явный выбор хранится отдельно для GK и
бензопилы. На другом враге отсутствующий вариант помечается unavailable;
подмена другим вариантом и его отрисовка запрещены. Пункт First available
выбирает первый доступный вариант, его имя указано в HUD.
При паузе окно сохраняет список последней цели с явной пометкой Paused,
чтобы переход в приложение не мешал выбору. На смене карты/процесса,
потере цели во время gameplay, устаревании данных или ошибке чтения список
очищается. Пауза не сохраняет активную игровую геометрию; после возврата
работает обычная проверка текущей цели и выбранного правила.

Рисуется только выбранная область: слабая объёмная оболочка для высоты,
яркий горизонтальный контур на текущей высоте physics origin игрока и золотая
отметка его позиции. Золотая линия ведёт к найденной допустимой точке области.
Это НЕ проекция на рельеф и НЕ построение маршрута: линия может пересечь
преграду. При отсутствии сечения на высоте ног ложный контур на полу не рисуется.
Контур голубой снаружи и зелёный внутри учтённой геометрии; это не признак
готовности всех проверок игры. Кривые отображаются вписанными многоугольниками.

HUD показывает фактическую дистанцию камера–native target и её отдельный
native предел. Дополнительно аналитически пересекается луч текущей стороны
подхода на фиксированной высоте ног со ВСЕМИ плоскостями, сферами и цилиндрами
выбранной области: выводится её диапазон в той же метрике камера–target.
Этот диапазон может быть уже общего camera cap. Если ненулевого интервала
нет, HUD предлагает изменить сторону или высоту. Отдельно перечисляются
нарушения направления, высоты, camera range, start-position XY и lunge.
Внутри области сообщаются другие совпавшие поддержанные GK-области.
Недавние native-кандидаты остаются отдельной строкой; несколько кандидатов
не означают гарантированного выбора выделенной анимации.

У бензопилы выбранные варианты показывают общий подтверждённый диапазон.
Уникальная позиционная граница cut_front/cut_back не подтверждена и не
выдумывается. Выбор в интерфейсе не заставляет игру исполнять эту анимацию.
Все ограничения исследования retail 1.1 и поздних проверок v3 сохраняются.

Проверки v4: целевые native-тесты сечений, принадлежности physics origin,
диагностики независимых ограничений, допустимости точки перемещения,
диапазона подхода уже camera cap, отсутствия диапазона с неподходящей
стороны/высоты, отсутствия геометрии невыбранных правил и общей области CS.
Протокол списков/идентичности и отдельное сохранение выбора проверяются
native и Launcher checks. Изолированная браузерная проверка выбора,
смены цели, потери цели и масштаба 125%/150% прошла; снимок 150% просмотрен.
Дополнительно проверены выбор на паузе и очистка по карте/выходу/потере цели.
Общие HUD, таймеры, техники и 22 синтетических профиля Viewer прошли регрессии.
Это не игровая проверка отрисовки, точных границ выбора или FPS.
Первый запуск UI-проверки в ограниченной сессии не создал WebView HWND;
повтор с отдельными тестовыми данными в обычной desktop-сессии прошёл.

Итог: native DLL SHA256
`C820A7C5CC0A92C195338D8E1FD0A100A3F85825F9EFFD4224B2BE8C22A533C7`.
Финальная .NET-сборка: 0 предупреждений, 0 ошибок. Native и Lighting DLL в
выходной папке сверены с исходными артефактами. Манифест:
`build/sync-guide-v4-build-manifest.json`. Игра не запускалась и не
перезапускалась, демонстрации/наблюдатели не запускались. Только локально.

## Предыдущая сборка: полная дальность взаимодействия (v3)

Локальная сборка `build/playtest-sync-ranges-v3/LABPlus.exe` заменяет
подогнанную к телу диаграмму v2 областями потенциальных позиций игрока.
В `Invisible numbers` под `Sync animation zones (1.1)` появился выбор
`Sync preview: Glory Kill / Chainsaw`. Выбор сохраняется в настройках.

Области строятся по проверенным ограничениям 1.1. Для GK пересекаются
авторские требования направления, цилиндр относительно стартовой позиции
анимации, ограничение высоты, предел от камеры и применимый предел рывка
от позиции игрока. Для бензопилы используется её отдельный предел от камеры:
проверенные QUICK_CHAINSAW пропускают часть ограничений обычного GK.
В прочитанном стандартном профиле какодемона предел от камеры составляет
9 м для GK и 10 м для бензопилы; это не универсальные значения для всех целей.
Учитываются текущее смещение камеры относительно игрока и текущая точка
наведения. Неизвестные или неподдерживаемые условия скрывают область;
возврата к условному объёму размером с тело больше нет.

Это потенциальные позиции при текущем наведении. Преграды, состояние цели,
поздние проверки и окончательный выбор между допустимыми анимациями
по-прежнему влияют на результат. Область не обещает конкретную анимацию.
Для бензопилы нет придуманной границы «спереди / сбоку / сзади»:
в наблюдениях `cut_front` и `cut_back` могли быть допустимы одновременно.
Пользователь дополнительно уточнил, что передний распил доступен и сбоку;
это не доказательство влияния точки прицеливания на такой результат.

В двух завершённых демонстрациях записаны четыре фактических GK
(`front_lower` три раза, `left_upper` один) и четыре убийства бензопилой
(`cut_front` один раз, `cut_back` три). Фактические клипы проверены по активным
участникам синхронной анимации. Эти записи не удостоверяют точные границы
всех областей: внешний опрос не совпадает с кадром потребления ввода.

Проверки v3: сборки native и .NET завершены; тесты геометрических ограничений,
общего HUD, таймеров и техник прошли. Проверки Viewer на 22 профилях являются
синтетическими, а не игровыми проверками других патчей. Проверки сохранения
настроек и интерфейса в отдельном браузере при масштабе 125%/150% прошли.
Read-only probe прочитал диапазоны и четыре направления какодемона в 53
целевых снимках. Новый модуль v3 пока не проверен визуально в игре;
замеры FPS/GPU и точных границ анимаций не выполнены.

Native SHA256 v3:
`60809B32E3F2A1312A82AD9AF0B83D4F00501C74CFD7F7C4BC9972F1B19798FF`.
Обе native DLL в папке сборки сверены с исходными артефактами. Работа остаётся
локальной, без PR, отправки кода и релиза.

## Требуемое отображение

Показывать геометрию только одного живого противника непосредственно под
прицелом. Пользователь предпочитает понятные полупрозрачные объёмные сектора
с различимыми границами и выделением текущей области наведения.

Направления front/back/left/right должны относиться к противнику. Произвольное
разбиение тела на равные углы нельзя выдавать за границы выбора анимации.
Область попадания, допустимость взаимодействия и окончательный выбор анимации
нужно различать. Если несколько вариантов допустимы, подпись должна сообщать
об этом, а не обещать единственный результат.

## Что подтверждено наблюдениями

В `idPlayer.syncMelee` находится `idSyncAttack` со списком
`possibleSyncInteractions`. В нём доступны декларация, признак `syncValid`,
ожидание трассировки, приоритет, позиция, матрица и данные анимации.

В демонстрации наведения без нажатия GK у какодемона менялись допустимые
варианты: `front_lower`, затем одновременно `front_lower` и `right_upper`,
затем `right_upper`. При последующем выполненном GK прямое поле
`savedInteraction` содержало `syncdeath/playervsai/cacodemon/front_lower`.
Это подтверждает наличие предварительных вариантов, но не доказывает точный
прогноз при каждом нажатии.

В отдельной демонстрации бензопилы было 3 563 снимка без ошибок чтения. До
выполнения одновременно встречались допустимые `chainsaw/cut_front` и
`chainsaw/cut_back` с одинаковым приоритетом. При выполнении прямое поле
`savedInteraction` содержало `chainsaw/cut_back` в 62 снимках. Опрос внешнего
наблюдателя не привязан к моменту потребления данных движком; эти 62 снимка
не являются отдельными убийствами.

Поле `interactionToExecute` не удалось застать ненулевым. Поле
`syncSystemIsReadyFrame` оставалось нулевым, поэтому использовать его как
подтверждение готовности результатов нельзя. Признаки допустимости меняются
при обновлении списка; одна проверка заголовка списка не гарантирует готовность
всех асинхронных проверок.

## Выбор из списка — отдельный этап

В функции RVA `18d2600` код читает `possibleSyncInteractions` (`+178`, число
`+180`), отбрасывает невалидные элементы (`+48`) и проверяет соответствие
ввода декларации через `18d0720`. Затем учитывает приоритет (`+4c`) и расстояние
до `adjustedParticipantPosition` (`+84`).

При нескольких оставшихся элементах есть дополнительный отбор по расстоянию
и случайный выбор: `18d29b3` обновляет общее состояние генератора
(`state * 0x19660d + 0x3c6ef35f`), после чего индекс берётся по модулю числа
элементов. Если остался один элемент, эта ветвь не выполняется.
Вызов выбора непосредственно перед исполнением найден, в частности, в
`15cbfcd` → `18d1a30`.

Это не основание называть любое GK случайным. Однако заранее обещать один
вариант при перекрытии кандидатов нельзя, просто прочитав `syncValid` либо
выбрав максимальный приоритет. Общее состояние генератора может измениться
между предпросмотром и нажатием.

Отдельно существует выбор анимации внутри декларации: RANDOMLY, LEASTUSED и
BESTHEIGHTFIT. У прочитанных деклараций какодемона было по одной анимации;
выбор между разными декларациями нельзя смешивать с этим режимом.

## Настоящая геометрия попаданий

У наблюдавшегося какодемона постоянное `idAnimatedEntity.sphereShape` было
нулевым. Нужная структура оказалась в `animatedEntityVolatile` (`+1a10`):
`sphereModelShape` (`+18`) и `spherePhysicsObj` (`+28`). Последний указывает на
`idHavokPhysics_SphereTree`; его собственный `sphereShape` (`+238`) совпадает
с volatile-полем.

В сохранённой структуре найден compound Havok kind 13 с 27 дочерними
примитивами kind 2. Есть трансформы дочерних экземпляров, радиусы, таблица
`instanceToIndex` из **short**, индексы суставов и `havokShapeData_t` с реальными
группами `head`, `chest`, `left_arm`, `right_arm`, `left_leg`, `right_leg`,
`mouth`. У корня нет группы; дочерние данные сопоставляются по ссылке материала,
а не по предположению, что индекс корневой строки совпадает с индексом ребёнка.
Снимок был дополнительно прочитан из сохранявшегося экземпляра после убийства;
это ещё не проверка стабильного обновления живой геометрии и не готовая
отрисовка.

Таблица `idGameViewInfo` в этой версии найдена через глобальный указатель RVA
`6128718` и смещение объекта `136140`. Подтверждён view владельца-player,
двойной буфер `processedEntities`, `resolvedProcessedEntities` и индекс
результатов. Это список сведений о целях, **не готовый ответ «враг под
прицелом»**. В нём есть дальние объекты и сам игрок. Нужны проверка луча,
видимости, поколения сущности и готовности данных.

## Предыдущая версия v2: направления анимаций

Переключатель в `Research → Invisible numbers` переименован в
`Sync animation zones (1.1)`. Анатомические сферы больше не рисуются:
они используются только для поиска ближайшего врага под прицелом.
Видимая геометрия — разбиение объёма вокруг выбранного врага угловыми
плоскостями. Размер этого объёма подогнан к живой геометрии врага и является
масштабом схемы, **не расстоянием активации и не границами по высоте**.

Направления берутся из относительных матриц INSTIGATOR/TARGET на нулевом
кадре родного joint cache. Полуширина — `validation.yawDeltaMax`, без
распознавания слов front/back/left/right в имени анимации и без равных
квадрантов. Используется текущая ось actor physics, которую читает
`idAI2::GetPositionAndAxis`, и `idPlayer.modelAxis` для текущего направления
игрока. Ось sphere physics не подменяет направление лица демона.

Перекрытия диапазонов разбиваются на непересекающиеся ячейки со смешанным
цветом; одинаковые прозрачные поверхности не накладываются друг на друга.
Текущая ячейка светлее. Внутренние границы показаны линиями, а оболочка имеет
прозрачную заливку. Это схема **условий направления**, не окончательный
пространственный предиктор всех GK: состояние врага, высота, расстояние,
скольжение, препятствия и поздние проверки остаются в движке.

Области пока рисуются для прямых, вертикальных, target-centric правил с
pitchDeltaMax = 90, yawDeltaMax не больше 90 и нулевым дополнительным поворотом. Особые facing rules,
instigator-centric выравнивание, наклонные позы, другие pitch-ограничения,
неготовый/неподходящий кэш не заменяются выдуманными областями. HUD сообщает
об отсутствии/частичности доступных правил. Строгие поля отражения, byte guard
бензопилы и getter оси проверяются только для 1.1.

В 1.1 ветка `18db396..18db39f` пропускает проверку положения/направления
`18d99f0` для QUICK_CHAINSAW в любом из двух input-слотов. Поэтому декларации
бензопилы не превращаются в GK-секторы по именам cut_front/cut_back.
У проверенного какодемона оба варианта могут быть допустимы одновременно.
Список родных кандидатов Chainsaw остаётся отдельной строкой; точный исход
при нескольких кандидатах может зависеть от окончательного отбора/RNG.

Текст кандидатов объединяет реально прочитанные согласованные имена в
125-миллисекундные окна. Внутри окна промежуточные пустые списки не стирают
названия каждый кадр. Смена цели/поколения/map-slot сбрасывает представление
сразу; окно только с несогласованными чтениями обозначается checking.
Список может описывать предыдущий завершённый короткий интервал, а не
обещать результат нажатия в текущий момент. Команда включения игрового HUD
больше не повторяется при каждом кратком отсутствии цели.

Внешний read-only production-reader на работающей 1.1 прочитал 96 из 96
снимков выбранного живого какодемона и четыре диапазона GK: направления
приблизительно 0,00247°, 90°, −84,6599° и −180°, каждый с полушириной 60°
в текущей мировой системе. Среднее внешнее чтение 0,299 мс, максимум
0,598 мс. Это проверка данных, **не визуальная проверка нового DLL в игре**
и не замер GPU/FPS. Сырая запись: `build/sync-facing-19640-b.tsv`.

Автоматические проверки охватывают матрицы и реальные yaw-пределы,
отказ от неподдерживаемых правил, перекрытия с сохранением площади без
двойной заливки, неизменную геометрию при неподвижных данных и короткие
окна представления при быстро меняющихся кандидатах. Проверки старого
Viewer на 22 синтетических профилях, общего HUD, таймеров и техник прошли.
Это не подтверждение работы новой функции на других патчах.

### Визуальная проверка v2 пользователем

После перезапуска пользователь подтвердил: «Области видны, текст читается».
Проверены путь запущенного LAB Plus (`playtest-sync-zones-v2`), версия игры
1.1 и загруженный DLL с SHA-256
`8A4BF34C98628C41B27FF44D4436A65B2FC17730079B2881A40B0A85AF0EEB06`.
Это подтверждает видимость угловой геометрии и читаемость строки кандидатов
в показанном случае. Совпадение каждой границы с фактически выполненным
добиванием, все типы врагов, полная проверка условий и другие патчи этим
ответом не подтверждены. Новых замеров FPS или GPU не выполнялось.

## Первая локальная реализация (заменена)

Добавлен выключенный по умолчанию `Research → Invisible numbers → Enemy hit zones (1.1)`.
Он работает независимо от включения Collision Viewer.
Проверены точные поля отражения этой версии и машинные инструкции, используемые
для адреса менеджера и признака line of sight. Другие патчи в этом режиме
отключены. Существующие категории геометрии и их оформление не меняются.

Выбор использует текущий буфер GameView владельца-player, признаки isAI/LoS,
реальный центральный луч камеры и ближайшее пересечение с анимированными
сферами. Проверяются ссылка map-slot, поколение, spawnId GameView, принадлежность
физики карте и `isDead`. Пересечение только с окружающим AABB не считается
попаданием. Неподдерживаемая геометрия не заменяется приблизительным объёмом.
Сферы сопоставлены с `hknpSphereShape` по RTTI и точной vtable 1.1, а группы —
с реальными material/contents-записями. Фильтр использует SHOTCLIP и исключает
NOCOLLISION/отключённые экземпляры. Неравномерный/нечисловой масштаб отвергается.

Группы тела различаются цветом, текущая группа выделяется ярче. В режиме Filled
показывается полупрозрачная передняя поверхность, в Outlines — основные контуры.
Используются прежний Vulkan-проход и проверка глубины. Геометрия обновляется
перед отрисовкой; для заливки и линий фиксируется один снимок. View distance
распространяется и на эту опцию. Смешанные/меняющиеся снимки пропускаются, с
одной ограниченной повторной попыткой. У выборки есть бюджет времени.

В отдельном последнем блоке игрового HUD выводятся выбранная группа и
**предварительные кандидаты** GK/Chainsaw. Разделение действий следует реальному
фильтру `syncInputs` 1.1, включая особую ветвь QUICK_CHAINSAW; требования
Berserk/Crucible не выдаются за обычное GK. Проверяются указатель и spawnId
цели кандидата, valid/waiting и стабильность читаемых данных. Это список
кандидатов до окончательного отбора, а не обещание одной анимации или наличия
всех условий для запуска. Строки используют понятные подписи из реальных
имён состояний (`kill_front_lower` → `Front lower`).

**Это пока объёмные hit-группы тела, а не готовые front/side/back-секторы
выбора анимации.** Последние по-прежнему требуют разбора всех условий ниже.

Проверка того же production-reader отдельной внешней программой прошла на
живом какодемоне 1.1: корректно найдены 27 объёмов, пересечения луча и нативные
кандидаты. Первый короткий замер: 389 снимков, 320 с выбранной целью;
среднее внешнее чтение 0,264 мс, максимум 1,965 мс. Это не замер CPU игрового
callback, GPU или влияния на FPS. Следующий замер с движением камеры содержит
367 снимков и 61 с целью; его среднее нельзя напрямую сравнивать с первым.

Синтетические проверки покрывают ближайшую цель, луч вне сферы внутри AABB,
смерть, смену map-slot/spawnId/владельца, LoS-фильтр, входы действий,
неподдерживаемые примитивы, нечисловой масштаб и ограниченную геометрию заливки.
Проверки общего HUD, геометрии старых профилей, таймеров и техник также прошли.
Тест реального браузерного интерфейса подтвердил сохранение переключателя,
компактную компоновку и масштабы 125/150%. Оба DLL включены в тестовый пакет.

Пользователь запустил эту сборку на 1.1. Прототип **не принят**: hit-группы
тела не соответствуют запрошенной визуализации пространственных условий
выбора GK/Chainsaw. Также подтверждено быстрое изменение текста кандидатов
Chainsaw в одной строке (не накопление копий строк). Это требует отдельной
стабилизации представления; нельзя считать текущий HUD читаемым и проверенным.
Нужна проверка частичного укрытия: используемый нативный LoS-флаг GameView
не является отдельной трассировкой именно центрального луча, а проверка глубины
относится к пикселям геометрии. В кадрах с изменением тела/буферов также нужно
оценить возможные пропуски и мерцание. Гарантия точного исхода GK/Chainsaw,
проверка всех врагов и совместимость новых функций с другими патчами не заявлены.

## Что ещё требуется для секторов выбора анимации

Уточнение пользователя: показывать области выбора анимации относительно
врага, а не отдельные руки, голову, грудь и прочие damage/hit-группы.
Реальные сферы попаданий нужны лишь внутри поиска врага под прицелом.
Равные сектора сферы не являются подтверждённым устройством движка.

Дополнительное чтение 1.1 подтвердило сохранённый `idSyncEntity` с
`animweb/syncmelee/cacodemon`: список участников с `attach00`/`attach01` и
14 кэшированных поз на кадре 0 (`cachedAnimJoints`, stride 0x3030).
Ноды деклараций сопоставлены с этим кэшем: `kill_front_lower` → 3,
`kill_back_upper` → 2, `kill_left_upper` → 5, `kill_right_upper` → 6,
`cut_back` → 9, `cut_front` → 10. Имена MD6-анимаций в кэше подтверждают
это сопоставление. Матрицы attach-суставов различаются между вариантами.
Это источник авторских начальных поз, а не готовая разметка границ выбора.

Полное чтение `idSyncEntity::GetDesiredSyncEntityPosAndAxis` (0x11763a0)
подтвердило использование attach-суставов участников и joint cache.
Его аргументы с участниками нельзя ошибочно трактовать как указатели на
animWeb/animNode: функция читает у них `attachJoint` по +0xaa.
Копирование кэша по nodeIndex происходит в 0x1177de0, извлечение
преобразования суставов — в 0x613b50. У Chainsaw `cut_back` и `cut_front`
различаются флаги instigator-centric; универсальное правило двух равных
полусфер для них не установлено.

В этом сеансе матрица `bodyAxis` sphere physics у какодемона единичная:
её нельзя автоматически считать направлением взгляда/лица демона.
Нативный `GetPositionAndAxis` idAI2 (0xbc3ba0) берёт ось из actor physics
через +0x58a0/+0x18 и virtual +0x70 (getter 0xd41ed0), а у player
(0x16cd7c0) есть отдельная ветка с `modelAxis`. Эти пути и условия
выравнивания ещё нужно полностью учесть перед рисованием угловых границ.
Получение начальных поз само по себе не означает готовый точный предиктор.

- Визуально проверить обновление живых примитивов и перевод в мир.
- Проверить центральный луч в том числе при частичном укрытии и движении.
- Разобрать все используемые проверки направления, высоты, позиции,
  focal/damage-групп и поздней валидации, необходимые для границ секторов.
- Измерить стоимость одной цели и проверить исчезновение геометрии при
  отведении прицела, смерти, смене карты и повторном использовании entity-slot.

## Локальные материалы

Сырые записи и вспомогательные инструменты находятся в игнорируемой папке
`build`: `sync-preview-1.1-21216.jsonl` (наведение/GK),
`sync-preview-1.1-21216-chainsaw.jsonl`, `sync-sphere-tree-1.1.json`,
`sync-sphere-groups-1.1.json`, `sync-view-entities-1.1.json`,
`sync-selection-1.1.log`, `sync-selection-callers-1.1.log`.
Промежуточная запись `sync-preview-1.1-21216-v2.jsonl` содержит только отсутствие
кандидатов и не является проверкой бензопилы.

В первой записи наблюдатель ошибочно прочитал начало managed-ref как указатель
цели. У `idManagedClassPtr` указатель находится по `+8`, а spawnId — по `+0`.
Названия деклараций, прямое `savedInteraction` и отдельное поле spawnId не
затронуты этой ошибкой; указатели целей/сущностей из первой записи использовать
нельзя. В записи бензопилы это исправлено.

Все наблюдения сделаны чтением памяти. Вызовов функций выбора, изменения
случайного состояния и принудительных GK/Chainsaw не выполнялось. Наблюдатели
остановлены. Новое поведение отрисовки и другие версии игры не протестированы.
