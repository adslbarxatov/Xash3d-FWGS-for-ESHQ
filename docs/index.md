# Xash3D FWGS Engine adaptation
### for mods **ESHQ** and **ES: Randomaze**
> **ƒ** &nbsp;RD AAOW FDL; 2.08.2024; 21:30

---

- [General information](#general-information)
- [Main changes](#main-changes)
- [Other notes](#other-notes)

---

### General information

This engine adaptation was created especially for [ESHQ mod](https://moddb.com/mods/eshq)
and [ES: Randomaze mod](https://moddb.com/mods/esrm) for Half-Life part 1.

It is based on one of the newest versions of [Xash3d FWGS engine](https://github.com/FWGS/xash3d-fwgs) (`v49/0.20`).

&nbsp;



### Main changes:

1. We have fixed some known bugs (like non-rotating `func_rotating`; crashes on some events triggered by `scientists`
   and `bullsquids`, etc) for which we had enough mind and time.
2. We have added the `trigger_sound` entity that replaces the `env_sound`. It works as well as `trigger_multiple`
   (with hardcoded `wait` value – one second). We cannot understand why there were no brush entities to set sound effect.
   Spherical `env_sound` has very weird and unpredictable behavior, and it is difficult to apply it in cases like
   long narrow building entrances (f. e., partially opened gates). This situation really needs a brush entity with
   bidirectional processing.
3. We have returned the green blood to some monsters. This feature was implemented but wasn’t enabled in client library.
   Also it has been added to `env_blood` and `monster_generic` entities (with an ability to set it up).
4. Also we have returned human-like gibs and red blood to zombie. We think that zombie is more like a scientist than
   a bullsquid.
5. We have removed the `cycler_sprite` entity from our maps and added the `non-solid` flag to the `cycler` entity. Also
   our `cycler` now have the `material` field (crowbar sound depends on it) and two fields that defines collision
   box endpoints (like the `color` setup). `cycler` and `env_sprite` entities can also accept `body`, `skin` and
   `sequence` fields. Also `cycler` now can trigger its target. Finally, our `cycler` now can be breakable (with the
   specified explosion and health coefficients).
6. Our doors (`momentary`, `rotating` and `simple`) have different fields for “Just opened” and “Just closed” sounds.
   Also we’ve fixed some bugs (basically, around the `starts open` flag) and expanded the list of sounds. Finally,
   our doors will not play `locked` sounds when they’re opened.
7. Our `turrets`, `sentries` and `apaches` can trigger something on their deaths. Also player cannot stuck in them
   anymore.
8. `Breakables` can spawn crowbars (why it wasn’t so?), gauss gun clips, axes, headcrabs and some other useful things.
9. Our `ambient_generic` (and all entities that can sound) has more accurate sound radius.
10. We’ve added the `item_key` entity in addition to `item_security` to simplify doors triggering. It can look
    like a card or like a bunch of keys.
11. We’ve added the `game_player_set_health` entity that sets absolute value of health and armor. It is applicable
    when you need to create an effect of immediate but controllable damage.
12. Our `func_illusionary` triggers its textures when get call (as well as `func_wall` or `func_button`).
13. Our `grunts`, `Barneys`, `scientists` and `zombies` got “burned” state. Now it is possible to add burned corpses
    to the map. Also zombie got “dead” animation.
14. Our gman has `killable` flag and two skins.
15. Our `monster_rat` can run and can be smashed (as well as `monster_cockroach`, but with red blood, of course).
    Also rats can trigger anything when die.
16. Our `.357` and `crossbow` got correct reload sounds.
17. We’ve added `don’t reset view angle / speed` flag to the `trigger_teleport` entity. It’s useful in case of
    [map space expanding](http://moddb.com/mods/eshq/news/engine-specifications-for-teleports).
18. Our `weapon_fastswitch` mode is now really fast (as it is in HL2). You just need to press slot button again
    to select the next weapon.
19. Our `game_end` entity now works correctly (it ends the game), and `player_loadsaved` can “kill” player (so you
    don’t need the `trigger_hurt` or some other “freezing” method).
20. Wood, glass and snow textures got their own sounds for player steps.
21. We have added the `trigger_ramdom` entity that can randomly trigger targets from a specified list with
    specified probabilities.
22. Our `item_security` and `item_antidote` are collectable . Their counts can be used to trigger events on
    maps and activate extra abilities.
23. Our lasers can be turned off correctly. The older version of an engine turned off a sprite, but didn’t turn
    off the damage field.
24. Added support for an achievement script. Our modification generates script with extended player’s abilities
    according to count of collected `item_antidote` items.
25. Added support of `origin` brush for `breakables` and `pushables` when they drop items on break. Now item will
    be dropped at the center of the `origin` brush or at the center of entity if this brush is not presented.
26. Range of sounds for `pushables` has been expanded. Now they depend on materials. Sound script for `pushables`
    has been improved (better behavior fitting).
27. The `scripted_sentence` entity now can play single sound (it should be prefixed with `!!`). Also you can add
    text message from `titles.txt` in addition to the sound sentence.
28. Some speed improvements applied to `shotgun`, `mp5`, `hornetgun`, `.357` and other weapons.
29. Fixed mouse wheel’s behavior and some inconveniences in the keyboard settings interface.
30. Fixed the `gag` flag’s behavior: now all scientists with `gag` will be really silent.
31. “Credits” section added to the main menu. The map for it can be specified in the game configuration.
32. Added the `locked sequence` flag for all monsters. When set, engine will loop the sequence that is specified
    in monster’s settings before its first damage, death or before the call from the `scripted_sequence` entity.
    This feature works without additional `scripted_sequence` entities.
33. Behavior of sounds of `momentary` doors has been reviewed: sounds of moving and stop are working properly now.
34. Fixed bug with stucking weapons that can be dropped by dead human grunts.
35. `Use only` flag for doors will now work as a lock. Triggering these doors by their names will unlock them
    without opening. If no name specified, the door will be initially unlocked.
36. Added replacements for entities from *HL: blue shift* and *Afraid of monsters* mods.
37. Barnacles can shoot no gibs on death.
38. Fixed the too long stuck on automated jumping from the water.
39. Switching between walk and run now works as well as in HL2 (run requires `/` key holding by default).
    `Always run` flag has been added to `Advanced controls` menu (it disables this behavior).
40. Fixed the mapping for additive textures: now they are able to be transparent and semitransparent conveyors.
41. Breakables now have sounds that depend on their sizes (including the explosion).
42. HUD now can display extra abilities (superflashlight, invisibility for enemies, damageproof).
43. Pushables are now reacting on explosions, shooting and hitting by a crowbar (they move from the source of damage).
44. Implemented the “meat mode”: corpses can now be destroyed by bullets and crowbar in one hit. It requires
    the line `meat_mode "1"` in file `config.cfg`.
45. Our `trains` can have parts of their paths where they will not emit the sound of moving. The corresponding flag
    has been added to `path_corner` entity.
46. Our `trigger_gravity` now can set the coefficient for all objects on the map and save it for the next game loading.
    All values greater than `80` will be treated this way.
47. Our adaptation is now able to save the current `room type` (echo effect) for the next game loading.
48. Our `monstermakers` are now able to emit teleportation sounds and show specified sprites when activated.
49. Our brush entity `trigger_fog` can smoothly turn on / off the fog effect of the desired color and density.
50. Our sparks can emit small flashes of light onto nearby surfaces.

&nbsp;



## Other notes

- The current build is not multiplatform as its mother project (support code has been removed).
- The current build contains renderer based on [SDL library](https://libsdl.org).
- The current assembly doesn’t support the voice chat because of absence of required Opus codec.
- This assembly completely adapted for building with Visual Studio 2022.
- This assembly is enough to run Half-Life (WON) and some compatible mods.
- This assembly is a fork of original Xash3D FWGS engine with the same license
