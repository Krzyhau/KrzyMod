#include <string>
#include <vector>
#include <thread>

#pragma once
class TwitchConnection{
public:
	struct Message{
		std::string username;
		std::string message;
	};
private:
	std::string channel;
	int socketID;
	char sockbuff[4096];
	bool active = false;
	bool reading = false;
	std::thread connThread;
	std::vector<Message> messageBuffer;
public:
	bool Connect();
	bool IsActive();
	void Disconnect();
	void SetChannel(std::string name);
	std::string GetChannel() { return channel; }
	std::vector<Message> GetNewMessages();
};

