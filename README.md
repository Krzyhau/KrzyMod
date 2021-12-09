[![CI](https://github.com/p2sr/SourceAutoRecord/workflows/CI/badge.svg)](https://github.com/Krzyhau/KrzyMod/actions?query=workflow%3ACI+branch%3Amaster)
[![CD](https://github.com/p2sr/SourceAutoRecord/workflows/CD/badge.svg)](https://github.com/Krzyhau/KrzyMod/actions?query=workflow%3ACD+branch%3Amaster)

# KrzyMod - Chaos Mod for Portal 2
**KrzyMod** is a plugin for Portal 2 that allows you to play the game while a bunch of random effects try to stop you from succeeding in that goal.

Every once in a while, the plugin will trigger a random effect, which can be anything from spawning a cube to giving you laser eyes or making all props randomly colored.

This allows for a playthrough/speedrun full of surprises and it's up to you to figure out a way to use them/deal with them.

So far, **70** different effects have been implemented and the list will probably grow with time.
You can find a list of all effects available in the mod [here](doc/list.md).

The mod also includes *Twitch support*, allowing your viewers to vote for one of four random effects.

## Installation
To use the plugin, install it by downloading the newest release from [releases tab](https://github.com/Krzyhau/KrzyMod/). Then, copy a `.dll` library (or `.so` library if you're on Linux) into the main Portal 2 directory where the game's executable is located (usually it's `Steam/steamapps/common/Portal 2`). Once a library is in there, you can open up the game and type `plugin_load krzymod` in a console.

It should be noted that SourceAutoRecord doesn't cooperate well with plugins being loaded after SAR has been loaded, therefore make sure to load KrzyMod **before** SAR is loaded.

This plugin is known to be compatible with the newest release of Portal 2 on Windows and Linux. Although Portal 2 modifications should also work with it, they are not directly supported (aka. if something breaks in there, i dont care lmao).

## Commands
These commands and console variables will allow you to control the behaviour of KrzyMod.
|Name|Description|
|---|---|
|`krzymod_enabled <enabled>`|Enables or disables KrzyMod based on given value (disabled by default).|
|`krzymod_time_base <time>`|Sets how frequently effects will be aplied, in seconds (30 by default). It's also used to determine how long effects themselves are.|
|`krzymod_vote_enabled <enabled>`|Enables or disables Twitch Chat voting (enabled (1) by default). Set it to 2 in order to have votes enabled but voting window hidden.|
|`krzymod_vote_channel <twitch channel name>`|Sets twitch channel (lowercase) from which KrzyMod should fetch messages.|
|`krzymod_vote_proportional <enabled>`|Set it to 1 if you want voting to be proportional - effect with X percent of votes will have X percent of chances to be executed (enabled by default).|
|`krzymod_vote_double_numbering <enabled>`|Set it to 1 to enable double numbering - vote options will use numbers from 4 to 8 on every second voting (disabled by default).|
|`krzymod_primary_font <id>`|Changes the ID of the primary font used by KrzyMod (92 by default).|
|`krzymod_secondary_font <id>`|Changes the ID of the secondary font used by KrzyMod (97 by default).|

These commands and cvars are for debug purposes:
|Name|Description|
|---|---|
|`krzymod_list`|Lists all available effects in KrzyMod.|
|`krzymod_activate <name>`|Activates KrzyMod with given name.|
|`krzymod_deactivate <name>`|Deactivates active KrzyMod with given name.|
|`krzymod_vote <number>`|Manually votes for an effect with given number.|
|`krzymod_debug <enabled>`|Enables console debugging for KrzyMod.|

## Credits
This plugin is based on [SourceAutoRecord](https://github.com/p2sr/SourceAutoRecord/).