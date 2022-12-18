#pragma once

#include "Common.h"
#include "Message.h"
#include "ThreadSafeQueue.h"

#include <iostream>

namespace IRC
{
	template<typename T>
	class Connection : public std::enable_shared_from_this<Connection<T>>
	{
	public:
		enum class Sender
		{
			Server,
			Client
		};

		Connection(Sender s, boost::asio::io_context& context, 
			boost::asio::ip::tcp::socket soc, 
			ThreadSafeQueue<IdentifyingMessage<T>>& queueIn)
			: asioContext(context),
			socket(std::move(soc)),
			inQueue(queueIn),
			sender(s)
		{
		}

		virtual ~Connection()
		{
			disconnect();
		}

		auto get_user_ID() const -> uint32_t
		{
			return user_id;
		}

		auto set_nick(std::string _nick) -> void
		{
			nick = _nick;
		}

		auto get_nick() -> std::string
		{
			return nick;
		}

		// assign ID to the user, and initialize waiting for incoming messages
		auto connect_to_client(uint32_t uid = 0) -> void
		{
			if (sender == Sender::Server && socket.is_open())
			{
				user_id = uid;
				read_header();
			}
		}

		auto connect_to_server(const boost::asio::ip::tcp::resolver::results_type& endpoint) -> void
		{
			if (sender == Sender::Client)
			{
				boost::asio::async_connect(socket, endpoint,
					[this](std::error_code error, boost::asio::ip::tcp::endpoint endpoint) -> void
					{
						if (!error)
						{
							read_header();
						}
					});
			}
		}

		// post disconnection task to the context
		auto disconnect() -> void
		{
			if (is_connected())
			{
				boost::asio::post(asioContext, [this]() {socket.close(); });
			}
		}

		auto is_connected() -> bool
		{
			return socket.is_open();
		}

		// post transmission task to the context
		auto send(const Message<T>& msg) -> void
		{
			boost::asio::post(asioContext,
				[this, &msg]() -> void
				{
					// if the out queue is empty, it's safe to assume no outgoing transmission is happening
					bool writingRoutineIdle = outQueue.empty();
					outQueue.push_back(msg);
					if (writingRoutineIdle)
					{
						write_header();
					}
				});
		}

	private:
		// asynchronically read header
		auto read_header() -> void
		{
			boost::asio::async_read(socket, boost::asio::buffer(&tempMsg.header, sizeof(Header<T>)),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						// if the message is supposed to contain a body
						if (tempMsg.header.size > 0)
						{
							// prepare temporary message body size
							tempMsg.body.resize(tempMsg.header.size);
							// asynchronically read body
							read_body();
						}
						else
						{
							// push the message to inQueue for processing by the server
							push_to_inQueue();
						}
					}
					else
					{
						std::cout << "[" << user_id << "] read header failed\n";
						socket.close();
					}
				});
		}

		// asynchronically read body
		auto read_body() -> void
		{
			boost::asio::async_read(socket, boost::asio::buffer(tempMsg.body.data(), tempMsg.body.size()),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						push_to_inQueue();
					}
					else
					{
						std::cout << "[" << user_id << "] read body failed\n";
						socket.close();
					}
				});
		}

		// asynchronically transfer the header of the next message in the queue
		auto write_header() -> void
		{
			boost::asio::async_write(socket, boost::asio::buffer(&outQueue.front().header, sizeof(Header<T>)),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						// if there is a body to transfer, initialize sending the body
						if (outQueue.front().body.size() > 0)
						{
							write_body();
						}
						else
						{
							// pop transfered message
							outQueue.pop_front();

							// if the queue is not empty, initialize transfering another message
							if (!outQueue.empty())
							{
								write_header();
							}
						}
					}
					else
					{
						std::cout << "[" << user_id << "] write header failed\n";
						socket.close();
					}
				});
		}

		// asynchronically transfer body
		auto write_body() -> void
		{
			boost::asio::async_write(socket, boost::asio::buffer(outQueue.front().body.data(), outQueue.front().body.size()),
				[this](std::error_code error, std::size_t length)
				{
					if (!error)
					{
						// pop transfered message
						outQueue.pop_front();

						// if the queue is not empty, initialize transfering another message
						if (!outQueue.empty())
						{
							write_header();
						}
					}
					else
					{
						std::cout << "[" << user_id << "] write body failed\n";
						socket.close();
					}
				});
		}
		
		void push_to_inQueue()
		{
			if (sender == Sender::Server)
			{
				inQueue.push_back({ this->shared_from_this(), tempMsg });
			}
			else
			{
				inQueue.push_back({ nullptr, tempMsg });
			}

			read_header();
		}

	protected:
		boost::asio::ip::tcp::socket socket;
		boost::asio::io_context& asioContext;

		ThreadSafeQueue<Message<T>> outQueue;
		ThreadSafeQueue<IdentifyingMessage<T>>& inQueue;

		Message<T> tempMsg;

		Sender sender;
		uint32_t user_id;
		std::string nick;
 	};
}
