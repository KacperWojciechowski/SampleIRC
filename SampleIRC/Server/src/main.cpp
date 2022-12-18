#include "Server.h"

int main()
{
	IRCServer server(60000);
	server.Start();

	while (1)
	{
		server.Update();
	}
	return 0;
}
