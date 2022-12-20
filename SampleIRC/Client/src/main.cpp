#include <iostream>
#include "Client.h"

int main()
{
	IRCClient c;
	c.Connect("127.0.0.1", 60000);
	c.Run();

	return 0;
}