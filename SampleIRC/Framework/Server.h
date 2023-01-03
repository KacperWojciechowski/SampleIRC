#pragma once

#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Message.h"
#include "Connection.h"

namespace IRC
{
	template<typename T>
	class IServer
	{
	public:
		IServer(uint16_t port)
			: asioAcceptor(asioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
		{ }

		virtual ~IServer()
		{
			Stop();
		}

		bool Start()
		{
			try
			{
				WaitForClientConnection();

				contextThread = std::thread([this]() { asioContext.run(); });
			}
			catch (std::exception& e)
			{
				std::cerr << "[Server] Exception: " << e.what() << "\n";
				return false;
			}

			std::cout << "[Server] Started!\n";
			return true;
		}

		void Stop()
		{
			asioContext.stop();

			if (contextThread.joinable()) contextThread.join();

			std::cout << "[Server] Stopped!\n";
		}

		auto ProcessAcceptedConnection(std::shared_ptr<IRC::Connection<T>>& newConnection)
		{
			connections.push_back(std::move(newConnection));
			connections.back()->ConnectToClient(IDCounter++);
			std::cout << "[" << connections.back()->GetID() << "] Connection Approved\n";
		}

		void WaitForClientConnection()
		{
			asioAcceptor.async_accept(
				[this](std::error_code ec, boost::asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						std::cout << "[Server] New Connection: " << socket.remote_endpoint() << "\n";

						std::shared_ptr<IRC::Connection<T>> newConnection =
							std::make_shared<IRC::Connection<T>>(IRC::Connection<T>::Owner::server,
																 asioContext, 
																 std::move(socket), 
																 inQueue);

						if (OnClientConnect(newConnection))
						{
							ProcessAcceptedConnection(newConnection);
						}
						else
						{
							std::cout << "[Server] Connection Denied\n";
						}
					}
					else
					{
						std::cout << "[Server] New Connection Error: " << ec.message() << "\n";
					}

					WaitForClientConnection();
				});
		}

		auto IsClientAlive(std::shared_ptr<IRC::Connection<T>>& client) -> bool
		{
			return client && client->IsConnected();
		}

		void ProcessDisconnection(std::shared_ptr<IRC::Connection<T>>& client)
		{
			OnClientDisconnect(client);
			client.reset();
		}

		void MessageClient(std::shared_ptr<IRC::Connection<T>> client, const IRC::Message<T>& msg)
		{
			if (IsClientAlive(client))
			{
				client->Send(msg);
			}
			else
			{
				ProcessDisconnection(client);
				connections.erase(
					std::remove(connections.begin(), connections.end(), client), connections.end());
			}
		}

		void MessageAllClients(const IRC::Message<T>& msg, std::shared_ptr<IRC::Connection<T>> pIgnoreClient = nullptr)
		{
			bool isThereADeadClient = false;

			for (auto itr = connections.rbegin(); itr != connections.rend(); itr++)
			{
				if (IsClientAlive(*itr))
				{
					if ((*itr) != pIgnoreClient)
						(*itr)->Send(msg);
				}
				else
				{
					ProcessDisconnection(*itr);
					isThereADeadClient = true;
				}
			}

			if (isThereADeadClient)
				connections.erase(
					std::remove(connections.begin(), connections.end(), nullptr), connections.end());
		}

		void Update(size_t nMaxMessages = -1, bool bWait = false)
		{
			if (bWait) inQueue.wait();

			size_t nMessageCount = 0;
			while (nMessageCount < nMaxMessages && !inQueue.empty())
			{
				auto msg = inQueue.pop_front();

				OnMessage(msg.remote, msg.msg);

				nMessageCount++;
			}
		}

	protected:
		virtual bool OnClientConnect(std::shared_ptr<IRC::Connection<T>> client) { return false; }
		virtual void OnClientDisconnect(std::shared_ptr<IRC::Connection<T>> client) { }
		virtual void OnMessage(std::shared_ptr<IRC::Connection<T>> client, IRC::Message<T>& msg) { }


	protected:
		IRC::ThreadSafeQueue<IRC::IdentifyingMessage<T>> inQueue;

		std::deque<std::shared_ptr<IRC::Connection<T>>> connections;

		boost::asio::io_context asioContext;
		std::thread contextThread;

		boost::asio::ip::tcp::acceptor asioAcceptor;

		uint32_t IDCounter = 10000;
	};
}