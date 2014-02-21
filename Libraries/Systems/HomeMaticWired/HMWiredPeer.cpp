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
			Output::printWarning("Warning: Tried to initialize HomeMatic Wired peer's central config without xmlrpcDevice being set.");
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
						parameter.rpcParameter = *j;
						parameter.data = (*j)->convertToPacket((*j)->logicalParameter->getDefaultValue());
						valuesCentral[i->first][(*j)->id] = parameter;
						saveParameter(0, RPC::ParameterSet::Type::values, i->first, (*j)->id, parameter.data);
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

void HMWiredPeer::setConfigParameter(double index, double size, std::vector<uint8_t>& binaryValue)
{
	try
	{
		if(size < 0 || index < 0)
		{
			Output::printError("Error: Can't set configuration parameter. Index or size is negative.");
			return;
		}
		if(size > 0.8 && size < 1.0) size = 1.0;
		double byteIndex = std::floor(index);
		if(byteIndex != index || size <= 1.0) //0.8 == 8 Bits
		{
			if(binaryValue.empty()) binaryValue.push_back(0);
			int32_t intByteIndex = byteIndex;
			int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
			intByteIndex -= configBlockIndex;
			if(size > 1.0)
			{
				Output::printError("Error: HomeMatic Wired peer " + std::to_string(_peerID) + ": Can't set partial byte index > 1.");
				return;
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
				return;
			}
			if(bitSize == 8) configBlock->data.at(intByteIndex) = 0;
			else configBlock->data.at(intByteIndex) &= (~(_bitmask[bitSize] << indexBits));
			configBlock->data.at(intByteIndex) |= (binaryValue.at(binaryValue.size() - 1) << indexBits);
			if(indexBits + bitSize > 8) //Spread over two bytes
			{
				bitSize = (indexBits + bitSize) - 8;
				intByteIndex++;
				if(intByteIndex >= 0x10)
				{
					saveParameter(configBlock->databaseID, configBlock->data);
					configBlockIndex += 0x10;
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
					{
						binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
						saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
					}
					configBlock = &binaryConfig[configBlockIndex];
					if(configBlock->data.size() != 0x10)
					{
						Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
						return;
					}
					intByteIndex = 0;
				}
				configBlock->data.at(intByteIndex) &= (~_bitmask[bitSize]);
				configBlock->data.at(intByteIndex) |= (binaryValue.at(binaryValue.size() - 1));
			}
			saveParameter(configBlock->databaseID, configBlock->data);
		}
		else
		{
			int32_t intByteIndex = byteIndex;
			int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
			intByteIndex -= configBlockIndex;
			uint32_t bytes = (uint32_t)std::ceil(size);
			if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
			{
				binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
				saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
			}
			ConfigDataBlock* configBlock = &binaryConfig[configBlockIndex];
			if(binaryValue.empty()) return;
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
						saveParameter(configBlock->databaseID, configBlock->data);
						configBlockIndex += 0x10;
						if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
						{
							binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
							saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
						}
						configBlock = &binaryConfig[configBlockIndex];
						if(configBlock->data.size() != 0x10)
						{
							Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
							return;
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
							saveParameter(configBlock->databaseID, configBlock->data);
							configBlockIndex += 0x10;
							if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
							{
								binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
								saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
							}
							configBlock = &binaryConfig[configBlockIndex];
							if(configBlock->data.size() != 0x10)
							{
								Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
								return;
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
						saveParameter(configBlock->databaseID, configBlock->data);
						configBlockIndex += 0x10;
						if(binaryConfig.find(configBlockIndex) == binaryConfig.end())
						{
							binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
							saveParameter(0, configBlockIndex, binaryConfig[configBlockIndex].data);
						}
						configBlock = &binaryConfig[configBlockIndex];
						if(configBlock->data.size() != 0x10)
						{
							Output::printError("Error: Can't set configuration parameter. Can't read EEPROM.");
							return;
						}
					}
					configBlock->data.at(intByteIndex + missingBytes + i) = binaryValue.at(i);
				}
			}
			saveParameter(configBlock->databaseID, configBlock->data);
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

std::vector<uint8_t> HMWiredPeer::getConfigParameter(double index, double size, int32_t mask)
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
		if(binaryConfig.find(configBlockIndex) == binaryConfig.end()) binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
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
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end()) binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
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
					if(binaryConfig.find(configBlockIndex) == binaryConfig.end()) binaryConfig[configBlockIndex].data = getCentral()->readEEPROM(_address, configBlockIndex);
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

void HMWiredPeer::removePeer(int32_t channel, int32_t address, int32_t remoteChannel)
{
	try
	{
		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->address == address && (*i)->channel == remoteChannel)
			{
				_peers[channel].erase(i);
				_databaseMutex.lock();
				DataColumnVector data;
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn((int32_t)RPC::ParameterSet::Type::Enum::link)));
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn(channel)));
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn(address)));
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn(remoteChannel)));
				GD::db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND remotePeer=? AND remoteChannel=?", data);
				_databaseMutex.unlock();
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
				encoder.encodeInteger(encodedData, (*j)->address);
				encoder.encodeInteger(encodedData, (*j)->channel);
				encoder.encodeString(encodedData, (*j)->serialNumber);
				encoder.encodeBoolean(encodedData, (*j)->hidden);
				encoder.encodeString(encodedData, (*j)->linkName);
				encoder.encodeString(encodedData, (*j)->linkDescription);
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
				basicPeer->address = decoder.decodeInteger(serializedData, position);
				basicPeer->channel = decoder.decodeInteger(serializedData, position);
				basicPeer->serialNumber = decoder.decodeString(serializedData, position);
				basicPeer->hidden = decoder.decodeBoolean(serializedData, position);
				_peers[channel].push_back(basicPeer);
				basicPeer->linkName = decoder.decodeString(serializedData, position);
				basicPeer->linkDescription = decoder.decodeString(serializedData, position);
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
							if(rpcDevice->channels.at(l)->parameterSets.find(currentFrameValues.parameterSetType) == rpcDevice->channels.at(l)->parameterSets.end()) continue;
							if(!rpcDevice->channels.at(l)->parameterSets.at(currentFrameValues.parameterSetType)->getParameter((*k)->id)) continue;
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
							if(rpcDevice->channels.at(*l)->parameterSets.find(currentFrameValues.parameterSetType) == rpcDevice->channels.at(*l)->parameterSets.end()) continue;
							if(!rpcDevice->channels.at(*l)->parameterSets.at(currentFrameValues.parameterSetType)->getParameter((*k)->id)) continue;
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

void HMWiredPeer::packetReceived(std::shared_ptr<HMWiredPacket> packet)
{
	try
	{
		if(ignorePackets || !packet) return;
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
					parameter->data = i->second.value;
					saveParameter(parameter->databaseID, parameter->data);
					if(GD::debugLevel >= 4) Output::printInfo("Info: " + i->first + " of HomeMatic Wired peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + HelperFunctions::getHexString(i->second.value) + ".");

					 //Process service messages
					if(parameter->rpcParameter && (parameter->rpcParameter->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !i->second.value.empty())
					{
						if(parameter->rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeEnum)
						{
							serviceMessages->set(i->first, i->second.value.at(0), *j);
						}
						else if(parameter->rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeBoolean)
						{
							serviceMessages->set(i->first, (bool)i->second.value.at(0));
						}
					}

					valueKeys[*j]->push_back(i->first);
					rpcValues[*j]->push_back(rpcDevice->channels.at(*j)->parameterSets.at(a->parameterSetType)->getParameter(i->first)->convertFromPacket(i->second.value, true));
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
				GD::eventHandler.trigger(address, j->second, rpcValues.at(j->first));
				GD::rpcClient.broadcastEvent(address, j->second, rpcValues.at(j->first));
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
			description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			description->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			description->structValue->insert(RPC::RPCStructElement("CHILDREN", variable));

			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				if(i->second->hidden) continue;
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(i->first))));
			}

			if(_firmwareVersion != 0)
			{
				std::ostringstream stringStream;
				stringStream << std::setw(2) << std::hex << (int32_t)_firmwareVersion << std::dec;
				std::string firmware = stringStream.str();
				description->structValue->insert(RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(firmware.substr(0, 1) + "." + firmware.substr(1)))));
			}
			else
			{
				description->structValue->insert(RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("?")))));
			}

			int32_t uiFlags = (int32_t)rpcDevice->uiFlags;
			if(isTeam()) uiFlags |= RPC::Device::UIFlags::dontdelete;
			description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));

			variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			description->structValue->insert(RPC::RPCStructElement("PARAMSETS", variable));
			variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("MASTER")))); //Always MASTER

			description->structValue->insert(RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("")))));

			description->structValue->insert(RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::to_string(_address)))));

			if(!type.empty()) description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			description->structValue->insert(RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));
		}
		else
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
			if(rpcChannel->hidden) return description;

			description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			description->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));

			int32_t aesActive = 0;
			if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && !configCentral.at(channel).at("AES_ACTIVE").data.empty() && configCentral.at(channel).at("AES_ACTIVE").data.at(0) != 0)
			{
				aesActive = 1;
			}
			description->structValue->insert(RPC::RPCStructElement("AES_ACTIVE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(aesActive))));

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
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::master) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::master)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::values) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::values)->typeString())));

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

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getParamsetDescription(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel");
		std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set");

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);

		std::shared_ptr<RPC::ParameterSet> parameterSet;
		if(rpcChannel->specialParameter && rpcChannel->subconfig)
		{
			double size = rpcChannel->specialParameter->physicalParameter->size;
			if(size != 0.1)
			{
				Output::printError("Special paramater with size larger than 1 bit is not supported.");
				return RPC::RPCVariable::createError(-32500, "Special paramater with size larger than 1 bit is not supported.");
			}
			double index = rpcChannel->specialParameter->physicalParameter->address.index;
			int32_t bitStep = std::lround(rpcChannel->specialParameter->physicalParameter->address.step * 10);
			int32_t bitSteps = bitStep * (channel - rpcChannel->startIndex);
			while(bitSteps >= 8)
			{
				index += 1.0;
				bitSteps -= 8;
			}
			int32_t indexBits = std::lround(index * 10) % 10;
			if(indexBits + bitSteps >= 8)
			{
				index += 1.0;
				bitSteps = (indexBits + bitSteps) - 8;
			}
			index += ((double)bitSteps) / 10.0;
			std::vector<uint8_t> value = getConfigParameter(index, size);
			Output::printDebug("Debug: Special parameter at index " + std::to_string(index) + " is " + std::to_string(value.at(0)));
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
			if((*i)->id.empty()) continue;
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

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty())
		{
			remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);
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

std::shared_ptr<RPC::RPCVariable> HMWiredPeer::getParamset(int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == RPC::ParameterSet::Type::none) type = RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		std::shared_ptr<RPC::RPCVariable> variables(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::shared_ptr<BasicPeer> remotePeer;
		if(type == RPC::ParameterSet::Type::link && !remoteSerialNumber.empty()) remotePeer = getPeer(channel, remoteSerialNumber, remoteChannel);

		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty()) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::internal)) continue;
			std::shared_ptr<RPC::RPCVariable> element;
			if(type == RPC::ParameterSet::Type::Enum::values)
			{
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = valuesCentral[channel][(*i)->id].rpcParameter->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}
			else if(type == RPC::ParameterSet::Type::Enum::master)
			{
				//if(configCentral.find(channel) == configCentral.end()) continue;
				//if(configCentral[channel].find((*i)->id) == configCentral[channel].end()) continue;
				//element = configCentral[channel][(*i)->id].rpcParameter->convertFromPacket(configCentral[channel][(*i)->id].data);
			}
			else if(remotePeer)
			{
				//if(linksCentral.find(channel) == linksCentral.end()) continue;
				//if(linksCentral[channel][remotePeer->address][remotePeer->channel].find((*i)->id) == linksCentral[channel][remotePeer->address][remotePeer->channel].end()) continue;
				//if(remotePeer->channel != remoteChannel) continue;
				//element = linksCentral[channel][remotePeer->address][remotePeer->channel][(*i)->id].rpcParameter->convertFromPacket(linksCentral[channel][remotePeer->address][remotePeer->channel][(*i)->id].data);
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

} /* namespace HMWired */
