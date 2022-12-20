#pragma once

#include <Framework/Client.h>
#include <Framework/MessageTypes.h>


class IRCClient : public IRC::IClient<IRCMessageType>
{
public:
	auto PingServer() -> void;
	auto MessageAll() -> void;

	virtual auto Run() -> void;

private:
	auto CheckIncoming() -> void;
};