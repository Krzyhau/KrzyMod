# KrzyMod Effects

This is a list of 70 modifiers currently implemented into KrzyMod.

|Identifier*|Name|Description|Time Multiplier*|
|---|---|---|---|
|moveWStuck|Help My W Is Stuck|Makes you move forward.|2.5|
|viewQuakeFov|Quake FOV|Changes your FOV to a high value.|3.5|
|viewUpsideDown|Upside Down View|Rotates your viewport 180 degrees.|2.5|
|metaFasterDelay|x4 KrzyMod Speed|Forces effects to be applied 4 times faster.|1.0|
|metaPause|Pause KrzyMod|Stops effects from being applied.|1.0|
|visualSnapchatMode|Snapchat Mode|Displays normie-friendly interface on screen.|2.5|
|playerKill|kill.|kills.|0.0|
|moveStickyGround|Sticky Ground|Makes you unable to move when standing.|3.5|
|moveInverseControl|Inverse Controls|Inverses your controls (moving forward moves you back etc).|3.5|
|visualDvdLogo|DVD Logo|Displays bouncing DVD logo on screen.|4.5|
|visualHideCrosshair|Hide Crosshair|Hides your crosshair.|3.5|
|playerLaunchRandom|Yeet Player|Launches player towards random direction.|0.0|
|playerLaunchUp|Polish Space Program|Launches player upwards with maximum velocity.|0.0|
|playerUTurn|U-Turn|Reverses velocity and view angles.|0.0|
|moveAirlock|No Air Control|Makes you unable to move midair.|3.5|
|gameSmallTimescale|Timescale 0.2|Makes the game run 5 times slower.|1.5|
|gameLargeTimescale|Timescale x2|Makes the game run 2 times faster.|2.5|
|gameSmallPhysscale|Slowy Props|Slows down physics objects.|3.5|
|gameLargePhysscale|Speedy Props|Speeds up physics objects.|3.5|
|gameMaxBounciness|Maximum Repulsiveness|Makes repulsion gel VERY repulsive.|3.5|
|moveAutojump|Jump Script|Forces jumping, so you jump as soon as you touch the ground.|3.5|
|gameRemovePaint|Remove All Paint|Cleans all surfaces from paint in current map.|0.0|
|gameChangePortalgunLinkage|These aren't my portals!|Switches portal linkage in your current portal gun.|0.0|
|moveDrunk|Drunk|Affects your movement and view so it looks like Chell had a night out.|3.5|
|moveMarioJump|Mario Jump|Makes your jump 3 times larger.|3.5|
|moveStanleyParable|The Stanley Parable|Blocks jumping.|3.5|
|visualRainbowwPropss|RainbowwPropss|Changes color for all objects.|4.5|
|visualBlackout|Blackout|Enables deep, dark fog.|2.5|
|visualRtxOn|RTX On|Enables a couple of graphics effects so the game look omegarealistic.|2.5|
|visualPS2Graphics|PS2 Graphics|Enables a couple of graphics effects so the game look like PS2 game.|2.5|
|visualGrid|The Grid|Draws lightmap grid.|2.5|
|visualHideStatic|Hide Static World|Hides all static objects and world geometry.|2.5|
|visualHideDynamic|Hide Dynamic World|Hides all dynamic objects.|2.5|
|visualPaintItWhite|Paint It, White|Removes textures, making everything white.|2.5|
|visualOrtho|Orthographic View|Changes your camera mode to ortographic.|1.5|
|gamePortalMachineGun|Portal MacHINE GUN!|Sets portal gun fire delay to 0.|2.5|
|gamePressButtons|Press All Map Buttons|Presses all buttons in current map.|0.0|
|gameReleaseButtons|Release All Map Buttons|Releases all (floor) buttons in current map.|0.0|
|gameNoFriction|Caution! Wet Floor!|Decreases ground friction.|3.5|
|gameNegativeFriction|Weeeeeeeeee!!!|Sets ground friction to negative value.|3.5|
|gameMoonGravity|Moon Gravity|Makes gravity 3 times smaller.|2.5|
|viewBarrelRoll|Do A Barrel Roll!|Spins your camera. A lot.|1.5|
|gameRestartLevel|restart_level|Restarts current level.|0.0|
|gameLoadQuick|Load Quicksave|Loads quicksave file.|0.0|
|gameLoadAutosave|Load Autosave|Loads autosave file.|0.0|
|visualDeepFried|Deep Fried|Increases ambient light to a painfully large value.|1.5|
|moveFallDamage|Short Fall Boots|Enables fall damage.|3.5|
|visualClaustrophobia|Claustrophobia|Changes aspect ratio to uncomfortably large value.|2.5|
|moveSuperhot|SUPER HOT|Makes it so timescale is related to your movement. Time moves when you move.|3.5|
|moveAlwaysDuck|I'm A Duck!|Forces you to duck.|2.5|
|gameOpenSesame|Open Sesame!|Opens all doors in current map.|0.0|
|gameCloseSesame|Close Sesame!|Closes all doors in current map.|0.0|
|gameOnlyWallsPortalable|Portals Only On Walls|Blocks placing portals on surfaces that are not straight walls.|2.5|
|gameReversePortalSurfaces|Reverse Surface Portalability|Switches portalability flag for every surface.|2.5|
|gameLeastPortals|Least Portals|Block portals from being placed.|1.5|
|viewZoomFov|Magnifying Glass|Sets FOV to a small value.|2.5|
|viewGTA2|GTA II|Adjusts your camera for a top-down perspective, giving you the GTA II-like view.|3.5|
|gameLaserEyes|Laser Eyes|Emits lasers from your eyes and makes you invulnerable to them.|2.5|
|visualLoudNoise|Loud Music (!!!)|Plays loud music.|0.0|
|gameSpawnGelWater|Spawn Water Blob|Spawns water blob.|0.0|
|gameSpawnGelJump|Spawn Repulsion Blob|Spawns repulsion gel blob.|0.0|
|gameSpawnGelSpeed|Spawn Propulsion Blob|Spawns propulsion gel blob.|0.0|
|gameSpawnGelPortal|Spawn Conversion Blob|Spawns conversion gel blob.|0.0|
|gameSpawnTurret|Spawn Turret|Spawns a turret.|0.0|
|gameSpawnCompanionCube|Spawn Companion Cube|Spawns a companion cube.|0.0|
|gameGivePercent|Give%%%%|Spawns 30 cubes using `give` command.|0.0|
|moveDelayInput|Delayed Inputs|Delays your key and mouse inputs.|1.5|
|gameSpaceCoreOrbit|Space Core Orbit|Spawns a lot of space cores to orbit around you and annoy you.|1.6|
|gameIgniteAll|Ignite Everything|Puts everything on fire.|0.0|
|gameButterfingers|Butterfingers!|Makes your hands slippery - you'll randomly drop props.|2.5|

***Identifier** is a name of an effect that you give when using `krzymod_activate` or `krzymod_deactivate` commands.

***Time multiplier** is a number used to calculate a timespan of effect being active by multiplying it by `krzymod_time_base` value. A multiplier of 0 means the effect is instant and doesn't have continouos behaviour.