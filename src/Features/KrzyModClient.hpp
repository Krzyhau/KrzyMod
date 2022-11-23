#include "Utils/NetworkConnection.hpp"

#pragma once
class KrzyModClient : protected NetworkConnection {
private:
	bool host = false;
	int roomID = 0;

public:
	KrzyModClient(std::string ip, int port);
	void CreateRoom();
	void JoinRoom(int id);
	void HandlePackets();
	void SendPacket(int id, char* data);

	using NetworkConnection::Connect;
	using NetworkConnection::Disconnect;
	using NetworkConnection::IsConnected;
	using NetworkConnection::SendData;
	using NetworkConnection::ChangeAddress;
};
