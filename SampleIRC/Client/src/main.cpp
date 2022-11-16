#include <iostream>

#ifdef _WIN32
#define _WIN32_WINNT 0X0A00
#endif

// disable boost library warnings
#pragma warning(disable : 6255)
#pragma warning(disable : 6387)
#pragma warning(disable : 26495)
#pragma warning(disable : 6031)
#pragma warning(disable : 6258)
#pragma warning(disable : 6001)
#pragma warning(disable : 26498)
#pragma warning(disable : 26451)


#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

int main()
{

	return 0;
}
