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
#include "../../GD/GD.h"

namespace HMWired
{
std::shared_ptr<HMWiredCentral> HMWiredPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = std::dynamic_pointer_cast<HMWiredCentral>(GD::deviceFamilies.at(DeviceFamilies::HomeMaticWired)->getCentral());
		return _central;
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
	return std::shared_ptr<HMWiredCentral>();
}

HMWiredPeer::HMWiredPeer(uint32_t parentID, bool centralFeatures) : Peer(parentID, centralFeatures)
{
	if(centralFeatures) serviceMessages.reset(new ServiceMessages(this));
}

HMWiredPeer::HMWiredPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures) : Peer(id, address, serialNumber, parentID, centralFeatures)
{
}

HMWiredPeer::~HMWiredPeer()
{

}

void HMWiredPeer::initializeCentralConfig()
{
	try
	{
		if(!rpcDevice)
		{
			Output::printWarning("Warning: Tried to initialize HomeMatic Wired peer's central config without rpcDevice being set.");
			return;
		}
		GD::db.executeCommand("SAVEPOINT hmWiredPeerConfig" + std::to_string(_address));
		RPCConfigurationParameter parameter;
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			if(i->second->parameterSets.find(RPC::ParameterSet::Type::values) != i->second->parameterSets.end() && i->second->parameterSets[RPC::ParameterSet::Type::values])
			{
				std::shared_ptr<RPC::ParameterSet> valueSet = i->second->parameterSets[RPC::ParameterSet::Type::values];
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = valueSet->parameters.begin(); j != valueSet->parameters.end(); ++j)
				{
					if(!(*j)->id.empty() && valuesCentral[i->first].find((*j)->id) == valuesCentral[i->first].end())
					{
						parameter = RPCConfigurationParameter();
						parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
						saveParameter(0, RPC::ParameterSet::Type::values, i->first, (*j)->id, parameter.data);
						valuesCentral[i->first][(*j)->id] = parameter;
					}
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
    GD::db.executeCommand("RELEASE hmWiredPeerConfig" + std::to_string(_address));
}

void HMWiredPeer::initializeLinkConfig(int32_t channel, std::shared_ptr<BasicPeer> peer)
{
	try
	{
		std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, RPC::ParameterSet::Type::Enum::link);
		if(!parameterSet)
		{
			Output::printError("Error: No link parameter set found.");
			return;
		}
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		//Check if address data is continuous
		int32_t min = parameterSet->channelOffset;
		if(parameterSet->peerAddressOffset < min) min = parameterSet->peerAddressOffset;
		if(parameterSet->peerChannelOffset < min) min = parameterSet->peerChannelOffset;
		int32_t max = parameterSet->peerChannelOffset;
		if(parameterSet->peerAddressOffset + 3 > max) max = parameterSet->peerAddressOffset;
		if(parameterSet->channelOffset > max) max = parameterSet->channelOffset;
		if(max - min != 5)
		{
			Output::printError("Error: Address format of parameter set is not supported.");
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
			Output::printError("Error: Link config's EEPROM address is invalid.");
			return;
		}
		std::vector<int32_t> configBlocks = setConfigParameter((double)peer->configEEPROMAddress, 6.0, data);
		for(std::vector<int32_t>::iterator i = configBlocks.begin(); i != configBlocks.end(); ++i)
		{
			std::vector<uint8_t> configBlock = binaryConfig.at(*i).data;
			if(!getCentral()->writeEEPROM(_address, *i, configBlock)) Output::printError("Error: Could not write config to device's eeprom.");
		}

		if(!peer->isSender) return; //Nothing more to do

		std::shared_ptr<RPC::RPCVariable> variables(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			variables->structValue->insert(RPC::RPCStructElement((*i)->id, (*i)->logicalParameter->getDefaultValue()));
		}
		std::shared_ptr<RPC::RPCVariable> result = putParamset(channel, RPC::ParameterSet::Type::Enum::link, peer->id, peer->channel, variables);
		if(result->errorStruct) Output::printError("Error: " + result->structValue->at("faultString")->stringValue);
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
			for(std::unordered_map<uint32_t, ConfigDataBlock>::iterator i = binaryConfig.begin(); i != binaryConfig.end(); ++i)
			{
				stringStream << "0x" << std::hex << std::setfill('0') << std::setw(4) << i->first << "\t" << HelperFunctions::getHexString(i->second.data) << std::dec << std::endl;
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
			for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::iterator i = _peers.begin(); i != _peers.end(); ++i)
			{
				for(std::vector<std::shared_ptr<BasicPeer>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
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
			if(binaryConfig.find(address1) == binaryConfig.end()) binaryConfig[address1].data = getCentral()->readEEPROM(_address, address1);
			if(binaryConfig.find(address2) == binaryConfig.end()) binaryConfig[address2].data = getCentral()->readEEPROM(_address, address2);
			if(binaryConfig.find(address3) == binaryConfig.end()) binaryConfig[address3].data = getCentral()->readEEPROM(_address, address3);
			std::vector<uint8_t> oldConfig1 = binaryConfig.at(address1).data;
			std::vector<uint8_t> oldConfig2 = binaryConfig.at(address2).data;
			std::vector<uint8_t> oldConfig3 = binaryConfig.at(address3).data;

			//Test 1: Set two bytes within one config data block
			stringStream << "EEPROM before test 1:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x355.0, 2.0, data)\" with data 0xABCD:" << std::endl;
			std::vector<uint8_t> data({0xAB, 0xCD});
			setConfigParameter(853.0, 2.0, data);
			data = getConfigParameter(853.0, 2.0);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 1:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 1

			//Test 2: Set four bytes spanning over two data blocks
			stringStream << "EEPROM before test 2:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x35E.0, 4.0, data)\" with data 0xABCDEFAC:" << std::endl;
			data = std::vector<uint8_t>({0xAB, 0xCD, 0xEF, 0xAC});
			setConfigParameter(862.0, 4.0, data);
			data = getConfigParameter(862.0, 4.0);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 2:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 2

			//Test 3: Set 20 bytes spanning over three data blocks
			stringStream << "EEPROM before test 3:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x35F.0, 20.0, data)\" with data 0xABCDEFAC112233445566778899AABBCCDDEE1223:" << std::endl;
			data = std::vector<uint8_t>({0xAB, 0xCD, 0xEF, 0xAC, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x12, 0x23});
			setConfigParameter(863.0, 20.0, data);
			data = getConfigParameter(863.0, 20.0);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 3:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 3

			binaryConfig.at(address1).data = oldConfig1;
			binaryConfig.at(address2).data = oldConfig2;
			binaryConfig.at(address3).data = oldConfig3;

			//Test 4: Size 1.4
			stringStream << "EEPROM before test 4:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x352.0, 1.4, data)\" with data 0x0B5E:" << std::endl;
			data = std::vector<uint8_t>({0x0B, 0x5E});
			setConfigParameter(850.0, 1.4, data);
			data = getConfigParameter(850.0, 1.4);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 4:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address1 << "\t" << HelperFunctions::getHexString(binaryConfig[address1].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 4

			//Test 5: Size 0.6
			stringStream << "EEPROM before test 5:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x375.0, 0.6, data)\" with data 0x15:" << std::endl;
			data = std::vector<uint8_t>({0x15});
			setConfigParameter(885.0, 0.6, data);
			data = getConfigParameter(885.0, 0.6);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 5:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 5

			//Test 6: Size 0.6, index offset 0.2
			stringStream << "EEPROM before test 6:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x377.2, 0.6, data)\" with data 0x15:" << std::endl;
			data = std::vector<uint8_t>({0x15});
			setConfigParameter(887.2, 0.6, data);
			data = getConfigParameter(887.2, 0.6);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 6:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 6

			//Test 7: Size 0.3, index offset 0.6
			stringStream << "EEPROM before test 7:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x379.6, 0.3, data)\" with data 0x02:" << std::endl;
			data = std::vector<uint8_t>({0x02});
			setConfigParameter(889.6, 0.3, data);
			data = getConfigParameter(889.6, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 7:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 7

			//Set 0x36F to 0xFF for test
			data = std::vector<uint8_t>({0xFF});
			setConfigParameter(879.0, 1.0, data);

			//Test 8: Size 0.3, index offset 0.6 spanning over two data blocks
			stringStream << "EEPROM before test 8:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x36F.6, 0.3, data)\" with data 0x02:" << std::endl;
			data = std::vector<uint8_t>({0x02});
			setConfigParameter(879.6, 0.3, data);
			data = getConfigParameter(879.6, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 8:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 8

			//Set 0x36F and 0x370 to 0xFF for test
			data = std::vector<uint8_t>({0xFF});
			setConfigParameter(879.0, 1.0, data);
			setConfigParameter(880.0, 1.0, data);

			//Test 9: Size 0.5, index offset 0.6 spanning over two data blocks
			stringStream << "EEPROM before test 9:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(0x36F.6, 0.5, data)\" with data 0x0A:" << std::endl;
			data = std::vector<uint8_t>({0x0A});
			setConfigParameter(879.6, 0.5, data);
			data = getConfigParameter(879.6, 0.5);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 9:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address2 << "\t" << HelperFunctions::getHexString(binaryConfig[address2].data) << std::dec << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 9

			//Test 10: Test of steps: channelIndex 5, index offset 0.2, step 0.3
			stringStream << "EEPROM before test 10:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(5, 881.2, 0.3, 0.3, data)\" with data 0x03:" << std::endl;
			data = std::vector<uint8_t>({0x03});
			setMasterConfigParameter(5, 881.2, 0.3, 0.3, data);
			data = getMasterConfigParameter(5, 881.2, 0.3, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 10:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl << "============================================" << std::endl << std::endl;
			//End Test 10

			//Test 11: Test of steps: channelIndex 4, index offset 0.2, step 0.3
			stringStream << "EEPROM before test 11:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
			stringStream << std::endl;

			stringStream << "Executing \"setConfigParameter(4, 0x371.2, 0.3, 0.3, data)\" with data 0x02:" << std::endl;
			data = std::vector<uint8_t>({0x02});
			setMasterConfigParameter(4, 881.2, 0.3, 0.3, data);
			data = getMasterConfigParameter(4, 881.2, 0.3, 0.3);
			stringStream << "\"getConfigParameter\" returned: 0x" << HelperFunctions::getHexString(data) << std::endl;
			stringStream << std::endl;

			stringStream << "EEPROM after test 11:" << std::endl;
			stringStream << "    Address\tData" << std::endl;
			stringStream << "    0x" << std::hex << std::setfill('0') << std::setw(4) << address3 << "\t" << HelperFunctions::getHexString(binaryConfig[address3].data) << std::dec << std::endl;
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
    return "Error executing command. See log file for more details.\n";
}

std::vector<int32_t> HMWiredPeer::setConfigParameter(double index, double size, std::vector<uint8_t>& binaryValue)
{
	std::vector<int32_t> changedBlocks;
	try
	{
		if(size < 0 || index < 0)
		{
			Output::printError("Error: Can't set configuration parameter. Index or size is negative.");
			return changedBlocks;
		}
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
				Output::printError("Error: HomeMatic Wired peer " + std::to_string(_peerID) + ": Can't set partial byte index > 1.");
				return changedBlocks;
			}
			uint32_t bitSize = std::lround(size * 10);
			if(bitSize > 8) bitSize = 8;
			uint32_t indexBits = std::lround(index * 10) % 10;
			if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
			{
				binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
				saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
			}
			ConfigDataBlock* configBlock = &binaryConfig[configBlockIndex];
			if(configBlock->data.size() != 0x10)
			{
				Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
				return changedBlocks;
			}
			if(bitSize == 8) configBlock->data.at(intByteIndex) = 0;
			else configBlock->data.at(intByteIndex) &= (~(_bitmask[bitSize] << indexBits));
			configBlock->data.at(intByteIndex) |= (binaryValue.at(binaryValue.size() - 1) << indexBits);
			if(indexBits + bitSize > 8) //Spread over two bytes
			{
				uint32_t missingBits = (indexBits + bitSize) - 8;
				intByteIndex++;
				if(intByteIndex >= 0x10)
				{
					saveParameter(configBlock->databaseID, configBlockIndex, configBlock->data);
					configBlockIndex += 0x10;
					changedBlocks.push_back(configBlockIndex);
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
						saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
					}
					configBlock = &binaryConfig[configBlockIndex];
					if(configBlock->data.size() != 0x10)
					{
						Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
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
				binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
				saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
			}
			ConfigDataBlock* configBlock = &binaryConfig[configBlockIndex];
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
							binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
							saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
						}
						configBlock = &binaryConfig[configBlockIndex];
						if(configBlock->data.size() != 0x10)
						{
							Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
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
								binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
								saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
							}
							configBlock = &binaryConfig[configBlockIndex];
							if(configBlock->data.size() != 0x10)
							{
								Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
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
							binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
							saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
						}
						configBlock = &binaryConfig[configBlockIndex];
						if(configBlock->data.size() != 0x10)
						{
							Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
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
    return std::vector<int32_t>();
}

std::vector<int32_t> HMWiredPeer::setMasterConfigParameter(int32_t channel, std::shared_ptr<RPC::ParameterSet> parameterSet, std::shared_ptr<RPC::Parameter> parameter, std::vector<uint8_t>& binaryValue)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return std::vector<int32_t>();
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(parameter->physicalParameter->address.operation == RPC::PhysicalParameterAddress::Operation::none)
		{
			return setMasterConfigParameter(channel - rpcChannel->startIndex, parameter->physicalParameter->address.index, parameter->physicalParameter->address.step, parameter->physicalParameter->size, binaryValue);
		}
		else
		{
			if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1)
			{
				Output::printError("Error: Can't get parameter set. address_start or address_step is not set.");
				return std::vector<int32_t>();
			}
			int32_t channelIndex = channel - rpcChannel->startIndex;
			if(parameterSet->count > 0 && channelIndex >= parameterSet->count)
			{
				Output::printError("Error: Can't get parameter set. Out of bounds.");
				return std::vector<int32_t>();
			}
			return setMasterConfigParameter(channelIndex, parameterSet->addressStart, parameterSet->addressStep, parameter->physicalParameter->address.index, parameter->physicalParameter->size, binaryValue);
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
			Output::printError("Error: Can't get configuration parameter. Index or size is negative.");
			result.push_back(0);
			return result;
		}
		double byteIndex = std::floor(index);
		int32_t intByteIndex = byteIndex;
		int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
		if(configBlockIndex >= rpcDevice->eepSize)
		{
			Output::printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
			result.push_back(0);
			return result;
		}
		intByteIndex = intByteIndex - configBlockIndex;
		if(configBlockIndex >= rpcDevice->eepSize)
		{
			Output::printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
			result.clear();
			result.push_back(0);
			return result;
		}
		if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
		{
			if(onlyKnownConfig) return std::vector<uint8_t>();
			binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
		}
		std::vector<uint8_t>* configBlock = &binaryConfig[configBlockIndex].data;
		if(configBlock->size() != 0x10)
		{
			Output::printError("Error: Can't get configuration parameter. Can't read EEPROM.");
			result.push_back(0);
			return result;
		}
		if(byteIndex != index || size < 0.8) //0.8 == 8 Bits
		{
			if(size > 1)
			{
				Output::printError("Error: Can't get configuration parameter. Partial byte index > 1 requested.");
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
						Output::printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						if(onlyKnownConfig) return std::vector<uint8_t>();
						binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
					}
					configBlock = &binaryConfig[configBlockIndex].data;
					if(configBlock->size() != 0x10)
					{
						Output::printError("Error: Can't get configuration parameter. Can't read EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					intByteIndex = 0;
				}
				uint32_t missingBits = (indexBits + bitSize) - 8;
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
						Output::printError("Error: Can't get configuration parameter. Index is larger than EEPROM.");
						result.clear();
						result.push_back(0);
						return result;
					}
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						if(onlyKnownConfig) return std::vector<uint8_t>();
						binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
					}
					configBlock = &binaryConfig[configBlockIndex].data;
					if(configBlock->size() != 0x10)
					{
						Output::printError("Error: Can't get configuration parameter. Can't read EEPROM.");
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
    return std::vector<uint8_t>();
}

std::vector<uint8_t> HMWiredPeer::getMasterConfigParameter(int32_t channel, std::shared_ptr<RPC::ParameterSet> parameterSet, std::shared_ptr<RPC::Parameter> parameter)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return std::vector<uint8_t>();
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		std::vector<uint8_t> value;
		if(parameter->physicalParameter->address.operation == RPC::PhysicalParameterAddress::Operation::none)
		{
			value = getMasterConfigParameter(channel - rpcChannel->startIndex, parameter->physicalParameter->address.index, parameter->physicalParameter->address.step, parameter->physicalParameter->size);
		}
		else
		{
			if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1)
			{
				Output::printError("Error: Can't get parameter set. address_start or address_step is not set.");
				return std::vector<uint8_t>();
			}
			int32_t channelIndex = channel - rpcChannel->startIndex;
			if(parameterSet->count > 0 && channelIndex >= parameterSet->count)
			{
				Output::printError("Error: Can't get parameter set. Out of bounds.");
				return std::vector<uint8_t>();
			}
			value = getMasterConfigParameter(channelIndex, parameterSet->addressStart, parameterSet->addressStep, parameter->physicalParameter->address.index, parameter->physicalParameter->size);
		}
		return value;
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
    return std::vector<uint8_t>();
}

void HMWiredPeer::addPeer(int32_t channel, std::shared_ptr<BasicPeer> peer)
{
	try
	{
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return;
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
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

std::shared_ptr<BasicPeer> HMWiredPeer::getPeer(int32_t channel, std::string serialNumber, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->serialNumber.empty())
			{
				std::shared_ptr<HMWiredCentral> central = getCentral();
				if(central)
				{
					std::shared_ptr<HMWiredPeer> peer(central->getPeer((*i)->address));
					if(peer) (*i)->serialNumber = peer->getSerialNumber();
				}
			}
			if((*i)->serialNumber == serialNumber && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
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
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> HMWiredPeer::getPeer(int32_t channel, int32_t address, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == address && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
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
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> HMWiredPeer::getPeer(int32_t channel, uint64_t id, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->id == id && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
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
	return std::shared_ptr<BasicPeer>();
}

void HMWiredPeer::removePeer(int32_t channel, uint64_t id, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return;

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->id == id && (*i)->channel == remoteChannel)
			{
				std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, RPC::ParameterSet::Type::Enum::link);
				if(parameterSet && (*i)->configEEPROMAddress != -1 && parameterSet->addressStart > -1 && parameterSet->addressStep > 0)
				{
					std::vector<uint8_t> data(parameterSet->addressStep, 0xFF);
					Output::printDebug("Debug: Erasing " + std::to_string(data.size()) + " bytes in eeprom at address " + HelperFunctions::getHexString((*i)->configEEPROMAddress, 4));
					std::vector<int32_t> configBlocks = setConfigParameter((*i)->configEEPROMAddress, parameterSet->addressStep, data);
					for(std::vector<int32_t>::iterator j = configBlocks.begin(); j != configBlocks.end(); ++j)
					{
						if(!getCentral()->writeEEPROM(_address, *j, binaryConfig[*j].data)) Output::printError("Error: Could not write config to device's eeprom.");
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
    _databaseMutex.unlock();
}

int32_t HMWiredPeer::getFreeEEPROMAddress(int32_t channel, bool isSender)
{
	try
	{
		if(!rpcDevice) return -1;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return -1;
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(!rpcChannel->linkRoles)
		{
			if(!isSender && rpcChannel->linkRoles->targetNames.empty()) return -1;
			if(isSender && rpcChannel->linkRoles->sourceNames.empty()) return -1;
		}
		std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, RPC::ParameterSet::Type::link);
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
			Output::printError("Error: There are no free EEPROM addresses to store links.");
			return -1;
		}
		return currentAddress;
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
    return -1;
}

void HMWiredPeer::loadVariables(HMWiredDevice* device)
{
	try
	{
		_databaseMutex.lock();
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
		DataTable rows = GD::db.executeCommand("SELECT * FROM peerVariables WHERE peerID=?", data);
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 0:
				_firmwareVersion = row->second.at(3)->intValue;
				break;
			case 3:
				_deviceType = LogicalDeviceType(DeviceFamilies::HomeMaticWired, row->second.at(3)->intValue);
				if(_deviceType.type() == (uint32_t)DeviceType::none)
				{
					Output::printError("Error loading HomeMatic Wired peer " + std::to_string(_peerID) + ": Device id unknown: 0x" + HelperFunctions::getHexString(row->second.at(3)->intValue) + " Firmware version: " + std::to_string(_firmwareVersion));
				}
				break;
			case 5:
				_messageCounter = row->second.at(3)->intValue;
				break;
			case 12:
				unserializePeers(row->second.at(5)->binaryValue);
				break;
			case 15:
				if(_centralFeatures)
				{
					serviceMessages.reset(new ServiceMessages(this));
					serviceMessages->unserialize(row->second.at(5)->binaryValue);
				}
				break;
			}
		}
		if(_centralFeatures && !serviceMessages) serviceMessages.reset(new ServiceMessages(this));
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
	_databaseMutex.unlock();
}

bool HMWiredPeer::load(LogicalDevice* device)
{
	try
	{
		loadVariables((HMWiredDevice*)device);

		rpcDevice = GD::rpcDevices.find(_deviceType, _firmwareVersion, -1);
		if(!rpcDevice)
		{
			Output::printError("Error loading HomeMatic Wired peer " + std::to_string(_peerID) + ": Device type not found: 0x" + HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		std::string entry;
		uint32_t pos = 0;
		uint32_t dataSize;
		loadConfig();
		initializeCentralConfig();
		return true;
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
    return false;
}

void HMWiredPeer::save(bool savePeer, bool variables, bool centralConfig)
{
	try
	{
		Peer::save(savePeer, variables, centralConfig);
		if(!variables) saveServiceMessages();
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

void HMWiredPeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		saveVariable(0, _firmwareVersion);
		saveVariable(3, (int32_t)_deviceType.type());
		saveVariable(5, (int32_t)_messageCounter);
		savePeers(); //12
		saveServiceMessages(); //15
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

void HMWiredPeer::saveServiceMessages()
{
	try
	{
		if(!_centralFeatures || !serviceMessages) return;
		std::vector<uint8_t> serializedData;
		serviceMessages->serialize(serializedData);
		saveVariable(15, serializedData);
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

void HMWiredPeer::serializePeers(std::vector<uint8_t>& encodedData)
{
	try
	{
		BinaryEncoder encoder;
		encoder.encodeInteger(encodedData, _peers.size());
		for(std::unordered_map<int32_t, std::vector<std::shared_ptr<BasicPeer>>>::const_iterator i = _peers.begin(); i != _peers.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeInteger(encodedData, i->second.size());
			for(std::vector<std::shared_ptr<BasicPeer>>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
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

void HMWiredPeer::unserializePeers(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BinaryDecoder decoder;
		uint32_t position = 0;
		uint32_t peersSize = decoder.decodeInteger(serializedData, position);
		for(uint32_t i = 0; i < peersSize; i++)
		{
			uint32_t channel = decoder.decodeInteger(serializedData, position);
			uint32_t peerCount = decoder.decodeInteger(serializedData, position);
			for(uint32_t j = 0; j < peerCount; j++)
			{
				std::shared_ptr<BasicPeer> basicPeer(new BasicPeer());
				basicPeer->isSender = decoder.decodeBoolean(serializedData, position);
				basicPeer->id = decoder.decodeInteger(serializedData, position);
				basicPeer->address = decoder.decodeInteger(serializedData, position);
				basicPeer->channel = decoder.decodeInteger(serializedData, position);
				basicPeer->physicalIndexOffset = decoder.decodeInteger(serializedData, position);
				basicPeer->serialNumber = decoder.decodeString(serializedData, position);
				basicPeer->hidden = decoder.decodeBoolean(serializedData, position);
				_peers[channel].push_back(basicPeer);
				basicPeer->linkName = decoder.decodeString(serializedData, position);
				basicPeer->linkDescription = decoder.decodeString(serializedData, position);
				basicPeer->configEEPROMAddress = decoder.decodeInteger(serializedData, position);
				uint32_t dataSize = decoder.decodeInteger(serializedData, position);
				if(position + dataSize <= serializedData->size()) basicPeer->data.insert(basicPeer->data.end(), serializedData->begin() + position, serializedData->begin() + position + dataSize);
				position += dataSize;
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
    return 0;
}

void HMWiredPeer::restoreLinks()
{
	try
	{
		if(!rpcDevice) return;
		_peers.clear();
		std::map<uint32_t, bool> startIndexes;
		std::shared_ptr<HMWiredCentral> central = getCentral();
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			if(startIndexes.find(i->second->startIndex) != startIndexes.end()) continue;
			if(!i->second->linkRoles) continue;
			bool isSender = false;
			if(!i->second->linkRoles->sourceNames.empty())
			{
				if(!i->second->linkRoles->targetNames.empty())
				{
					Output::printWarning("Warning: Channel " + std::to_string(i->first) + " of peer " + std::to_string(_peerID) + " is defined as source and target of a link. I don't know what to do. Not adding link.");
					continue;
				}
				isSender = true;
			}
			startIndexes[i->second->startIndex] = true;
			std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(i->first, RPC::ParameterSet::Type::link);
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
				result = getConfigParameter(currentAddress + parameterSet->peerChannelOffset, 1.0, -1, true);
				if(result.size() != 1) continue;
				int32_t tempPeerChannel = result.at(0);
				result = getConfigParameter(currentAddress + parameterSet->channelOffset, 1.0, -1, true);
				if(result.size() != 1) continue;
				int32_t channel = result.at(0) - i->second->physicalIndexOffset;

				std::shared_ptr<HMWiredPeer> remotePeer = central->getPeer(peerAddress);
				if(!remotePeer || !remotePeer->rpcDevice)
				{
					Output::printWarning("Warning: Could not add link for peer " + std::to_string(_peerID) + " to peer with address 0x" + HelperFunctions::getHexString(peerAddress, 8) + ". The remote peer is not paired to Homegear.");
					continue;
				}

				std::shared_ptr<BasicPeer> basicPeer(new BasicPeer());
				bool channelFound = false;
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator j = remotePeer->rpcDevice->channels.begin(); j != remotePeer->rpcDevice->channels.end(); ++j)
				{
					if(j->first + j->second->physicalIndexOffset == tempPeerChannel)
					{
						channelFound = true;
						basicPeer->channel = tempPeerChannel - j->second->physicalIndexOffset;
						basicPeer->physicalIndexOffset = j->second->physicalIndexOffset;
					}
				}
				if(!channelFound)
				{
					Output::printWarning("Warning: Could not add link for peer " + std::to_string(_peerID) + " to peer with id " + std::to_string(remotePeer->getID()) + ". The remote channel does not exists.");
					continue;
				}
				basicPeer->isSender = !isSender;
				basicPeer->id = remotePeer->getID();
				basicPeer->address = remotePeer->getAddress();
				basicPeer->serialNumber = remotePeer->getSerialNumber();
				basicPeer->configEEPROMAddress = currentAddress;

				addPeer(channel, basicPeer);
				Output::printInfo("Info: Found link between peer " + std::to_string(_peerID) + " (channel " + std::to_string(channel) + ") and peer " + std::to_string(basicPeer->id) + " (channel " + std::to_string(basicPeer->channel) + ").");
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
}

void HMWiredPeer::reset()
{
	try
	{
		if(!rpcDevice) return;
		std::shared_ptr<HMWiredCentral> central = getCentral();
		if(!central) return;
		std::vector<uint8_t> data(16, 0xFF);
		for(int32_t i = 0; i < rpcDevice->eepSize; i+=0x10)
		{
			if(!central->writeEEPROM(_address, i, data))
			{
				Output::printError("Error: Error resetting HomeMatic Wired peer " + std::to_string(_peerID) + ". Could not write EEPROM.");
				return;
			}
		}
		std::vector<uint8_t> moduleReset({0x21, 0x21});
		central->getResponse(moduleReset, _address, false);
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

int32_t HMWiredPeer::getNewFirmwareVersion()
{
	try
	{
		std::string filenamePrefix = HelperFunctions::getHexString((int32_t)DeviceFamilies::HomeMaticWired, 4) + "." + HelperFunctions::getHexString(_deviceType.type(), 8);
		std::string versionFile(GD::settings.firmwarePath() + filenamePrefix + ".version");
		if(!HelperFunctions::fileExists(versionFile)) return 0;
		std::string versionHex = HelperFunctions::getFileContent(versionFile);
		return HelperFunctions::getNumber(versionHex, true) + 1;
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
	return 0;
}

bool HMWiredPeer::firmwareUpdateAvailable()
{
	try
	{
		return _firmwareVersion > 0 && _firmwareVersion < getNewFirmwareVersion();
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
	return false;
}

void HMWiredPeer::getValuesFromPacket(std::shared_ptr<HMWiredPacket> packet, std::vector<FrameValues>& frameValues)
{
	try
	{
		if(!rpcDevice) return;
		//equal_range returns all elements with "0" or an unknown element as argument
		if(packet->messageType() == 0 || rpcDevice->framesByMessageType.find(packet->messageType()) == rpcDevice->framesByMessageType.end()) return;
		std::pair<std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator,std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByMessageType.equal_range((uint32_t)packet->messageType());
		if(range.first == rpcDevice->framesByMessageType.end()) return;
		std::multimap<uint32_t, std::shared_ptr<RPC::DeviceFrame>>::iterator i = range.first;
		do
		{
			FrameValues currentFrameValues;
			std::shared_ptr<RPC::DeviceFrame> frame(i->second);
			if(!frame) continue;
			if(frame->direction == RPC::DeviceFrame::Direction::Enum::fromDevice && packet->senderAddress() != _address) continue;
			if(frame->direction == RPC::DeviceFrame::Direction::Enum::toDevice && packet->destinationAddress() != _address) continue;
			if(packet->payload()->empty()) continue;
			if(frame->subtype > -1 && frame->subtypeIndex >= 9 && (signed)packet->payload()->size() > (frame->subtypeIndex - 9) && packet->payload()->at(frame->subtypeIndex - 9) != (unsigned)frame->subtype) continue;
			int32_t channelIndex = frame->channelField;
			int32_t channel = -1;
			if(channelIndex >= 9 && (signed)packet->payload()->size() > (channelIndex - 9)) channel = packet->payload()->at(channelIndex - 9) - frame->channelIndexOffset;
			if(channel > -1 && frame->channelFieldSize < 1.0) channel &= (0xFF >> (8 - std::lround(frame->channelFieldSize * 10) % 10));
			if(frame->fixedChannel > -1) channel = frame->fixedChannel;
			currentFrameValues.frameID = frame->id;

			for(std::vector<RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				std::vector<uint8_t> data;
				if(j->size > 0 && j->index > 0)
				{
					if(((int32_t)j->index) - 9 >= (signed)packet->payload()->size()) continue;
					data = packet->getPosition(j->index, j->size, -1);

					if(j->constValue > -1)
					{
						int32_t intValue = 0;
						HelperFunctions::memcpyBigEndian(intValue, data);
						if(intValue != j->constValue) break; else continue;
					}
				}
				else if(j->constValue > -1)
				{
					HelperFunctions::memcpyBigEndian(data, j->constValue);
				}
				else continue;
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
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
						for(uint32_t l = startChannel; l <= endChannel; l++)
						{
							if(rpcDevice->channels.find(l) == rpcDevice->channels.end()) continue;
							std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(l, currentFrameValues.parameterSetType);
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
							std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(*l, currentFrameValues.parameterSetType);
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

std::shared_ptr<HMWiredPacket> HMWiredPeer::getResponse(std::shared_ptr<HMWiredPacket> packet)
{
	try
	{
		std::shared_ptr<HMWiredPacket> request(packet);
		std::shared_ptr<HMWiredPacket> response = getCentral()->sendPacket(request, true);
		//Don't send ok here! It's sent in packetReceived if necessary.
		return response;
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
	return std::shared_ptr<HMWiredPacket>();
}

std::shared_ptr<RPC::ParameterSet> HMWiredPeer::getParameterSet(int32_t channel, RPC::ParameterSet::Type::Enum type)
{
	try
	{
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		std::shared_ptr<RPC::ParameterSet> parameterSet;
		if(rpcChannel->specialParameter && rpcChannel->subconfig)
		{
			std::vector<uint8_t> value = getMasterConfigParameter(channel - rpcChannel->startIndex, rpcChannel->specialParameter->physicalParameter->address.index, rpcChannel->specialParameter->physicalParameter->address.step, rpcChannel->specialParameter->physicalParameter->size);
			if(value.at(0))
			{
				if(rpcChannel->subconfig->parameterSets.find(type) == rpcChannel->subconfig->parameterSets.end())
				{
					Output::printWarning("Parameter set of type " + std::to_string(type) + " not found in subconfig for channel " + std::to_string(channel));
					return std::shared_ptr<RPC::ParameterSet>();
				}
				parameterSet = rpcChannel->subconfig->parameterSets[type];
			} else parameterSet = rpcChannel->parameterSets[type];
		}
		else
		{
			if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end())
			{
				Output::printWarning("Parameter set of type " + std::to_string(type) + " not found for channel " + std::to_string(channel));
				return std::shared_ptr<RPC::ParameterSet>();
			}
			parameterSet = rpcChannel->parameterSets[type];
		}
		return parameterSet;
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
	return std::shared_ptr<RPC::ParameterSet>();
}

void HMWiredPeer::packetReceived(std::shared_ptr<HMWiredPacket> packet)
{
	try
	{
		if(ignorePackets || !packet || packet->type() != HMWiredPacketType::iMessage) return;
		if(!_centralFeatures || _disposing) return;
		if(packet->senderAddress() != _address) return;
		if(!rpcDevice) return;
		setLastPacketReceived();
		serviceMessages->endUnreach();
		std::vector<FrameValues> frameValues;
		getValuesFromPacket(packet, frameValues);
		std::shared_ptr<HMWiredPacket> sentPacket;
		std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> valueKeys;
		std::map<uint32_t, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>>> rpcValues;
		//Loop through all matching frames
		for(std::vector<FrameValues>::iterator a = frameValues.begin(); a != frameValues.end(); ++a)
		{
			std::shared_ptr<RPC::DeviceFrame> frame;
			if(!a->frameID.empty()) frame = rpcDevice->framesByID.at(a->frameID);

			std::vector<FrameValues> sentFrameValues;
			bool pushPendingQueues = false;
			if(packet->type() == HMWiredPacketType::ackMessage) //ACK packet: Check if all values were set correctly. If not set value again
			{
				sentPacket = getCentral()->getSentPacket(_address);
				if(sentPacket && sentPacket->messageType() > 0 && !sentPacket->payload()->empty())
				{
					RPC::ParameterSet::Type::Enum sentParameterSetType;
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
						rpcValues[*j].reset(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
					}
					RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
					if(rpcDevice->channels.find(*j) == rpcDevice->channels.end())
					{
						Output::printWarning("Warning: Can't set value of parameter " + i->first + " for channel " + std::to_string(*j) + ". Channel not found.");
						continue;
					}
					std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(*j, a->parameterSetType);
					if(!parameterSet)
					{
						Output::printWarning("Warning: Can't set value of parameter " + i->first + " for channel " + std::to_string(*j) + ". Value parameter set not found.");
						continue;
					}
					std::shared_ptr<RPC::Parameter> currentParameter = parameterSet->getParameter(i->first);
					if(!currentParameter)
					{
						Output::printWarning("Warning: Can't set value of parameter " + i->first + " for channel " + std::to_string(*j) + ". Value parameter set not found.");
						continue;
					}
					parameter->data = i->second.value;
					saveParameter(parameter->databaseID, a->parameterSetType, *j, i->first, parameter->data);
					if(GD::debugLevel >= 4) Output::printInfo("Info: " + i->first + " of HomeMatic Wired peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + HelperFunctions::getHexString(i->second.value) + ".");

					 //Process service messages
					if((currentParameter->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !i->second.value.empty())
					{
						if(currentParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeEnum)
						{
							serviceMessages->set(i->first, i->second.value.at(0), *j);
						}
						else if(currentParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeBoolean)
						{
							serviceMessages->set(i->first, (bool)i->second.value.at(0));
						}
					}

					valueKeys[*j]->push_back(i->first);
					rpcValues[*j]->push_back(currentParameter->convertFromPacket(i->second.value, true));
				}
			}
		}
		if(packet->type() == HMWiredPacketType::iMessage && packet->destinationAddress() == getCentral()->getAddress())
		{
			getCentral()->sendOK(packet->senderMessageCounter(), packet->senderAddress());
		}
		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::const_iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;
				std::string address(_serialNumber + ":" + std::to_string(j->first));
				GD::eventHandler.trigger(_peerID, j->first, j->second, rpcValues.at(j->first));
				GD::rpcClient.broadcastEvent(_peerID, j->first, address, j->second, rpcValues.at(j->first));
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
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getDeviceDescription(int32_t channel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::string type;
		std::shared_ptr<RPC::DeviceType> rpcDeviceType = rpcDevice->getType(_deviceType, _firmwareVersion);
		if(rpcDeviceType) type = rpcDeviceType->id;
		else if(_deviceType.type() == (uint32_t)DeviceType::HMRCV50) type = "HM-RCV-50";
		else
		{
			if(!rpcDevice->supportedTypes.empty()) type = rpcDevice->supportedTypes.at(0)->id;
		}

		if(channel == -1) //Base device
		{
			description->structValue->insert(RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)DeviceFamilies::HomeMaticWired))));
			description->structValue->insert(RPC::RPCStructElement("FAMILY_STRING", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(GD::deviceFamilies.at(DeviceFamilies::HomeMaticWired)->getName()))));
			description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			description->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			std::shared_ptr<RPC::RPCVariable> variable2 = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			description->structValue->insert(RPC::RPCStructElement("CHILDREN", variable));
			description->structValue->insert(RPC::RPCStructElement("CHANNELS", variable2));

			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				if(i->second->hidden) continue;
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(i->first))));
				variable2->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(i->first)));
			}

			if(_firmwareVersion != 0) description->structValue->insert(RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(HelperFunctions::getHexString(_firmwareVersion >> 8) + "." + HelperFunctions::getHexString(_firmwareVersion & 0xFF, 2)))));
			else description->structValue->insert(RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("?")))));

			int32_t newFirmwareVersion = getNewFirmwareVersion();
			if(newFirmwareVersion > _firmwareVersion) description->structValue->insert(RPC::RPCStructElement("AVAILABLE_FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(HelperFunctions::getHexString(newFirmwareVersion >> 8) + "." + HelperFunctions::getHexString(newFirmwareVersion & 0xFF, 2)))));

			int32_t uiFlags = (int32_t)rpcDevice->uiFlags;
			if(isTeam()) uiFlags |= RPC::Device::UIFlags::dontdelete;
			description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));

			//Compatibility
			description->structValue->insert(RPC::RPCStructElement("INTERFACE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(getCentral()->getSerialNumber()))));

			variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			description->structValue->insert(RPC::RPCStructElement("PARAMSETS", variable));
			variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("MASTER")))); //Always MASTER

			description->structValue->insert(RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("")))));

			description->structValue->insert(RPC::RPCStructElement("PHYSICAL_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::to_string(_address)))));

			//Compatibility
			description->structValue->insert(RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::to_string(_address)))));

			//Compatibility
			description->structValue->insert(RPC::RPCStructElement("ROAMING", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(0))));

			if(!type.empty()) description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			description->structValue->insert(RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));
		}
		else
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
			if(rpcChannel->hidden) return description;

			description->structValue->insert(RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)DeviceFamilies::HomeMaticWired))));
			description->structValue->insert(RPC::RPCStructElement("FAMILY_STRING", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(GD::deviceFamilies.at(DeviceFamilies::HomeMaticWired)->getName()))));
			description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			description->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));

			//Compatibility
			description->structValue->insert(RPC::RPCStructElement("AES_ACTIVE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(0))));

			int32_t direction = 0;
			std::ostringstream linkSourceRoles;
			std::ostringstream linkTargetRoles;
			if(rpcChannel->linkRoles)
			{
				for(std::vector<std::string>::iterator k = rpcChannel->linkRoles->sourceNames.begin(); k != rpcChannel->linkRoles->sourceNames.end(); ++k)
				{
					//Probably only one direction is supported, but just in case I use the "or"
					if(!k->empty())
					{
						if(direction & 1) linkSourceRoles << " ";
						linkSourceRoles << *k;
						direction |= 1;
					}
				}
				for(std::vector<std::string>::iterator k = rpcChannel->linkRoles->targetNames.begin(); k != rpcChannel->linkRoles->targetNames.end(); ++k)
				{
					//Probably only one direction is supported, but just in case I use the "or"
					if(!k->empty())
					{
						if(direction & 2) linkTargetRoles << " ";
						linkTargetRoles << *k;
						direction |= 2;
					}
				}
			}

			//Overwrite direction when manually set
			if(rpcChannel->direction != RPC::DeviceChannel::Direction::Enum::none) direction = (int32_t)rpcChannel->direction;
			description->structValue->insert(RPC::RPCStructElement("DIRECTION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(direction))));

			int32_t uiFlags = (int32_t)rpcChannel->uiFlags;
			if(isTeam()) uiFlags |= RPC::DeviceChannel::UIFlags::dontdelete;
			description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));

			description->structValue->insert(RPC::RPCStructElement("INDEX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));

			description->structValue->insert(RPC::RPCStructElement("LINK_SOURCE_ROLES", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(linkSourceRoles.str()))));

			description->structValue->insert(RPC::RPCStructElement("LINK_TARGET_ROLES", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(linkTargetRoles.str()))));

			std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			description->structValue->insert(RPC::RPCStructElement("PARAMSETS", variable));
			for(std::map<RPC::ParameterSet::Type::Enum, std::shared_ptr<RPC::ParameterSet>>::iterator j = rpcChannel->parameterSets.begin(); j != rpcChannel->parameterSets.end(); ++j)
			{
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second->typeString())));
			}

			description->structValue->insert(RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			if(!type.empty()) description->structValue->insert(RPC::RPCStructElement("PARENT_TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->type))));

			description->structValue->insert(RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));
		}
		return description;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> HMWiredPeer::getDeviceDescription()
{
	try
	{
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> descriptions(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
		descriptions->push_back(getDeviceDescription(-1));

		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			std::shared_ptr<RPC::RPCVariable> description = getDeviceDescription(i->first);
			if(!description->structValue->empty()) descriptions->push_back(description);
		}

		return descriptions;
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
    return std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>>();
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getParamsetDescription(int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel");
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set");

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && remoteID > 0) remotePeer = getPeer(channel, remoteID, remoteChannel);

		std::shared_ptr<RPC::ParameterSet> parameterSet;
		if(rpcChannel->specialParameter && rpcChannel->subconfig)
		{
			std::vector<uint8_t> value = getMasterConfigParameter(channel - rpcChannel->startIndex, rpcChannel->specialParameter->physicalParameter->address.index, rpcChannel->specialParameter->physicalParameter->address.step, rpcChannel->specialParameter->physicalParameter->size);
			Output::printDebug("Debug: Special parameter is " + std::to_string(value.at(0)));
			if(value.at(0))
			{
				if(rpcChannel->subconfig->parameterSets.find(type) == rpcChannel->subconfig->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set");
				parameterSet = rpcChannel->subconfig->parameterSets[type];
			} else parameterSet = rpcChannel->parameterSets[type];
		}
		else parameterSet = rpcChannel->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> descriptions(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> element;
		uint32_t index = 0;
		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::internal)  && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::transform))
			{
				Output::printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeBoolean)
			{
				RPC::LogicalParameterBoolean* parameter = (RPC::LogicalParameterBoolean*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
					element->booleanValue = parameter->defaultValue;
					description->structValue->insert(RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(RPC::RPCStructElement("ID", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->max;
				description->structValue->insert(RPC::RPCStructElement("MAX", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->min;
				description->structValue->insert(RPC::RPCStructElement("MIN", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = "BOOL";
				description->structValue->insert(RPC::RPCStructElement("TYPE", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeString)
			{
				RPC::LogicalParameterString* parameter = (RPC::LogicalParameterString*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = parameter->defaultValue;
					description->structValue->insert(RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(RPC::RPCStructElement("ID", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->max;
				description->structValue->insert(RPC::RPCStructElement("MAX", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->min;
				description->structValue->insert(RPC::RPCStructElement("MIN", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = "STRING";
				description->structValue->insert(RPC::RPCStructElement("TYPE", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeAction)
			{
				RPC::LogicalParameterAction* parameter = (RPC::LogicalParameterAction*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
					element->booleanValue = parameter->defaultValue;
					description->structValue->insert(RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(RPC::RPCStructElement("ID", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->max;
				description->structValue->insert(RPC::RPCStructElement("MAX", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->min;
				description->structValue->insert(RPC::RPCStructElement("MIN", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = "ACTION";
				description->structValue->insert(RPC::RPCStructElement("TYPE", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeInteger)
			{
				RPC::LogicalParameterInteger* parameter = (RPC::LogicalParameterInteger*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
					element->integerValue = parameter->defaultValue;
					description->structValue->insert(RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(RPC::RPCStructElement("ID", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->max;
				description->structValue->insert(RPC::RPCStructElement("MAX", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->min;
				description->structValue->insert(RPC::RPCStructElement("MIN", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", element));

				if(!parameter->specialValues.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(RPC::RPCStructElement("VALUE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second))));
						element->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(RPC::RPCStructElement("SPECIAL", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = "INTEGER";
				description->structValue->insert(RPC::RPCStructElement("TYPE", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeEnum)
			{
				RPC::LogicalParameterEnum* parameter = (RPC::LogicalParameterEnum*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
					element->integerValue = parameter->defaultValue;
					description->structValue->insert(RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(RPC::RPCStructElement("ID", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->max;
				description->structValue->insert(RPC::RPCStructElement("MAX", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->min;
				description->structValue->insert(RPC::RPCStructElement("MIN", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = "ENUM";
				description->structValue->insert(RPC::RPCStructElement("TYPE", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(RPC::RPCStructElement("UNIT", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				for(std::vector<RPC::ParameterOption>::iterator j = parameter->options.begin(); j != parameter->options.end(); ++j)
				{
					element->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->id)));
				}
				description->structValue->insert(RPC::RPCStructElement("VALUE_LIST", element));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeFloat)
			{
				RPC::LogicalParameterFloat* parameter = (RPC::LogicalParameterFloat*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
					element->floatValue = parameter->defaultValue;
					description->structValue->insert(RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(RPC::RPCStructElement("ID", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
				element->floatValue = parameter->max;
				description->structValue->insert(RPC::RPCStructElement("MAX", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcFloat));
				element->floatValue = parameter->min;
				description->structValue->insert(RPC::RPCStructElement("MIN", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", element));

				if(!parameter->specialValues.empty())
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, double>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(RPC::RPCStructElement("VALUE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second))));
						element->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(RPC::RPCStructElement("SPECIAL", element));
				}

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = "FLOAT";
				description->structValue->insert(RPC::RPCStructElement("TYPE", element));

				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(RPC::RPCStructElement("UNIT", element));
			}

			index++;
			descriptions->structValue->insert(RPC::RPCStructElement((*i)->id, description));
			description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		}
		return descriptions;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getLink(int32_t channel, int32_t flags, bool avoidDuplicates)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> element;
		if(channel > -1) //Get link of single channel
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			bool isSender = false;
			//Return if there are no link roles defined
			std::shared_ptr<RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
			if(!linkRoles) return array;
			if(!linkRoles->sourceNames.empty()) isSender = true;
			else if(linkRoles->targetNames.empty()) return array;
			std::shared_ptr<HMWiredCentral> central = getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<HMWiredPeer> remotePeer(central->getPeer((*i)->address));
				if(!remotePeer)
				{
					Output::printDebug("Debug: Can't return link description for peer with address 0x" + HelperFunctions::getHexString((*i)->address, 6) + ". The peer is not paired to Homegear.");
					continue;
				}
				bool peerKnowsMe = false;
				if(remotePeer && remotePeer->getPeer((*i)->channel, _address, channel)) peerKnowsMe = true;

				//Don't continue if peer is sender and exists in central's peer array to avoid generation of duplicate results when requesting all links (only generate results when we are sender)
				if(!isSender && peerKnowsMe && avoidDuplicates) return array;
				//If we are receiver this point is only reached, when the sender is not paired to this central

				uint64_t peerID = (*i)->id;
				std::string peerSerialNumber = (*i)->serialNumber;
				int32_t brokenFlags = 0;
				if(peerID == 0 || peerSerialNumber.empty())
				{
					if(peerKnowsMe ||
					  (*i)->address == _address) //Link to myself with non-existing (virtual) channel (e. g. switches use this)
					{
						(*i)->id = remotePeer->getID();
						(*i)->serialNumber = remotePeer->getSerialNumber();
						peerID = (*i)->id;
					}
					else
					{
						//Peer not paired to central
						std::ostringstream stringstream;
						stringstream << '@' << std::hex << std::setw(6) << std::setfill('0') << (*i)->address;
						peerSerialNumber = stringstream.str();
						if(isSender) brokenFlags = 2; //LINK_FLAG_RECEIVER_BROKEN
						else brokenFlags = 1; //LINK_FLAG_SENDER_BROKEN
					}
				}
				//Relevent for switches
				if(peerID == _peerID && rpcDevice->channels.find((*i)->channel) == rpcDevice->channels.end())
				{
					if(isSender) brokenFlags = 2 | 4; //LINK_FLAG_RECEIVER_BROKEN | PEER_IS_ME
					else brokenFlags = 1 | 4; //LINK_FLAG_SENDER_BROKEN | PEER_IS_ME
				}
				if(brokenFlags == 0 && remotePeer && remotePeer->serviceMessages->getUnreach()) brokenFlags = 2;
				if(serviceMessages->getUnreach()) brokenFlags |= 1;
				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
				element->structValue->insert(RPC::RPCStructElement("DESCRIPTION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->linkDescription))));
				element->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(brokenFlags))));
				element->structValue->insert(RPC::RPCStructElement("NAME", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->linkName))));
				if(isSender)
				{
					element->structValue->insert(RPC::RPCStructElement("RECEIVER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peerSerialNumber + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(RPC::RPCStructElement("RECEIVER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)remotePeer->getID()))));
					element->structValue->insert(RPC::RPCStructElement("RECEIVER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->channel))));
					if(flags & 4)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, RPC::ParameterSet::Type::Enum::link, _peerID, channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 2) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel);
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(RPC::RPCStructElement("SENDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(RPC::RPCStructElement("SENDER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID))));
					element->structValue->insert(RPC::RPCStructElement("SENDER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));
					if(flags & 2)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1)) paramset = getParamset(channel, RPC::ParameterSet::Type::Enum::link, peerID, (*i)->channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 1)) description = getDeviceDescription(channel);
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("SENDER_DESCRIPTION", description));
					}
				}
				else //When sender is broken
				{
					element->structValue->insert(RPC::RPCStructElement("RECEIVER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(RPC::RPCStructElement("RECEIVER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID))));
					element->structValue->insert(RPC::RPCStructElement("RECEIVER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));
					if(flags & 4)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = getParamset(channel, RPC::ParameterSet::Type::Enum::link, peerID, (*i)->channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 2)) description = getDeviceDescription(channel);
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(RPC::RPCStructElement("SENDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peerSerialNumber + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(RPC::RPCStructElement("SENDER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)remotePeer->getID()))));
					element->structValue->insert(RPC::RPCStructElement("SENDER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->channel))));
					if(flags & 2)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, RPC::ParameterSet::Type::Enum::link, _peerID, channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 1) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel);
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(RPC::RPCStructElement("SENDER_DESCRIPTION", description));
					}
				}
				array->arrayValue->push_back(element);
			}
		}
		else
		{
			flags &= 7; //Remove flag 8 and 16. Both are not processed, when getting links for all devices.
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				element = getLink(i->first, flags, avoidDuplicates);
				array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
			}
		}
		return array;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getLinkInfo(int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(_peers.find(senderChannel) == _peers.end()) return RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverID, receiverChannel);
		if(!remotePeer) return RPC::RPCVariable::createError(-2, "Peer not found.");
		std::shared_ptr<RPC::RPCVariable> response(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		response->structValue->insert(RPC::RPCStructElement("DESCRIPTION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(remotePeer->linkDescription))));
		response->structValue->insert(RPC::RPCStructElement("NAME", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(remotePeer->linkName))));
		return response;
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getLinkPeers(int32_t channel, bool returnID)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		if(channel > -1)
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			//Return if there are no link roles defined
			std::shared_ptr<RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
			if(!linkRoles) return array;
			std::shared_ptr<HMWiredCentral> central = getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<HMWiredPeer> peer(central->getPeer((*i)->address));
				bool peerKnowsMe = false;
				if(peer && peer->getPeer(channel, _peerID)) peerKnowsMe = true;

				std::string peerSerial = (*i)->serialNumber;
				if((*i)->serialNumber.empty())
				{
					if(peerKnowsMe)
					{
						(*i)->serialNumber = peer->getSerialNumber();
						peerSerial = (*i)->serialNumber;
					}
					else
					{
						//Peer not paired to central
						std::ostringstream stringstream;
						stringstream << '@' << std::hex << std::setw(6) << std::setfill('0') << (*i)->address;
						peerSerial = stringstream.str();
					}
				}
				if(returnID)
				{
					std::shared_ptr<RPC::RPCVariable> address(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					array->arrayValue->push_back(address);
					address->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->id)));
					address->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->channel)));
				}
				else array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peerSerial + ":" + std::to_string((*i)->channel))));
			}
		}
		else
		{
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				std::shared_ptr<RPC::RPCVariable> linkPeers = getLinkPeers(i->first, returnID);
				array->arrayValue->insert(array->arrayValue->end(), linkPeers->arrayValue->begin(), linkPeers->arrayValue->end());
			}
		}
		return array;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, type);
		if(!parameterSet) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::RPCVariable> variables(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::internal) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::transform))
			{
				Output::printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			std::shared_ptr<RPC::RPCVariable> element;
			if(type == RPC::ParameterSet::Type::Enum::values)
			{
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = (*i)->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}
			else if(type == RPC::ParameterSet::Type::Enum::master)
			{
				std::vector<uint8_t> value = getMasterConfigParameter(channel, parameterSet, *i);
				if(value.empty()) return RPC::RPCVariable::createError(-32500, "Could not read config parameter. See log for more details.");
				element = (*i)->convertFromPacket(value);
			}
			else if(type == RPC::ParameterSet::Type::Enum::link)
			{
				std::shared_ptr<BasicPeer> remotePeer;
				if(remoteID > 0) remotePeer = getPeer(channel, remoteID, remoteChannel);
				if(!remotePeer) return RPC::RPCVariable::createError(-3, "Not paired to this peer.");
				if(remotePeer->configEEPROMAddress == -1) return RPC::RPCVariable::createError(-3, "No parameter set eeprom address set.");
				if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1) return RPC::RPCVariable::createError(-3, "Storage type of link parameter set not supported.");

				std::vector<uint8_t> value = getConfigParameter(remotePeer->configEEPROMAddress + (*i)->physicalParameter->address.index, (*i)->physicalParameter->size);
				if(value.empty()) return RPC::RPCVariable::createError(-32500, "Could not read config parameter. See log for more details.");
				element = (*i)->convertFromPacket(value);
			}

			if(!element) continue;
			if(element->type == RPC::RPCVariableType::rpcVoid) continue;
			variables->structValue->insert(RPC::RPCStructElement((*i)->id, element));
		}
		return variables;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && remoteID > 0)
		{
			remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return RPC::RPCVariable::createError(-2, "Unknown remote peer.");
		}

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->channels[channel]->parameterSets[type]->id));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getServiceMessages(bool returnID)
{
	if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
	if(!serviceMessages) return RPC::RPCVariable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get(returnID);
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getValue(uint32_t channel, std::string valueKey)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!rpcDevice) return RPC::RPCVariable::createError(-32500, "Unknown application error.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, RPC::ParameterSet::Type::Enum::values);
		if(!parameterSet) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::Parameter> parameter = parameterSet->getParameter(valueKey);
		if(!parameter) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		return parameter->convertFromPacket(valuesCentral[channel][valueKey].data);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::putParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> variables, bool putUnchanged, bool onlyPushing)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, type);
		if(!parameterSet) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		if(variables->structValue->empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

		std::map<int32_t, bool> changedBlocks;
		if(type == RPC::ParameterSet::Type::Enum::master)
		{
			for(RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::shared_ptr<RPC::Parameter> currentParameter = parameterSet->getParameter(i->first);
				if(!currentParameter) continue;
				std::vector<uint8_t> value = currentParameter->convertToPacket(i->second);
				std::vector<int32_t> result = setMasterConfigParameter(channel, parameterSet, currentParameter, value);
				Output::printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " was set to 0x" + HelperFunctions::getHexString(value) + ".");
				//Only send to device when parameter is of type eeprom
				if(currentParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::eeprom) continue;
				for(std::vector<int32_t>::iterator i = result.begin(); i != result.end(); ++i) changedBlocks[*i] = true;
			}
		}
		else if(type == RPC::ParameterSet::Type::Enum::values)
		{
			for(RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				setValue(channel, i->first, i->second);
			}
		}
		else if(type == RPC::ParameterSet::Type::Enum::link)
		{
			std::shared_ptr<BasicPeer> remotePeer;
			if(remoteID > 0) remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return RPC::RPCVariable::createError(-3, "Not paired to this peer.");
			if(remotePeer->configEEPROMAddress == -1) return RPC::RPCVariable::createError(-3, "No parameter set eeprom address set.");
			if(parameterSet->addressStart == -1 || parameterSet->addressStep == -1) return RPC::RPCVariable::createError(-3, "Storage type of link parameter set not supported.");

			for(RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::shared_ptr<RPC::Parameter> currentParameter = parameterSet->getParameter(i->first);
				if(!currentParameter) continue;
				if(currentParameter->physicalParameter->address.operation == RPC::PhysicalParameterAddress::Operation::Enum::none) continue;
				std::vector<uint8_t> value = currentParameter->convertToPacket(i->second);
				std::vector<int32_t> result = setConfigParameter(remotePeer->configEEPROMAddress + currentParameter->physicalParameter->address.index, currentParameter->physicalParameter->size, value);
				Output::printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " was set to 0x" + HelperFunctions::getHexString(value) + ".");
				//Only send to device when parameter is of type eeprom
				if(currentParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::eeprom) continue;
				for(std::vector<int32_t>::iterator i = result.begin(); i != result.end(); ++i) changedBlocks[*i] = true;
			}
		}

		if(changedBlocks.empty()) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

		std::shared_ptr<HMWiredCentral> central = getCentral();
		for(std::map<int32_t, bool>::iterator i = changedBlocks.begin(); i != changedBlocks.end(); ++i)
		{
			if(!central->writeEEPROM(_address, i->first, binaryConfig[i->first].data))
			{
				Output::printError("Error: Could not write config to device's eeprom.");
				return RPC::RPCVariable::createError(-32500, "Could not write config to device's eeprom.");
			}
		}
		GD::rpcClient.broadcastUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), RPC::Client::Hint::Enum::updateHintAll);

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::setLinkInfo(int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(_peers.find(senderChannel) == _peers.end()) return RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverID, receiverChannel);
		if(!remotePeer) return RPC::RPCVariable::createError(-2, "Peer not found.");
		remotePeer->linkDescription = description;
		remotePeer->linkName = name;
		savePeers();
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return RPC::RPCVariable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Unknown parameter.");

		std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(channel, RPC::ParameterSet::Type::Enum::values);
		if(!parameterSet) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::Parameter> rpcParameter = parameterSet->getParameter(valueKey);
		if(!rpcParameter) return RPC::RPCVariable::createError(-5, "Unknown parameter.");
		RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string> {valueKey});
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values(new std::vector<std::shared_ptr<RPC::RPCVariable>> { value });
		if(rpcParameter->physicalParameter->interface == RPC::PhysicalParameter::Interface::Enum::store)
		{
			parameter->data = rpcParameter->convertToPacket(value);
			saveParameter(parameter->databaseID, RPC::ParameterSet::Type::Enum::values, channel, valueKey, parameter->data);
			GD::rpcClient.broadcastEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
		}
		else if(rpcParameter->physicalParameter->interface != RPC::PhysicalParameter::Interface::Enum::command) return RPC::RPCVariable::createError(-6, "Parameter is not settable.");
		if(!rpcParameter->conversion.empty() && rpcParameter->conversion.at(0)->type == RPC::ParameterConversion::Type::Enum::toggle)
		{
			//Handle toggle parameter
			std::string toggleKey = rpcParameter->conversion.at(0)->stringValue;
			if(toggleKey.empty()) return RPC::RPCVariable::createError(-6, "No toggle parameter specified (parameter attribute value is empty).");
			if(valuesCentral[channel].find(toggleKey) == valuesCentral[channel].end()) return RPC::RPCVariable::createError(-5, "Toggle parameter not found.");
			RPCConfigurationParameter* toggleParam = &valuesCentral[channel][toggleKey];
			std::shared_ptr<RPC::Parameter> toggleRPCParam = parameterSet->getParameter(toggleKey);
			if(!toggleRPCParam) return RPC::RPCVariable::createError(-5, "Toggle parameter not found.");
			std::shared_ptr<RPC::RPCVariable> toggleValue;
			if(toggleRPCParam->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeBoolean)
			{
				toggleValue = toggleRPCParam->convertFromPacket(toggleParam->data);
				toggleValue->booleanValue = !toggleValue->booleanValue;
			}
			else if(toggleRPCParam->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeInteger ||
					toggleRPCParam->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeFloat)
			{
				int32_t currentToggleValue = (int32_t)toggleParam->data.at(0);
				std::vector<uint8_t> temp({0});
				if(currentToggleValue != toggleRPCParam->conversion.at(0)->on) temp.at(0) = toggleRPCParam->conversion.at(0)->on;
				else temp.at(0) = toggleRPCParam->conversion.at(0)->off;
				toggleValue = toggleRPCParam->convertFromPacket(temp);
			}
			else return RPC::RPCVariable::createError(-6, "Toggle parameter has to be of type boolean, float or integer.");
			return setValue(channel, toggleKey, toggleValue);
		}
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return RPC::RPCVariable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return RPC::RPCVariable::createError(-6, "No frame was found for parameter " + valueKey);
		std::shared_ptr<RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		std::vector<uint8_t> data = rpcParameter->convertToPacket(value);
		parameter->data = data;
		saveParameter(parameter->databaseID, RPC::ParameterSet::Type::Enum::values, channel, valueKey, data);
		if(GD::debugLevel > 4) Output::printDebug("Debug: " + valueKey + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to " + HelperFunctions::getHexString(data) + ".");

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

		std::shared_ptr<HMWiredPacket> packet(new HMWiredPacket(HMWiredPacketType::iMessage, getCentral()->getAddress(), _address, false, _messageCounter, 0, 0, payload));
		for(std::vector<RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				std::vector<uint8_t> data;
				HelperFunctions::memcpyBigEndian(data, i->constValue);
				packet->setPosition(i->index, i->size, data);
				continue;
			}
			RPCConfigurationParameter* additionalParameter = nullptr;
			//We can't just search for param, because it is ambiguous (see for example LEVEL for HM-CC-TC.
			if(i->param.empty() && !i->additionalParameter.empty() && valuesCentral[channel].find(i->additionalParameter) != valuesCentral[channel].end())
			{
				additionalParameter = &valuesCentral[channel][i->additionalParameter];
				int32_t intValue = 0;
				HelperFunctions::memcpyBigEndian(intValue, additionalParameter->data);
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
				for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = parameterSet->parameters.begin(); j != parameterSet->parameters.end(); ++j)
				{
					//Only compare id. Till now looking for value_id was not necessary.
					if(i->param == (*j)->physicalParameter->id)
					{
						packet->setPosition(i->index, i->size, valuesCentral[channel][(*j)->id].data);
						paramFound = true;
						break;
					}
				}
				if(!paramFound) Output::printError("Error constructing packet. param \"" + i->param + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
			}
		}
		if(!rpcParameter->physicalParameter->resetAfterSend.empty())
		{
			for(std::vector<std::string>::iterator j = rpcParameter->physicalParameter->resetAfterSend.begin(); j != rpcParameter->physicalParameter->resetAfterSend.end(); ++j)
			{
				if(valuesCentral.at(channel).find(*j) == valuesCentral.at(channel).end()) continue;
				std::shared_ptr<RPC::Parameter> currentParameter = parameterSet->getParameter(*j);
				if(!currentParameter) continue;
				std::shared_ptr<RPC::RPCVariable> logicalDefaultValue = currentParameter->logicalParameter->getDefaultValue();
				std::vector<uint8_t> defaultValue = currentParameter->convertToPacket(logicalDefaultValue);
				if(defaultValue != valuesCentral.at(channel).at(*j).data)
				{
					RPCConfigurationParameter* tempParam = &valuesCentral.at(channel).at(*j);
					tempParam->data = defaultValue;
					saveParameter(tempParam->databaseID, RPC::ParameterSet::Type::Enum::values, channel, *j, tempParam->data);
					Output::printInfo( "Info: Parameter \"" + *j + "\" was reset to " + HelperFunctions::getHexString(defaultValue) + ". Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
					valueKeys->push_back(*j);
					values->push_back(logicalDefaultValue);
				}
			}
		}
		setMessageCounter(_messageCounter + 1);

		std::shared_ptr<HMWiredPacket> response = getResponse(packet);
		if(!response)
		{
			Output::printWarning("Error: Error sending packet to peer " + std::to_string(_peerID) + ". Peer did not respond.");
			return RPC::RPCVariable::createError(-32500, "Error sending packet to peer. Peer did not respond.");
		}
		else
		{
			GD::rpcClient.broadcastEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. See error log for more details.");
}

} /* namespace HMWired */
