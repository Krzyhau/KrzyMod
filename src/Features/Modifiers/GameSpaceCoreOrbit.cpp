#include "Features/KrzyMod.hpp"

#include "Modules/Server.hpp"
#include "Modules/VScript.hpp"
#include "Modules/Engine.hpp"

CREATE_KRZYMOD(gameSpaceCoreOrbit, "Space Core Orbit", 1.6f, 0) {
	static bool precachedLastFrame = false;
	static int lastSound = 0;
	if (info.execType == ENGINE_TICK && info.preCall) {
		void *player = server->GetPlayer(1);
		if (!player) return;

		// the pain i have to get through to make these models function properly
		vscript->RunScript(R"NUT(
			local name = "__krzymod_space_core_";
			local relay = null;
			if(!(relay = Entities.FindByName(null, name+"relay"))){
				if(!(Entities.FindByClassname(null, "dynamic_prop"))){
					SendToConsole("prop_dynamic_create npcs/personality_sphere/personality_sphere_skins");
				}else{
					for(local i = 0; i < 20; i++){
						local e = Entities.CreateByClassname("prop_dynamic");
						e.SetModel("models/npcs/personality_sphere/personality_sphere_skins.mdl");
						e.__KeyValueFromString("targetname", name+i);
						e.__KeyValueFromString("DefaultAnim", "sphere_tuberide_long_twirl");
						e.__KeyValueFromString("skin", "2");
						EntFireByHandle(e, "TurnOn", "", 0, null, null);
						EntFireByHandle(e, "SetAnimation", "sphere_tuberide_long_twirl", i*0.5, null, null);
					}
					relay = Entities.CreateByClassname("logic_relay");
					relay.__KeyValueFromString("targetname", name+"relay");
					EntFireByHandle(relay, "AddOutput", "OnTrigger __krzymod_space_core_*,Kill,,0.2,-1", 0, null, null);
					SendToConsole("ent_fire dynamic_prop kill");
				}
			}else{
				for(local i = 0; i < 20; i++){
					local e = Entities.FindByName(null, name+i);
					local ppos = GetPlayer().GetOrigin();
					local xang = i*14.35 + Time();
					local yang = i*16.0 + Time();
					if(i%2==0)xang *= -1.0;
					e.SetOrigin(ppos + Vector(cos(xang)*69,sin(xang)*69,64+64*sin(yang)));
				}
				EntFireByHandle(relay, "CancelPending", "", 0, null, null);
				EntFireByHandle(relay, "Trigger", "", 0, null, null);
			}
		)NUT");


		// playing random space core sound every 0.25 second
		if (lastSound <= 0) {
			std::string sound = "playvol vo/core01/";
			if (Math::RandomNumber(0.0f, 1.0f) > 0.5f) {
				sound += Utils::ssprintf("space%02d.wav 0.5", Math::RandomNumber(1, 24));
			} else {
				sound += Utils::ssprintf("babbleb%02d.wav 0.5", Math::RandomNumber(1, 35));
			}
			engine->ExecuteCommand(sound.c_str());
			lastSound += 20;
		} else {
			lastSound--;
		}
	}
}
