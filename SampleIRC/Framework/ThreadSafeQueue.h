#pragma once

#include "Common.h"

#include <mutex>
#include <deque>

namespace IRC
{
	// Thread safe queue for async processing, implemented via double ended queue
	template<typename T>
	class ThreadSafeQueue
	{
	public:
		ThreadSafeQueue() = default;
		ThreadSafeQueue(const ThreadSafeQueue& tsq) = delete;

		const T& front()
		{
			std::scoped_lock lock(queueMux);
			return queue.front();
		}

		const T& back()
		{
			std::scoped_lock lock(queueMux);
			return queue.back();
		}

		void push_back(const T& item)
		{
			std::scoped_lock lock(queueMux);
			queue.push_back(std::move(item));
		}

		void push_front(const T& item)
		{
			std::scoped_lock lock(queueMux);
			queue.push_front(std::move(item));
		}

		bool empty()
		{
			std::scoped_lock lock(queueMux);
			return queue.empty();
		}

		std::size_t count()
		{
			std::scoped_lock lock(queueMux);
			return queue.size();
		}

		void clear()
		{
			std::scoped_lock lock(queueMux);
			queue.clear();
		}

		T pop_front()
		{
			std::scoped_lock lock(queueMux);
			auto item = std::move(queue.front());
			queue.pop_front();
			return item;
		}

		T pop_back()
		{
			std::scoped_lock lock(queueMux);
			auto item = std::move(queue.back());
			queue.pop_back();
			return item;
		}

		virtual ~ThreadSafeQueue()
		{
			clear();
		}

	protected:
		std::mutex queueMux;
		std::deque<T> queue;
	};
}
