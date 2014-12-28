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

#include "MAXPacket.h"
#include "GD.h"

namespace MAX
{
MAXPacket::MAXPacket()
{

}

MAXPacket::MAXPacket(std::vector<uint8_t>& packet, bool rssiByte, int64_t timeReceived)
{
	_timeReceived = timeReceived;
	import(packet, rssiByte);
}

MAXPacket::MAXPacket(std::string packet, int64_t timeReceived)
{
	_timeReceived = timeReceived;
    import(packet);
}

MAXPacket::MAXPacket(uint8_t messageCounter, uint8_t messageType, uint8_t messageSubtype, int32_t senderAddress, int32_t destinationAddress, std::vector<uint8_t> payload, bool burst)
{
    _length = 9 + _payload.size();
    _messageCounter = messageCounter;
    _messageType = messageType;
    _messageSubtype = messageSubtype;
    _senderAddress = senderAddress;
    _destinationAddress = destinationAddress;
    _payload = payload;
    _burst = burst;
}

MAXPacket::~MAXPacket()
{
}

void MAXPacket::import(std::vector<uint8_t>& packet, bool rssiByte)
{
	try
	{
		if(packet.size() < 10) return;
		if(packet.size() > 200)
		{
			GD::out.printWarning("Warning: Tried to import MAX packet larger than 200 bytes.");
			return;
		}
		_messageCounter = packet[1];
		_messageSubtype = packet[2];
		_messageType = packet[3];
		_senderAddress = (packet[4] << 16) + (packet[5] << 8) + packet[6];
		_destinationAddress = (packet[7] << 16) + (packet[8] << 8) + packet[9];
		_payload.clear();
		if(packet.size() == 10)
		{
			_length = packet.size();
		}
		else
		{
			if(rssiByte)
			{
				_payload.insert(_payload.end(), packet.begin() + 10, packet.end() - 1);
				int32_t rssiDevice = packet.back();
				//1) Read the RSSI status register
				//2) Convert the reading from a hexadecimal
				//number to a decimal number (RSSI_dec)
				//3) If RSSI_dec ≥ 128 then RSSI_dBm =
				//(RSSI_dec - 256)/2 – RSSI_offset
				//4) Else if RSSI_dec < 128 then RSSI_dBm =
				//(RSSI_dec)/2 – RSSI_offset
				if(rssiDevice >= 128) rssiDevice = ((rssiDevice - 256) / 2) - 74;
				else rssiDevice = (rssiDevice / 2) - 74;
				_rssiDevice = rssiDevice * -1;
			}
			else _payload.insert(_payload.end(), packet.begin() + 10, packet.end());
			_length = 9 + _payload.size();
		}
		if(_length != packet[0])
		{
			GD::out.printWarning("Warning: Packet with wrong length byte received.");
			return;
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

void MAXPacket::import(std::string& packet, bool removeFirstCharacter)
{
	try
	{
		uint32_t startIndex = removeFirstCharacter ? 1 : 0;
		if(packet.size() < startIndex + 20)
		{
			GD::out.printError("Error: Packet is too short: " + packet);
			return;
		}
		if(packet.size() > 400)
		{
			GD::out.printWarning("Warning: Tried to import BidCoS packet larger than 200 bytes.");
			return;
		}
		_length = getByte(packet.substr(startIndex, 2));
		_messageCounter = getByte(packet.substr(startIndex + 2, 2));
		_messageSubtype = getByte(packet.substr(startIndex + 4, 2));
		_messageType = getByte(packet.substr(startIndex + 6, 2));
		_senderAddress = getInt(packet.substr(startIndex + 8, 6));
		_destinationAddress = getInt(packet.substr(startIndex + 14, 6));

		uint32_t tailLength = 0;
		if(packet.back() == '\n') tailLength = 2;
		uint32_t endIndex = startIndex + 2 + (_length * 2) - 1;
		if(endIndex >= packet.size())
		{
			GD::out.printWarning("Warning: Packet is shorter than value of packet length byte: " + packet);
			endIndex = packet.size() - 1;
		}
		_payload.clear();
		uint32_t i;
		for(i = startIndex + 20; i < endIndex; i+=2)
		{
			_payload.push_back(getByte(packet.substr(i, 2)));
		}
		if(i < packet.size() - tailLength)
		{
			int32_t rssiDevice = getByte(packet.substr(i, 2));
			//1) Read the RSSI status register
			//2) Convert the reading from a hexadecimal
			//number to a decimal number (RSSI_dec)
			//3) If RSSI_dec ≥ 128 then RSSI_dBm =
			//(RSSI_dec - 256)/2 – RSSI_offset
			//4) Else if RSSI_dec < 128 then RSSI_dBm =
			//(RSSI_dec)/2 – RSSI_offset
			if(rssiDevice >= 128) rssiDevice = ((rssiDevice - 256) / 2) - 74;
			else rssiDevice = (rssiDevice / 2) - 74;
			_rssiDevice = rssiDevice * -1;
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

std::string MAXPacket::hexString()
{
	try
	{
		if(_payload.size() > 200) return "";
		std::ostringstream stringStream;
		stringStream << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
		stringStream << std::setw(2) << (9 + _payload.size());
		stringStream << std::setw(2) << (int32_t)_messageCounter;
		stringStream << std::setw(2) << (int32_t)_messageSubtype;
		stringStream << std::setw(2) << (int32_t)_messageType;
		stringStream << std::setw(6) << _senderAddress;
		stringStream << std::setw(6) << _destinationAddress;
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

std::vector<uint8_t> MAXPacket::byteArray()
{
	try
	{
		std::vector<uint8_t> data;
		if(_payload.size() > 200) return data;
		data.push_back(9 + _payload.size());
		data.push_back(_messageCounter);
		data.push_back(_messageSubtype);
		data.push_back(_messageType);
		data.push_back(_senderAddress >> 16);
		data.push_back((_senderAddress >> 8) & 0xFF);
		data.push_back(_senderAddress & 0xFF);
		data.push_back(_destinationAddress >> 16);
		data.push_back((_destinationAddress >> 8) & 0xFF);
		data.push_back(_destinationAddress & 0xFF);
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
    return std::vector<uint8_t>();
}

uint8_t MAXPacket::getByte(std::string hexString)
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

int32_t MAXPacket::getInt(std::string hexString)
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

void MAXPacket::setPosition(double index, double size, std::vector<uint8_t>& value)
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
			GD::out.printError("Error: Packet index < 9 requested.");
			return;
		}
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

std::vector<uint8_t> MAXPacket::getPosition(double index, double size, int32_t mask)
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
			if(size > 0.8)
			{
				GD::out.printError("Error: Packet index < 9 and size > 1 requested.");
				result.push_back(0);
				return result;
			}

			uint32_t bitSize = std::lround(size * 10);
			uint32_t intIndex = std::lround(std::floor(index));
			if(intIndex == 0) result.push_back((_messageCounter >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 1) result.push_back((_messageSubtype >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 2) result.push_back((_messageType >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 3) result.push_back(((_senderAddress >> 16) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 4) result.push_back(((_senderAddress >> 8) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 5) result.push_back((_senderAddress >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 6) result.push_back(((_destinationAddress >> 16) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 7) result.push_back(((_destinationAddress >> 8) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 8) result.push_back((_destinationAddress >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
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

bool MAXPacket::equals(std::shared_ptr<MAXPacket>& rhs)
{
	if(_messageCounter != rhs->messageCounter()) return false;
	if(_messageType != rhs->messageType()) return false;
	if(_messageSubtype != rhs->messageSubtype()) return false;
	if(_payload.size() != rhs->payload()->size()) return false;
	if(_senderAddress != rhs->senderAddress()) return false;
	if(_destinationAddress != rhs->destinationAddress()) return false;
	if(_payload == (*rhs->payload())) return true;
	return false;
}
}
