#pragma once

#include "Common.h"
#include "Message.h"
#include "ThreadSafeQueue.h"
#include "Connection.h"

#include <thread>
#include <memory>
#include <limits>

namespace IRC
{
	template<typename T>
	class ClientInterface
	{
	public:
		ClientInterface() {}

		virtual ~ClientInterface()
		{
			Disconnect();
		}

		auto Connect(const std::string& host, const uint16_t port) -> bool
		{
			try
			{
				boost::asio::ip::tcp::resolver resolver(context);
				auto endpoint = resolver.resolve(host, std::to_string(port));

				connection = std::make_unique<Connection<T>>(Connection<T>::Sender::Client,
					context, boost::asio::ip::tcp::socket(context), inQueue);

				threadContext = std::thread([this]() {context.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "Client exception: " << e.what() << "\n";
				return false;
			}
			return true;
		}

		auto Disconnect() -> void
		{
			if (IsConnected())
			{
				connection->disconnect();
			}
			context.stop();

			if (threadContext.joinable())
			{
				threadContext.join();
			}
			connection.release();
		}

		auto IsConnected() -> bool
		{
			if (connection)
			{
				return connection->is_connected();
			}
			else
			{
				return false;
			}
		}

		auto Update(std::size_t messagesToProcess = std::numeric_limits<std::size_t>::max()) -> void
		{
			std::size_t processedCount = 0;

			while (processedCount < messagesToProcess && not inQueue.empty())
			{
				auto msg = inQueue.pop_front();
				OnMessage(msg.sender, msg.msg);
				processedCount++;
			}
		}

		void Send(const Message<T>& msg)
		{
			if (IsConnected())
			{
				connection->send(msg);
			}
		}

	protected:
		auto OnMessage(std::shared_ptr<Connection<T>>, IRC::Message<T>& msg) -> void {}

		boost::asio::io_context context;
		std::thread threadContext;

		std::unique_ptr<Connection<T>> connection;

	private:
		ThreadSafeQueue<IdentifyingMessage<T>> inQueue;
	};
}
