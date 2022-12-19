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
		auto front() -> const T&
		{
			std::scoped_lock lock(queueMutex);
			return queue.front();
		}

		auto back() -> const T&
		{
			std::scoped_lock lock(queueMutex);
			return queue.back();
		}

		auto pop_front() -> T
		{
			std::scoped_lock lock(queueMutex);
			auto t = std::move(queue.front());
			queue.pop_front();
			return t;
		}

		auto pop_back() -> T
		{
			std::scoped_lock lock(queueMutex);
			auto t = std::move(queue.back());
			queue.pop_back();
			return t;
		}

		auto push_back(const T& item) -> void
		{
			std::scoped_lock lock(queueMutex);
			queue.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(blockingMutex);
			cvBlocking.notify_one();
		}

		auto push_front(const T& item) -> void
		{
			std::scoped_lock lock(queueMutex);
			queue.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(blockingMutex);
			cvBlocking.notify_one();
		}

		auto empty() -> bool
		{
			std::scoped_lock lock(queueMutex);
			return queue.empty();
		}

		auto count() -> std::size_t
		{
			std::scoped_lock lock(queueMutex);
			return queue.size();
		}

		auto clear() -> void
		{
			std::scoped_lock lock(queueMutex);
			queue.clear();
		}

		auto wait() -> void
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
