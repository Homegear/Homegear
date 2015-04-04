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

#include "WebSocket.h"
#include "../HelperFunctions/HelperFunctions.h"
#include <iostream>

namespace BaseLib
{
WebSocket::WebSocket()
{
	_content.reset(new std::vector<char>());
}

void WebSocket::setFinished()
{
	_finished = true;
}

void WebSocket::reset()
{
	_header = Header();
	_content.reset(new std::vector<char>());
	_finished = false;
	_dataProcessingStarted = false;
	_oldContentSize = 0;
}

void WebSocket::process(char* buffer, int32_t bufferLength)
{
	if(bufferLength <= 0 || _finished) return;
	if(!_header.parsed) processHeader(&buffer, bufferLength);
	if(_header.length == 0 || _header.rsv1 || _header.rsv2 || _header.rsv3 || (_header.opcode != Header::Opcode::continuation && _header.opcode != Header::Opcode::text  && _header.opcode != Header::Opcode::binary && _header.opcode != Header::Opcode::ping && _header.opcode != Header::Opcode::pong))
	{
		_header.close = true;
		_dataProcessingStarted = true;
		setFinished();
		return;
	}
	_dataProcessingStarted = true;
	processContent(buffer, bufferLength);
}

void WebSocket::processHeader(char** buffer, int32_t& bufferLength)
{
	if(bufferLength < 2) throw WebSocketException("Not enough data to process header");
	uint32_t lengthBytes = 0;
	_header.fin = (*buffer)[0] & 0x80;
	_header.rsv1 = (*buffer)[0] & 0x40;
	_header.rsv2 = (*buffer)[0] & 0x20;
	_header.rsv3 = (*buffer)[0] & 0x10;
	_header.opcode = (Header::Opcode::Enum)((*buffer)[0] & 0x0F);
	_header.hasMask = (*buffer)[1] & 0x80;
	(*buffer)[1] &= 0x7F;
	if((*buffer)[1] == 126) lengthBytes = 2;
	else if((*buffer)[1] == 127) lengthBytes = 8;
	else
	{
		lengthBytes = 0;
		_header.length = (uint8_t)(*buffer)[1];
	}
	uint32_t headerSize = 2 + lengthBytes + (_header.hasMask ? 4 : 0);
	if((unsigned)bufferLength < headerSize) throw WebSocketException("Not enough data to process header");
	if(lengthBytes == 2)
	{
		_header.length = (((uint32_t)(uint8_t)(*buffer)[2]) << 8) + (uint8_t)(*buffer)[3];
	}
	else if(lengthBytes == 8)
	{
		_header.length = (((uint64_t)(uint8_t)(*buffer)[2]) << 56) + (((uint64_t)(uint8_t)(*buffer)[3]) << 48) + (((uint64_t)(uint8_t)(*buffer)[4]) << 40) + (((uint64_t)(uint8_t)(*buffer)[5]) << 32) + (((uint64_t)(uint8_t)(*buffer)[6]) << 24) + (((uint64_t)(uint8_t)(*buffer)[7]) << 16) + (((uint64_t)(uint8_t)(*buffer)[8]) << 8) + (uint8_t)(*buffer)[9];
	}
	if(_header.hasMask)
	{
		_header.maskingKey.reserve(4);
		_header.maskingKey.push_back((*buffer)[2 + lengthBytes]);
		_header.maskingKey.push_back((*buffer)[2 + lengthBytes + 1]);
		_header.maskingKey.push_back((*buffer)[2 + lengthBytes + 2]);
		_header.maskingKey.push_back((*buffer)[2 + lengthBytes + 3]);
	}
	*buffer += headerSize;
	bufferLength -= headerSize;
	_header.parsed = true;
}

void WebSocket::processContent(char* buffer, int32_t bufferLength)
{
	uint32_t currentContentSize = _content->size() - _oldContentSize;
	if(currentContentSize + bufferLength > 10485760) throw WebSocketException("Data is larger than 10MiB.");
	if(currentContentSize + bufferLength > _header.length) bufferLength -= (currentContentSize + bufferLength) - _header.length;
	_content->insert(_content->end(), buffer, buffer + bufferLength);
	if(currentContentSize + bufferLength == _header.length)
	{
		if(_header.fin)
		{
			applyMask();
			_finished = true;
		}
		else
		{
			_oldContentSize = _content->size();
			_header.parsed = false;
		}
	}
}

void WebSocket::applyMask()
{
	if(!_header.hasMask) return;
	for(uint32_t i = 0; i < _content->size(); i++)
	{
		_content->operator [](i) ^= _header.maskingKey[i % 4];
	}
}

void WebSocket::encode(const std::vector<char>& data, Header::Opcode::Enum messageType, std::vector<char>& output)
{
	output.clear();
	int32_t lengthBytes = 0;
	if(data.size() < 126) lengthBytes = 0;
	else if(data.size() <= 0xFFFF) lengthBytes = 3;
	else lengthBytes = 9;
	output.reserve(2 + lengthBytes + data.size());
	if(messageType == Header::Opcode::continuation) output.push_back(0);
	else if(messageType == Header::Opcode::text) output.push_back(1);
	else if(messageType == Header::Opcode::binary) output.push_back(2);
	else if(messageType == Header::Opcode::close) output.push_back(8);
	else if(messageType == Header::Opcode::ping) output.push_back(9);
	else if(messageType == Header::Opcode::pong) output.push_back(10);
	else throw WebSocketException("Unknown message type.");

	if(messageType != Header::Opcode::continuation) output[0] |= 0x80;

	if(lengthBytes == 0) output.push_back(data.size());
	else if(lengthBytes == 3)
	{
		output.push_back(126);
		output.push_back(data.size() >> 8);
		output.push_back(data.size() & 0xFF);
	}
	else
	{
		output.push_back(127);
		output.push_back(((uint64_t)data.size()) >> 56);
		output.push_back((((uint64_t)data.size()) >> 48) & 0xFF);
		output.push_back((((uint64_t)data.size()) >> 40) & 0xFF);
		output.push_back((((uint64_t)data.size()) >> 32) & 0xFF);
		output.push_back((data.size() >> 24) & 0xFF);
		output.push_back((data.size() >> 16) & 0xFF);
		output.push_back((data.size() >> 8) & 0xFF);
		output.push_back(data.size() & 0xFF);
	}

	if(!data.empty()) output.insert(output.end(), data.begin(), data.end());
}

void WebSocket::encodeClose(std::vector<char>& output)
{
	output.clear();
	output.reserve(2);
	output.push_back(0x80 | (char)Header::Opcode::close);
	output.push_back(0);
}
}
