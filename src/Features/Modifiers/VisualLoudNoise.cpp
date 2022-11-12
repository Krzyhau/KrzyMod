#include "Features/KrzyMod.hpp"

#include "Modules/Engine.hpp"

CREATE_KRZYMOD_INSTANT(visualLoudNoise, "Loud Music (!!!)", 0) {
	engine->ExecuteCommand("snd_musicvolume 1");
	engine->ExecuteCommand("playvol music/sp_a2_bts2_x1.wav 1");
}
