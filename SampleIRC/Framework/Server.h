#pragma once

#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Message.h"
#include "Connection.h"

namespace IRC
{
	template<typename T>
	class server_interface
	{
	public:
		server_interface(uint16_t port)
			: m_asioAcceptor(m_asioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
		{ }

		virtual ~server_interface()
		{
			Stop();
		}

		bool Start()
		{
			try
			{
				WaitForClientConnection();

				m_threadContext = std::thread([this]() { m_asioContext.run(); });
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
			m_asioContext.stop();

			if (m_threadContext.joinable()) m_threadContext.join();

			std::cout << "[Server] Stopped!\n";
		}

		auto ProcessAcceptedConnection(std::shared_ptr<IRC::Connection<T>>& newconn)
		{
			m_deqConnections.push_back(std::move(newconn));
			m_deqConnections.back()->ConnectToClient(nIDCounter++);
			std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";
		}

		void WaitForClientConnection()
		{
			m_asioAcceptor.async_accept(
				[this](std::error_code ec, boost::asio::ip::tcp::socket socket)
				{
					if (!ec)
					{
						std::cout << "[Server] New Connection: " << socket.remote_endpoint() << "\n";

						std::shared_ptr<IRC::Connection<T>> newconn =
							std::make_shared<IRC::Connection<T>>(IRC::Connection<T>::Owner::server,
								m_asioContext, std::move(socket), m_qMessagesIn);

						if (OnClientConnect(newconn))
						{
							ProcessAcceptedConnection(newconn);
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
				m_deqConnections.erase(
					std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
			}
		}

		void MessageAllClients(const IRC::Message<T>& msg, std::shared_ptr<IRC::Connection<T>> pIgnoreClient = nullptr)
		{
			bool isThereADeadClient = false;

			for (auto& client : m_deqConnections)
			{
				if (IsClientAlive(client))
				{
					if (client != pIgnoreClient)
						client->Send(msg);
				}
				else
				{
					ProcessDisconnection(client);
					isThereADeadClient = true;
				}
			}

			if (isThereADeadClient)
				m_deqConnections.erase(
					std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
		}

		void Update(size_t nMaxMessages = -1, bool bWait = false)
		{
			if (bWait) m_qMessagesIn.wait();

			size_t nMessageCount = 0;
			while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty())
			{
				auto msg = m_qMessagesIn.pop_front();

				OnMessage(msg.remote, msg.msg);

				nMessageCount++;
			}
		}

	protected:
		virtual bool OnClientConnect(std::shared_ptr<IRC::Connection<T>> client) { return false; }
		virtual void OnClientDisconnect(std::shared_ptr<IRC::Connection<T>> client) { }
		virtual void OnMessage(std::shared_ptr<IRC::Connection<T>> client, IRC::Message<T>& msg) { }


	protected:
		IRC::ThreadSafeQueue<IRC::IdentifyingMessage<T>> m_qMessagesIn;

		std::deque<std::shared_ptr<IRC::Connection<T>>> m_deqConnections;

		boost::asio::io_context m_asioContext;
		std::thread m_threadContext;

		boost::asio::ip::tcp::acceptor m_asioAcceptor;

		uint32_t nIDCounter = 10000;
	};
}