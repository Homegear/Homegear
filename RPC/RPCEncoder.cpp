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
		else if(variable->type == RPCVariableType::rpcBase64)
		{
			encodeBase64(packet, variable);
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
		_encoder.encodeInteger(packet, variable->structValue->size());
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			_encoder.encodeString(packet, (*i)->name);
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
		_encoder.encodeInteger(packet, variable->arrayValue->size());
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
	_encoder.encodeInteger(packet, (int32_t)type);
}

void RPCEncoder::encodeInteger(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	encodeType(packet, RPCVariableType::rpcInteger);
	_encoder.encodeInteger(packet, variable->integerValue);
}

void RPCEncoder::encodeFloat(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcFloat);
		_encoder.encodeFloat(packet, variable->floatValue);
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
	_encoder.encodeBoolean(packet, variable->booleanValue);
}

void RPCEncoder::encodeString(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcString);
		//We could call encodeRawString here, but then the string would have to be copied and that would cost time.
		_encoder.encodeInteger(packet, variable->stringValue.size());
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

void RPCEncoder::encodeBase64(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcBase64);
		//We could call encodeRawString here, but then the string would have to be copied and that would cost time.
		_encoder.encodeInteger(packet, variable->stringValue.size());
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

void RPCEncoder::encodeVoid(std::shared_ptr<std::vector<char>>& packet)
{
	std::shared_ptr<RPCVariable> string(new RPCVariable(RPCVariableType::rpcString));
	encodeString(packet, string);
}

} /* namespace RPC */
