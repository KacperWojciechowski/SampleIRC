#pragma once

#include <Framework/Client.h>
#include <Framework/MessageTypes.h>

#include <chrono>
#include <fstream>

class IRCLoadClient : public IRC::IClient<IRCMessageType>
{
	public:
		IRCLoadClient(bool& stopFlag, bool logFlag)
			: stopFlag(stopFlag),
			logFlag(logFlag),
			clientID(0)
		{
			instanceCounter++;
		}

		IRCLoadClient(const IRCLoadClient& other) = default;
		IRCLoadClient(IRCLoadClient&& other) = default;

		auto PingServer() -> void;
		auto MessageAll() -> void;

		virtual auto Run() -> void;

		~IRCLoadClient()
		{
			if (logFlag)
			{
				logFile.close();
			}
		}

	private:
		auto CheckIncoming() -> void;
		auto PrepareLogFile() -> bool;
		auto SendDummyMessage() -> void;
		auto AppendLog() -> void;
		auto ProcessServerMessage(IRC::Message<IRCMessageType>& msg) -> void;

		bool& stopFlag;
		bool logFlag;
		bool received = false;
		uint32_t clientID;

		std::ofstream logFile;

		std::chrono::system_clock::time_point sendingTimepoint = {};
		std::chrono::system_clock::time_point receivingTimepoint = {};

		static int instanceCounter;
		int messageCounter = 0;
};
