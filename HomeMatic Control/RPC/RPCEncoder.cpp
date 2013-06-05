#include "RPCEncoder.h"

namespace RPC
{

uint32_t RPCEncoder::encodeResponse(std::shared_ptr<char>& packet, std::shared_ptr<RPCVariable> value)
{
	//8 byte for "Bin" + 0x01 + packet length
	uint32_t packetLength = 8 + calculateLength(value);
	packet.reset(new char[packetLength]);
	uint32_t position = 0;
	memcpy(packet.get(), _packetStart, 4);
	encodeInteger(packet.get(), &position, packetLength);
	if(value->type == RPCVariableType::rpcVoid)
	{
		encodeVoid(packet.get(), &position);
	}
	else if(value->type == RPCVariableType::rpcInteger)
	{
		encodeInteger(packet.get(), &position, value->integerValue);
	}
	else if(value->type == RPCVariableType::rpcString)
	{
		encodeString(packet.get(), &position, value->stringValue);
	}
	else if(value->type == RPCVariableType::rpcStruct)
	{

	}
	else if(value->type == RPCVariableType::rpcArray)
	{

	}
	return position;
}

uint32_t RPCEncoder::calculateLength(std::shared_ptr<RPCVariable> value)
{
	uint32_t length = 0;
	if(value->type == RPCVariableType::rpcVoid)
	{
		length = 8;
	}
	else if(value->type == RPCVariableType::rpcInteger)
	{
		length = 8;
	}
	else if(value->type == RPCVariableType::rpcString)
	{
		length = 8 + value->stringValue.size();
	}
	else if(value->type == RPCVariableType::rpcStruct)
	{
		length = 8; //Type and struct length
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = value->structValue->begin(); i != value->structValue->end(); ++i)
		{
			length += 4 + (*i)->name.size() + calculateLength(*i);
		}
	}
	else if(value->type == RPCVariableType::rpcArray)
	{
		length = 8; //Type and array length
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = value->arrayValue->begin(); i != value->arrayValue->end(); ++i)
		{
			length += calculateLength(*i);
		}
	}
	return length;
}

void RPCEncoder::encodeInteger(char* packet, uint32_t* position, int32_t integer)
{
	char data[4];
	data[0] = integer >> 24;
	data[1] = (integer >> 16) & 0xFF;
	data[2] = (integer >> 8) & 0xFF;
	data[3] = integer &0xFF;
	memcpy(packet + *position, data, 4);
	*position += 4;
}

void RPCEncoder::encodeString(char* packet,  uint32_t* position, std::string string)
{
	encodeInteger(packet, position, 3);
	encodeInteger(packet, position, string.size());
	if(string.size() > 0)
	{
		memcpy(packet + *position, string.c_str(), string.size());
		*position += string.size();
	}
}

void RPCEncoder::encodeVoid(char* packet, uint32_t* position)
{
	encodeString(packet, position, "");
}

} /* namespace RPC */
