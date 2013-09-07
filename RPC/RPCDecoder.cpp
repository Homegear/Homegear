#include "RPCDecoder.h"
#include "../HelperFunctions.h"

namespace RPC
{

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName)
{
	try
	{
		uint32_t position = 0;
		methodName = _decoder.decodeString(packet, position);
		uint32_t parameterCount = _decoder.decodeInteger(packet, position);
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

RPCVariableType RPCDecoder::decodeType(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	return (RPCVariableType)_decoder.decodeInteger(packet, position);
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeParameter(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		RPCVariableType type = decodeType(packet, position);
		std::shared_ptr<RPCVariable> variable(new RPCVariable(type));
		if(type == RPCVariableType::rpcString || type == RPCVariableType::rpcBase64)
		{
			variable->stringValue = _decoder.decodeString(packet, position);
		}
		else if(type == RPCVariableType::rpcInteger)
		{
			variable->integerValue = _decoder.decodeInteger(packet, position);
		}
		else if(type == RPCVariableType::rpcFloat)
		{
			variable->floatValue = _decoder.decodeFloat(packet, position);
		}
		else if(type == RPCVariableType::rpcBoolean)
		{
			variable->booleanValue = _decoder.decodeBoolean(packet, position);
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
		uint32_t arrayLength = _decoder.decodeInteger(packet, position);
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
		uint32_t structLength = _decoder.decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> rpcStruct(new std::vector<std::shared_ptr<RPCVariable>>());
		for(uint32_t i = 0; i < structLength; i++)
		{
			std::string name = _decoder.decodeString(packet, position);
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
