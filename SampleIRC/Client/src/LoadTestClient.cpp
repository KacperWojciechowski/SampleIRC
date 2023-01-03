#include "LoadTestClient.h"

#include <iostream>

int IRCLoadClient::instanceCounter = 0;

auto IRCLoadClient::PingServer() -> void
{
	IRC::Message<IRCMessageType> msg;
	msg.header.id = IRCMessageType::ServerPing;

	std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

	msg << timeNow;
	Send(msg);
}

auto IRCLoadClient::MessageAll() -> void
{
	IRC::Message<IRCMessageType> msg;
	msg.header.id = IRCMessageType::MessageAll;
	Send(msg);
}

static auto ProcessPing(IRC::Message<IRCMessageType>& msg) -> void
{
	std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point timeThen;
	msg >> timeThen;
	std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() * 1000.0 << "ms \n";
}

auto IRCLoadClient::ProcessServerMessage(IRC::Message<IRCMessageType>& msg) -> void
{
	uint32_t clientID;
	msg >> clientID;
	if (clientID == this->clientID)
	{
		receivingTimepoint = std::chrono::system_clock::now();
		received = true;
		std::cout << "Received back from server\n";
	}
}

auto IRCLoadClient::CheckIncoming() -> void
{
	if (not Incoming().empty())
	{
		auto msg = Incoming().pop_front().msg;

		switch (msg.header.id)
		{
		case IRCMessageType::ServerAccept:
			std::cout << "Server Accepted Connection\n";
			msg >> clientID;
			break;
		case IRCMessageType::ServerDeny:
			std::cout << "Server denied connection\n";
			break;

		case IRCMessageType::ServerPing:
			ProcessPing(msg);
			break;

		case IRCMessageType::MessageAll:
			ProcessServerMessage(msg);
			break;

		case IRCMessageType::ServerMessage:
			ProcessServerMessage(msg);
			break;
		}
	}
}

auto IRCLoadClient::PrepareLogFile() -> bool
{
	logFile.open("performanceLogs.csv");
	return logFile.good();
}

auto IRCLoadClient::SendDummyMessage() -> void
{
	IRC::Message<IRCMessageType> msg;
	msg.header.id = IRCMessageType::MessageAll;

	msg << "Lorem ipsum dolor sit amet, consectetur adipiscing elit.Nullam nec arcu ac diam blandit aliquam eu.";
	messageCounter++;
	sendingTimepoint = std::chrono::system_clock::now();
	Send(msg);
}

auto IRCLoadClient::AppendLog() -> void
{
	std::string line = std::format("{},{},{}\n", instanceCounter,
											     messageCounter,
											     std::chrono::duration<double>(receivingTimepoint - sendingTimepoint).count() * 1000000.0);
	logFile << line;
}

auto IRCLoadClient::Run() -> void
{
	if (logFlag)
	{
		if (!PrepareLogFile())
		{
			std::cout << "Log file error\n";
			return;
		}
	}
	if (IsConnected())
	{
		SendDummyMessage();
	}

	while (!stopFlag)
	{
		if (IsConnected())
		{
			CheckIncoming();
			if (logFlag && received)
			{
				received = false;
				AppendLog();
				SendDummyMessage();
			}
		}
		else
		{
			std::cout << "Server Down\n";
			break;
		}
	}
}