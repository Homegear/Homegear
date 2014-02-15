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

#include "RPCEncoder.h"

namespace RPC
{
RPCEncoder::RPCEncoder()
{
	strncpy(&_packetStartRequest[0], "Bin", 3);
	_packetStartResponse[3] = 0;
	strncpy(&_packetStartResponse[0], "Bin", 3);
	_packetStartResponse[3] = 1;
	strncpy(&_packetStartError[0], "Bin", 3);
	_packetStartError[3] = 0xFF;
}

std::shared_ptr<std::vector<char>> RPCEncoder::encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters, std::shared_ptr<RPCHeader> header)
{
	//The "Bin", the type byte after that and the length itself are not part of the length
	std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
	try
	{
		packet->insert(packet->begin(), _packetStartRequest, _packetStartRequest + 4);
		uint32_t headerSize = 0;
		if(header)
		{
			packet->at(3) |= 0x40;
			headerSize = encodeHeader(packet, header) + 4;
		}
		_encoder.encodeString(packet, methodName);
		if(!parameters) _encoder.encodeInteger(packet, 0);
		else _encoder.encodeInteger(packet, parameters->size());
		if(parameters)
		{
			for(std::list<std::shared_ptr<RPCVariable>>::iterator i = parameters->begin(); i != parameters->end(); ++i)
			{
				encodeVariable(packet, (*i));
			}
		}

		uint32_t dataSize = packet->size() - 4 - headerSize;
		char result[4];
		HelperFunctions::memcpyBigEndian(result, (char*)&dataSize, 4);
		packet->insert(packet->begin() + 4 + headerSize, result, result + 4);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return packet;
}

std::shared_ptr<std::vector<char>> RPCEncoder::encodeResponse(std::shared_ptr<RPCVariable> variable)
{
	//The "Bin", the type byte after that and the length itself are not part of the length
	std::shared_ptr<std::vector<char>> packet(new std::vector<char>());
	try
	{
		if(!variable) return packet;
		if(variable->errorStruct) packet->insert(packet->begin(), _packetStartError, _packetStartError + 4);
		else packet->insert(packet->begin(), _packetStartResponse, _packetStartResponse + 4);

		encodeVariable(packet, variable);

		uint32_t dataSize = packet->size() - 4;
		char result[4];
		HelperFunctions::memcpyBigEndian(result, (char*)&dataSize, 4);
		packet->insert(packet->begin() + 4, result, result + 4);

		return packet;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return packet;
}

void RPCEncoder::insertHeader(std::shared_ptr<std::vector<char>> packet, std::shared_ptr<RPCHeader> header)
{
	std::shared_ptr<std::vector<char>> headerData(new std::vector<char>());
	uint32_t headerSize = encodeHeader(headerData, header);
	if(headerSize > 0)
	{
		packet->at(3) |= 0x40;
		packet->insert(packet->begin() + 4, headerData->begin(), headerData->end());
	}
}

uint32_t RPCEncoder::encodeHeader(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCHeader>& header)
{
	uint32_t oldPacketSize = packet->size();
	uint32_t parameterCount = 0;
	if(!header->authorization.empty())
	{
		parameterCount++;
		std::string temp("Authorization");
		_encoder.encodeString(packet, temp);
		_encoder.encodeString(packet, header->authorization);
	}
	char result[4];
	HelperFunctions::memcpyBigEndian(result, (char*)&parameterCount, 4);
	packet->insert(packet->begin() + oldPacketSize, result, result + 4);

	uint32_t headerSize = packet->size() - oldPacketSize;
	HelperFunctions::memcpyBigEndian(result, (char*)&headerSize, 4);
	packet->insert(packet->begin() + oldPacketSize, result, result + 4);
	return headerSize;
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeStruct(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable)
{
	try
	{
		encodeType(packet, RPCVariableType::rpcStruct);
		_encoder.encodeInteger(packet, variable->structValue->size());
		for(RPCStruct::iterator i = variable->structValue->begin(); i != variable->structValue->end(); ++i)
		{
			if(i->first.empty() || !i->second) continue;
			std::string name = i->first;
			_encoder.encodeString(packet, name);
			encodeVariable(packet, i->second);
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCEncoder::encodeVoid(std::shared_ptr<std::vector<char>>& packet)
{
	std::shared_ptr<RPCVariable> string(new RPCVariable(RPCVariableType::rpcString));
	encodeString(packet, string);
}

} /* namespace RPC */
