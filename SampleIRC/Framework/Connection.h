﻿#pragma once

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
				   IRC::ThreadSafeQueue<olc::net::owned_message<T>>& qIn)
			: asioContext(_asioContext), 
			socket(std::move(_socket)), 
			inQueue(qIn),
			owner(parent)
		{
		}

		virtual ~Connection()
		{}

		auto GetID() const -> uint32_t
		{
			return id;
		}

		auto ConnectToClient(uint32_t uid = 0) -> void
		{
			if (owner == Owner::server)
			{
				if (socket.is_open())
				{
					id = uid;
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

		auto Send(const olc::net::message<T>& msg) -> void
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
			boost::asio::async_write(socket, boost::asio::buffer(&outQueue.front().header, sizeof(olc::net::message_header<T>)),
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
						std::cout << "[" << id << "] Write Header Fail.\n";
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
						std::cout << "[" << id << "] Write Body Fail.\n";
						socket.close();
					}
				});
		}

		auto ReadHeader() -> void
		{
			boost::asio::async_read(socket, boost::asio::buffer(&tempMsg.header, sizeof(olc::net::message_header<T>)),
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
						std::cout << "[" << id << "] Read Header Fail.\n";
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
						std::cout << "[" << id << "] Read Body Fail.\n";
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

		IRC::ThreadSafeQueue<olc::net::message<T>> outQueue;
		IRC::ThreadSafeQueue<olc::net::owned_message<T>>& inQueue;

		olc::net::message<T> tempMsg;

		Owner owner = Owner::server;

		uint32_t id = 0;
	};
}