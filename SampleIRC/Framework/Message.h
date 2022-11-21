#pragma once

#include <cstdint>
#include <vector>

namespace IRC
{
	// message ordering information for display on chat
	#pragma pack(push, 1)
	struct Date
	{
		uint16_t year;
		uint8_t day;
		uint8_t month;
	};
	#pragma pack(pop)

	struct Time
	{
		uint8_t hour;
		uint8_t minute;
		uint8_t second;
	};

	// message header allowing for custom frame-type IDs
	template<typename T>
	struct Header
	{
		T id = {};
		uint32_t size = 0;
		Date date = {};
		Time time = {};
	};
	
	// message structure containing header and body
	template<typename T>
	class Message
	{
	public:
		// publicly available message parts
		Header<T> header = {};
		std::vector<uint8_t> body;

		auto size() const -> std::size_t
		{
			return sizeof(Header<T>) + body.size();
		}

		// assumes little-endian byte order
		template<typename DataType>
		friend auto operator << (Message<T>& msg, const DataType& data) -> Message<T>&
		{
			static_assert(std::is_standard_layout<DataType>::value, "Data too complex to push onto vector");

			std::size_t prevSize = msg.body.size();

			msg.body.resize(msg.body.size() + sizeof(DataType));
			std::memcpy(msg.body.data() + prevSize, &data, sizeof(DataType));

			msg.header.size = static_cast<uint32_t>(msg.body.size());

			return msg;
		}

		// assumes little-endian byte order
		template<typename DataType>
		friend auto operator >> (Message<T>& msg, DataType& data) -> Message<T>&
		{
			static_assert(std::is_standard_layout<DataType>::value, "Data too complex to extract from the vector");

			std::size_t newSize = msg.body.size() - sizeof(DataType);
			std::memcpy(&data, msg.body.data() + newSize, sizeof(DataType));

			msg.header.size() = static_cast<uint32_t>(msg.body.size());

			return msg;
		}
	};

	// forward declaration
	template<typename T>
	class Connection;

	// decorator for message processing by the server, allowing to identify the sender
	template<typename T>
	struct IdentifyingMessage
	{
		std::shared_ptr<Connection<T>> sender = nullptr;
		Message<T> msg;
	};
}
