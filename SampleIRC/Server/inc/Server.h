#include <iostream>
#include <Framework/Server.h>
#include <Framework/MessageTypes.h>

class IRCServer : public IRC::IServer<IRCMessageType>
{
public:
	IRCServer(uint16_t nPort) : IRC::IServer<IRCMessageType>(nPort) { }
	auto Run() -> void;

protected:
	virtual bool OnClientConnect(std::shared_ptr<IRC::Connection<IRCMessageType>> client) override;
	virtual void OnClientDisconnect(std::shared_ptr<IRC::Connection<IRCMessageType>> client) override;
	virtual void OnMessage(std::shared_ptr<IRC::Connection<IRCMessageType>> client, IRC::Message<IRCMessageType>& msg) override;
	
	auto ProcessMessageAll(std::shared_ptr<IRC::Connection<IRCMessageType>> client, IRC::Message<IRCMessageType>& msg) -> void;
	
};
