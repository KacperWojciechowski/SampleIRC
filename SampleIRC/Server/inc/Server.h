#pragma once

#include <cstdint>
#include "Framework/Server.h"

enum class MessageType : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	Message,
	Nickname
};

class IRCServer : public IRC::ServerInterface<MessageType>
{
public:
	IRCServer(uint16_t port) : IRC::ServerInterface<MessageType>(port) {}

protected:
	virtual bool OnClientConnect(std::shared_ptr<IRC::Connection<MessageType>> client) override;

	virtual void OnClientDisconnect(std::shared_ptr<IRC::Connection<MessageType>> client) override;

	virtual void OnMessage(std::shared_ptr<IRC::Connection<MessageType>> client, IRC::Message<MessageType>& msg) override;
};
