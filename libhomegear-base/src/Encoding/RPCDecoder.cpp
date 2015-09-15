/* Copyright 2013-2015 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "RPCDecoder.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace RPC
{

RPCDecoder::RPCDecoder(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
	_decoder = std::unique_ptr<BinaryDecoder>(new BinaryDecoder(baseLib));
}

std::shared_ptr<RPCHeader> RPCDecoder::decodeHeader(std::vector<char>& packet)
{
	std::shared_ptr<RPCHeader> header(new RPCHeader());
	try
	{
		if(!(packet.size() < 12 || (packet.at(3) & 0x40))) return header;
		uint32_t position = 4;
		uint32_t headerSize = 0;
		headerSize = _decoder->decodeInteger(packet, position);
		if(headerSize < 4) return header;
		uint32_t parameterCount = _decoder->decodeInteger(packet, position);
		for(uint32_t i = 0; i < parameterCount; i++)
		{
			std::string field = _decoder->decodeString(packet, position);
			HelperFunctions::toLower(field);
			std::string value = _decoder->decodeString(packet, position);
			if(field == "authorization") header->authorization = value;
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return header;
}

std::shared_ptr<RPCHeader> RPCDecoder::decodeHeader(std::vector<uint8_t>& packet)
{
	std::shared_ptr<RPCHeader> header(new RPCHeader());
	try
	{
		if(!(packet.size() < 12 || (packet.at(3) & 0x40))) return header;
		uint32_t position = 4;
		uint32_t headerSize = 0;
		headerSize = _decoder->decodeInteger(packet, position);
		if(headerSize < 4) return header;
		uint32_t parameterCount = _decoder->decodeInteger(packet, position);
		for(uint32_t i = 0; i < parameterCount; i++)
		{
			std::string field = _decoder->decodeString(packet, position);
			HelperFunctions::toLower(field);
			std::string value = _decoder->decodeString(packet, position);
			if(field == "authorization") header->authorization = value;
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return header;
}

std::shared_ptr<std::vector<std::shared_ptr<Variable>>> RPCDecoder::decodeRequest(std::vector<char>& packet, std::string& methodName)
{
	try
	{
		uint32_t position = 4;
		uint32_t headerSize = 0;
		if(packet.at(3) & 0x40) headerSize = _decoder->decodeInteger(packet, position) + 4;
		position = 8 + headerSize;
		methodName = _decoder->decodeString(packet, position);
		uint32_t parameterCount = _decoder->decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<Variable>>> parameters(new std::vector<std::shared_ptr<Variable>>());
		if(parameterCount > 100)
		{
			_bl->out.printError("Parameter count of RPC request is larger than 100.");
			return parameters;
		}
		for(uint32_t i = 0; i < parameterCount; i++)
		{
			parameters->push_back(decodeParameter(packet, position));
		}
		return parameters;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<Variable>>>();
}

std::shared_ptr<std::vector<std::shared_ptr<Variable>>> RPCDecoder::decodeRequest(std::vector<uint8_t>& packet, std::string& methodName)
{
	try
	{
		uint32_t position = 4;
		uint32_t headerSize = 0;
		if(packet.at(3) & 0x40) headerSize = _decoder->decodeInteger(packet, position) + 4;
		position = 8 + headerSize;
		methodName = _decoder->decodeString(packet, position);
		uint32_t parameterCount = _decoder->decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<Variable>>> parameters(new std::vector<std::shared_ptr<Variable>>());
		if(parameterCount > 100)
		{
			_bl->out.printError("Parameter count of RPC request is larger than 100.");
			return parameters;
		}
		for(uint32_t i = 0; i < parameterCount; i++)
		{
			parameters->push_back(decodeParameter(packet, position));
		}
		return parameters;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<Variable>>>();
}

std::shared_ptr<Variable> RPCDecoder::decodeResponse(std::vector<char>& packet, uint32_t offset)
{
	uint32_t position = offset + 8;
	std::shared_ptr<Variable> response = decodeParameter(packet, position);
	if(packet.size() < 4) return response; //response is Void when packet is empty.
	if(packet.at(3) == 0xFF)
	{
		response->errorStruct = true;
		if(response->structValue->find("faultCode") == response->structValue->end()) response->structValue->insert(StructElement("faultCode", PVariable(new Variable(-1))));
		if(response->structValue->find("faultString") == response->structValue->end()) response->structValue->insert(StructElement("faultString", PVariable(new Variable(std::string("undefined")))));
	}
	return response;
}

std::shared_ptr<Variable> RPCDecoder::decodeResponse(std::vector<uint8_t>& packet, uint32_t offset)
{
	uint32_t position = offset + 8;
	std::shared_ptr<Variable> response = decodeParameter(packet, position);
	if(packet.size() < 4) return response; //response is Void when packet is empty.
	if(packet.at(3) == 0xFF)
	{
		response->errorStruct = true;
		if(response->structValue->find("faultCode") == response->structValue->end()) response->structValue->insert(StructElement("faultCode", PVariable(new Variable(-1))));
		if(response->structValue->find("faultString") == response->structValue->end()) response->structValue->insert(StructElement("faultString", PVariable(new Variable(std::string("undefined")))));
	}
	return response;
}

void RPCDecoder::decodeResponse(PVariable& variable, uint32_t offset)
{
	uint32_t position = offset + 8;
	decodeParameter(variable, position);
	if(variable->binaryValue.size() < 4) return; //response is Void when packet is empty.
	if(variable->binaryValue.at(3) == 0xFF)
	{
		variable->errorStruct = true;
		if(variable->structValue->find("faultCode") == variable->structValue->end()) variable->structValue->insert(StructElement("faultCode", PVariable(new Variable(-1))));
		if(variable->structValue->find("faultString") == variable->structValue->end()) variable->structValue->insert(StructElement("faultString", PVariable(new Variable(std::string("undefined")))));
	}
}

VariableType RPCDecoder::decodeType(std::vector<char>& packet, uint32_t& position)
{
	return (VariableType)_decoder->decodeInteger(packet, position);
}

VariableType RPCDecoder::decodeType(std::vector<uint8_t>& packet, uint32_t& position)
{
	return (VariableType)_decoder->decodeInteger(packet, position);
}

std::shared_ptr<Variable> RPCDecoder::decodeParameter(std::vector<char>& packet, uint32_t& position)
{
	try
	{
		VariableType type = decodeType(packet, position);
		std::shared_ptr<Variable> variable(new Variable(type));
		if(type == VariableType::tString || type == VariableType::tBase64)
		{
			variable->stringValue = _decoder->decodeString(packet, position);
		}
		else if(type == VariableType::tInteger)
		{
			variable->integerValue = _decoder->decodeInteger(packet, position);
		}
		else if(type == VariableType::tFloat)
		{
			variable->floatValue = _decoder->decodeFloat(packet, position);
		}
		else if(type == VariableType::tBoolean)
		{
			variable->booleanValue = _decoder->decodeBoolean(packet, position);
		}
		else if(type == VariableType::tArray)
		{
			variable->arrayValue = decodeArray(packet, position);
		}
		else if(type == VariableType::tStruct)
		{
			variable->structValue = decodeStruct(packet, position);
		}
		return variable;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<Variable>();
}

std::shared_ptr<Variable> RPCDecoder::decodeParameter(std::vector<uint8_t>& packet, uint32_t& position)
{
	try
	{
		VariableType type = decodeType(packet, position);
		std::shared_ptr<Variable> variable(new Variable(type));
		if(type == VariableType::tString || type == VariableType::tBase64)
		{
			variable->stringValue = _decoder->decodeString(packet, position);
		}
		else if(type == VariableType::tInteger)
		{
			variable->integerValue = _decoder->decodeInteger(packet, position);
		}
		else if(type == VariableType::tFloat)
		{
			variable->floatValue = _decoder->decodeFloat(packet, position);
		}
		else if(type == VariableType::tBoolean)
		{
			variable->booleanValue = _decoder->decodeBoolean(packet, position);
		}
		else if(type == VariableType::tArray)
		{
			variable->arrayValue = decodeArray(packet, position);
		}
		else if(type == VariableType::tStruct)
		{
			variable->structValue = decodeStruct(packet, position);
		}
		return variable;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<Variable>();
}

void RPCDecoder::decodeParameter(PVariable& variable, uint32_t& position)
{
	try
	{
		variable->type = decodeType(variable->binaryValue, position);
		if(variable->type == VariableType::tString || variable->type == VariableType::tBase64)
		{
			variable->stringValue = _decoder->decodeString(variable->binaryValue, position);
		}
		else if(variable->type == VariableType::tInteger)
		{
			variable->integerValue = _decoder->decodeInteger(variable->binaryValue, position);
		}
		else if(variable->type == VariableType::tFloat)
		{
			variable->floatValue = _decoder->decodeFloat(variable->binaryValue, position);
		}
		else if(variable->type == VariableType::tBoolean)
		{
			variable->booleanValue = _decoder->decodeBoolean(variable->binaryValue, position);
		}
		else if(variable->type == VariableType::tArray)
		{
			variable->arrayValue = decodeArray(variable->binaryValue, position);
		}
		else if(variable->type == VariableType::tStruct)
		{
			variable->structValue = decodeStruct(variable->binaryValue, position);
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

PArray RPCDecoder::decodeArray(std::vector<char>& packet, uint32_t& position)
{
	try
	{
		uint32_t arrayLength = _decoder->decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<Variable>>> array(new std::vector<std::shared_ptr<Variable>>());
		for(uint32_t i = 0; i < arrayLength; i++)
		{
			array->push_back(decodeParameter(packet, position));
		}
		return array;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<Variable>>>();
}

PArray RPCDecoder::decodeArray(std::vector<uint8_t>& packet, uint32_t& position)
{
	try
	{
		uint32_t arrayLength = _decoder->decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<Variable>>> array(new std::vector<std::shared_ptr<Variable>>());
		for(uint32_t i = 0; i < arrayLength; i++)
		{
			array->push_back(decodeParameter(packet, position));
		}
		return array;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<Variable>>>();
}

PStruct RPCDecoder::decodeStruct(std::vector<char>& packet, uint32_t& position)
{
	try
	{
		uint32_t structLength = _decoder->decodeInteger(packet, position);
		PStruct rpcStruct(new Struct());
		for(uint32_t i = 0; i < structLength; i++)
		{
			std::string name = _decoder->decodeString(packet, position);
			rpcStruct->insert(StructElement(name, decodeParameter(packet, position)));
		}
		return rpcStruct;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return PStruct();
}

PStruct RPCDecoder::decodeStruct(std::vector<uint8_t>& packet, uint32_t& position)
{
	try
	{
		uint32_t structLength = _decoder->decodeInteger(packet, position);
		PStruct rpcStruct(new Struct());
		for(uint32_t i = 0; i < structLength; i++)
		{
			std::string name = _decoder->decodeString(packet, position);
			rpcStruct->insert(StructElement(name, decodeParameter(packet, position)));
		}
		return rpcStruct;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return PStruct();
}

}
}
