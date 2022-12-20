#include "Server.h"

#include <ctime>
#include <format>

bool IRCServer::OnClientConnect(std::shared_ptr<IRC::Connection<IRCMessageType>> client)
{
	IRC::Message<IRCMessageType> msg;
	msg.header.id = IRCMessageType::ServerAccept;
	client->Send(msg);
	return true;
}

struct Timestamp
{
	int hour;
	int minutes;
};

auto GetTimestamp() -> Timestamp
{
	auto timer = std::time(nullptr);
	auto currTime = *localtime(&timer);

	return { .hour = currTime.tm_hour, .minutes = currTime.tm_min };
}

auto FormatTimestamp() -> std::string
{
	auto timestamp = GetTimestamp();
	return std::format("[%d:%d] ", timestamp.hour, timestamp.minutes);
}

void IRCServer::OnClientDisconnect(std::shared_ptr<IRC::Connection<IRCMessageType>> client)
{
	std::cout << FormatTimestamp() << "[Server] <" << client->GetID() << "> disconnected\n";
}

auto IRCServer::ProcessMessageAll(std::shared_ptr<IRC::Connection<IRCMessageType>> client, IRC::Message<IRCMessageType>& msg) -> void
{
	msg.header.id = IRCMessageType::ServerMessage;
	msg << client->GetID();
	MessageAllClients(msg, client);
}

void IRCServer::OnMessage(std::shared_ptr<IRC::Connection<IRCMessageType>> client, IRC::Message<IRCMessageType>& msg)
{
	switch (msg.header.id)
	{
	case IRCMessageType::ServerPing:
	
		std::cout << FormatTimestamp() << "<" << client->GetID() << ">: Server Ping\n";
		client->Send(msg);
	
		break;

	case IRCMessageType::MessageAll:
	
		std::cout << FormatTimestamp() << "[Server] <" << client->GetID() << ">: Message All\n";
		ProcessMessageAll(client, msg);
	
		break;
	}
}

auto IRCServer::Run() -> void
{
	while (1)
	{
		Update(-1, true);
	}
}
