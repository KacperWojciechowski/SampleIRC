#include "Client.h"

int main()
{
	IRCClient c;
	c.Connect("127.0.0.1", 60000);

	return 0;
}
