/* Copyright 2013-2014 Sathya Laufer
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

std::shared_ptr<RPCHeader> RPCDecoder::decodeHeader(std::shared_ptr<std::vector<char>> packet)
{
	std::shared_ptr<RPCHeader> header(new RPCHeader());
	try
	{
		if(!(packet->size() < 12 || packet->at(3) & 0x40)) return header;
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
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return header;
}

std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> RPCDecoder::decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName)
{
	try
	{
		uint32_t position = 4;
		uint32_t headerSize = 0;
		if(packet->at(3) & 0x40) headerSize = _decoder->decodeInteger(packet, position) + 4;
		position = 8 + headerSize;
		methodName = _decoder->decodeString(packet, position);
		uint32_t parameterCount = _decoder->decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters(new std::vector<std::shared_ptr<RPCVariable>>());
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
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>();
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeResponse(std::shared_ptr<std::vector<char>> packet, uint32_t offset)
{
	uint32_t position = offset + 8;
	std::shared_ptr<RPCVariable> response = decodeParameter(packet, position);
	if(packet->size() < 4) return response; //response is Void when packet is empty.
	if(packet->at(3) == 0xFF)
	{
		response->errorStruct = true;
		if(response->structValue->find("faultCode") == response->structValue->end()) response->structValue->insert(RPC::RPCStructElement("faultCode", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(-1))));
		if(response->structValue->find("faultString") == response->structValue->end()) response->structValue->insert(RPC::RPCStructElement("faultString", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("undefined")))));
	}
	return response;
}

RPCVariableType RPCDecoder::decodeType(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	return (RPCVariableType)_decoder->decodeInteger(packet, position);
}

std::shared_ptr<RPCVariable> RPCDecoder::decodeParameter(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		RPCVariableType type = decodeType(packet, position);
		std::shared_ptr<RPCVariable> variable(new RPCVariable(type));
		if(type == RPCVariableType::rpcString || type == RPCVariableType::rpcBase64)
		{
			variable->stringValue = _decoder->decodeString(packet, position);
		}
		else if(type == RPCVariableType::rpcInteger)
		{
			variable->integerValue = _decoder->decodeInteger(packet, position);
		}
		else if(type == RPCVariableType::rpcFloat)
		{
			variable->floatValue = _decoder->decodeFloat(packet, position);
		}
		else if(type == RPCVariableType::rpcBoolean)
		{
			variable->booleanValue = _decoder->decodeBoolean(packet, position);
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPCVariable>();
}

std::shared_ptr<RPCArray> RPCDecoder::decodeArray(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		uint32_t arrayLength = _decoder->decodeInteger(packet, position);
		std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> array(new std::vector<std::shared_ptr<RPCVariable>>());
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
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>>();
}

std::shared_ptr<RPCStruct> RPCDecoder::decodeStruct(std::shared_ptr<std::vector<char>>& packet, uint32_t& position)
{
	try
	{
		uint32_t structLength = _decoder->decodeInteger(packet, position);
		std::shared_ptr<RPCStruct> rpcStruct(new RPCStruct());
		for(uint32_t i = 0; i < structLength; i++)
		{
			std::string name = _decoder->decodeString(packet, position);
			rpcStruct->insert(RPCStructElement(name, decodeParameter(packet, position)));
		}
		return rpcStruct;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<RPCStruct>();
}

}
}
