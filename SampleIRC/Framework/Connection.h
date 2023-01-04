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

		Connection(Owner parent, 
				   boost::asio::io_context& _asioContext, 
				   boost::asio::ip::tcp::socket _socket, 
				   IRC::ThreadSafeQueue<IRC::IdentifyingMessage<T>>& queueIn)
			: asioContext(_asioContext), 
			socket(std::move(_socket)), 
			inQueue(queueIn),
			owner(parent)
		{
		}

		virtual ~Connection()
		{}

		auto GetID() const -> uint32_t
		{
			return id;
		}

		auto SetID(uint32_t ID) -> void
		{
			id = ID;
		}

		auto ConnectToClient(uint32_t uid = 0) -> void
		{
			if (owner == Owner::server)
			{
				if (socket.is_open())
				{
					ReadHeader();
				}
			}
		}

		auto ConnectToServer(const boost::asio::ip::tcp::resolver::results_type& endpoints) -> void
		{
			if (owner == Owner::client)
			{
				boost::asio::async_connect(socket, endpoints,
					[this](std::error_code ec, boost::asio::ip::tcp::endpoint endpoint)
					{
						if (!ec)
						{
							ReadHeader();
						}
					});
			}
		}


		auto Disconnect() -> void
		{
			if (IsConnected())
				boost::asio::post(asioContext, [this]() { socket.close(); });
		}

		auto IsConnected() const -> bool
		{
			return socket.is_open();
		}

		auto StartListening() -> void
		{

		}

		auto Send(const IRC::Message<T>& msg) -> void
		{
			boost::asio::post(asioContext,
				[this, msg]()
				{
					bool outQueueIdle = outQueue.empty();
					outQueue.push_back(msg);
					if (outQueueIdle)
					{
						WriteHeader();
					}
				});
		}

	private:
		auto WriteHeader() -> void
		{
			boost::asio::async_write(socket, boost::asio::buffer(&outQueue.front().header, sizeof(IRC::Header<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (outQueue.front().body.size() > 0)
						{
							WriteBody();
						}
						else
						{
							outQueue.pop_front();

							if (!outQueue.empty())
							{
								WriteHeader();
							}
						}
					}
					else
					{
						printf("[%d] Write Header Fail.\n", id);
						socket.close();
					}
				});
		}

		auto WriteBody() -> void
		{
			boost::asio::async_write(socket, boost::asio::buffer(outQueue.front().body.data(), outQueue.front().body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						outQueue.pop_front();

						if (!outQueue.empty())
						{
							WriteHeader();
						}
					}
					else
					{
						printf("[%d] Write Body Fail.\n", id);
						socket.close();
					}
				});
		}

		auto ReadHeader() -> void
		{
			boost::asio::async_read(socket, boost::asio::buffer(&tempMsg.header, sizeof(IRC::Header<T>)),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						if (tempMsg.header.size > 0)
						{
							tempMsg.body.resize(tempMsg.header.size);
							ReadBody();
						}
						else
						{
							PushIncoming();
						}
					}
					else
					{
						printf("[%d] Read Header Fail.\n", id);
						socket.close();
					}
				});
		}

		auto ReadBody() -> void
		{
			boost::asio::async_read(socket, boost::asio::buffer(tempMsg.body.data(), tempMsg.body.size()),
				[this](std::error_code ec, std::size_t length)
				{
					if (!ec)
					{
						PushIncoming();
					}
					else
					{
						printf("[%d] Read Body Fail.\n", id);
						socket.close();
					}
				});
		}

		auto PushIncoming() -> void
		{
			if (owner == Owner::server)
				inQueue.push_back({ this->shared_from_this(), tempMsg });
			else
				inQueue.push_back({ nullptr, tempMsg });

			ReadHeader();
		}

	protected:
		boost::asio::ip::tcp::socket socket;
		boost::asio::io_context& asioContext;

		IRC::ThreadSafeQueue<IRC::Message<T>> outQueue;
		IRC::ThreadSafeQueue<IRC::IdentifyingMessage<T>>& inQueue;

		IRC::Message<T> tempMsg;

		Owner owner = Owner::server;

		uint32_t id = 0;
	};
}