#include <iostream>
#include <thread>
#include <memory>

#include "Client.h"
#include "LoadTestClient.h"

int main()
{
	std::vector<std::shared_ptr<IRCLoadClient>> clients;
	std::vector<std::thread> threads;

	bool stopFlag = false;

	//std::shared_ptr<IRCLoadClient> ptr = std::make_shared<IRCLoadClient>(stopFlag, true);

	clients.push_back(std::make_shared<IRCLoadClient>(stopFlag, true));
	clients[clients.size() - 1]->Connect("127.0.0.1", 60000);
	int counter = 1;
	threads.emplace_back([&clients, counter]() {clients[counter-1]->Run(); });
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	std::cout << counter << '\n';
	
	for (int i = 1; i < 100; i++)
	{
		clients.emplace_back(std::make_shared<IRCLoadClient>(stopFlag, false));
		counter++;
		std::cout << counter << "\n";
		clients[counter - 1]->Connect("127.0.0.1", 60000);
		threads.emplace_back([&clients, i]() {clients[i]->Run(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	stopFlag = true;

	std::cout << "Joining threads\n";

	for (auto& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	threads.clear();
	clients.clear();

	std::cout << "Test finished\n";

	return 0;
}