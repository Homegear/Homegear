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
#include "../Exception.h"
#include "../HelperFunctions.h"

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
					HelperFunctions::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
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
							HelperFunctions::printError("CRC for HomeMatic Wired packet failed: " + HelperFunctions::getHexString(packet) + HelperFunctions::getHexString(_checksum, 4));
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
					HelperFunctions::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
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
							HelperFunctions::printError("CRC for HomeMatic Wired packet failed: " + HelperFunctions::getHexString(packet) + HelperFunctions::getHexString(_checksum, 4));
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
				HelperFunctions::printError("HomeMatic Wired packet has invalid length: " + HelperFunctions::getHexString(packet));
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
				HelperFunctions::printError("HomeMatic Wired system packet has unknown type: " + HelperFunctions::getHexString(packet));
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
						HelperFunctions::printError("CRC for HomeMatic Wired packet failed: " + HelperFunctions::getHexString(packet) + HelperFunctions::getHexString(_checksum, 4));
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
			HelperFunctions::printError("HomeMatic Wired packet has unknown type: " + HelperFunctions::getHexString(packet));
			return;
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

void HMWiredPacket::import(std::string packetHex)
{
	try
	{
		std::vector<uint8_t> packet(HelperFunctions::getUBinary(packetHex));
		import(packet);
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

void HMWiredPacket::generateControlByte()
{
	try
	{
		_controlByte = 0x10; //F bit is always set
		if(_type == HMWiredPacketType::iMessage)
		{
			if(_synchronizationBit) _controlByte &= 0x80;
			_controlByte |= ((_receiverMessageCounter & 3) << 5);
			_controlByte |= 0x08; //Packet must contain sender address
			_controlByte |= ((_senderMessageCounter & 3) << 1);
		}
		else if(_type == HMWiredPacketType::ackMessage)
		{
			_controlByte |= ((_receiverMessageCounter & 3) << 5);
			_controlByte |= 0x08; //Packet must contain sender address
			_controlByte |= 1;
		}
		else if(_type == HMWiredPacketType::discovery)
		{
			_controlByte |= 3;
			_controlByte |= ((_addressMask & 0x1F) << 3);
		}
		else if(_type == HMWiredPacketType::system)
		{
			_controlByte |= ((_receiverMessageCounter & 3) << 5);
			_controlByte |= 1;
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
			HelperFunctions::printError("Cannot create HomeMatic Wired packet with a payload size larger than 128 bytes.");
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
			_packet.push_back(_destinationAddress >> 24);
			_packet.push_back((_destinationAddress >> 16) & 0xFF);
			_packet.push_back((_destinationAddress >> 8) & 0xFF);
			_packet.push_back(_destinationAddress & 0xFF);
			_packet.push_back(_payload.size() + 2);
			_packet.insert(_packet.end(), _payload.begin(), _payload.end());
		}
		else if(_type == HMWiredPacketType::system)
		{
			_packet.push_back(0xFE);
			_packet.push_back(_destinationAddress & 0xFF); //Only one byte, that's correct!
			_packet.push_back(_controlByte);
			_packet.push_back(2);
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
		}
		else if(_type == HMWiredPacketType::discoveryResponse)
		{
			_packet.push_back(0xF8); //No CRC
		}

		escapePacket();
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

} /* namespace HMWired */
