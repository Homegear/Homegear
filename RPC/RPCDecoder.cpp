#include "RPCDecoder.h"
#include "../HelperFunctions.h"

namespace RPC
{

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName)
{
	try
	{
		uint32_t position = 0;
		methodName = decodeString(packet, position);
		uint32_t parameterCount = decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters(new std::vector<std::shared_ptr<RPCVariable>>());
		for(uint32_t i = 0; i < parameterCount; i++)
		{
			parameters->push_back(decodeParameter(packet, position));
		}
		return parameters;
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
    return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>();
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeResponse(std::shared_ptr<std::vector<char>> packet, uint32_t offset)
{
	uint32_t position = offset;
	return decodeParameter(packet, position);
}

int32_t RPCDecoder::decodeInteger(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	int32_t integer = 0;
	try
	{
		if(position + 4 > packet->size())
		{
			if(position + 1 > packet->size()) return 0;
			//IP-Symcon encodes integers as string => Difficult to interpret. This works for numbers up to 3 digits:
			std::string string(&packet->at(position), &packet->at(packet->size() - 1) + 1);
			position = packet->size();
			integer = HelperFunctions::getNumber(string);
			return integer;
		}
		HelperFunctions::memcpyBigEndian((char*)&integer, &packet->at(position), 4);
		position += 4;
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
	return integer;
}

double RPCDecoder::decodeFloat(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		if(position + 8 > packet->size()) return 0;
		int32_t mantissa = 0;
		int32_t exponent = 0;
		HelperFunctions::memcpyBigEndian((char*)&mantissa, &packet->at(position), 4);
		position += 4;
		HelperFunctions::memcpyBigEndian((char*)&exponent, &packet->at(position), 4);
		position += 4;
		double floatValue = (double)mantissa / 0x40000000;
		floatValue *= std::pow(2, exponent);
		return floatValue;
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
    return 0;
}

bool RPCDecoder::decodeBoolean(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		if(position + 1 > packet->size()) return 0;
		bool boolean = (bool)packet->at(position);
		position += 1;
		return boolean;
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
    return false;
}

RPCVariableType RPCDecoder::decodeType(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	return (RPCVariableType)decodeInteger(packet, position);
}

std::string RPCDecoder::decodeString(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		int32_t stringLength = decodeInteger(packet, position);
		if(position + stringLength > packet->size() || stringLength == 0) return "";
		std::string string(&packet->at(position), &packet->at(position) + stringLength);
		position += stringLength;
		return string;
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
    return "";
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeParameter(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		RPCVariableType type = decodeType(packet, position);
		std::shared_ptr<RPCVariable> variable(new RPCVariable(type));
		if(type == RPCVariableType::rpcString)
		{
			variable->stringValue = decodeString(packet, position);
		}
		else if(type == RPCVariableType::rpcInteger)
		{
			variable->integerValue = decodeInteger(packet, position);
		}
		else if(type == RPCVariableType::rpcFloat)
		{
			variable->floatValue = decodeFloat(packet, position);
		}
		else if(type == RPCVariableType::rpcBoolean)
		{
			variable->booleanValue = decodeBoolean(packet, position);
		}
		else if(type == RPCVariableType::rpcArray)
		{
			variable->arrayValue = decodeArray(packet, position);
		}
		else if(type == RPCVariableType::rpcStruct)
		{
			variable->structValue = decodeStruct(packet, position);
		}
		return variable;
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
    return std::shared_ptr<RPCVariable>();
}

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeArray(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		uint32_t arrayLength = decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> array(new std::vector<std::shared_ptr<RPCVariable>>());
		for(uint32_t i = 0; i < arrayLength; i++)
		{
			array->push_back(decodeParameter(packet, position));
		}
		return array;
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
    return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>();
}

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeStruct(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		uint32_t structLength = decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct(new std::vector<std::shared_ptr<RPCVariable>>());
		for(uint32_t i = 0; i < structLength; i++)
		{
			std::string name = decodeString(packet, position);
			rpcStruct->push_back(decodeParameter(packet, position));
			rpcStruct->back()->name = name;
		}
		return rpcStruct;
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
    return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>();
}

} /* namespace RPC */
