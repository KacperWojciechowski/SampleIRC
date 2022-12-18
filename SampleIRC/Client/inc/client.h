#pragma once

#include <cstdint>

#include <Framework/Client.h>

enum class MessageType : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	Message,
	Nickname
};

class IRCClient : public IRC::ClientInterface<MessageType>
{

};