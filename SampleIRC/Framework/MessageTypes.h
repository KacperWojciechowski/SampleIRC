#pragma once

#include <cstdint>

enum class IRCMessageType : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
};