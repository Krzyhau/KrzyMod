#include "TwitchConnection.hpp"
#include <iostream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif


bool TwitchConnection::Connect()
{
	if (IsActive())Disconnect();

#ifdef WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

	if ((socketID = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return false;

	addrinfo hints, * res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	if ((getaddrinfo("irc.chat.twitch.tv", NULL, &hints, &res)) != 0) return false;
	sockaddr_in addr;
	addr.sin_addr.S_un = ((sockaddr_in*)res->ai_addr)->sin_addr.S_un;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(6667);
	freeaddrinfo(res);

	if (connect(socketID, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;

	std::string command = "NICK justinfan8\nJOIN #" + channel + "\n";
	send(socketID, command.c_str(), command.size(), 0);

	active = true;
	connThread = std::thread([&](){
		while (IsActive()) {
			memset(&sockbuff, '\0', sizeof(sockbuff));
			int length = recv(socketID, sockbuff, sizeof(sockbuff)-1, 0);
			if (length <= 0) break;
			
			char* startChar = sockbuff;
			while (reading);
			for (char* c = sockbuff; *c != '\0'; c++) {
				if (*c == '\n') {
					int len = (c - startChar);
					std::string message(startChar, len);

					if (message[0] == 'P') {
						std::string cmd = "PONG :tmi.twitch.tv\n";
						send(socketID, cmd.c_str(), cmd.size(), 0);
					}
					else if (message[0] == ':' && std::strstr(message.c_str(),"PRIVMSG")) {
						std::string nickname(startChar+1, message.find('!')-1);
						std::string msg(message.begin()+message.find(':', 1)+1, message.end());
						messageBuffer.push_back({ nickname, msg });
					}

					startChar += len + 1;
				}
			}
		}
		active = false;
	});

	return true;
}

bool TwitchConnection::IsActive()
{
	return active;
}

void TwitchConnection::Disconnect()
{
	closesocket(socketID);
	active = false;
	connThread.join();
#ifdef WIN32
	WSACleanup();
#endif
}

void TwitchConnection::SetChannel(std::string name)
{
	channel = name;
	if (IsActive()) {
		Disconnect();
		Connect();
	}
}

std::vector<TwitchConnection::Message> TwitchConnection::GetNewMessages()
{
	reading = true;
	std::vector<TwitchConnection::Message> messages;
	for (int i = 0; i < messageBuffer.size(); i++) {
		messages.push_back(messageBuffer[i]);
	}
	messageBuffer.clear();
	reading = false;
	return messages;
}
