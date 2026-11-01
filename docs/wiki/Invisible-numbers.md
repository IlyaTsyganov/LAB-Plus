# Invisible numbers

Open **Research → Invisible numbers** to inspect living enemies, timers and speedrun-technique observations. Enable each optional in-game display separately:

```text
labplus_enemies 1
labplus_enemy_hp_bars 1
labplus_timers 1
labplus_techs 1
```

Use 0 to hide a display. The window can inspect data without enabling every HUD group. Compatible meathook and the current native layer are required; fully restart after updating or first enabling it.

## Enemies and HP panels

The table groups living enemies by their **active encounter**, with current/max HP. Nearby enemies belonging to another encounter are listed separately.

World panels also cover living AI outside active encounter lists. They sit above enemies, show current/max HP and use a filled bar that changes from green through yellow to red as health falls. Walls can occlude the panels. Dormant objects without a valid live position are not drawn.

The gold GK countdown appears during the supported stagger/glory-kill window and follows the game's actual deadline. It follows paused game time rather than continuing as a desktop stopwatch. It disappears when that window ends or the enemy is no longer a valid live target.

## Equipment and powerups

| State | Meaning |
| --- | --- |
| Ready | Acquired, usable equipment has a charge available |
| Cooldown | A verified recharge/reuse countdown is running |
| Active | A verified duration, powerup or animation is running |
| Not acquired | The item is confirmed absent from the usable inventory |
| Inactive | The observed effect or animation is not active |
| Unavailable | A required source or supported interpretation is missing |

Frag grenade, ice bomb and flame belch use their current engine timers. Powerups include supported active quad damage, berserk and overdrive effects. When a verified total is available, the display shows **remaining / maximum**; an unknown maximum stays unspecified. Equipment totals may remain unknown until the game has scheduled a recharge.

The compact game HUD can omit inactive, unacquired or unavailable rows. Open the window for their detailed state and explanation.

## Sticky Bomb charges

Sticky Bomb has two recovery modes:

- With charges remaining, missing charges return **in order**. Each missing charge gets its countdown in restoration order.
- After an empty-magazine reload, all missing charges share the same full-reload deadline and return together.

Numbered entries show when each missing charge will return. Already-ready charges need no countdown.

Capacity and recovery time reflect the acquired mod and its upgrades.

## Sync animations, mantle and Player lock

Mantle has its own ledge-grab countdown.

Other supported sync rows show the **current animation clip**. Simultaneous animations can have separate rows.

**Game 1.1, supported Praetor Suit Point pickup:** the **Player lock** row counts down until player control returns. It ends even if the NPC continues its animation.

Player lock currently covers this pickup on 1.1. Other interactions and game versions retain current-clip timers. A current-clip countdown can differ from the time until control returns or the whole sequence finishes. Animations without a reliable countdown show Unavailable.

## Speedrun techniques

**BATS** shows accelerated weapon raising on supported weapons. Melee preserves the active indication while the buff remains. It clears when weapon raising returns to normal.

**Fast DBlade** activates on the prepared Arbalest hit, before selecting Destroyer Blade. It can remain active while another weapon is held, until the actual one-shot charge override is spent. An ordinary mastered Arbalest recharge or a completed ordinary mod switch is not enough to activate it.

These are game-state observations, not automatic performance of a technique. See [Compatibility](Compatibility) for supported builds and configuration limits.

## Arrange the HUD

The shared order is Volume inspector, Support plane, Active encounters, Living enemies, Timers and Techniques. Hidden blocks leave no fixed gap. Text uses the full viewport with wrapping and columns.

```text
com_debugHUD 1
labplus_debughud_scale 1.25
```

The scale range is 0.5–3; no argument queries it. Reduce scale or hide unused groups if the viewport becomes crowded. Menus/pause, the console and stale data can hide overlays.
