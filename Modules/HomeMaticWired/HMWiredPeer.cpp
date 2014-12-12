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

#include "HMWiredPeer.h"
#include "Devices/HMWiredCentral.h"
#include "GD.h"

namespace HMWired
{
std::shared_ptr<BaseLib::Systems::Central> HMWiredPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = GD::family->getCentral();
		return _central;
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
	return std::shared_ptr<BaseLib::Systems::Central>();
}

std::shared_ptr<BaseLib::Systems::LogicalDevice> HMWiredPeer::getDevice(int32_t address)
{
	try
	{
		return GD::family->get(address);
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
	return std::shared_ptr<BaseLib::Systems::LogicalDevice>();
}

HMWiredPeer::HMWiredPeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(GD::bl, parentID, centralFeatures, eventHandler)
{
	_lastPing = BaseLib::HelperFunctions::getTime() - (BaseLib::HelperFunctions::getRandomNumber(1, 60) * 10000);
}

HMWiredPeer::HMWiredPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(GD::bl, id, address, serialNumber, parentID, centralFeatures, eventHandler)
{
	_lastPing = BaseLib::HelperFunctions::getTime() - (BaseLib::HelperFunctions::getRandomNumber(1, 60) * 10000);
}

HMWiredPeer::~HMWiredPeer()
{
	_pingThreadMutex.lock();
	if(_pingThread.joinable()) _pingThread.join();
	_pingThreadMutex.unlock();
}

void HMWiredPeer::worker()
{
	if(!_centralFeatures || _disposing) return;
	try
	{
		int64_t time = BaseLib::HelperFunctions::getTime();
		if(rpcDevice)
		{
			serviceMessages->checkUnreach(rpcDevice->cyclicTimeout, getLastPacketReceived());
			if(serviceMessages->getUnreach())
			{
				if(time - _lastPing > 600000)
				{
					_pingThreadMutex.lock();
					if(!_disposing && !deleting && _lastPing < time) //Check that _lastPing wasn't set in putParamset after locking the mutex
					{
						_lastPing = time; //Set here to avoid race condition between worker thread and ping thread
						if(_pingThread.joinable()) _pingThread.join();
						_pingThread = std::thread(&HMWiredPeer::pingThread, this);
					}
					_pingThreadMutex.unlock();
				}
			}
			else
			{
				if(_centralFeatures && configCentral[0].find("POLLING") != configCentral[0].end() && configCentral[0].at("POLLING").data.size() > 0 && configCentral[0].at("POLLING").data.at(0) > 0 && configCentral[0].find("POLLING_INTERVAL") != configCentral[0].end())
				{
					//Polling is enabled
					BaseLib::Systems::RPCConfigurationParameter* parameter = &configCentral[0]["POLLING_INTERVAL"];
					int32_t data = 0;
					_bl->hf.memcpyBigEndian(data, parameter->data); //Shortcut to save resources. The normal way would be to call "convertFromPacket".
					int64_t pollingInterval = data * 60000;
					if(pollingInterval < 600000) pollingInterval = 600000;
					if(time - _lastPing >= pollingInterval)
					{
						int64_t timeSinceLastPacket = time - ((int64_t)_lastPacketReceived * 1000);
						if(timeSinceLastPacket > 0 && timeSinceLastPacket >= pollingInterval)
						{
							_pingThreadMutex.lock();
							if(!_disposing && !deleting && _lastPing < time) //Check that _lastPing wasn't set in putParamset after locking the mutex
							{
								_lastPing = time; //Set here to avoid race condition between worker thread and ping thread
								if(_pingThread.joinable()) _pingThread.join();
								_pingThread = std::thread(&HMWiredPeer::pingThread, this);
							}
							_pingThreadMutex.unlock();
						}
					}
				}
				else _lastPing = time; //Set _lastPing, so there is a delay of 10 minutes after the device is unreachable before the first ping.
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

void HMWiredPeer::initializeLinkConfig(int32_t channel, std::shared_ptr<BaseLib::Systems::BasicPeer> peer)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, BaseLib::RPC::ParameterSet::Type::Enum::link);
		if(!parameterSet)
		{
			GD::out.printError("Error: No link parameter set found.");
			return;
		}
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		//Check if address data is continuous
		int32_t min = parameterSet->channelOffset;
		if(parameterSet->peerAddressOffset < min) min = parameterSet->peerAddressOffset;
		if(parameterSet->peerChannelOffset < min) min = parameterSet->peerChannelOffset;
		int32_t max = parameterSet->peerChannelOffset;
		if(parameterSet->peerAddressOffset + 3 > max) max = parameterSet->peerAddressOffset;
		if(parameterSet->channelOffset > max) max = parameterSet->channelOffset;
		if(max - min != 5)
		{
			GD::out.printError("Error: Address format of parameter set is not supported.");
			return;
		}

		std::vector<uint8_t> data(6);
		data.at(parameterSet->channelOffset) = channel + rpcChannel->physicalIndexOffset;
		data.at(parameterSet->peerAddressOffset) = peer->address >> 24;
		data.at(parameterSet->peerAddressOffset + 1) = (peer->address >> 16) & 0xFF;
		data.at(parameterSet->peerAddressOffset + 2) = (peer->address >> 8) & 0xFF;
		data.at(parameterSet->peerAddressOffset + 3) = peer->address & 0xFF;
		data.at(parameterSet->peerChannelOffset) = peer->channel + peer->physicalIndexOffset;
		if(peer->configEEPROMAddress == -1)
		{
			GD::out.printError("Error: Link config's EEPROM address is invalid.");
			return;
		}
		std::vector<int32_t> configBlocks = setConfigParameter((double)peer->configEEPROMAddress, 6.0, data);
		for(std::vector<int32_t>::iterator i = configBlocks.begin(); i != configBlocks.end(); ++i)
		{
			std::vector<uint8_t> configBlock = binaryConfig.at(*i).data;
			if(!std::dynamic_pointer_cast<HMWiredCentral>(getCentral())->writeEEPROM(_address, *i, configBlock)) GD::out.printError("Error: Could not write config to device's eeprom.");
		}

		if(!peer->isSender) return; //Nothing more to do

		std::shared_ptr<BaseLib::RPC::Variable> variables(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			variables->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, (*i)->logicalParameter->getDefaultValue()));
		}
		std::shared_ptr<BaseLib::RPC::Variable> result = putParamset(channel, BaseLib::RPC::ParameterSet::Type::Enum::link, peer->id, peer->channel, variables);
		if(result->errorStruct) GD::out.printError("Error: " + result->structValue->at("faultString")->stringValue);
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

std::string HMWiredPeer::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			stringStream << "channel count\t\tPrint the number of channels of this peer" << std::endl;
			stringStream << "eeprom print\t\tPrints the known areas of the eeprom" << std::endl;
			stringStream << "peers list\t\tLists all peers paired to this peer" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 13, "channel count") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints this peer's number of channels." << std::endl;
						stringStream << "Usage: channel count" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Peer has " << rpcDevice->channels.size() << " channels." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "eeprom print") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints the known areas of this peer's eeprom." << std::endl;
						stringStream << "Usage: eeprom print" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Address\tData" << std::endl;
			for(std::unordered_map<uint32_t, BaseLib::Systems::ConfigDataBlock>::iterator i = binaryConfig.begin(); i != binaryConfig.end(); ++i)
			{
				stringStream << "0x" << std::hex << std::setfill('0') << std::setw(4) << i->first << "\t" << BaseLib::HelperFunctions::getHexString(i->second.data) << std::dec << std::endl;
			}
			return stringStream.str();
		}
		else if(command.compare(0, 10, "peers list") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command lists all peers paired to this peer." << std::endl;
						stringStream << "Usage: peers list" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			if(_peers.empty())
			{
				stringStream << "No peers are paired to this peer." << std::endl;
				return stringStream.str();
			}
			for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
			{
				for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					stringStream << "Channel: " << i->first << "\tAddress: 0x" << std::hex << (*j)->address << "\tRemote channel: " << std::dec << (*j)->channel << "\tSerial number: " << (*j)->serialNumber << std::endl << std::dec;
				}
			}
			return stringStream.str();
		}
		else if(command.compare(0, 11, "test config") == 0)
		{
			int32_t address1 = 0x350;
			int32_t address2 = 0x360;
			int32_t address3 = 0x370;
			if(binaryConfig.find(address1) == binaryConfig.end()) binaryConfig[address1].data = std::dynamic_pointer_cast<HMWiredCentral>(getCentral())->readEEPROM(_address, address1);
			if(binaryConfig.find(address2) == binaryConfig.end()) binaryConfig[address2].data = std::dynamic_pointer_cast<HMWiredCentral>(getCentral())->readEEPROM(_address, address2);
			if(binaryConfig.find(address3) == binaryConfig.end()) binaryConfig[address3].data = std::dynamic_pointer_cast<HMWiredCentral>(getCentral())->readEEPROM(_address, address3);
			std::vector<uint8_t> oldConfig1 = binaryConfig.at(address1).data;
			std::vector<uint8_t> oldConfig2 = binaryConfig.at(address2).data;
			std::vector<uint8_t> oldConfig3 = binaryConfig.at(address3).data;

			//Test 1: Set two bytes within one config data block
			stringStream << "EEPROM before test 1:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x355.0, 2.0, data)\" with data 0xABCD:" << std::endl;
			std::vector<uint8_t> data({0xAB, 0xCD});
			setConfigParameter(853.0, 2.0, data);
			data = getConfigParameter(853.0, 2.0);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 1:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 1

			//Test 2: Set four bytes spanning over two data blocks
			stringStream << "EEPROM before test 2:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x35E.0, 4.0, data)\" with data 0xABCDEFAC:" << std::endl;
			data = std::vector<uint8_t>({0xAB, 0xCD, 0xEF, 0xAC});
			setConfigParameter(862.0, 4.0, data);
			data = getConfigParameter(862.0, 4.0);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 2:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 2

			//Test 3: Set 20 bytes spanning over three data blocks
			stringStream << "EEPROM before test 3:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x35F.0, 20.0, data)\" with data 0xABCDEFAC112233445566778899AABBCCDDEE1223:" << std::endl;
			data = std::vector<uint8_t>({0xAB, 0xCD, 0xEF, 0xAC, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x12, 0x23});
			setConfigParameter(863.0, 20.0, data);
			data = getConfigParameter(863.0, 20.0);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 3:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 3

			binaryConfig.at(address1).data = oldConfig1;
			binaryConfig.at(address2).data = oldConfig2;
			binaryConfig.at(address3).data = oldConfig3;

			//Test 4: Size 1.4
			stringStream << "EEPROM before test 4:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x352.0, 1.4, data)\" with data 0x0B5E:" << std::endl;
			data = std::vector<uint8_t>({0x0B, 0x5E});
			setConfigParameter(850.0, 1.4, data);
			data = getConfigParameter(850.0, 1.4);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 4:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 4

			//Test 5: Size 0.6
			stringStream << "EEPROM before test 5:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x375.0, 0.6, data)\" with data 0x15:" << std::endl;
			data = std::vector<uint8_t>({0x15});
			setConfigParameter(885.0, 0.6, data);
			data = getConfigParameter(885.0, 0.6);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 5:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 5

			//Test 6: Size 0.6, index offset 0.2
			stringStream << "EEPROM before test 6:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x377.2, 0.6, data)\" with data 0x15:" << std::endl;
			data = std::vector<uint8_t>({0x15});
			setConfigParameter(887.2, 0.6, data);
			data = getConfigParameter(887.2, 0.6);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 6:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 6

			//Test 7: Size 0.3, index offset 0.6
			stringStream << "EEPROM before test 7:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x379.6, 0.3, data)\" with data 0x02:" << std::endl;
			data = std::vector<uint8_t>({0x02});
			setConfigParameter(889.6, 0.3, data);
			data = getConfigParameter(889.6, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 7:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 7

			//Set 0x36F to 0xFF for test
			data = std::vector<uint8_t>({0xFF});
			setConfigParameter(879.0, 1.0, data);

			//Test 8: Size 0.3, index offset 0.6 spanning over two data blocks
			stringStream << "EEPROM before test 8:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x36F.6, 0.3, data)\" with data 0x02:" << std::endl;
			data = std::vector<uint8_t>({0x02});
			setConfigParameter(879.6, 0.3, data);
			data = getConfigParameter(879.6, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 8:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 8

			//Set 0x36F and 0x370 to 0xFF for test
			data = std::vector<uint8_t>({0xFF});
			setConfigParameter(879.0, 1.0, data);
			setConfigParameter(880.0, 1.0, data);

			//Test 9: Size 0.5, index offset 0.6 spanning over two data blocks
			stringStream << "EEPROM before test 9:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x36F.6, 0.5, data)\" with data 0x0A:" << std::endl;
			data = std::vector<uint8_t>({0x0A});
			setConfigParameter(879.6, 0.5, data);
			data = getConfigParameter(879.6, 0.5);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 9:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 9

			//Test 10: Test of steps: channelIndex 5, index offset 0.2, step 0.3
			stringStream << "EEPROM before test 10:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(5, 881.2, 0.3, 0.3, data)\" with data 0x03:" << std::endl;
			data = std::vector<uint8_t>({0x03});
			setMasterConfigParameter(5, 881.2, 0.3, 0.3, data);
			data = getMasterConfigParameter(5, 881.2, 0.3, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 10:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 10

			//Test 11: Test of steps: channelIndex 4, index offset 0.2, step 0.3
			stringStream << "EEPROM before test 11:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(4, 0x371.2, 0.3, 0.3, data)\" with data 0x02:" << std::endl;
			data = std::vector<uint8_t>({0x02});
			setMasterConfigParameter(4, 881.2, 0.3, 0.3, data);
			data = getMasterConfigParameter(4, 881.2, 0.3, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << BaseLib::HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 11:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << BaseLib::HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 11

			binaryConfig.at(address1).data = oldConfig1;
			binaryConfig.at(address2).data = oldConfig2;
			binaryConfig.at(address3).data = oldConfig3;
			return stringStream.str();
		}
		else return "Unknown command.\n";
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
    return "Error executing command. See log file for more details.\n";
}

std::vector<int32_t> HMWiredPeer::setConfigParameter(double index, double size, std::vector<uint8_t>& binaryValue)
{
	std::vector<int32_t> changedBlocks;
	try
	{
		if(size < 0 || index < 0)
		{
			GD::out.printError("Error: Can't set configuration parameter. Index or size is negative.");
			return changedBlocks;
		}
		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));
		if(size > 0.8 && size < 1.0) size = 1.0;
		double byteIndex = std::floor(index);
		if(byteIndex != index || size <= 1.0) //0.8 == 8 Bits
		{
			if(binaryValue.empty()) binaryValue.push_back(0);
			int32_t intByteIndex = byteIndex;
			int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
			changedBlocks.push_back(configBlockIndex);
			intByteIndex -= configBlockIndex;
			if(size > 1.0)
			{
				GD::out.printError("Error: HomeMatic Wired peer " + std::to_string(_peerID) + ": Can't set partial byte index > 1.");
				return changedBlocks;
			}
			uint32_t bitSize = std::lround(size * 10);
			if(bitSize > 8) bitSize = 8;
			uint32_t indexBits = std::lround(index * 10) % 10;
			if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
			{
				binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
				saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
			}
			BaseLib::Systems::ConfigDataBlock* configBlock = &binaryConfig[configBlockIndex];
			if(configBlock->data.size() != 0x10)
			{
				GD::out.printError("Error: Can't set configuration parameter. Can't read EEPROM.");
				return changedBlocks;
			}
			if(bitSize == 8) configBlock->data.at(intByteIndex) = 0;
			else configBlock->data.at(intByteIndex) &= (~(_bitmask[bitSize] << indexBits));
			configBlock->data.at(intByteIndex) |= (binaryValue.at(binaryValue.size() - 1) << indexBits);
			if(indexBits + bitSize > 8) //Spread over two bytes
			{
				uint32_t missingBits = (indexBits + bitSize) - 8;
				if(missingBits > 8)
				{
					GD::out.printError("Error: missingBits in function setConfigParameter is out of bounds.");
					return changedBlocks;
				}
				intByteIndex++;
				if(intByteIndex >= 0x10)
				{
					saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
					configBlockIndex += 0x10;
					changedBlocks.push_back(configBlockIndex);
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
						saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
					}
					configBlock = &binaryConfig[configBlockIndex];
					if(configBlock->data.size() != 0x10)
					{
						GD::out.printError("Error: Can't set configuration parameter. Can't read EEPROM.");
						return changedBlocks;
					}
					intByteIndex = 0;
				}
				configBlock->data.at(intByteIndex) &= (~_bitmask[missingBits]);
				configBlock->data.at(intByteIndex) |= (binaryValue.at(binaryValue.size() - 1) >> (bitSize - missingBits));
			}
			saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
		}
		else
		{
			int32_t intByteIndex = byteIndex;
			int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
			changedBlocks.push_back(configBlockIndex);
			intByteIndex -= configBlockIndex;
			uint32_t bytes = (uint32_t)std::ceil(size);
			if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
			{
				binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
				saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
			}
			BaseLib::Systems::ConfigDataBlock* configBlock = &binaryConfig[configBlockIndex];
			if(binaryValue.empty()) return changedBlocks;
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			if(bytes <= binaryValue.size())
			{
				configBlock->data.at(intByteIndex) &= (~_bitmask[bitSize]);
				configBlock->data.at(intByteIndex) |= (binaryValue.at(0) & _bitmask[bitSize]);
				for(uint32_t i = 1; i < bytes; i++)
				{
					if(intByteIndex + i >= 0x10)
					{
						intByteIndex = -i;
						saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
						configBlockIndex += 0x10;
						changedBlocks.push_back(configBlockIndex);
						if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
						{
							binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
							saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
						}
						configBlock = &binaryConfig[configBlockIndex];
						if(configBlock->data.size() != 0x10)
						{
							GD::out.printError("Error: Can't set configuration parameter. Can't read EEPROM.");
							return changedBlocks;
						}
					}
					configBlock->data.at(intByteIndex + i) = binaryValue.at(i);
				}
			}
			else
			{
				uint32_t missingBytes = bytes - binaryValue.size();
				for(uint32_t i = 0; i < missingBytes; i++)
				{
					configBlock->data.at(intByteIndex) &= (~_bitmask[bitSize]);
					for(uint32_t i = 1; i < bytes; i++)
					{
						if(intByteIndex + i >= 0x10)
						{
							intByteIndex = -i;
							saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
							configBlockIndex += 0x10;
							changedBlocks.push_back(configBlockIndex);
							if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
							{
								binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
								saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
							}
							configBlock = &binaryConfig[configBlockIndex];
							if(configBlock->data.size() != 0x10)
							{
								GD::out.printError("Error: Can't set configuration parameter. Can't read EEPROM.");
								return changedBlocks;
							}
						}
						configBlock->data.at(intByteIndex + i) = 0;
					}
				}
				for(uint32_t i = 0; i < binaryValue.size(); i++)
				{
					if(intByteIndex + missingBytes + i >= 0x10)
					{
						intByteIndex = -(missingBytes + i);
						saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
						configBlockIndex += 0x10;
						changedBlocks.push_back(configBlockIndex);
						if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
						{
							binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
							saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
						}
						configBlock = &binaryConfig[configBlockIndex];
						if(configBlock->data.size() != 0x10)
						{
							GD::out.printError("Error: Can't set configuration parameter. Can't read EEPROM.");
							return changedBlocks;
						}
					}
					configBlock->data.at(intByteIndex + missingBytes + i) = binaryValue.at(i);
				}
			}
			saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
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
	return changedBlocks;
}

std::vector<int32_t> HMWiredPeer::setMasterConfigParameter(int32_t channelIndex, double index, double step, double size, std::vector<uint8_t>& binaryValue)
{
	try
	{
		int32_t bitStep = (std::lround(step * 10) % 10) + (((int32_t)step) * 8);
		int32_t bitSteps = bitStep * channelIndex;
		while(bitSteps >= 8)
		{
			index += 1.0;
			bitSteps -= 8;
		}
		int32_t indexBits = std::lround(index * 10) % 10;
		if(indexBits + bitSteps >= 8)
		{
			index = std::ceil(index);
			bitSteps = (indexBits + bitSteps) - 8;
		}
		index += ((double)bitSteps) / 10.0;
		return setConfigParameter(index, size, binaryValue);
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
    return std::vector<int32_t>();
}

std::vector<int32_t> HMWiredPeer::setMasterConfigParameter(int32_t channelIndex, int32_t addressStart, int32_t addressStep, double indexOffset, double size, std::vector<uint8_t>& binaryValue)
{
	try
	{
		double index = addressStart + (channelIndex * addressStep) + indexOffset;
		return setConfigParameter(index, size, binaryValue);
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
    return std::vector<int32_t>();
}

std::vector<int32_t> HMWiredPeer::setMasterConfigParameter(int32_t channel, std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet, std::shared_ptr<BaseLib::RPC::Parameter> parameter, std::vector<uint8_t>& binaryValue)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return std::vector<int32_t>();
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(parameter->physicalParameter->address.operation == BaseLib::RPC::PhysicalParameterAddress::Operation::none)
		{
			return setMasterConfigParameter(channel - rpcChannel->startIndex, parameter->physicalParameter->address.index, parameter->physicalParameter->address.step, parameter->physicalParameter->size, binaryValue);
		}
		else
		{
			if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1)
			{
				GD::out.printError("Error: Can't get parameter set. address_start or address_step is not set.");
				return std::vector<int32_t>();
			}
			int32_t channelIndex = channel - rpcChannel->startIndex;
			if(parameterSet->count > 0 && channelIndex >= parameterSet->count)
			{
				GD::out.printError("Error: Can't get parameter set. Out of bounds.");
				return std::vector<int32_t>();
			}
			return setMasterConfigParameter(channelIndex, parameterSet->addressStart, parameterSet->addressStep, parameter->physicalParameter->address.index, parameter->physicalParameter->size, binaryValue);
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
    return std::vector<int32_t>();
}

std::vector<uint8_t> HMWiredPeer::getConfigParameter(double index, double size, int32_t mask, bool onlyKnownConfig)
{
	try
	{
		std::vector<uint8_t> result;
		if(!rpcDevice)
		{
			result.push_back(0);
			return result;
		}
		if(size < 0 || index < 0)
		{
			GD::out.printError("Error: Can't get configuration parameter. Index or size is negative.");
			result.push_back(0);
			return result;
		}
		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));
		double byteIndex = std::floor(index);
		int32_t intByteIndex = byteIndex;
		int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
		if(configBlockIndex >= rpcDevice->eepSize)
		{
			GD::out.printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
			result.push_back(0);
			return result;
		}
		intByteIndex = intByteIndex - configBlockIndex;
		if(configBlockIndex >= rpcDevice->eepSize)
		{
			GD::out.printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
			result.clear();
			result.push_back(0);
			return result;
		}
		if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
		{
			if(onlyKnownConfig) return std::vector<uint8_t>();
			binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
		}
		std::vector<uint8_t>* configBlock = &binaryConfig[configBlockIndex].data;
		if(configBlock->size() != 0x10)
		{
			GD::out.printError("Error: Can't get configuration parameter. Can't read EEPROM.");
			result.push_back(0);
			return result;
		}
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(size > 1)
			{
				GD::out.printError("Error: Can't get configuration parameter. Partial byte index > 1 requested.");
				result.push_back(0);
				return result;
			}
			//The round is necessary, because for example (uint32_t)(0.2 * 10) is 1
			uint32_t bitSize = std::lround(size * 10);
			if(bitSize > 8) bitSize = 8;
			uint32_t indexBits = std::lround(index * 10) % 10;
			if(indexBits + bitSize > 8) //Split over two bytes
			{
				result.push_back(configBlock->at(intByteIndex) >> indexBits);
				intByteIndex++;
				if(intByteIndex >= 0x10)
				{
					configBlockIndex += 0x10;
					if(configBlockIndex >= rpcDevice->eepSize)
					{
						GD::out.printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						if(onlyKnownConfig) return std::vector<uint8_t>();
						binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
					}
					configBlock = &binaryConfig[configBlockIndex].data;
					if(configBlock->size() != 0x10)
					{
						GD::out.printError("Error: Can't get configuration parameter. Can't read EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					intByteIndex = 0;
				}
				uint32_t missingBits = (indexBits + bitSize) - 8;
				if(missingBits > 8)
				{
					GD::out.printError("Error: missingBits is out of bounds.");
					return result;
				}
				result.at(0) |= (((configBlock->at(intByteIndex) & _bitmask[missingBits])) << (bitSize - missingBits));
			}
			else result.push_back((configBlock->at(intByteIndex) >> indexBits) & _bitmask[bitSize]);
		}
		else
		{
			uint32_t bytes = (uint32_t)std::ceil(size);
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			uint8_t currentByte = configBlock->at(intByteIndex) & _bitmask[bitSize];
			if(mask != -1 && bytes <= 4) currentByte &= (mask >> ((bytes - 1) * 8));
			result.push_back(currentByte);
			for(uint32_t i = 1; i < bytes; i++)
			{
				if((intByteIndex + i) >= 0x10)
				{
					configBlockIndex += 0x10;
					if(configBlockIndex >= rpcDevice->eepSize)
					{
						GD::out.printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						if(onlyKnownConfig) return std::vector<uint8_t>();
						binaryConfig[configBlockIndex].data = central->readEEPROM(_address, configBlockIndex);
					}
					configBlock = &binaryConfig[configBlockIndex].data;
					if(configBlock->size() != 0x10)
					{
						GD::out.printError("Error: Can't get configuration parameter. Can't read EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					intByteIndex = -i;
				}
				currentByte = configBlock->at(intByteIndex + i);
				if(mask != -1 && bytes <= 4) currentByte &= (mask >> ((bytes - i - 1) * 8));
				result.push_back(currentByte);
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
	return std::vector<uint8_t>();
}

std::vector<uint8_t> HMWiredPeer::getMasterConfigParameter(int32_t channelIndex, double index, double step, double size)
{
	try
	{
		int32_t bitStep = (std::lround(step * 10) % 10) + (((int32_t)step) * 8);
		int32_t bitSteps = bitStep * channelIndex;
		while(bitSteps >= 8)
		{
			index += 1.0;
			bitSteps -= 8;
		}
		int32_t indexBits = std::lround(index * 10) % 10;
		if(indexBits + bitSteps >= 8)
		{
			index = std::ceil(index);
			bitSteps = (indexBits + bitSteps) - 8;
		}
		index += ((double)bitSteps) / 10.0;
		return getConfigParameter(index, size);
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

std::vector<uint8_t> HMWiredPeer::getMasterConfigParameter(int32_t channelIndex, int32_t addressStart, int32_t addressStep, double indexOffset, double size)
{
	try
	{
		double index = addressStart + (channelIndex * addressStep) + indexOffset;
		return getConfigParameter(index, size);
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

std::vector<uint8_t> HMWiredPeer::getMasterConfigParameter(int32_t channel, std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet, std::shared_ptr<BaseLib::RPC::Parameter> parameter)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return std::vector<uint8_t>();
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		std::vector<uint8_t> value;
		if(parameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::store)
		{
			if(configCentral.find(channel) == configCentral.end()) return value;;
			if(configCentral[channel].find(parameter->id) == configCentral[channel].end()) return value;
			value = configCentral[channel][parameter->id].data;
		}
		else if(parameter->physicalParameter->address.operation == BaseLib::RPC::PhysicalParameterAddress::Operation::none)
		{
			value = getMasterConfigParameter(channel - rpcChannel->startIndex, parameter->physicalParameter->address.index, parameter->physicalParameter->address.step, parameter->physicalParameter->size);
		}
		else
		{
			if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1)
			{
				GD::out.printError("Error: Can't get parameter set. address_start or address_step is not set.");
				return std::vector<uint8_t>();
			}
			int32_t channelIndex = channel - rpcChannel->startIndex;
			if(parameterSet->count > 0 && channelIndex >= parameterSet->count)
			{
				GD::out.printError("Error: Can't get parameter set. Out of bounds.");
				return std::vector<uint8_t>();
			}
			value = getMasterConfigParameter(channelIndex, parameterSet->addressStart, parameterSet->addressStep, parameter->physicalParameter->address.index, parameter->physicalParameter->size);
		}
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
    return std::vector<uint8_t>();
}

bool HMWiredPeer::ping(int32_t packetCount, bool waitForResponse)
{
	try
	{
		std::shared_ptr<HMWiredCentral> central = std::dynamic_pointer_cast<HMWiredCentral>(getCentral());
		if(!central) return false;
		uint32_t time = BaseLib::HelperFunctions::getTimeSeconds();
		_lastPing = (int64_t)time * 1000;
		if(rpcDevice && !rpcDevice->valueRequestFrames.empty())
		{
			for(std::map<int32_t, std::map<std::string, std::shared_ptr<BaseLib::RPC::DeviceFrame>>>::iterator i = rpcDevice->valueRequestFrames.begin(); i != rpcDevice->valueRequestFrames.end(); ++i)
			{
				for(std::map<std::string, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
				{
					if(j->second->associatedValues.empty()) continue;
					std::shared_ptr<BaseLib::RPC::Variable> result = getValueFromDevice(j->second->associatedValues.at(0), i->first, !waitForResponse);
					if(!result || result->errorStruct || result->type == BaseLib::RPC::VariableType::rpcVoid) return false;
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
    return true;
}

void HMWiredPeer::pingThread()
{
	if(ping(3, true)) serviceMessages->endUnreach();
	else serviceMessages->setUnreach(true, false);
}

void HMWiredPeer::addPeer(int32_t channel, std::shared_ptr<BaseLib::Systems::BasicPeer> peer)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == peer->address && (*i)->channel == peer->channel)
			{
				_peers[channel].erase(i);
				break;
			}
		}
		_peers[channel].push_back(peer);
		savePeers();
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

void HMWiredPeer::removePeer(int32_t channel, uint64_t id, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return;
		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));

		for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->id == id && (*i)->channel == remoteChannel)
			{
				std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, BaseLib::RPC::ParameterSet::Type::Enum::link);
				if(parameterSet && (*i)->configEEPROMAddress != -1 && parameterSet->addressStart > -1 && parameterSet->addressStep > 0)
				{
					std::vector<uint8_t> data(parameterSet->addressStep, 0xFF);
					GD::out.printDebug("Debug: Erasing " + std::to_string(data.size()) + " bytes in eeprom at address " + BaseLib::HelperFunctions::getHexString((*i)->configEEPROMAddress, 4));
					std::vector<int32_t> configBlocks = setConfigParameter((*i)->configEEPROMAddress, parameterSet->addressStep, data);
					for(std::vector<int32_t>::iterator j = configBlocks.begin(); j != configBlocks.end(); ++j)
					{
						if(!central->writeEEPROM(_address, *j, binaryConfig[*j].data)) GD::out.printError("Error: Could not write config to device's eeprom.");
					}
				}
				_peers[channel].erase(i);
				savePeers();
				return;
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
    _databaseMutex.unlock();
}

int32_t HMWiredPeer::getFreeEEPROMAddress(int32_t channel, bool isSender)
{
	try
	{
		if(!rpcDevice) return -1;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return -1;
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(!rpcChannel->linkRoles)
		{
			if(!isSender && rpcChannel->linkRoles->targetNames.empty()) return -1;
			if(isSender && rpcChannel->linkRoles->sourceNames.empty()) return -1;
		}
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, BaseLib::RPC::ParameterSet::Type::link);
		if(!parameterSet || parameterSet->addressStart < 0 || parameterSet->addressStep <= 0 || parameterSet->peerAddressOffset < 0) return -1;
		int32_t max = parameterSet->addressStart + (parameterSet->count * parameterSet->addressStep);

		int32_t currentAddress = 0;
		for(currentAddress = parameterSet->addressStart; currentAddress < max; currentAddress += parameterSet->addressStep)
		{
			std::vector<uint8_t> result = getConfigParameter(currentAddress + parameterSet->peerAddressOffset, 4.0);
			if(result.size() != 4) continue;
			//Endianness doesn't matter
			if(*((int32_t*)&result.at(0)) == -1) return currentAddress;
		}
		if(currentAddress >= max)
		{
			GD::out.printError("Error: There are no free EEPROM addresses to store links.");
			return -1;
		}
		return currentAddress;
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
    return -1;
}

void HMWiredPeer::loadVariables(BaseLib::Systems::LogicalDevice* device, std::shared_ptr<BaseLib::Database::DataTable> rows)
{
	try
	{
		if(!rows) rows = raiseGetPeerVariables();
		Peer::loadVariables(device, rows);
		_databaseMutex.lock();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			switch(row->second.at(2)->intValue)
			{
			case 5:
				_messageCounter = row->second.at(3)->intValue;
				break;
			case 12:
				unserializePeers(row->second.at(5)->binaryValue);
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
	_databaseMutex.unlock();
}

bool HMWiredPeer::load(BaseLib::Systems::LogicalDevice* device)
{
	try
	{
		loadVariables((HMWiredDevice*)device);

		rpcDevice = GD::rpcDevices.find(_deviceType, _firmwareVersion, -1);
		if(!rpcDevice)
		{
			GD::out.printError("Error loading HomeMatic Wired peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		std::string entry;
		loadConfig();
		initializeCentralConfig();

		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();

		return true;
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
    return false;
}

void HMWiredPeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		Peer::saveVariables();
		saveVariable(5, (int32_t)_messageCounter);
		savePeers(); //12
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

void HMWiredPeer::savePeers()
{
	try
	{
		std::vector<uint8_t> serializedData;
		serializePeers(serializedData);
		saveVariable(12, serializedData);
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

void HMWiredPeer::serializePeers(std::vector<uint8_t>& encodedData)
{
	try
	{
		BaseLib::BinaryEncoder encoder(_bl);
		encoder.encodeInteger(encodedData, _peers.size());
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeInteger(encodedData, i->second.size());
			for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!*j) continue;
				encoder.encodeBoolean(encodedData, (*j)->isSender);
				encoder.encodeInteger(encodedData, (*j)->id);
				encoder.encodeInteger(encodedData, (*j)->address);
				encoder.encodeInteger(encodedData, (*j)->channel);
				encoder.encodeInteger(encodedData, (*j)->physicalIndexOffset);
				encoder.encodeString(encodedData, (*j)->serialNumber);
				encoder.encodeBoolean(encodedData, (*j)->hidden);
				encoder.encodeString(encodedData, (*j)->linkName);
				encoder.encodeString(encodedData, (*j)->linkDescription);
				encoder.encodeInteger(encodedData, (*j)->configEEPROMAddress);
				encoder.encodeInteger(encodedData, (*j)->data.size());
				encodedData.insert(encodedData.end(), (*j)->data.begin(), (*j)->data.end());
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

void HMWiredPeer::unserializePeers(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BaseLib::BinaryDecoder decoder(_bl);
		uint32_t position = 0;
		uint32_t peersSize = decoder.decodeInteger(*serializedData, position);
		for(uint32_t i = 0; i < peersSize; i++)
		{
			uint32_t channel = decoder.decodeInteger(*serializedData, position);
			uint32_t peerCount = decoder.decodeInteger(*serializedData, position);
			for(uint32_t j = 0; j < peerCount; j++)
			{
				std::shared_ptr<BaseLib::Systems::BasicPeer> basicPeer(new BaseLib::Systems::BasicPeer());
				basicPeer->isSender = decoder.decodeBoolean(*serializedData, position);
				basicPeer->id = decoder.decodeInteger(*serializedData, position);
				basicPeer->address = decoder.decodeInteger(*serializedData, position);
				basicPeer->channel = decoder.decodeInteger(*serializedData, position);
				basicPeer->physicalIndexOffset = decoder.decodeInteger(*serializedData, position);
				basicPeer->serialNumber = decoder.decodeString(*serializedData, position);
				basicPeer->hidden = decoder.decodeBoolean(*serializedData, position);
				_peers[channel].push_back(basicPeer);
				basicPeer->linkName = decoder.decodeString(*serializedData, position);
				basicPeer->linkDescription = decoder.decodeString(*serializedData, position);
				basicPeer->configEEPROMAddress = decoder.decodeInteger(*serializedData, position);
				uint32_t dataSize = decoder.decodeInteger(*serializedData, position);
				if(position + dataSize <= serializedData->size()) basicPeer->data.insert(basicPeer->data.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
				position += dataSize;
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

int32_t HMWiredPeer::getPhysicalIndexOffset(int32_t channel)
{
	try
	{
		if(!rpcDevice) return 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return 0;
		return rpcDevice->channels.at(channel)->physicalIndexOffset;
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

void HMWiredPeer::restoreLinks()
{
	try
	{
		if(!rpcDevice) return;
		_peers.clear();
		std::map<uint32_t, bool> startIndexes;
		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));
		for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			if(startIndexes.find(i->second->startIndex) != startIndexes.end()) continue;
			if(!i->second->linkRoles) continue;
			bool isSender = false;
			if(!i->second->linkRoles->sourceNames.empty())
			{
				if(!i->second->linkRoles->targetNames.empty())
				{
					GD::out.printWarning("Warning: Channel " + std::to_string(i->first) + " of peer " + std::to_string(_peerID) + " is defined as source and target of a link. I don't know what to do. Not adding link.");
					continue;
				}
				isSender = true;
			}
			startIndexes[i->second->startIndex] = true;
			std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(i->first, BaseLib::RPC::ParameterSet::Type::link);
			if(!parameterSet || parameterSet->addressStart < 0 || parameterSet->addressStep <= 0 || parameterSet->peerAddressOffset < 0) continue;
			int32_t max = parameterSet->addressStart + (parameterSet->count * parameterSet->addressStep);
			for(int32_t currentAddress = parameterSet->addressStart; currentAddress < max; currentAddress += parameterSet->addressStep)
			{
				std::vector<uint8_t> result = getConfigParameter(currentAddress + parameterSet->peerAddressOffset, 4.0, -1, true);
				if(result.size() != 4) continue;
				//Endianness doesn't matter
				if(*((int32_t*)&result.at(0)) == -1) continue;
				int32_t peerAddress = result.at(0) << 24;
				peerAddress |= result.at(1) << 16;
				peerAddress |= result.at(2) << 8;
				peerAddress |= result.at(3);
				if(peerAddress == 0)
				{
					GD::out.printWarning("Warning: Could not add link for peer " + std::to_string(_peerID) + ". The remote peer address is invalid (0x00000000).");
					continue;
				}
				result = getConfigParameter(currentAddress + parameterSet->peerChannelOffset, 1.0, -1, true);
				if(result.size() != 1) continue;
				int32_t tempPeerChannel = result.at(0);
				result = getConfigParameter(currentAddress + parameterSet->channelOffset, 1.0, -1, true);
				if(result.size() != 1) continue;
				int32_t channel = result.at(0) - i->second->physicalIndexOffset;

				std::shared_ptr<HMWiredPeer> remotePeer = central->getPeer(peerAddress);
				if(!remotePeer || !remotePeer->rpcDevice)
				{
					GD::out.printWarning("Warning: Could not add link for peer " + std::to_string(_peerID) + " to peer with address 0x" + BaseLib::HelperFunctions::getHexString(peerAddress, 8) + ". The remote peer is not paired to Homegear.");
					continue;
				}

				std::shared_ptr<BaseLib::Systems::BasicPeer> basicPeer(new BaseLib::Systems::BasicPeer());
				bool channelFound = false;
				for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator j = remotePeer->rpcDevice->channels.begin(); j != remotePeer->rpcDevice->channels.end(); ++j)
				{
					if((signed)j->first + j->second->physicalIndexOffset == tempPeerChannel)
					{
						channelFound = true;
						basicPeer->channel = tempPeerChannel - j->second->physicalIndexOffset;
						basicPeer->physicalIndexOffset = j->second->physicalIndexOffset;
					}
				}
				if(!channelFound)
				{
					GD::out.printWarning("Warning: Could not add link for peer " + std::to_string(_peerID) + " to peer with id " + std::to_string(remotePeer->getID()) + ". The remote channel does not exists.");
					continue;
				}
				basicPeer->isSender = !isSender;
				basicPeer->id = remotePeer->getID();
				basicPeer->address = remotePeer->getAddress();
				basicPeer->serialNumber = remotePeer->getSerialNumber();
				basicPeer->configEEPROMAddress = currentAddress;

				addPeer(channel, basicPeer);
				GD::out.printInfo("Info: Found link between peer " + std::to_string(_peerID) + " (channel " + std::to_string(channel) + ") and peer " + std::to_string(basicPeer->id) + " (channel " + std::to_string(basicPeer->channel) + ").");
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

void HMWiredPeer::reset()
{
	try
	{
		if(!rpcDevice) return;
		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));
		if(!central) return;
		std::vector<uint8_t> data(16, 0xFF);
		for(int32_t i = 0; i < rpcDevice->eepSize; i+=0x10)
		{
			if(!central->writeEEPROM(_address, i, data))
			{
				GD::out.printError("Error: Error resetting HomeMatic Wired peer " + std::to_string(_peerID) + ". Could not write EEPROM.");
				return;
			}
		}
		std::vector<uint8_t> moduleReset({0x21, 0x21});
		central->getResponse(moduleReset, _address, false);
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

int32_t HMWiredPeer::getNewFirmwareVersion()
{
	try
	{
		std::string filenamePrefix = BaseLib::HelperFunctions::getHexString((int32_t)BaseLib::Systems::DeviceFamilies::HomeMaticWired, 4) + "." + BaseLib::HelperFunctions::getHexString(_deviceType.type(), 8);
		std::string versionFile(_bl->settings.firmwarePath() + filenamePrefix + ".version");
		if(!BaseLib::HelperFunctions::fileExists(versionFile)) return 0;
		std::string versionHex = BaseLib::HelperFunctions::getFileContent(versionFile);
		return BaseLib::Math::getNumber(versionHex, true);
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

std::string HMWiredPeer::getFirmwareVersionString(int32_t firmwareVersion)
{
	try
	{
		return GD::bl->hf.getHexString(firmwareVersion >> 8) + "." + GD::bl->hf.getHexString(firmwareVersion & 0xFF);
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

bool HMWiredPeer::firmwareUpdateAvailable()
{
	try
	{
		return _firmwareVersion > 0 && _firmwareVersion < getNewFirmwareVersion();
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
	return false;
}

void HMWiredPeer::getValuesFromPacket(std::shared_ptr<HMWiredPacket> packet, std::vector<FrameValues>& frameValues)
{
	try
	{
		if(!rpcDevice) return;
		//equal_range returns all elements with "0" or an unknown element as argument
		if(packet->messageType() == 0 || rpcDevice->framesByMessageType.find(packet->messageType()) == rpcDevice->framesByMessageType.end()) return;
		std::pair<std::multimap<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator,std::multimap<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByMessageType.equal_range((uint32_t)packet->messageType());
		if(range.first == rpcDevice->framesByMessageType.end()) return;
		std::multimap<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator i = range.first;
		do
		{
			FrameValues currentFrameValues;
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame(i->second);
			if(!frame) continue;
			if(frame->direction == BaseLib::RPC::DeviceFrame::Direction::Enum::fromDevice && packet->senderAddress() != _address) continue;
			if(frame->direction == BaseLib::RPC::DeviceFrame::Direction::Enum::toDevice && packet->destinationAddress() != _address) continue;
			if(packet->payload()->empty()) continue;
			if(frame->subtype > -1 && frame->subtypeIndex >= 9 && (signed)packet->payload()->size() > (frame->subtypeIndex - 9) && packet->payload()->at(frame->subtypeIndex - 9) != (unsigned)frame->subtype) continue;
			int32_t channelIndex = frame->channelField;
			int32_t channel = -1;
			if(channelIndex >= 9 && (signed)packet->payload()->size() > (channelIndex - 9)) channel = packet->payload()->at(channelIndex - 9) - frame->channelIndexOffset;
			if(channel > -1 && frame->channelFieldSize < 1.0) channel &= (0xFF >> (8 - std::lround(frame->channelFieldSize * 10) % 10));
			if(frame->fixedChannel > -1) channel = frame->fixedChannel;
			currentFrameValues.frameID = frame->id;

			for(std::vector<BaseLib::RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				std::vector<uint8_t> data;
				if(j->size > 0 && j->index > 0)
				{
					if(((int32_t)j->index) - 9 >= (signed)packet->payload()->size()) continue;
					data = packet->getPosition(j->index, j->size, -1);

					if(j->constValue > -1)
					{
						int32_t intValue = 0;
						_bl->hf.memcpyBigEndian(intValue, data);
						if(intValue != j->constValue) break; else continue;
					}
				}
				else if(j->constValue > -1)
				{
					_bl->hf.memcpyBigEndian(data, j->constValue);
				}
				else continue;
				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
				{
					if((*k)->physicalParameter->valueID != j->param) continue;
					currentFrameValues.parameterSetType = (*k)->parentParameterSet->type;
					bool setValues = false;
					if(currentFrameValues.paramsetChannels.empty()) //Fill paramsetChannels
					{
						int32_t startChannel = (channel < 0) ? 0 : channel;
						int32_t endChannel;
						//When fixedChannel is -2 (means '*') cycle through all channels
						if(frame->fixedChannel == -2)
						{
							startChannel = 0;
							endChannel = (rpcDevice->channels.end()--)->first;
						}
						else endChannel = startChannel;
						for(int32_t l = startChannel; l <= endChannel; l++)
						{
							if(rpcDevice->channels.find(l) == rpcDevice->channels.end()) continue;
							std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(l, currentFrameValues.parameterSetType);
							if(!parameterSet) continue;
							if(!parameterSet->getParameter((*k)->id)) continue;
							currentFrameValues.paramsetChannels.push_back(l);
							currentFrameValues.values[(*k)->id].channels.push_back(l);
							setValues = true;
						}
					}
					else //Use paramsetChannels
					{
						for(std::list<uint32_t>::const_iterator l = currentFrameValues.paramsetChannels.begin(); l != currentFrameValues.paramsetChannels.end(); ++l)
						{
							if(rpcDevice->channels.find(*l) == rpcDevice->channels.end()) continue;
							std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(*l, currentFrameValues.parameterSetType);
							if(!parameterSet) continue;
							if(!parameterSet->getParameter((*k)->id)) continue;
							currentFrameValues.values[(*k)->id].channels.push_back(*l);
							setValues = true;
						}
					}
					if(setValues) currentFrameValues.values[(*k)->id].value = data;
				}
			}
			if(!currentFrameValues.values.empty()) frameValues.push_back(currentFrameValues);
		} while(++i != range.second && i != rpcDevice->framesByMessageType.end());
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

std::shared_ptr<HMWiredPacket> HMWiredPeer::getResponse(std::shared_ptr<HMWiredPacket> packet)
{
	try
	{
		std::shared_ptr<HMWiredPacket> request(packet);
		std::shared_ptr<HMWiredPacket> response = std::dynamic_pointer_cast<HMWiredCentral>(getCentral())->sendPacket(request, true);
		//Don't send ok here! It's sent in packetReceived if necessary.
		return response;
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
	return std::shared_ptr<HMWiredPacket>();
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredPeer::getValueFromDevice(std::shared_ptr<BaseLib::RPC::Parameter>& parameter, int32_t channel, bool asynchronous)
{
	try
	{
		if(!parameter) return BaseLib::RPC::Variable::createError(-32500, "parameter is nullptr.");
		std::string getRequestFrame = parameter->physicalParameter->getRequest;
		std::string getResponseFrame = parameter->physicalParameter->getResponse;
		if(getRequestFrame.empty()) return BaseLib::RPC::Variable::createError(-6, "Parameter can't be requested actively.");
		if(rpcDevice->framesByID.find(getRequestFrame) == rpcDevice->framesByID.end()) return BaseLib::RPC::Variable::createError(-6, "No frame was found for parameter " + parameter->id);
		std::shared_ptr<BaseLib::RPC::DeviceFrame> frame = rpcDevice->framesByID[getRequestFrame];
		std::shared_ptr<BaseLib::RPC::DeviceFrame> responseFrame;
		if(rpcDevice->framesByID.find(getResponseFrame) != rpcDevice->framesByID.end()) responseFrame = rpcDevice->framesByID[getResponseFrame];

		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(parameter->id) == valuesCentral[channel].end()) return BaseLib::RPC::Variable::createError(-5, "Unknown parameter.");

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, BaseLib::RPC::ParameterSet::Type::Enum::values);
		if(!parameterSet) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");

		std::vector<uint8_t> payload({ (uint8_t)frame->type });
		if(frame->subtype > -1 && frame->subtypeIndex >= 9)
		{
			while((signed)payload.size() - 1 < frame->subtypeIndex - 9) payload.push_back(0);
			payload.at(frame->subtypeIndex - 9) = (uint8_t)frame->subtype;
		}
		if(frame->channelField >= 9)
		{
			while((signed)payload.size() - 1 < frame->channelField - 9) payload.push_back(0);
			payload.at(frame->channelField - 9) = (uint8_t)channel + rpcDevice->channels.at(channel)->physicalIndexOffset;
		}

		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, getCentral()->physicalAddress(), _address, false, _messageCounter, 0, 0, payload));
		for(std::vector<BaseLib::RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				std::vector<uint8_t> data;
				_bl->hf.memcpyBigEndian(data, i->constValue);
				packet->setPosition(i->index, i->size, data);
				continue;
			}

			bool paramFound = false;
			for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = parameterSet->parameters.begin(); j != parameterSet->parameters.end(); ++j)
			{
				//Only compare id. Till now looking for value_id was not necessary.
				if(i->param == (*j)->physicalParameter->id)
				{
					packet->setPosition(i->index, i->size, valuesCentral[channel][(*j)->id].data);
					paramFound = true;
					break;
				}
			}
			if(!paramFound) GD::out.printError("Error constructing packet. param \"" + i->param + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
		}
		setMessageCounter(_messageCounter + 1);

		std::shared_ptr<HMWiredPacket> response = getResponse(packet);
		if(!response) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));

		return parameter->convertFromPacket(valuesCentral[channel][parameter->id].data, true);
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
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::ParameterSet> HMWiredPeer::getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet;
		if(rpcChannel->specialParameter && rpcChannel->subconfig)
		{
			std::vector<uint8_t> value = getMasterConfigParameter(channel - rpcChannel->startIndex, rpcChannel->specialParameter->physicalParameter->address.index, rpcChannel->specialParameter->physicalParameter->address.step, rpcChannel->specialParameter->physicalParameter->size);
			if(value.at(0))
			{
				if(rpcChannel->subconfig->parameterSets.find(type) == rpcChannel->subconfig->parameterSets.end())
				{
					GD::out.printWarning("Parameter set of type " + std::to_string(type) + " not found in subconfig for channel " + std::to_string(channel));
					return std::shared_ptr<BaseLib::RPC::ParameterSet>();
				}
				parameterSet = rpcChannel->subconfig->parameterSets[type];
			} else parameterSet = rpcChannel->parameterSets[type];
		}
		else
		{
			if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end())
			{
				GD::out.printWarning("Parameter set of type " + std::to_string(type) + " not found for channel " + std::to_string(channel));
				return std::shared_ptr<BaseLib::RPC::ParameterSet>();
			}
			parameterSet = rpcChannel->parameterSets[type];
		}
		return parameterSet;
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
	return std::shared_ptr<BaseLib::RPC::ParameterSet>();
}

void HMWiredPeer::packetReceived(std::shared_ptr<HMWiredPacket> packet)
{
	try
	{
		if(ignorePackets || !packet || packet->type() != HMWiredPacketType::iMessage) return;
		if(!_centralFeatures || _disposing) return;
		if(packet->senderAddress() != _address) return;
		if(!rpcDevice) return;
		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));
		if(!central) return;
		setLastPacketReceived();
		serviceMessages->endUnreach();
		std::vector<FrameValues> frameValues;
		getValuesFromPacket(packet, frameValues);
		std::shared_ptr<HMWiredPacket> sentPacket;
		std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> valueKeys;
		std::map<uint32_t, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>>> rpcValues;
		//Loop through all matching frames
		for(std::vector<FrameValues>::iterator a = frameValues.begin(); a != frameValues.end(); ++a)
		{
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame;
			if(!a->frameID.empty()) frame = rpcDevice->framesByID.at(a->frameID);

			std::vector<FrameValues> sentFrameValues;
			if(packet->type() == HMWiredPacketType::ackMessage) //ACK packet: Check if all values were set correctly. If not set value again
			{
				sentPacket = central->getSentPacket(_address);
				if(sentPacket && sentPacket->messageType() > 0 && !sentPacket->payload()->empty())
				{
					getValuesFromPacket(sentPacket, sentFrameValues);
				}
			}

			for(std::map<std::string, FrameValue>::iterator i = a->values.begin(); i != a->values.end(); ++i)
			{
				for(std::list<uint32_t>::const_iterator j = a->paramsetChannels.begin(); j != a->paramsetChannels.end(); ++j)
				{
					if(std::find(i->second.channels.begin(), i->second.channels.end(), *j) == i->second.channels.end()) continue;
					if(!valueKeys[*j] || !rpcValues[*j])
					{
						valueKeys[*j].reset(new std::vector<std::string>());
						rpcValues[*j].reset(new std::vector<std::shared_ptr<BaseLib::RPC::Variable>>());
					}
					BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
					if(rpcDevice->channels.find(*j) == rpcDevice->channels.end())
					{
						GD::out.printWarning("Warning: Can't set value of parameter " + i->first + " for channel " + std::to_string(*j) + ". Channel not found.");
						continue;
					}
					std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(*j, a->parameterSetType);
					if(!parameterSet)
					{
						GD::out.printWarning("Warning: Can't set value of parameter " + i->first + " for channel " + std::to_string(*j) + ". Value parameter set not found.");
						continue;
					}
					std::shared_ptr<BaseLib::RPC::Parameter> currentParameter = parameterSet->getParameter(i->first);
					if(!currentParameter)
					{
						GD::out.printWarning("Warning: Can't set value of parameter " + i->first + " for channel " + std::to_string(*j) + ". Value parameter set not found.");
						continue;
					}
					parameter->data = i->second.value;
					saveParameter(parameter->databaseID, a->parameterSetType, *j, i->first, parameter->data);
					if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + i->first + " of HomeMatic Wired peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(i->second.value) + ".");

					 //Process service messages
					if((currentParameter->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !i->second.value.empty())
					{
						if(currentParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeEnum)
						{
							serviceMessages->set(i->first, i->second.value.at(0), *j);
						}
						else if(currentParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
						{
							serviceMessages->set(i->first, (bool)i->second.value.at(0));
						}
					}

					valueKeys[*j]->push_back(i->first);
					rpcValues[*j]->push_back(currentParameter->convertFromPacket(i->second.value, true));
				}
			}
		}
		if(packet->type() == HMWiredPacketType::iMessage && packet->destinationAddress() == central->getAddress())
		{
			if(packet->timeReceived() != 0 && BaseLib::HelperFunctions::getTime() - packet->timeReceived() < 150) central->sendOK(packet->senderMessageCounter(), packet->senderAddress());
		}
		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::const_iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;
				std::string address(_serialNumber + ":" + std::to_string(j->first));
				raiseEvent(_peerID, j->first, j->second, rpcValues.at(j->first));
				raiseRPCEvent(_peerID, j->first, address, j->second, rpcValues.at(j->first));
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

std::shared_ptr<BaseLib::RPC::Variable> HMWiredPeer::getDeviceInfo(std::map<std::string, bool> fields)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::Variable> info(Peer::getDeviceInfo(fields));
		if(info->errorStruct) return info;

		if(fields.empty() || fields.find("INTERFACE") != fields.end()) info->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(GD::physicalInterface->getID()))));

		return info;
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
    return std::shared_ptr<BaseLib::RPC::Variable>();
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredPeer::getParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end()) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, type);
		if(!parameterSet) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::Variable> variables(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));

		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::internal) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::transform))
			{
				GD::out.printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			std::shared_ptr<BaseLib::RPC::Variable> element;
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
			{
				if(!((*i)->operations & BaseLib::RPC::Parameter::Operations::read) && !((*i)->operations & BaseLib::RPC::Parameter::Operations::event)) continue;
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = (*i)->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}
			else if(type == BaseLib::RPC::ParameterSet::Type::Enum::master)
			{
				std::vector<uint8_t> value = getMasterConfigParameter(channel, parameterSet, *i);
				if(value.empty()) return BaseLib::RPC::Variable::createError(-32500, "Could not read config parameter. See log for more details.");
				element = (*i)->convertFromPacket(value);
			}
			else if(type == BaseLib::RPC::ParameterSet::Type::Enum::link)
			{
				std::shared_ptr<BaseLib::Systems::BasicPeer> remotePeer;
				if(remoteID > 0) remotePeer = getPeer(channel, remoteID, remoteChannel);
				if(!remotePeer) return BaseLib::RPC::Variable::createError(-3, "Not paired to this peer.");
				if(remotePeer->configEEPROMAddress == -1) return BaseLib::RPC::Variable::createError(-3, "No parameter set eeprom address set.");
				if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1) return BaseLib::RPC::Variable::createError(-3, "Storage type of link parameter set not supported.");

				std::vector<uint8_t> value = getConfigParameter(remotePeer->configEEPROMAddress + (*i)->physicalParameter->address.index, (*i)->physicalParameter->size);
				if(value.empty()) return BaseLib::RPC::Variable::createError(-32500, "Could not read config parameter. See log for more details.");
				element = (*i)->convertFromPacket(value);
			}

			if(!element) continue;
			if(element->type == BaseLib::RPC::VariableType::rpcVoid) continue;
			variables->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, element));
		}
		return variables;
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredPeer::getParamsetDescription(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel");
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end()) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set");
		if(type == BaseLib::RPC::ParameterSet::Type::link && remoteID > 0)
		{
			std::shared_ptr<BaseLib::Systems::BasicPeer> remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return BaseLib::RPC::Variable::createError(-2, "Unknown remote peer.");
		}

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet;
		if(rpcChannel->specialParameter && rpcChannel->subconfig)
		{
			std::vector<uint8_t> value = getMasterConfigParameter(channel - rpcChannel->startIndex, rpcChannel->specialParameter->physicalParameter->address.index, rpcChannel->specialParameter->physicalParameter->address.step, rpcChannel->specialParameter->physicalParameter->size);
			GD::out.printDebug("Debug: Special parameter is " + std::to_string(value.at(0)));
			if(value.at(0))
			{
				if(rpcChannel->subconfig->parameterSets.find(type) == rpcChannel->subconfig->parameterSets.end()) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set");
				parameterSet = rpcChannel->subconfig->parameterSets[type];
			} else parameterSet = rpcChannel->parameterSets[type];
		}
		else parameterSet = rpcChannel->parameterSets[type];

		return Peer::getParamsetDescription(parameterSet);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredPeer::putParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::Variable> variables, bool onlyPushing)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::Variable::createError(-2, "Not a central peer.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, type);
		if(!parameterSet) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty()) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));

		_pingThreadMutex.lock();
		_lastPing = BaseLib::HelperFunctions::getTime(); //No ping now
		if(_pingThread.joinable()) _pingThread.join();
		_pingThreadMutex.unlock();

		std::map<int32_t, bool> changedBlocks;
		if(type == BaseLib::RPC::ParameterSet::Type::Enum::master)
		{
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::shared_ptr<BaseLib::RPC::Parameter> currentParameter = parameterSet->getParameter(i->first);
				if(!currentParameter) continue;
				std::vector<uint8_t> value;
				currentParameter->convertToPacket(i->second, value);
				std::vector<int32_t> result;
				if(currentParameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::Enum::eeprom) result = setMasterConfigParameter(channel, parameterSet, currentParameter, value);
				else if(currentParameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::Enum::store)
				{
					if(configCentral.find(channel) == configCentral.end() || configCentral[channel].find(i->first) == configCentral[channel].end()) continue;
					BaseLib::Systems::RPCConfigurationParameter* parameter = &configCentral[channel][i->first];
					parameter->data = value;
					saveParameter(parameter->databaseID, parameter->data);
				}
				GD::out.printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(value) + ".");
				//Only send to device when parameter is of type eeprom
				if(currentParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::eeprom) continue;
				for(std::vector<int32_t>::iterator i = result.begin(); i != result.end(); ++i) changedBlocks[*i] = true;
			}
		}
		else if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
		{
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				setValue(channel, i->first, i->second);
			}
		}
		else if(type == BaseLib::RPC::ParameterSet::Type::Enum::link)
		{
			std::shared_ptr<BaseLib::Systems::BasicPeer> remotePeer;
			if(remoteID > 0) remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return BaseLib::RPC::Variable::createError(-3, "Not paired to this peer.");
			if(remotePeer->configEEPROMAddress == -1) return BaseLib::RPC::Variable::createError(-3, "No parameter set eeprom address set.");
			if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1) return BaseLib::RPC::Variable::createError(-3, "Storage type of link parameter set not supported.");

			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::shared_ptr<BaseLib::RPC::Parameter> currentParameter = parameterSet->getParameter(i->first);
				if(!currentParameter) continue;
				if(currentParameter->physicalParameter->address.operation == BaseLib::RPC::PhysicalParameterAddress::Operation::Enum::none) continue;
				std::vector<uint8_t> value;
				currentParameter->convertToPacket(i->second, value);
				std::vector<int32_t> result = setConfigParameter(remotePeer->configEEPROMAddress + currentParameter->physicalParameter->address.index, currentParameter->physicalParameter->size, value);
				GD::out.printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(value) + ".");
				//Only send to device when parameter is of type eeprom
				if(currentParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::eeprom) continue;
				for(std::vector<int32_t>::iterator i = result.begin(); i != result.end(); ++i) changedBlocks[*i] = true;
			}
		}

		if(changedBlocks.empty()) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));

		std::shared_ptr<HMWiredCentral> central(std::dynamic_pointer_cast<HMWiredCentral>(getCentral()));
		for(std::map<int32_t, bool>::iterator i = changedBlocks.begin(); i != changedBlocks.end(); ++i)
		{
			if(!central->writeEEPROM(_address, i->first, binaryConfig[i->first].data))
			{
				GD::out.printError("Error: Could not write config to device's eeprom.");
				return BaseLib::RPC::Variable::createError(-32500, "Could not write config to device's eeprom.");
			}
		}
		raiseRPCUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), 0);

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> HMWiredPeer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::Variable> value)
{
	try
	{
		Peer::setValue(channel, valueKey, value);
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::Variable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return BaseLib::RPC::Variable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return BaseLib::RPC::Variable::createError(-5, "Unknown parameter.");

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, BaseLib::RPC::ParameterSet::Type::Enum::values);
		if(!parameterSet) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = parameterSet->getParameter(valueKey);
		if(!rpcParameter) return BaseLib::RPC::Variable::createError(-5, "Unknown parameter.");
		if(rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeAction && !value->booleanValue) return BaseLib::RPC::Variable::createError(-5, "Parameter of type action cannot be set to \"false\".");
		BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values(new std::vector<std::shared_ptr<BaseLib::RPC::Variable>>());
		if((rpcParameter->operations & BaseLib::RPC::Parameter::Operations::read) || (rpcParameter->operations & BaseLib::RPC::Parameter::Operations::event))
		{
			valueKeys->push_back(valueKey);
			values->push_back(value);
		}
		if(rpcParameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::Enum::store)
		{
			rpcParameter->convertToPacket(value, parameter->data);
			saveParameter(parameter->databaseID, BaseLib::RPC::ParameterSet::Type::Enum::values, channel, valueKey, parameter->data);
			if(!valueKeys->empty()) raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
		}
		else if(rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::command) return BaseLib::RPC::Variable::createError(-6, "Parameter is not settable.");
		if(!rpcParameter->conversion.empty() && rpcParameter->conversion.at(0)->type == BaseLib::RPC::ParameterConversion::Type::Enum::toggle)
		{
			//Handle toggle parameter
			std::string toggleKey = rpcParameter->conversion.at(0)->stringValue;
			if(toggleKey.empty()) return BaseLib::RPC::Variable::createError(-6, "No toggle parameter specified (parameter attribute value is empty).");
			if(valuesCentral[channel].find(toggleKey) == valuesCentral[channel].end()) return BaseLib::RPC::Variable::createError(-5, "Toggle parameter not found.");
			BaseLib::Systems::RPCConfigurationParameter* toggleParam = &valuesCentral[channel][toggleKey];
			std::shared_ptr<BaseLib::RPC::Parameter> toggleRPCParam = parameterSet->getParameter(toggleKey);
			if(!toggleRPCParam) return BaseLib::RPC::Variable::createError(-5, "Toggle parameter not found.");
			std::shared_ptr<BaseLib::RPC::Variable> toggleValue;
			if(toggleRPCParam->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
			{
				toggleValue = toggleRPCParam->convertFromPacket(toggleParam->data);
				toggleValue->booleanValue = !toggleValue->booleanValue;
			}
			else if(toggleRPCParam->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeInteger ||
					toggleRPCParam->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeFloat)
			{
				int32_t currentToggleValue = (int32_t)toggleParam->data.at(0);
				std::vector<uint8_t> temp({0});
				if(currentToggleValue != toggleRPCParam->conversion.at(0)->on) temp.at(0) = toggleRPCParam->conversion.at(0)->on;
				else temp.at(0) = toggleRPCParam->conversion.at(0)->off;
				toggleValue = toggleRPCParam->convertFromPacket(temp);
			}
			else return BaseLib::RPC::Variable::createError(-6, "Toggle parameter has to be of type boolean, float or integer.");
			return setValue(channel, toggleKey, toggleValue);
		}
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return BaseLib::RPC::Variable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return BaseLib::RPC::Variable::createError(-6, "No frame was found for parameter " + valueKey);
		std::shared_ptr<BaseLib::RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		std::vector<uint8_t> data;
		rpcParameter->convertToPacket(value, data);
		parameter->data = data;
		saveParameter(parameter->databaseID, BaseLib::RPC::ParameterSet::Type::Enum::values, channel, valueKey, data);
		if(_bl->debugLevel > 4) GD::out.printDebug("Debug: " + valueKey + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to " + BaseLib::HelperFunctions::getHexString(data) + ".");

		std::vector<uint8_t> payload({ (uint8_t)frame->type });
		if(frame->subtype > -1 && frame->subtypeIndex >= 9)
		{
			while((signed)payload.size() - 1 < frame->subtypeIndex - 9) payload.push_back(0);
			payload.at(frame->subtypeIndex - 9) = (uint8_t)frame->subtype;
		}
		if(frame->channelField >= 9)
		{
			while((signed)payload.size() - 1 < frame->channelField - 9) payload.push_back(0);
			payload.at(frame->channelField - 9) = (uint8_t)channel + rpcDevice->channels.at(channel)->physicalIndexOffset;
		}

		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, getCentral()->physicalAddress(), _address, false, _messageCounter, 0, 0, payload));
		for(std::vector<BaseLib::RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				std::vector<uint8_t> data;
				_bl->hf.memcpyBigEndian(data, i->constValue);
				packet->setPosition(i->index, i->size, data);
				continue;
			}
			BaseLib::Systems::RPCConfigurationParameter* additionalParameter = nullptr;
			//We can't just search for param, because it is ambiguous (see for example LEVEL for HM-CC-TC.
			if(i->param.empty() && !i->additionalParameter.empty() && valuesCentral[channel].find(i->additionalParameter) != valuesCentral[channel].end())
			{
				additionalParameter = &valuesCentral[channel][i->additionalParameter];
				int32_t intValue = 0;
				_bl->hf.memcpyBigEndian(intValue, additionalParameter->data);
				if(!i->omitIfSet || intValue != i->omitIf)
				{
					//Don't set ON_TIME when value is false
					if(i->additionalParameter != "ON_TIME" || (rpcParameter->physicalParameter->valueID == "STATE" && value->booleanValue) || (rpcParameter->physicalParameter->valueID == "LEVEL" && value->floatValue > 0)) packet->setPosition(i->index, i->size, additionalParameter->data);
				}
			}
			//param sometimes is ambiguous (e. g. LEVEL of HM-CC-TC), so don't search and use the given parameter when possible
			else if(i->param == rpcParameter->physicalParameter->valueID || i->param == rpcParameter->physicalParameter->id) packet->setPosition(i->index, i->size, valuesCentral[channel][valueKey].data);
			//Search for all other parameters
			else
			{
				bool paramFound = false;
				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = parameterSet->parameters.begin(); j != parameterSet->parameters.end(); ++j)
				{
					//Only compare id. Till now looking for value_id was not necessary.
					if(i->param == (*j)->physicalParameter->id)
					{
						packet->setPosition(i->index, i->size, valuesCentral[channel][(*j)->id].data);
						paramFound = true;
						break;
					}
				}
				if(!paramFound) GD::out.printError("Error constructing packet. param \"" + i->param + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
			}
		}
		if(!rpcParameter->physicalParameter->resetAfterSend.empty())
		{
			for(std::vector<std::string>::iterator j = rpcParameter->physicalParameter->resetAfterSend.begin(); j != rpcParameter->physicalParameter->resetAfterSend.end(); ++j)
			{
				if(valuesCentral.at(channel).find(*j) == valuesCentral.at(channel).end()) continue;
				std::shared_ptr<BaseLib::RPC::Parameter> currentParameter = parameterSet->getParameter(*j);
				if(!currentParameter) continue;
				std::shared_ptr<BaseLib::RPC::Variable> logicalDefaultValue = currentParameter->logicalParameter->getDefaultValue();
				std::vector<uint8_t> defaultValue;
				currentParameter->convertToPacket(logicalDefaultValue, defaultValue);
				if(defaultValue != valuesCentral.at(channel).at(*j).data)
				{
					BaseLib::Systems::RPCConfigurationParameter* tempParam = &valuesCentral.at(channel).at(*j);
					tempParam->data = defaultValue;
					saveParameter(tempParam->databaseID, BaseLib::RPC::ParameterSet::Type::Enum::values, channel, *j, tempParam->data);
					GD::out.printInfo( "Info: Parameter \"" + *j + "\" was reset to " + BaseLib::HelperFunctions::getHexString(defaultValue) + ". Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					if((rpcParameter->operations & BaseLib::RPC::Parameter::Operations::read) || (rpcParameter->operations & BaseLib::RPC::Parameter::Operations::event))
					{
						valueKeys->push_back(*j);
						values->push_back(logicalDefaultValue);
					}
				}
			}
		}
		setMessageCounter(_messageCounter + 1);

		std::shared_ptr<HMWiredPacket> response = getResponse(packet);
		if(!response)
		{
			GD::out.printWarning("Error: Error sending packet to peer " + std::to_string(_peerID) + ". Peer did not respond.");
			return BaseLib::RPC::Variable::createError(-32500, "Error sending packet to peer. Peer did not respond.");
		}
		else
		{
			if(!valueKeys->empty()) raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

} /* namespace HMWired */
