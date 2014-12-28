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

#include "InsteonPacket.h"
#include "GD.h"

namespace Insteon
{
InsteonPacket::InsteonPacket()
{
}

InsteonPacket::InsteonPacket(std::string packet, std::string interfaceID, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	_interfaceID = interfaceID;
	import(packet);
}

InsteonPacket::InsteonPacket(std::vector<char>& packet, std::string interfaceID, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	_interfaceID = interfaceID;
	import(packet);
}

InsteonPacket::InsteonPacket(std::vector<uint8_t>& packet, std::string interfaceID, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	_interfaceID = interfaceID;
	import(packet);
}

InsteonPacket::InsteonPacket(uint8_t messageType, uint8_t messageSubtype, int32_t destinationAddress, uint8_t hopsLeft, uint8_t hopsMax, InsteonPacketFlags flags, std::vector<uint8_t> payload)
{
	_length = 9 + _payload.size();
	_hopsLeft = hopsLeft & 3;
	_hopsMax = hopsMax & 3;
	_flags = flags;
	_messageType = messageType;
	_messageSubtype = messageSubtype;
	_destinationAddress = destinationAddress;
	_payload = payload;
	_extended = !_payload.empty();
	if(_extended)
	{
		while(_payload.size() < 13) _payload.push_back(0);
		//Calculate checksum if not within payload already
		if(_payload.size() < 14)
		{
			uint8_t checksum = 0;
			checksum -= _messageType;
			checksum -= _messageSubtype;
			for(std::vector<uint8_t>::iterator i = _payload.begin(); i != _payload.end(); ++i)
			{
				checksum -= *i;
			}
			_payload.push_back(checksum);
		}
	}
}

InsteonPacket::~InsteonPacket()
{
}

void InsteonPacket::import(std::vector<char>& packet)
{
	try
	{
		if(packet.size() < 9) return;
		if(packet.size() > 200)
		{
			GD::out.printWarning("Warning: Tried to import Insteon packet larger than 200 bytes.");
			return;
		}
		_messageType = packet[7];
		_messageSubtype = packet[8];
		_flags = (InsteonPacketFlags)(packet[6] >> 5);
		_hopsLeft = (packet[6] >> 2) & 3;
		_hopsMax = packet[6] & 3;
		_senderAddress = (packet[0] << 16) + (packet[1] << 8) + packet[2];
		_destinationAddress = (packet[3] << 16) + (packet[4] << 8) + packet[5];
		_payload.clear();
		if(packet.size() == 9)
		{
			_length = packet.size();
		}
		else
		{
			_payload.insert(_payload.end(), packet.begin() + 9, packet.end());
			_length = 9 + _payload.size();
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonPacket::import(std::vector<uint8_t>& packet)
{
	try
	{
		if(packet.size() < 9) return;
		if(packet.size() > 200)
		{
			GD::out.printWarning("Warning: Tried to import Insteon packet larger than 200 bytes.");
			return;
		}
		_messageType = packet[7];
		_messageSubtype = packet[8];
		_flags = (InsteonPacketFlags)(packet[6] >> 5);
		_hopsLeft = (packet[6] >> 2) & 3;
		_hopsMax = packet[6] & 3;
		_senderAddress = (packet[0] << 16) + (packet[1] << 8) + packet[2];
		_destinationAddress = (packet[3] << 16) + (packet[4] << 8) + packet[5];
		_payload.clear();
		if(packet.size() == 9)
		{
			_length = packet.size();
		}
		else
		{
			_payload.insert(_payload.end(), packet.begin() + 9, packet.end());
			_length = 9 + _payload.size();
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonPacket::import(std::string packetHex)
{
	try
	{
		if(packetHex.size() % 2 != 0)
		{
			GD::out.printWarning("Warning: Packet has invalid size.");
			return;
		}
		std::vector<char> packet(GD::bl->hf.getBinary(packetHex));
		import(packet);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void InsteonPacket::calculateChecksum()
{
	if(_payload.empty() || _payload.size() == 14) return;
	while(_payload.size() < 14) _payload.push_back(0);
	//Calculate checksum if not within payload already
	uint8_t checksum = 0;
	checksum -= _messageType;
	checksum -= _messageSubtype;
	for(std::vector<uint8_t>::iterator i = _payload.begin(); i != _payload.end(); ++i)
	{
		checksum -= *i;
	}
	_payload.at(13) = checksum;
}

std::string InsteonPacket::hexString()
{
	try
	{
		if(_payload.size() > 200) return "";
		calculateChecksum();
		std::ostringstream stringStream;
		stringStream << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
		stringStream << std::setw(6) << _senderAddress;
		stringStream << std::setw(6) << _destinationAddress;
		stringStream << std::setw(2) << (int32_t)(((uint8_t)_flags << 5) + ((uint8_t)_extended << 4) + (_hopsLeft << 2) + _hopsMax);
		stringStream << std::setw(2) << (int32_t)_messageType;
		stringStream << std::setw(2) << (int32_t)_messageSubtype;
		std::for_each(_payload.begin(), _payload.end(), [&](uint8_t element) {stringStream << std::setw(2) << (int32_t)element;});
		return stringStream.str();
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return "";
}

std::vector<char> InsteonPacket::byteArray()
{
	try
	{
		std::vector<char> data;
		if(_payload.size() > 200) return data;
		calculateChecksum();
		data.push_back(_senderAddress >> 16);
		data.push_back((_senderAddress >> 8) & 0xFF);
		data.push_back(_senderAddress & 0xFF);
		data.push_back(_destinationAddress >> 16);
		data.push_back((_destinationAddress >> 8) & 0xFF);
		data.push_back(_destinationAddress & 0xFF);
		data.push_back(((uint8_t)_flags << 5) + ((uint8_t)_extended << 4) + (_hopsLeft << 2) + _hopsMax);
		data.push_back(_messageType);
		data.push_back(_messageSubtype);
		data.insert(data.end(), _payload.begin(), _payload.end());
		return data;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::vector<char>();
}


uint8_t InsteonPacket::getByte(std::string hexString)
{
	try
	{
		uint8_t value = 0;
		try	{ value = std::stoi(hexString, 0, 16); } catch(...) {}
		return value;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return 0;
}

int32_t InsteonPacket::getInt(std::string hexString)
{
	try
	{
		int32_t value = 0;
		try	{ value = std::stoll(hexString, 0, 16); } catch(...) {}
		return value;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return 0;
}

void InsteonPacket::setPosition(double index, double size, std::vector<uint8_t>& value)
{
	try
	{
		if(size < 0)
		{
			GD::out.printError("Error: Negative size not allowed.");
			return;
		}
		if(index < 9)
		{
			if(index < 7) GD::out.printError("Error: Tried to set packet index < 7.");
			if(size != 1.0 || std::floor(index) != index) GD::out.printError("Error: Only whole bytes are allowed for packet indexes < 9.");
			if(value.empty()) value.push_back(0);
			if(index == 7.0) _messageType = value.at(0);
			else if(index == 8.0) _messageSubtype = value.at(0);
			return;
		}
		_extended = true;
		index -= 9;
		double byteIndex = std::floor(index);
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(value.empty()) value.push_back(0);
			int32_t intByteIndex = byteIndex;
			if(size > 1.0)
			{
				GD::out.printError("Error: Can't set partial byte index > 1.");
				return;
			}
			while((signed)_payload.size() - 1 < intByteIndex)
			{
				_payload.push_back(0);
			}
			_payload.at(intByteIndex) |= value.at(value.size() - 1) << (std::lround(index * 10) % 10);
		}
		else
		{
			uint32_t intByteIndex = byteIndex;
			uint32_t bytes = (uint32_t)std::ceil(size);
			while(_payload.size() < intByteIndex + bytes)
			{
				_payload.push_back(0);
			}
			if(value.empty()) return;
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			//if(bytes > value.size()) bytes = value.size();
			if(bytes <= value.size())
			{
				_payload.at(intByteIndex) |= value.at(0) & _bitmask[bitSize];
				for(uint32_t i = 1; i < bytes; i++)
				{
					_payload.at(intByteIndex + i) |= value.at(i);
				}
			}
			else
			{
				uint32_t missingBytes = bytes - value.size();
				for(uint32_t i = 0; i < value.size(); i++)
				{
					_payload.at(intByteIndex + missingBytes + i) |= value.at(i);
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _length = 9 + _payload.size();
}

std::vector<uint8_t> InsteonPacket::getPosition(double index, double size, int32_t mask)
{
	std::vector<uint8_t> result;
	try
	{
		if(size < 0)
		{
			GD::out.printError("Error: Negative size not allowed.");
			result.push_back(0);
			return result;
		}
		if(index < 0)
		{
			GD::out.printError("Error: Packet index < 0 requested.");
			result.push_back(0);
			return result;
		}
		if(index < 9)
		{
			if(size > 1.0)
			{
				GD::out.printError("Error: Packet index < 9 and size > 1 requested.");
				result.push_back(0);
				return result;
			}

			uint32_t bitSize = std::lround(size * 10);
			if(bitSize > 8) bitSize = 8;
			uint32_t intIndex = std::lround(std::floor(index));
			if(intIndex == 0) result.push_back(((_senderAddress >> 16) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 1) result.push_back(((_senderAddress >> 8) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 2) result.push_back((_senderAddress >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 3) result.push_back(((_destinationAddress >> 16) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 4) result.push_back(((_destinationAddress >> 8) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 5) result.push_back((_destinationAddress >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 6)
			{
				if(index >= 6.5) result.push_back((uint8_t)_flags & _bitmask[bitSize]);
				else if(index >= 6.4) result.push_back((uint8_t)_extended);
				else if(index >= 6.2) result.push_back(_hopsLeft >> 2);
				else result.push_back(_hopsMax);
			}
			else if(intIndex == 7) result.push_back((_messageType >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 8) result.push_back((_messageSubtype >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			return result;
		}
		index -= 9;
		double byteIndex = std::floor(index);
		int32_t intByteIndex = byteIndex;
		if(byteIndex >= _payload.size())
		{
			result.push_back(0);
			return result;
		}
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(size > 1)
			{
				GD::out.printError("Error: Partial byte index > 1 requested.");
				result.push_back(0);
				return result;
			}
			//The round is necessary, because for example (uint32_t)(0.2 * 10) is 1
			uint32_t bitSize = std::lround(size * 10);
			if(bitSize > 8) bitSize = 8;
			result.push_back((_payload.at(intByteIndex) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
		}
		else
		{
			uint32_t bytes = (uint32_t)std::ceil(size);
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			uint8_t currentByte = _payload.at(intByteIndex) & _bitmask[bitSize];
			if(mask != -1 && bytes <= 4) currentByte &= (mask >> ((bytes - 1) * 8));
			result.push_back(currentByte);
			for(uint32_t i = 1; i < bytes; i++)
			{
				if((intByteIndex + i) >= _payload.size()) result.push_back(0);
				else
				{
					currentByte = _payload.at(intByteIndex + i);
					if(mask != -1 && bytes <= 4) currentByte &= (mask >> ((bytes - i - 1) * 8));
					result.push_back(currentByte);
				}
			}
		}
		if(result.empty()) result.push_back(0);
		return result;
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    result.push_back(0);
    return result;
}

bool InsteonPacket::equals(std::shared_ptr<InsteonPacket>& rhs)
{
	if(_messageType != rhs->messageType()) return false;
	if(_messageSubtype != rhs->messageSubtype()) return false;
	if(_payload.size() != rhs->payload()->size()) return false;
	if(_senderAddress != rhs->senderAddress()) return false;
	if(_destinationAddress != rhs->destinationAddress()) return false;
	if(_flags != rhs->flags()) return false;
	if(_extended != rhs->extended()) return false;
	if(_hopsMax != rhs->hopsMax()) return false;
	if(_payload == (*rhs->payload())) return true;
	return false;
}
}
