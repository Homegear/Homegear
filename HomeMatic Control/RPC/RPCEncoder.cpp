#include "RPCEncoder.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC
{

uint32_t RPCEncoder::encodeResponse(std::shared_ptr<char>& packet, std::shared_ptr<RPCVariable> variable)
{
	//The "Bin", the type byte after that and the length itself are not part of the length
	uint32_t packetLength = calculateLength(variable);
	packet.reset(new char[packetLength + 8]);
	std::cout << "Packet length: " << packetLength << std::endl;
	uint32_t position = 0;
	if(variable->errorStruct) memcpy(packet.get(), _packetStartError, 4);
	else memcpy(packet.get(), _packetStart, 4);
	position += 4;
	encodeRawInteger(packet.get(), &position, packetLength + 8, packetLength);
	encodeVariable(packet.get(), &position, packetLength + 8, variable);
	return position;
}

void RPCEncoder::encodeVariable(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	if(variable->type == RPCVariableType::rpcVoid)
	{
		encodeVoid(packet, position, packetLength);
	}
	else if(variable->type == RPCVariableType::rpcInteger)
	{
		encodeInteger(packet, position, packetLength, variable);
	}
	else if(variable->type == RPCVariableType::rpcFloat)
	{
		encodeFloat(packet, position, packetLength, variable);
	}
	else if(variable->type == RPCVariableType::rpcBoolean)
	{
		encodeBoolean(packet, position, packetLength, variable);
	}
	else if(variable->type == RPCVariableType::rpcString)
	{
		encodeString(packet, position, packetLength, variable);
	}
	else if(variable->type == RPCVariableType::rpcStruct)
	{
		encodeStruct(packet, position, packetLength, variable);
	}
	else if(variable->type == RPCVariableType::rpcArray)
	{
		encodeArray(packet, position, packetLength, variable);
	}
}

uint32_t RPCEncoder::calculateLength(std::shared_ptr<RPCVariable> variable)
{
	uint32_t length = 0;
	if(variable->type == RPCVariableType::rpcVoid)
	{
		length = 8;
	}
	else if(variable->type == RPCVariableType::rpcInteger)
	{
		length = 8;
	}
	else if(variable->type == RPCVariableType::rpcFloat)
	{
		length = 12;
	}
	else if(variable->type == RPCVariableType::rpcBoolean)
	{
		length = 8;
	}
	else if(variable->type == RPCVariableType::rpcString)
	{
		length = 8 + variable->stringValue.size();
	}
	else if(variable->type == RPCVariableType::rpcStruct)
	{
		length = 8; //Type and struct length
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			length += 4 + (*i)->name.size() + calculateLength(*i);
		}
	}
	else if(variable->type == RPCVariableType::rpcArray)
	{
		length = 8; //Type and array length
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->arrayValue->begin(); i != variable->arrayValue->end(); ++i)
		{
			length += calculateLength(*i);
		}
	}
	return length;
}

void RPCEncoder::encodeStruct(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	encodeType(packet, position, packetLength, RPCVariableType::rpcStruct);
	encodeRawInteger(packet, position, packetLength, variable->structValue->size());
	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
	{
		encodeRawString(packet, position, packetLength, (*i)->name);
		encodeVariable(packet, position, packetLength, *i);
	}
}

void RPCEncoder::encodeArray(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	encodeType(packet, position, packetLength, RPCVariableType::rpcArray);
	encodeRawInteger(packet, position, packetLength, variable->arrayValue->size());
	for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->arrayValue->begin(); i != variable->arrayValue->end(); ++i)
	{
		encodeVariable(packet, position, packetLength, *i);
	}
}

void RPCEncoder::encodeType(char* packet, uint32_t* position, uint32_t packetLength, RPCVariableType type)
{
	encodeRawInteger(packet, position, packetLength, (int32_t)type);
}

void RPCEncoder::encodeRawInteger(char* packet, uint32_t* position, uint32_t packetLength, int32_t integer)
{
	if(*position + 4 > packetLength)
	{
		if(GD::debugLevel >= 1) std::cout << "Critical: RPC data exceeds calculated packet length." << std::endl;
		return;
	}
	HelperFunctions::memcpyBigEndian(packet + *position, (char*)&integer, 4);
	*position += 4;
}

void RPCEncoder::encodeInteger(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	encodeType(packet, position, packetLength, RPCVariableType::rpcInteger);
	encodeRawInteger(packet, position, packetLength, variable->integerValue);
}

void RPCEncoder::encodeFloat(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	encodeType(packet, position, packetLength, RPCVariableType::rpcFloat);
	if(*position + 8 > packetLength)
	{
		if(GD::debugLevel >= 1) std::cout << "Critical: RPC data exceeds calculated packet length." << std::endl;
		return;
	}
	double temp = std::abs(variable->floatValue);
	int32_t exponent = 0;
	if(temp < 0.5)
	{
		while(temp < 0.5)
		{
			temp *= 2;
			exponent--;
		}
	}
	else while(temp >= 1)
	{
		temp /= 2;
		exponent++;
	}
	int32_t mantissa = std::roundl(temp * 0x40000000);
	if(variable->floatValue < 0) mantissa |= 0x80000000;
	HelperFunctions::memcpyBigEndian(packet + *position, (char*)&mantissa, 4);
	*position += 4;
	HelperFunctions::memcpyBigEndian(packet + *position, (char*)&exponent, 4);
	*position += 4;
}

void RPCEncoder::encodeBoolean(char* packet, uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	encodeType(packet, position, packetLength, RPCVariableType::rpcBoolean);
	if(*position + 1 > packetLength)
	{
		if(GD::debugLevel >= 1) std::cout << "Critical: RPC data exceeds calculated packet length." << std::endl;
		return;
	}
	char data[1];
	data[0] = (char)variable->booleanValue;
	memcpy(packet + *position, data, 1);
	*position += 1;
}

void RPCEncoder::encodeString(char* packet,  uint32_t* position, uint32_t packetLength, std::shared_ptr<RPCVariable> variable)
{
	encodeType(packet, position, packetLength, RPCVariableType::rpcString);
	//We could call encodeRawString here, but then the string would have to be copied and that would cost time.
	encodeRawInteger(packet, position, packetLength, variable->stringValue.size());
	if(variable->stringValue.size() > 0)
	{
		if(*position + variable->stringValue.size() > packetLength)
		{
			if(GD::debugLevel >= 1) std::cout << "Critical: RPC data exceeds calculated packet length." << std::endl;
			return;
		}
		memcpy(packet + *position, variable->stringValue.c_str(), variable->stringValue.size());
		*position += variable->stringValue.size();
	}
}

void RPCEncoder::encodeRawString(char* packet,  uint32_t* position, uint32_t packetLength, std::string string)
{
	encodeRawInteger(packet, position, packetLength, string.size());
	if(string.size() > 0)
	{
		if(*position + string.size() > packetLength)
		{
			if(GD::debugLevel >= 1) std::cout << "Critical: RPC data exceeds calculated packet length." << std::endl;
			return;
		}
		memcpy(packet + *position, string.c_str(), string.size());
		*position += string.size();
	}
}

void RPCEncoder::encodeVoid(char* packet, uint32_t* position, uint32_t packetLength)
{
	encodeString(packet, position, packetLength, std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcString)));
}

} /* namespace RPC */
