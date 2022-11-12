#include "Features/KrzyMod.hpp"
#include "Modules/Client.hpp"
#include "Modules/Engine.hpp"
#include "Modules/Vscript.hpp"

CREATE_KRZYMOD(gameLaserEyes, "Laser Eyes", 2.5f, 0) {
	if (info.execType == INITIAL) {
		KRZYMOD_CONTROL_CVAR(sv_player_collide_with_laser, 0);
	}

	if (info.execType == OVERRIDE_CAMERA) {
		void *player = client->GetPlayer(1);
		if (!player) return;

		Vector pos = client->GetAbsOrigin(player) + client->GetViewOffset(player);
		QAngle angles = engine->GetAngles(0);

		Vector left = {-sinf(DEG2RAD(angles.y)), cosf(DEG2RAD(angles.y)), 0};

		Vector leftEye = pos + left * 8.0;
		Vector rightEye = pos - left * 8.0;

		char buff[256];
		snprintf(buff, sizeof(buff), "local pos = [Vector(%.03f,%.03f,%.03f), Vector(%.03f,%.03f,%.03f)];local ang=Vector(%.03f,%.03f,%.03f);", leftEye.x, leftEye.y, leftEye.z, rightEye.x, rightEye.y, rightEye.z, angles.x, angles.y, angles.z);
		std::string strPos = buff;

		std::string script = R"NUT(
			local name = "__krzymod_laser_eye_";
			local relay = null;
			if(!(relay = Entities.FindByName(null, name+"relay"))){
				for(local i = 0; i < 2; i++){
					local e = Entities.CreateByClassname("env_portal_laser");
					e.__KeyValueFromString("targetname", name+i);
					EntFireByHandle(e, "TurnOn", "", 0, null, null);
				}
				relay = Entities.CreateByClassname("logic_relay");
				relay.__KeyValueFromString("targetname", name+"relay");
				EntFireByHandle(relay, "AddOutput", "OnTrigger __krzymod_laser_eye_*,Kill,,0.2,-1", 0, null, null);
			}
			for(local i = 0; i < 2; i++){
				local e = Entities.FindByName(null, name+i);
				e.SetOrigin(pos[i]);
				e.SetAngles(ang.x,ang.y,ang.z);
			}
			EntFireByHandle(relay, "CancelPending", "", 0, null, null);
			EntFireByHandle(relay, "Trigger", "", 0, null, null);
		)NUT";

		script = strPos + script;
		vscript->RunScript(script.c_str());
	}
}
