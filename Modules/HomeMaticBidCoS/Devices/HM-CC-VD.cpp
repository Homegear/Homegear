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

#include "HM-CC-VD.h"
#include "../../Base/BaseLib.h"
#include "../GD.h"

namespace BidCoS
{
HM_CC_VD::HM_CC_VD(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : HomeMaticDevice(deviceID, serialNumber, address, eventHandler)
{
	try
	{
		_deviceType = (uint32_t)DeviceType::HMCCVD;
		_firmwareVersion = 0x20;
		_deviceClass = 0x58;
		_channelMin = 0x01;
		_channelMax = 0x01;
		_deviceTypeChannels[0x39] = 1;

		_config[1][5][0x09] = 0;
		_offsetPosition = &(_config[1][5][0x09]);
		_config[1][5][0x0A] = 15;
		_errorPosition = &(_config[1][5][0x0A]);

		init();
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

HM_CC_VD::HM_CC_VD(IDeviceEventSink* eventHandler) : HomeMaticDevice(eventHandler)
{
	init();
}

void HM_CC_VD::init()
{
	HomeMaticDevice::init();

    setUpBidCoSMessages();
}

HM_CC_VD::~HM_CC_VD()
{
	dispose();
}

void HM_CC_VD::saveVariables()
{
	try
	{
		if(_deviceID == 0) return;
		HomeMaticDevice::saveVariables();
		saveVariable(1000, _valveState);
		saveVariable(1001, (int32_t)_valveDriveBlocked);
		saveVariable(1002, (int32_t)_valveDriveLoose);
		saveVariable(1003, (int32_t)_adjustingRangeTooSmall);
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

void HM_CC_VD::loadVariables()
{
	try
	{
		HomeMaticDevice::loadVariables();
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetDeviceVariables();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1000:
				_valveState = row->second.at(3)->intValue;
				break;
			case 1001:
				_valveDriveBlocked = row->second.at(3)->intValue;
				break;
			case 1002:
				_valveDriveLoose = row->second.at(3)->intValue;
				break;
			case 1003:
				_adjustingRangeTooSmall = row->second.at(3)->intValue;
				break;
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
}

void HM_CC_VD::setUpBidCoSMessages()
{
	try
	{
		HomeMaticDevice::setUpBidCoSMessages();

		_messages->add(std::shared_ptr<BidCoSMessage>(new BidCoSMessage(0x58, this, ACCESSPAIREDTOSENDER, NOACCESS, &HomeMaticDevice::handleDutyCyclePacket)));
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

std::string HM_CC_VD::handleCLICommand(std::string command)
{
	return HomeMaticDevice::handleCLICommand(command);
}

void HM_CC_VD::handleDutyCyclePacket(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::handleDutyCyclePacket(messageCounter, packet);
		std::shared_ptr<BidCoSPeer> peer = getPeer(packet->senderAddress());
		if(!peer || peer->getDeviceType().type() != (uint32_t)DeviceType::HMCCTC) return;
		int32_t oldValveState = _valveState;
		_valveState = (packet->payload()->at(1) * 100) / 256;
		GD::out.printInfo("Info: HomeMatic BidCoS device " + std::to_string(_deviceID) + ": New valve state " + std::to_string(_valveState));
		if(packet->destinationAddress() != _address) return; //Unidirectional packet (more than three valve drives connected to one room thermostat) or packet to other valve drive
		sendDutyCycleResponse(packet->senderAddress(), oldValveState, packet->payload()->at(0));
		if(_justPairedToOrThroughCentral)
		{
			std::chrono::milliseconds sleepingTime(2); //Seems very short, but the real valve drive also sends the next packet after 2 ms already
			std::this_thread::sleep_for(sleepingTime);
			_messageCounter[0]++;
			sendConfigParamsType2(_messageCounter[0], packet->senderAddress());
			_justPairedToOrThroughCentral = false;
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

void HM_CC_VD::sendDutyCycleResponse(int32_t destinationAddress, unsigned char oldValveState, unsigned char adjustmentCommand)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer = getPeer(destinationAddress);
		if(!peer) return;
		HomeMaticDevice::sendDutyCycleResponse(destinationAddress);

		std::vector<uint8_t> payload;
		payload.push_back(0x01); //Subtype
		payload.push_back(0x01); //Channel
		payload.push_back(oldValveState * 2);
		unsigned char byte3 = 0x00;
		switch(adjustmentCommand)
		{
			case 0: //Do nothing
				break;
			case 1: //?
				break;
			case 2: //OFF
				if(oldValveState > _valveState) byte3 |= 0b00100000; //Closing
				break;
			case 3: //ON or new valve state
				if(oldValveState <= _valveState) //When the valve state is unchanged and the command is 3, the HM-CC-TC is set to "ON" and the response needs to be "opening". That's the reason for the "<=".
				{
					byte3 |= 0b00010000; //Opening
				}
				else
				{
					byte3 |= 0b00100000; //Closing
				}
				break;
			case 4: //?
				break;
		}
		if(_valveDriveBlocked) byte3 |= 0b00000010;
		if(_valveDriveLoose) byte3 |= 0b00000100;
		if(_adjustingRangeTooSmall) byte3 |= 0b00000110;
		if(_lowBattery) byte3 |= 0b00001000;

		payload.push_back(byte3);
		payload.push_back(0x37);
		sendOKWithPayload(peer->getMessageCounter(), destinationAddress, payload, true);
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

void HM_CC_VD::sendConfigParamsType2(int32_t messageCounter, int32_t destinationAddress)
{
	try
	{
		HomeMaticDevice::sendConfigParamsType2(messageCounter, destinationAddress);

		std::vector<uint8_t> payload;
		payload.push_back(0x04);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x00);
		payload.push_back(0x05);
		payload.push_back(0x09);
		payload.push_back(*_offsetPosition);
		payload.push_back(0x0A);
		payload.push_back(*_errorPosition);
		payload.push_back(0x00);
		payload.push_back(0x00);
		std::shared_ptr<BidCoSPacket> config(new BidCoSPacket(messageCounter, 0x80, 0x10, _address, destinationAddress, payload));
		sendPacket(_physicalInterface, config);
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

std::shared_ptr<BidCoSPeer> HM_CC_VD::createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet, bool save)
{
	try
	{
		std::shared_ptr<BidCoSPeer> peer(new BidCoSPeer(_deviceID, false, this));
		peer->setAddress(address);
		peer->setFirmwareVersion(firmwareVersion);
		peer->setDeviceType(deviceType);
		peer->setMessageCounter(0);
		peer->setRemoteChannel(remoteChannel);
		if(deviceType.type() == (uint32_t)DeviceType::HMCCTC || deviceType.type() == (uint32_t)DeviceType::none) peer->setLocalChannel(1); else peer->setLocalChannel(0);
		peer->setSerialNumber(serialNumber);
		if(save) peer->save(true, true, false);
		return peer;
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
    return std::shared_ptr<BidCoSPeer>();
}

void HM_CC_VD::reset()
{
	try
	{
		HomeMaticDevice::reset();
		*_errorPosition = 0x0F;
		*_offsetPosition = 0x00;
		_valveState = 0x00;
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

void HM_CC_VD::handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet)
{
	try
	{
		HomeMaticDevice::handleConfigPeerAdd(messageCounter, packet);

		int32_t address = (packet->payload()->at(2) << 16) + (packet->payload()->at(3) << 8) + (packet->payload()->at(4));
		_peersMutex.lock();
		if(_peers.size() > 20)
		{
			_peersMutex.unlock();
			return;
		}
		_peers[address]->setDeviceType(BaseLib::Systems::LogicalDeviceType(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS, (uint32_t)DeviceType::HMCCTC));
		_peersMutex.unlock();
		getPeer(address)->save(true, true, false);
	}
	catch(const std::exception& ex)
    {
		_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_peersMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void HM_CC_VD::setValveDriveBlocked(bool valveDriveBlocked)
{
    _valveDriveBlocked = valveDriveBlocked;
    sendDutyCycleResponse(0x1D8DDD, 0x00, 0x00);
}

void HM_CC_VD::setValveDriveLoose(bool valveDriveLoose)
{
    _valveDriveLoose = valveDriveLoose;
    sendDutyCycleResponse(0x1D8DDD, 0x00, 0x00);
}

void HM_CC_VD::setAdjustingRangeTooSmall(bool adjustingRangeTooSmall)
{
    _adjustingRangeTooSmall = adjustingRangeTooSmall;
    sendDutyCycleResponse(0x1D8DDD, 0x00, 0x00);
}
}
