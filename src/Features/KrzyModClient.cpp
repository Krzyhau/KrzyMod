#include "KrzyModClient.hpp"

#include "Modules/Console.hpp"
#include "KrzyMod.hpp"
#include "Utils.hpp"
#include "Variable.hpp"

KrzyModClient::KrzyModClient()
	: NetworkConnection("", 0) {
	
}

void KrzyModClient::CreateRoom(std::string ip, int port) {
	JoinRoom(ip, port, -1);
	host = true;
}


void KrzyModClient::JoinRoom(std::string ip, int port, int id) {
	joined = false;
	ChangeAddress(ip, port);

	bool status = Connect();
	if (!status) {
		console->Warning("Failed to connect to KrzyMod server.");
		Disconnect();
		return;
	}

	auto name = Variable("name");

	// connect, join given room
	SendData(Utils::ssprintf("1;%s;%d\n", name.GetString(), id));

	host = false;
}

void KrzyModClient::HandlePackets() {
	TryProcessData([&](char *sockbuff, int messageLen) {
		std::vector<std::string> messages = Utils::SplitString(std::string(sockbuff, messageLen), "\n");
		for (auto const &msg : messages) {
			if (msg.length() == 0) continue;
			std::vector<std::string> tokens = Utils::SplitString(msg, ";");
			int packetID = std::atoi(tokens[0].c_str());

			switch (packetID) {
			case 0: {  // disconnect
				Disconnect();
				std::string err = tokens.size() > 1 ? tokens[1] : "reason unknown";
				console->Warning("Disconnected from KrzyMod server - %s.\n", err);
				return;
			}
			case 1: {  // joined successfully
				if (tokens.size() < 2) {
					Disconnect();
					return;
				}
				roomID = std::atoi(tokens[1].c_str());
				joined = true;
				if (host) {
					console->Print("Successfully created room in KrzyMod server. Room ID: %d\n", roomID);
				} else {
					console->Print("Successfully joined room ID %d in KrzyMod server!\n", roomID);
				}
				break;
			}
			case 2: {  // new effect
				if (host) break;
				if (tokens.size() < 4) break;
				std::string effectName = tokens[1];
				float duration = std::atof(tokens[2].c_str());
				float time = std::atof(tokens[3].c_str());

				auto effect = krzyMod.GetEffectByName(effectName);

				if (effect != nullptr) {
					krzyMod.ActivateEffect(effect, duration, time);
					krzyMod.ResetTimer();
				}

				break;
			}
			case 3: {  // update timer
				if (host) break;
				if (tokens.size() < 3) break;
				float currTimer = std::atof(tokens[1].c_str());
				float timeBase = std::atof(tokens[2].c_str());
				hostedTimeBase = timeBase;
				krzyMod.UpdateTimer(timeBase, currTimer);
				break;
			}
			default: break;
			}
		}
	});
}