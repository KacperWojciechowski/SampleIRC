#pragma once

#include <iostream>
#include <limits>
#include <cstdint>
#include <thread>

#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Connection.h"
#include "Message.h"

namespace IRC
{
	template<typename T>
	class ServerInterface
	{
	public:
		ServerInterface(uint16_t port)
			: asioAcceptor(asioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
		{}

		virtual ~ServerInterface()
		{
			Stop();
		}

		auto Start() -> bool
		{
			try
			{
				WaitForClientConnection();
				threadContext = std::thread([this]() {asioContext.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "[Server] Exception: " << e.what() << "\n";
				return false;
			}

			std::cout << "[Server] Started!\n";
			return true;
		}

		auto Stop()
		{
			asioContext.stop();

			if (threadContext.joinable())
			{
				threadContext.join();
			}
			std::cout << "[Server] Stopped!\n";
		}

		auto acceptSocket(boost::asio::ip::tcp::socket& socket) -> std::shared_ptr<Connection<T>>
		{
			std::cout << "[Server] New Connection: " << socket.remote_endpoint() << "\n";
			std::shared_ptr<Connection<T>> con =
				std::make_shared<Connection<T>>(Connection<T>::Sender::Client,
					asioContext, std::move(socket), inMessagesQueue);
			return con;
		}

		auto acceptConnection(std::shared_ptr<Connection<T>> connection) -> void
		{
			if (OnClientConnect(connection))
			{
				connections.push_back(std::move(connection));
				connections.back()->connect_to_client(IDCounter++);
				std::cout << "[" << connections.back()->get_user_ID() << "] Approved\n";
			}
			else
			{
				std::cout << "[" << connections.back()->get_user_ID() << "] Denied\n";
			}
		}

		auto WaitForClientConnection() -> void
		{
			asioAcceptor.async_accept(
				[this](std::error_code ec, boost::asio::ip::tcp::socket socket)
				{
					if (not ec)
					{
						auto connection = std::move(acceptSocket(socket));
						acceptConnection(connection);
					}
					else
					{
						std::cout << "[Server] Error accepting connection: " << ec.message() << "\n";
					}

					WaitForClientConnection();
				}
			);
		}

		auto Message(std::shared_ptr<Connection<T>> client, const Message<T>& msg)
		{
			if (!attemptSending(client, msg))
				connections.erase(
					std::remove(connections.begin(), connections.end(), client), connections.end()
				);
		}

		auto processDisconnetion(std::shared_ptr<Connection<T>>& client)
		{
			OnClientDisconnect(client);
			client.reset();
		}

		auto attemptSending(std::shared_ptr<Connection<T>>& client, const IRC::Message<T>& msg)
		{
			if (client && client->is_connected())
			{
				client->send(msg);
				return true;
			}
			else
			{
				processDisconnection(client);
				return false;
			}
		}

		auto MessageAll(const IRC::Message<T>& msg, std::shared_ptr<Connection<T>> ignoredClient = nullptr)
		{
			bool deadClientPresent = false;

			for (auto& client : connections)
			{
				if (client != ignoredClient && not attemptSending(client, msg))
					deadClientPresent = true;
			}

			if (deadClientPresent)
			{
				connections.erase(
					std::remove(connections.begin(), connections.end(), nullptr), connections.end()
				);
			
			}
		}

		void Update(std::size_t messagesToProcess = std::numeric_limits<std::size_t>::max())
		{
			std::size_t processedCount = 0;

			while (processedCount < messagesToProcess && not inMessagesQueue.empty())
			{
				auto msg = inMessagesQueue.pop_front();
				OnMessage(msg.sender, msg.msg);
				processedCount++;
			}
		}

	protected:
		virtual auto OnClientConnect(std::shared_ptr<Connection<T>> client) -> bool { return false; }
		virtual auto OnClientDisconnect(std::shared_ptr<Connection<T>> client) -> void {}
		virtual auto OnMessage(std::shared_ptr<Connection<T>> sender, IRC::Message<T>& msg) -> void {}

	private:
		ThreadSafeQueue<IdentifyingMessage<T>> inMessagesQueue;

		std::deque<std::shared_ptr<Connection<T>>> connections;

		std::thread threadContext;

		boost::asio::io_context asioContext;
		boost::asio::ip::tcp::acceptor asioAcceptor;

		uint32_t IDCounter = 10000;
	};
}
