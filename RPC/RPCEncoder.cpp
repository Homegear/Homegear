#include "RPCEncoder.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC
{

std::shared_ptr<std::vector<char>> RPCEncoder::encodeResponse(std::shared_ptr<RPCVariable> variable)
{
	//The "Bin", the type byte after that and the length itself are not part of the length
	std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
	try
	{
		if(variable->errorStruct) packet->insert(packet->begin(), _packetStartError, _packetStartError + 4);
		else packet->insert(packet->begin(), _packetStart, _packetStart + 4);

		encodeVariable(packet, variable);
		uint32_t dataSize = packet->size() - 4;

		char result[4];
		HelperFunctions::memcpyBigEndian(result, (char*)&dataSize, 4);
		packet->insert(packet->begin() + 4, result, result + 4);

		return packet;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return packet;
}

void RPCEncoder::encodeVariable(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		if(variable->type == RPCVariableType::rpcVoid)
		{
			encodeVoid(packet);
		}
		else if(variable->type == RPCVariableType::rpcInteger)
		{
			encodeInteger(packet, variable);
		}
		else if(variable->type == RPCVariableType::rpcFloat)
		{
			encodeFloat(packet, variable);
		}
		else if(variable->type == RPCVariableType::rpcBoolean)
		{
			encodeBoolean(packet, variable);
		}
		else if(variable->type == RPCVariableType::rpcString)
		{
			encodeString(packet, variable);
		}
		else if(variable->type == RPCVariableType::rpcStruct)
		{
			encodeStruct(packet, variable);
		}
		else if(variable->type == RPCVariableType::rpcArray)
		{
			encodeArray(packet, variable);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeStruct(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcStruct);
		encodeRawInteger(packet, variable->structValue->size());
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			encodeRawString(packet, (*i)->name);
			encodeVariable(packet, *i);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeArray(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcArray);
		encodeRawInteger(packet, variable->arrayValue->size());
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->arrayValue->begin(); i != variable->arrayValue->end(); ++i)
		{
			encodeVariable(packet, *i);
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeType(std::shared_ptr<std::vector<char>>& packet, RPCVariableType type)
{
	encodeRawInteger(packet, (int32_t)type);
}

void RPCEncoder::encodeRawInteger(std::shared_ptr<std::vector<char>>& packet, int32_t integer)
{
	try
	{
		char result[4];
		HelperFunctions::memcpyBigEndian(result, (char*)&integer, 4);
		packet->insert(packet->end(), result, result + 4);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeInteger(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	encodeType(packet, RPCVariableType::rpcInteger);
	encodeRawInteger(packet, variable->integerValue);
}

void RPCEncoder::encodeFloat(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcFloat);
		double temp = std::abs(variable->floatValue);
		int32_t exponent = 0;
		if(temp != 0 && temp < 0.5)
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
		if(variable->floatValue < 0) temp *= -1;
		int32_t mantissa = std::lround(temp * 0x40000000);
		char data[8];
		HelperFunctions::memcpyBigEndian(data, (char*)&mantissa, 4);
		HelperFunctions::memcpyBigEndian(data + 4, (char*)&exponent, 4);
		packet->insert(packet->end(), data, data + 8);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeBoolean(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	encodeType(packet, RPCVariableType::rpcBoolean);
	packet->push_back((char)variable->booleanValue);
}

void RPCEncoder::encodeString(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcString);
		//We could call encodeRawString here, but then the string would have to be copied and that would cost time.
		encodeRawInteger(packet, variable->stringValue.size());
		if(variable->stringValue.size() > 0)
		{
			packet->insert(packet->end(), variable->stringValue.begin(), variable->stringValue.end());
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeRawString(std::shared_ptr<std::vector<char>>& packet, std::string& string)
{
	try
	{
		encodeRawInteger(packet, string.size());
		if(string.size() > 0)
		{
			packet->insert(packet->end(), string.begin(), string.end());
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeVoid(std::shared_ptr<std::vector<char>>& packet)
{
	std::shared_ptr<RPCVariable> string(new RPCVariable(RPCVariableType::rpcString));
	encodeString(packet, string);
}

} /* namespace RPC */
