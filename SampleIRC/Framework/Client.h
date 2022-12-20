#pragma once
#include "Common.h"
#include "Message.h"
#include "ThreadSafeQueue.h"
#include "Connection.h"

namespace IRC
{
	template <typename T>
	class IClient
	{
	public:
		IClient()
		{}

		virtual ~IClient()
		{
			Disconnect();
		}

		auto ResolveHost(const std::string& host, const uint16_t port)
		{
			boost::asio::ip::tcp::resolver resolver(asioContext);
			return resolver.resolve(host, std::to_string(port));
		}

		bool Connect(const std::string& host, const uint16_t port)
		{
			try
			{
				auto endpoints = ResolveHost(host, port);

				connection = std::make_unique<IRC::Connection<T>>(IRC::Connection<T>::Owner::client, 
																  asioContext, 
																  boost::asio::ip::tcp::socket(asioContext), 
																  inQueue);

				connection->ConnectToServer(endpoints);

				contextThread = std::thread([this]() { asioContext.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "Client Exception: " << e.what() << "\n";
				return false;
			}
			return true;
		}

		void Disconnect()
		{
			if (IsConnected())
			{
				connection->Disconnect();
			}

			asioContext.stop();
			if (contextThread.joinable())
				contextThread.join();

			connection.release();
		}

		bool IsConnected()
		{
			if (connection)
				return connection->IsConnected();
			else
				return false;
		}

		void Send(const IRC::Message<T>& msg)
		{
			if (IsConnected())
				connection->Send(msg);
		}

		IRC::ThreadSafeQueue<IRC::IdentifyingMessage<T>>& Incoming()
		{
			return inQueue;
		}

	protected:
		boost::asio::io_context asioContext;
		std::thread contextThread;
		std::unique_ptr<IRC::Connection<T>> connection;

	private:
		IRC::ThreadSafeQueue<IRC::IdentifyingMessage<T>> inQueue;
	};
}
