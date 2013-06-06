#include "RPCDecoder.h"

namespace RPC
{

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeRequest(std::shared_ptr<char> packet, uint32_t packetLength, std::string* methodName)
{
	uint32_t position = 0;
	*methodName = decodeString(packet.get(), packetLength, &position);
	uint32_t parameterCount = decodeInteger(packet.get(), packetLength, &position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters(new std::vector<std::shared_ptr<RPCVariable>>());
	for(uint32_t i = 0; i < parameterCount; i++)
	{
		parameters->push_back(decodeParameter(packet.get(), packetLength, &position));
	}
	return parameters;
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeResponse(std::shared_ptr<char> packet, uint32_t packetLength, uint32_t offset)
{
	uint32_t position = offset;
	return decodeParameter(packet.get(), packetLength, &position);
}

int32_t RPCDecoder::decodeInteger(char* packet, uint32_t packetLength, uint32_t* position)
{
	if(*position + 4 > packetLength) return 0;
	int32_t integer = (((unsigned char)*(packet + *position)) << 24) + (((unsigned char)*(packet + *position + 1)) << 16) + (((unsigned char)*(packet + *position + 2)) << 8) + ((unsigned char)*(packet + *position + 3));
	*position += 4;
	return integer;
}

RPCVariableType RPCDecoder::decodeType(char* packet, uint32_t packetLength, uint32_t* position)
{
	return (RPCVariableType)decodeInteger(packet, packetLength, position);
}

std::string RPCDecoder::decodeString(char* packet, uint32_t packetLength, uint32_t* position)
{
	int32_t stringLength = decodeInteger(packet, packetLength, position);
	if(*position + stringLength > packetLength || stringLength == 0) return "";
	char string[stringLength + 1];
	memcpy(string, packet + *position, stringLength);
	string[stringLength] = '\0';
	*position += stringLength;
	return std::string(string);
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeParameter(char* packet, uint32_t packetLength, uint32_t* position)
{
	RPCVariableType type = decodeType(packet, packetLength, position);
	std::shared_ptr<RPCVariable> variable(new RPCVariable(type));
	if(type == RPCVariableType::rpcString)
	{
		variable->stringValue = decodeString(packet, packetLength, position);
	}
	else if(type == RPCVariableType::rpcInteger)
	{
		variable->integerValue = decodeInteger(packet, packetLength, position);
	}
	else if(type == RPCVariableType::rpcArray)
	{
		variable->arrayValue = decodeArray(packet, packetLength, position);
	}
	else if(type == RPCVariableType::rpcStruct)
	{
		variable->structValue = decodeStruct(packet, packetLength, position);
	}
	return variable;
}

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeArray(char* packet, uint32_t packetLength, uint32_t* position)
{
	uint32_t arrayLength = decodeInteger(packet, packetLength, position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> array(new std::vector<std::shared_ptr<RPCVariable>>());
	for(uint32_t i = 0; i < arrayLength; i++)
	{
		array->push_back(decodeParameter(packet, packetLength, position));
	}
	return array;
}

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeStruct(char* packet, uint32_t packetLength, uint32_t* position)
{
	uint32_t structLength = decodeInteger(packet, packetLength, position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct(new std::vector<std::shared_ptr<RPCVariable>>());
	for(uint32_t i = 0; i < structLength; i++)
	{
		std::string name = decodeString(packet, packetLength, position);
		rpcStruct->push_back(decodeParameter(packet, packetLength, position));
		rpcStruct->back()->name = name;
	}
	return rpcStruct;
}

} /* namespace RPC */
