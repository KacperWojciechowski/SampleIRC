#include "Client.h"
#include <iostream>

auto IRCClient::PingServer() -> void
{
	IRC::Message<IRCMessageType> msg;
	msg.header.id = IRCMessageType::ServerPing;

	std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

	msg << timeNow;
	Send(msg);
}

auto IRCClient::MessageAll() -> void
{
	IRC::Message<IRCMessageType> msg;
	msg.header.id = IRCMessageType::MessageAll;
	Send(msg);
}

auto ProcessPing(IRC::Message<IRCMessageType>& msg) -> void
{
	std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point timeThen;
	msg >> timeThen;
	std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() * 1000.0 << "ms \n";
}

auto ProcessServerMessage(IRC::Message<IRCMessageType>& msg) -> void
{
	uint32_t clientID;
	msg >> clientID;
	std::cout << "Hello from [" << clientID << "]\n";
}

auto IRCClient::CheckIncoming() -> void
{
	if (not Incoming().empty())
	{
		auto msg = Incoming().pop_front().msg;

		switch (msg.header.id)
		{
		case IRCMessageType::ServerAccept:
			std::cout << "Server Accepted Connection\n";
			break;


		case IRCMessageType::ServerPing:
			ProcessPing(msg);
			break;

		case IRCMessageType::ServerMessage:
			ProcessServerMessage(msg);
			break;
		}
	}
}

auto IRCClient::Run() -> void
{

	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	bool bQuit = false;
	while (!bQuit)
	{
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		}

		if (key[0] && !old_key[0]) PingServer();
		if (key[1] && !old_key[1]) MessageAll();
		if (key[2] && !old_key[2]) bQuit = true;

		for (int i = 0; i < 3; i++) old_key[i] = key[i];

		if (IsConnected())
		{
			CheckIncoming();
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}

	}
}
