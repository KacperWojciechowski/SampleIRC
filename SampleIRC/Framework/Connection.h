#pragma once

#include "Common.h"
#include "ThreadSafeQueue.h"
#include "Message.h"


namespace IRC
{
	template<typename T>
	class Connection : public std::enable_shared_from_this<Connection<T>>
	{
	public:
		enum class Owner
		{
			server,
			client
		};

	public:
		Connection(Owner parent, boost::asio::io_context& asioContext, boost::asio::ip::tcp::socket socket, IRC::ThreadSafeQueue<olc::net::owned_message<T>>& qIn)
			: m_asioContext(asioContext), m_socket(std::move(socket)), m_qMessagesIn(qIn)
		{
			m_nOwnerType = parent;
		}

		virtual ~Connection()
		{}

		uint32_t GetID() const
		{
			return id;
		}

	public:
		void ConnectToClient(uint32_t uid = 0)
		{
			if (m_nOwnerType == Owner::server)
			{
				if (m_socket.is_open())
				{
					id = uid;
					ReadHeader();
				}
			}
		}

		void ConnectToServer(const boost::asio::ip::tcp::resolver::results_type& endpoints)
		{
			if (m_nOwnerType == Owner::client)
			{
				boost::asio::async_connect(m_socket, endpoints,
					[this](std::error_code ec, boost::asio::ip::tcp::endpoint endpoint)
					{
						if (!ec)
						{
							ReadHeader();
						}
					});
			}
		}


		void Disconnect()
		{
			if (IsConnected())
				boost::asio::post(m_asioContext, [this]() { m_socket.close(); });
		}

		bool IsConnected() const
		{
			return m_socket.is_open();
		}

		void StartListening()
		{

		}

	public:
		void Send(const olc::net::message<T>& msg)
		{
			boost::asio::post(m_asioContext,
				[this, msg]()
				{
					bool bWritingMessage = !m_qMessagesOut.empty();
					m_qMessagesOut.push_back(msg);
					if (!bWritingMessage)
					{
						WriteHeader();
					}
				});
		}



	private:
		void WriteHeader()
		{
			boost::asio::async_write(m_socket, boost::asio::buffer(&m_qMessagesOut.front().header, sizeof(olc::net::message_header<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_qMessagesOut.front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							m_qMessagesOut.pop_front();

							if (!m_qMessagesOut.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						std::cout << "[" << id << "] Write Header Fail.\n";
						m_socket.close();
					}
				});
		}

		void WriteBody()
		{
			boost::asio::async_write(m_socket, boost::asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						m_qMessagesOut.pop_front();

						if (!m_qMessagesOut.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						std::cout << "[" << id << "] Write Body Fail.\n";
						m_socket.close();
					}
				});
		}

		void ReadHeader()
		{
			boost::asio::async_read(m_socket, boost::asio::buffer(&m_msgTemporaryIn.header, sizeof(olc::net::message_header<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (m_msgTemporaryIn.header.size > 0)
						{
							m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
							ReadBody();
						}
						else
						{
							AddToIncomingMessageQueue();
						}
					}
					else
					{
						std::cout << "[" << id << "] Read Header Fail.\n";
						m_socket.close();
					}
				});
		}

		void ReadBody()
		{
			boost::asio::async_read(m_socket, boost::asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						AddToIncomingMessageQueue();
					}
					else
					{
						std::cout << "[" << id << "] Read Body Fail.\n";
						m_socket.close();
					}
				});
		}

		void AddToIncomingMessageQueue()
		{
			if (m_nOwnerType == Owner::server)
				m_qMessagesIn.push_back({ this->shared_from_this(), m_msgTemporaryIn });
			else
				m_qMessagesIn.push_back({ nullptr, m_msgTemporaryIn });

			ReadHeader();
		}

	protected:
		boost::asio::ip::tcp::socket m_socket;
		boost::asio::io_context& m_asioContext;

		IRC::ThreadSafeQueue<olc::net::message<T>> m_qMessagesOut;

		IRC::ThreadSafeQueue<olc::net::owned_message<T>>& m_qMessagesIn;

		olc::net::message<T> m_msgTemporaryIn;

		Owner m_nOwnerType = Owner::server;

		uint32_t id = 0;

	};
}