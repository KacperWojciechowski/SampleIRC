#include "Server.h"

bool IRCServer::OnClientConnect(std::shared_ptr<IRC::Connection<MessageType>> client)
{
    return false;
}

void IRCServer::OnClientDisconnect(std::shared_ptr<IRC::Connection<MessageType>> client)
{
}

void IRCServer::OnMessage(std::shared_ptr<IRC::Connection<MessageType>> client, IRC::Message<MessageType>& msg)
{
}
