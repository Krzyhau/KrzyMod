#include "Utils/NetworkConnection.hpp"

#pragma once
class KrzyModClient : protected NetworkConnection {
private:
	bool host = false;
	int roomID = 0;
	bool joined = false;
	float hostedTimeBase = 0.0f;

public:
	KrzyModClient();
	void CreateRoom(std::string ip, int port);
	void JoinRoom(std::string ip, int port, int id);
	void HandlePackets();

	bool IsHost() { return host; }
	bool Joined() { return IsConnected() && joined; }
	bool JoinedAsHost() { return Joined() && IsHost(); }
	bool JoinedAsClient() { return Joined() && !IsHost(); }
	float GetHostedTimeBase() { return hostedTimeBase; }

	using NetworkConnection::Connect;
	using NetworkConnection::Disconnect;
	using NetworkConnection::IsConnected;
	using NetworkConnection::SendData;
	using NetworkConnection::ChangeAddress;
};
