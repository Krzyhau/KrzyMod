#include "KrzyModClient.hpp"

KrzyModClient::KrzyModClient(std::string ip, int port)
	: NetworkConnection(ip, port) {
	
}

void KrzyModClient::CreateRoom() {
}



void KrzyModClient::JoinRoom(int id) {
	Connect();
}

void KrzyModClient::HandlePackets() {
	TryProcessData([&](char *sockbuff, int messageLen) {
		
	});
}

void KrzyModClient::SendPacket(int id, char *data) {
}