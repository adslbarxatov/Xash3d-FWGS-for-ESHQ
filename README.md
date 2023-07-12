# Xash3D FWGS Engine adaptation
### for mods **ESHQ** v 13.5 (and newer) and **ES: Randomaze** v 4.13 (and newer)

#

This engine adaptation was created especially for [ESHQ mod](https://moddb.com/mods/eshq) for Half-Life part 1.

It was based on one of the newest versions of [Xash3d FWGS engine](https://github.com/FWGS/xash3d-fwgs) (`v49/0.20`)

&nbsp;



## Main changes:

1. We have fixed some known bugs (like non-rotating `func_rotating`; crashes at killed scientists sentences, etc) for which we had enough mind and time.

2. We have added the `trigger_sound` entity that replaces the `env_sound`. It works as well as `trigger_multiple` (with hardcoded “wait” value – 1 s). We cannot understand why there were no brush entities to set sound effect. Spherical `env_sound` has very weird and unpredictable behavior, and it is difficult to apply it in cases like long narrow building entrances (f. e., partially opened gates). This situation really needs two brushes – before and after the gate – to trigger sound effects.

3. Our `trigger_sound` now can be bidirectional: it can set room types behind and after it.

4. We have returned green blood to some monsters. This feature was implemented but wasn’t enabled in client library. Also it has been added to `env_blood` and `monster_generic` entities (with an ability to set it up).

5. Also we have returned human-like gibs and red blood to zombie. We think that zombie is more like a scientist than a bullsquid.

6. We have removed the `cycler_sprite` entity from our maps and added “Non-solid” flag to the `cycler` entity. Also our `cycler` now have the “Material” field (crowbar sound depends on it) and two fields that defines collision box endpoints (like the “Color” setup). `cycler` and `env_sprite` entities can also accept “body”, “skin” and “sequence” fields. Also `cycler` now can trigger its target.

7. Our doors (momentary, rotating and simple) have different fields for “Just opened” and “Just closed” sounds. We’re planning to split “Opening” sound to “Opening” and “Closing”. But it’s not necessary for now. Also we’ve fixed some bugs (basically, around the “Starts open” flag) and expanded the list of sounds (not only replaced exist ones). Finally, our doors will not play “locked” sounds when they’re opened.

8. Our turrets, sentries and apaches can trigger something on their deaths.

9. Breakables can spawn crowbars (why it wasn’t so?) and gauss gun clips.

10. Our `ambient_generic` (and all entities that can sound) has more accurate sound radius (minimal as default).

11. We’ve added the `item_key` entity in addition to `item_security` to simplify doors triggering. It can look like a card or like a bunch of keys. 

12. We’ve added the `game_player_set_health` entity that sets absolute value of health and armor. It is applicable when you need to create an effect of immediate but controllable damage.

13. Our `func_illusionary` triggers its textures when get call (as well as `func_wall` or `func_button`).

14. Our grunts, Barneys, scientists and zombies got “burned” state. Now it is possible to add burned corpses to the map. Also zombie got “dead” animation.

15. Our gman has “Killable” flag and two skins.

16. Our `monster_rat` can run and can be smashed (as well as `monster_cockroach`, but with red blood, of course). And trigger anything by this event.

17. Our .357 and crossbow got correct reload sounds.

18. We really want to create fog entity. We think that `func_water` without oxygen loss and swimming-like movement (and some other sound effect) can be used for it.

19. We’ve added “Don’t reset view angle / speed” flag to the `trigger_teleport` entity. It’s useful in case of [map space expanding](http://moddb.com/mods/eshq/news/engine-specifications-for-teleports).

20. Our `weapon_fastswitch` mode is now really fast (as it is in HL2). You just need to press slot button again to select the next weapon.

21. Our `game_end` entity now works correctly (ends the game), and `player_loadsaved` can “kill” player (so you don’t need the `trigger_hurt` or some other “freezing” method).

22. Wood, glass and snow textures got their own sounds for player steps.

23. We have added the `trigger_ramdom` entity that can randomly trigger targets from a specified list with specified probabilities. No more lasers needed!

24. Our `item_security` and `item_antidote` are collectable now. Their counts can be used to trigger events on maps and activate extra abilities.

25. Our lasers can be turned off correctly. The older version of an engine turned off a sprite, but didn’t turn off the damage field.

26. Added support for an achievement script. Our modification generates script with extended player’s abilities according to count of collected `item_antidote` items.

27. Added support of “origin” brush for breakables and pushables when they drop items on break. Now item will be dropped at the center of the “origin” brush or at the center of entity if the brush is not presented.

28. Range of sounds for pushables has been expanded and now they depend of materials. Sound script for pushables has been improved (better behavior fitting).

29. The `scripted_sentence` entity now can play single sound (it must be prefixed with `!!`). Also you can add text message from `titles.txt` in addition to the sound sentence.

30. Some speed improvements applied to shotgun, mp5, hornetgun, 357 and others.

31. Fixed mouse wheel’s behavior and some inconveniences in the keyboard settings interface.

32. Fixed “gag” flag’s behavior: now all scientists with “gag” will be silent.

33. “Credits” section added to main menu; map for it can be specified in the game configuration.

34. Added “Locked sequence” flag for all monsters. When set, engine will loop the sequence that is specified in monster’s settings before its first damage, death or before the call from the `scripted_sequence` entity. This feature works without additional `scripted_sequence` entities.

35. Behavior of sounds of momentary doors reviewed: sounds of moving and stop are working properly now.

36. Fixed bug with stuck weapons that can be dropped by dead human grunts.

37. “Use only” flag for doors will now work as a lock. Triggering these doors by their names will unlock them without opening. If no name specified, the door will be initially unlocked.

38. Added replacements for entities from *HL: blue shift* and *Afraid of monsters*.

39. Barnacles can shoot nothing on death.

40. Fixed too long time stuck on jumping from the water.

41. Switching between walk and run now works as well as in HL2 (run requires `/` key holding by default). “Always run” flag has been added to Advanced controls menu (disables this behavior).

42. Fixed mapping for additive textures: now they are able to be transparent and semitransparent conveyors.

43. Breakables now have sounds that depend on their sizes.

44. HUD now can display extra abilities (superflashlight, invisibility for enemies, damageproof).

45. Pushables are now reacting on explosions, shooting and hitting by a crowbar.

46. Implemented the “meat mode”: corpses can now be destroyed by bullets and crowbar in one hit. Requires the line `meat_mode "1"` in file `config.cfg`.

47. Our trains can have parts of their paths where they will not emit the sound of moving.

48. Our `trigger_gravity` now can set the coefficient for all objects on the map and save it for the next game loading. All values greater than `80` will be treated this way.

49. Our adaptation is now able to save the current room type (echo effect) for the next game loading.

50. Our monstermakers are now able to emit teleportation sounds and show specified sprites when activated.

&nbsp;



## Other notes

- The current build is not multiplatform as its mother project (support code has been removed).
- The current build contains GL-based renderer based on [SDL](https://libsdl.org) library.
- The current assembly doesn’t support the voice chat because of absence of required Opus codec. It may be attached later.
- This assembly completely adapted for building with Visual Studio 2022.
- This assembly is enough to launch Half-Life (WON) and some compatible mods.
- This assembly is a fork of original Xash3D FWGS engine with the same license.
- This assembly comes with the same third-party software that is described on the engine page

&nbsp;



## [Development policy and EULA](https://adslbarxatov.github.io/ADP)

This Policy (ADP), its positions, conclusion, EULA and application methods
describes general rules that we follow in all of our development processes, released applications and implemented ideas.
***It must be acquainted by participants and users before using any of laboratory’s products.
By downloading them, you agree and accept this Policy!***
