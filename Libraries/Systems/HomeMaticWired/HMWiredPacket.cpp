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

#include "HMWiredPacket.h"

namespace HMWired
{
std::map<uint16_t, uint16_t> CRC16::_crcTable;

void CRC16::init()
{
	if(_crcTable.empty()) initCRCTable();
}

void CRC16::initCRCTable()
{
	uint32_t bit, crc;

	for (uint32_t i = 0; i < 256; i++)
	{
		crc = i << 8;

		for (uint32_t j = 0; j < 8; j++) {

			bit = crc & 0x8000;
			crc <<= 1;
			if(bit) crc ^= 0x1002;
		}

		crc &= 0xFFFF;
		_crcTable[i]= crc;
	}
}

uint16_t CRC16::calculate(std::vector<uint8_t>& data)
{
	uint16_t crc = 0xf1e2;
	for(int32_t i = 0; i < data.size(); i++)
	{
		crc = (crc << 8) ^ _crcTable[((crc >> 8) & 0xff) ^ data[i]];
	}

    return crc;
}

HMWiredPacket::HMWiredPacket()
{
	init();
}

HMWiredPacket::HMWiredPacket(std::string packet, int64_t timeReceived)
{
	init();
	_timeReceived = timeReceived;
	import(packet);
}

HMWiredPacket::HMWiredPacket(std::vector<uint8_t>& packet, int64_t timeReceived)
{
	init();
	_timeReceived = timeReceived;
	import(packet);
}

HMWiredPacket::HMWiredPacket(HMWiredPacketType type, int32_t senderAddress, int32_t destinationAddress, bool synchronizationBit, uint8_t senderMessageCounter, uint8_t receiverMessageCounter, uint8_t addressMask, std::vector<uint8_t>& payload)
{
	init();
	reset();
	_type = type;
	_senderAddress = senderAddress;
	_destinationAddress = destinationAddress;
	_synchronizationBit = synchronizationBit;
	_senderMessageCounter = senderMessageCounter;
	_receiverMessageCounter = receiverMessageCounter;
	_addressMask = addressMask;
	_payload = payload;
	generateControlByte();
}

HMWiredPacket::~HMWiredPacket()
{
}

void HMWiredPacket::init()
{
	CRC16::init();
}

void HMWiredPacket::reset()
{
	_packet.clear();
	_escapedPacket.clear();
	_type = HMWiredPacketType::none;
	_checksum = 0;
	_addressMask = 0;
	_senderMessageCounter = 0;
	_receiverMessageCounter = 0;
	_senderAddress = 0;
	_destinationAddress = 0;
	_payload.clear();
	_synchronizationBit = false;
}

//Import expects non escaped packet with CRC16
void HMWiredPacket::import(std::vector<uint8_t>& packet)
{
	try
	{
		reset();

		if(packet.empty()) return;
		_packet = packet;
		if(packet.at(0) == 0xFD)
		{
			if(packet.size() > 9)
			{
				_length = packet[10]; //Frame length
				if(packet.size() != _length + 11 && packet.size() != _length + 9)
				{
					reset();
					Output::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
					return;
				}
				_controlByte = packet[5];
				_type = ((_controlByte & 1) == 1) ? HMWiredPacketType::ackMessage : HMWiredPacketType::iMessage;
				if(_type == HMWiredPacketType::iMessage)
				{
					_senderMessageCounter = (_controlByte >> 1) & 3;
					_synchronizationBit = (bool)(_controlByte & 0x80);
				}
				_receiverMessageCounter = (_controlByte >> 5) & 3;

				if(_controlByte & 8) _senderAddress = (packet[6] << 24) + (packet[7] << 16) + (packet[8] << 8) + packet[9];
				_destinationAddress = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
				if(_length >= 2)
				{
					if(_packet.size() > 11)  _payload.insert(_payload.end(), packet.begin() + 11, packet.end() - 2);
					if(packet.size() == _length + 11)
					{
						_checksum = (packet.at(packet.size() - 2) << 8) + packet.at(packet.size() - 1);
						packet.erase(packet.end() - 2, packet.end());
						if(CRC16::calculate(packet) != _checksum)
						{
							reset();
							Output::printError("CRC for HomeMatic Wired packet failed: " + HelperFunctions::getHexString(packet) + HelperFunctions::getHexString(_checksum, 4));
							return;
						}
					}
					else
					{
						_checksum = CRC16::calculate(packet);
						_packet.push_back(_checksum >> 8);
						_packet.push_back(_checksum & 0xFF);
					}
				}
			}
			else if(packet.size() == 9 || packet.size() == 7)
			{
				_type = HMWiredPacketType::discovery;
				_controlByte = packet[5];
				if(!(_controlByte & 3))
				{
					reset();
					Output::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
					return;
				}
				_destinationAddress = (packet[1] << 24) + (packet[2] << 16) + (packet[3] << 8) + packet[4];
				_addressMask = packet[5] >> 3;
				_length = packet[6];
				if(_length == 2)
				{
					if(packet.size() == _length + 7)
					{
						_checksum = (packet.at(packet.size() - 2) << 8) + packet.at(packet.size() - 1);
						packet.erase(packet.end() - 2, packet.end());
						if(CRC16::calculate(packet) != _checksum)
						{
							reset();
							Output::printError("CRC for HomeMatic Wired packet failed: " + HelperFunctions::getHexString(packet) + HelperFunctions::getHexString(_checksum, 4));
							return;
						}
					}
					else
					{
						_checksum = CRC16::calculate(packet);
						_packet.push_back(_checksum >> 8);
						_packet.push_back(_checksum & 0xFF);
					}
				}
			}
			else
			{
				reset();
				Output::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
				return;
			}
		}
		else if(packet.at(0) == 0xFE && packet.size() > 3)
		{
			_type = HMWiredPacketType::system;
			_destinationAddress = packet[1];
			_controlByte = packet[2];
			if((_controlByte & 0x1F) != 0x11) //ACK message
			{
				reset();
				Output::printError("HomeMatic Wired system packet has unknown type: " + HelperFunctions::getHexString(packet));
				return;
			}
			_receiverMessageCounter = (_controlByte >> 5) & 3;
			_length = packet[3];
			if(_length == 2)
			{
				if(packet.size() == _length + 4)
				{
					_checksum = (packet.at(packet.size() - 2) << 8) + packet.at(packet.size() - 1);
					packet.erase(packet.end() - 2, packet.end());
					if(CRC16::calculate(packet) != _checksum)
					{
						reset();
						Output::printError("CRC for HomeMatic Wired packet failed: " + HelperFunctions::getHexString(packet) + HelperFunctions::getHexString(_checksum, 4));
						return;
					}
				}
				else
				{
					_checksum = CRC16::calculate(packet);
					_packet.push_back(_checksum >> 8);
					_packet.push_back(_checksum & 0xFF);
				}
			}
		}
		else if(packet.at(0) == 0xF8 && packet.size() == 1) _type = HMWiredPacketType::discoveryResponse;
		else
		{
			reset();
			Output::printError("HomeMatic Wired packet has unknown type: " + HelperFunctions::getHexString(packet));
			return;
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

void HMWiredPacket::import(std::string packetHex)
{
	try
	{
		std::vector<uint8_t> packet(HelperFunctions::getUBinary(packetHex));
		import(packet);
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

void HMWiredPacket::escapePacket()
{
	try
	{
		_escapedPacket.clear();
		if(_packet.empty()) return;
		_escapedPacket.push_back(_packet[0]);
		for(uint32_t i = 1; i < _packet.size(); i++)
		{
			if(_packet[i] == 0xFC || _packet[i] == 0xFD || _packet[i] == 0xFE)
			{
				_escapedPacket.push_back(0xFC);
				_escapedPacket.push_back(_packet[i] & 0x7F);
			}
			else _escapedPacket.push_back(_packet[i]);
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

void HMWiredPacket::generateControlByte()
{
	try
	{
		if(_type == HMWiredPacketType::iMessage)
		{
			_controlByte = 0x10; //F bit
			if(_synchronizationBit) _controlByte |= 0x80;
			_controlByte |= ((_receiverMessageCounter & 3) << 5);
			_controlByte |= 0x08; //Packet must contain sender address
			_controlByte |= ((_senderMessageCounter & 3) << 1);
		}
		else if(_type == HMWiredPacketType::ackMessage)
		{
			_controlByte = 0x10; //F bit
			_controlByte |= ((_receiverMessageCounter & 3) << 5);
			_controlByte |= 0x08; //Packet must contain sender address
			_controlByte |= 1;
		}
		else if(_type == HMWiredPacketType::discovery)
		{
			_controlByte = 0;
			_controlByte |= 3;
			_controlByte |= ((_addressMask & 0x1F) << 3);
		}
		else if(_type == HMWiredPacketType::system)
		{
			_controlByte = 0x10; //F bit
			_controlByte |= ((_receiverMessageCounter & 3) << 5);
			_controlByte |= 1;
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

std::vector<uint8_t> HMWiredPacket::byteArray()
{
	try
	{
		if(!_escapedPacket.empty()) return _escapedPacket;
		if(!_packet.empty())
		{
			escapePacket();
			return _escapedPacket;
		}

		if(_type == HMWiredPacketType::none) return _escapedPacket;

		if(_payload.size() > 128)
		{
			Output::printError("Cannot create HomeMatic Wired packet with a payload size larger than 128 bytes.");
			return _escapedPacket;
		}

		if(_controlByte == 0) generateControlByte();

		if(_type == HMWiredPacketType::iMessage || _type == HMWiredPacketType::ackMessage)
		{
			_packet.push_back(0xFD);
			_packet.push_back(_destinationAddress >> 24);
			_packet.push_back((_destinationAddress >> 16) & 0xFF);
			_packet.push_back((_destinationAddress >> 8) & 0xFF);
			_packet.push_back(_destinationAddress & 0xFF);
			_packet.push_back(_controlByte);
			_packet.push_back(_senderAddress >> 24);
			_packet.push_back((_senderAddress >> 16) & 0xFF);
			_packet.push_back((_senderAddress >> 8) & 0xFF);
			_packet.push_back(_senderAddress & 0xFF);
			_packet.push_back(_payload.size() + 2);
			_packet.insert(_packet.end(), _payload.begin(), _payload.end());
			if(_checksum == 0) _checksum = CRC16::calculate(_packet);
			_packet.push_back(_checksum >> 8);
			_packet.push_back(_checksum & 0xFF);
		}
		else if(_type == HMWiredPacketType::system)
		{
			_packet.push_back(0xFE);
			_packet.push_back(_destinationAddress & 0xFF); //Only one byte, that's correct!
			_packet.push_back(_controlByte);
			_packet.push_back(2);
			if(_checksum == 0) _checksum = CRC16::calculate(_packet);
			_packet.push_back(_checksum >> 8);
			_packet.push_back(_checksum & 0xFF);
		}
		else if(_type == HMWiredPacketType::discovery)
		{
			_packet.push_back(0xFD);
			_packet.push_back(_destinationAddress >> 24);
			_packet.push_back((_destinationAddress >> 16) & 0xFF);
			_packet.push_back((_destinationAddress >> 8) & 0xFF);
			_packet.push_back(_destinationAddress & 0xFF);
			_packet.push_back(_controlByte);
			_packet.push_back(2); //Length
			if(_checksum == 0) _checksum = CRC16::calculate(_packet);
			_packet.push_back(_checksum >> 8);
			_packet.push_back(_checksum & 0xFF);
		}
		else if(_type == HMWiredPacketType::discoveryResponse)
		{
			_packet.push_back(0xF8); //No CRC
		}

		escapePacket();
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
    return _escapedPacket;
}

std::string HMWiredPacket::hexString()
{
	try
	{
		return HelperFunctions::getHexString(byteArray());
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
    return "";
}

void HMWiredPacket::setPosition(double index, double size, std::vector<uint8_t>& value)
{
	try
	{
		if(size < 0)
		{
			Output::printError("Error: Negative size not allowed.");
			return;
		}
		if(index < 9)
		{
			Output::printError("Error: Packet index < 9 requested.");
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
				Output::printError("Error: Can't set partial byte index > 1.");
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
				_payload.at(intByteIndex) = value.at(0) & _bitmask[bitSize];
				for(uint32_t i = 1; i < bytes; i++)
				{
					_payload.at(intByteIndex + i) = value.at(i);
				}
			}
			else
			{
				uint32_t missingBytes = bytes - value.size();
				for(uint32_t i = 0; i < value.size(); i++)
				{
					_payload.at(intByteIndex + missingBytes + i) = value.at(i);
				}
			}
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
    _length = 9 + _payload.size();
}

std::vector<uint8_t> HMWiredPacket::getPosition(double index, double size, int32_t mask)
{
	std::vector<uint8_t> result;
	try
	{
		if(size < 0)
		{
			Output::printError("Error: Negative size not allowed.");
			result.push_back(0);
			return result;
		}
		if(index < 0)
		{
			Output::printError("Error: Packet index < 0 requested.");
			result.push_back(0);
			return result;
		}
		if(index < 9)
		{
			if(size > 0.8)
			{
				Output::printError("Error: Packet index < 9 and size > 1 requested.");
				result.push_back(0);
				return result;
			}

			uint32_t bitSize = std::lround(size * 10);
			uint32_t intIndex = std::lround(std::floor(index));
			if(intIndex == 0) result.push_back(((_destinationAddress >> 24) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 1) result.push_back(((_destinationAddress >> 16) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 2) result.push_back(((_destinationAddress >> 8) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 3) result.push_back((_destinationAddress >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 4) result.push_back((_controlByte >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 5) result.push_back(((_senderAddress >> 24) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 6) result.push_back(((_senderAddress >> 16) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 7) result.push_back(((_senderAddress >> 8) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			else if(intIndex == 8) result.push_back((_senderAddress >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
			return result;
		}
		index -= 9;
		double byteIndex = std::floor(index);
		if(byteIndex >= _payload.size())
		{
			result.push_back(0);
			return result;
		}
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(size > 1)
			{
				Output::printError("Error: Partial byte index > 1 requested.");
				result.push_back(0);
				return result;
			}
			//The round is necessary, because for example (uint32_t)(0.2 * 10) is 1
			uint32_t bitSize = std::lround(size * 10);
			result.push_back((_payload.at(byteIndex) >> (std::lround(index * 10) % 10)) & _bitmask[bitSize]);
		}
		else
		{
			uint32_t bytes = (uint32_t)std::ceil(size);
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			uint8_t currentByte = _payload.at(index) & _bitmask[bitSize];
			if(mask != -1 && bytes <= 4) currentByte &= (mask >> ((bytes - 1) * 8));
			result.push_back(currentByte);
			for(uint32_t i = 1; i < bytes; i++)
			{
				if((index + i) >= _payload.size()) result.push_back(0);
				else
				{
					currentByte = _payload.at(index + i);
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
    result.push_back(0);
    return result;
}

} /* namespace HMWired */
