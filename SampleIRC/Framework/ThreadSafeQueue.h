#pragma once

#include "Common.h"

namespace IRC
{
	template<typename T>
	class ThreadSafeQueue
	{
	public:
		ThreadSafeQueue() = default;
		ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
		virtual ~ThreadSafeQueue() { clear(); }

	public:
		const T& front()
		{
			std::scoped_lock lock(queueMutex);
			return queue.front();
		}

		const T& back()
		{
			std::scoped_lock lock(queueMutex);
			return queue.back();
		}

		T pop_front()
		{
			std::scoped_lock lock(queueMutex);
			auto t = std::move(queue.front());
			queue.pop_front();
			return t;
		}

		T pop_back()
		{
			std::scoped_lock lock(queueMutex);
			auto t = std::move(queue.back());
			queue.pop_back();
			return t;
		}

		void push_back(const T& item)
		{
			std::scoped_lock lock(queueMutex);
			queue.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(blockingMutex);
			cvBlocking.notify_one();
		}

		void push_front(const T& item)
		{
			std::scoped_lock lock(queueMutex);
			queue.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(blockingMutex);
			cvBlocking.notify_one();
		}

		bool empty()
		{
			std::scoped_lock lock(queueMutex);
			return queue.empty();
		}

		size_t count()
		{
			std::scoped_lock lock(queueMutex);
			return queue.size();
		}

		void clear()
		{
			std::scoped_lock lock(queueMutex);
			queue.clear();
		}

		void wait()
		{
			while (empty())
			{
				std::unique_lock<std::mutex> ul(blockingMutex);
				cvBlocking.wait(ul);
			}
		}

	protected:
		std::mutex queueMutex;
		std::deque<T> queue;
		std::condition_variable cvBlocking;
		std::mutex blockingMutex;
	};
}
