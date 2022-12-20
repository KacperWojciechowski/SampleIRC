#include "Server.h"

int main()
{
	IRCServer server(60000);
	server.Start();
	server.Run();

	return 0;
}