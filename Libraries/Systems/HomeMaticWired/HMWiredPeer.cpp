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
#include "PendingHMWiredQueues.h"
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
	if(centralFeatures)
	{
		serviceMessages.reset(new ServiceMessages(this));
		pendingHMWiredQueues.reset(new PendingHMWiredQueues());
	}
}

HMWiredPeer::HMWiredPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures) : Peer(id, address, serialNumber, parentID, centralFeatures)
{
}

HMWiredPeer::~HMWiredPeer()
{

}

void HMWiredPeer::setConfigParameter(double index, double size, std::vector<uint8_t>& binaryValue)
{
	try
	{
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
				Output::printError("Error: HomeMatic Wired Peer 0x" + HelperFunctions::getHexString(_address, 8) + ": Can't set partial byte index > 1.");
				return;
			}
			uint32_t bitSize = std::lround(size * 10);
			std::vector<uint8_t>* configBlock = &binaryConfig[configBlockIndex].data;
			if(bitSize > 8) bitSize = 8;
			while(configBlock->size() - 1 < intByteIndex) configBlock->push_back(0xFF);
			if(bitSize == 8)
			{
				configBlock->at(intByteIndex) = 0;
			}
			else
			{
				configBlock->at(intByteIndex) &= (~_bitmask[bitSize]);
			}
			configBlock->at(intByteIndex) |= (binaryValue.at(binaryValue.size() - 1) << (std::lround(index * 10) % 10));
		}
		else
		{
			int32_t intByteIndex = byteIndex;
			int32_t configBlockIndex = (intByteIndex / 0x10) * 0x10;
			intByteIndex -= configBlockIndex;
			uint32_t bytes = (uint32_t)std::ceil(size);
			std::vector<uint8_t>* configBlock = &binaryConfig[configBlockIndex].data;
			while(configBlock->size() < intByteIndex + bytes && configBlock->size() < 10) configBlock->push_back(0xFF);
			if(binaryValue.empty()) return;
			uint32_t bitSize = std::lround(size * 10) % 10;
			if(bitSize > 8) bitSize = 8;
			if(bytes == 0) bytes = 1; //size is 0 - assume 1
			if(bytes <= binaryValue.size())
			{
				configBlock->at(intByteIndex) &= (~_bitmask[bitSize]);
				configBlock->at(intByteIndex) |= (binaryValue.at(0) & _bitmask[bitSize]);
				for(uint32_t i = 1; i < bytes; i++)
				{
					if(intByteIndex + i > 9)
					{
						intByteIndex = -i;
						configBlockIndex += 0x10;
						configBlock = &binaryConfig[configBlockIndex].data;
						while(configBlock->size() < 10) configBlock->push_back(0xFF);
					}
					configBlock->at(intByteIndex + i) = binaryValue.at(i);
				}
			}
			else
			{
				uint32_t missingBytes = bytes - binaryValue.size();
				for(uint32_t i = 0; i < missingBytes; i++)
				{
					configBlock->at(intByteIndex) &= (~_bitmask[bitSize]);
					for(uint32_t i = 1; i < bytes; i++)
					{
						if(intByteIndex + i > 9)
						{
							intByteIndex = -i;
							configBlockIndex += 0x10;
							configBlock = &binaryConfig[configBlockIndex].data;
							while(configBlock->size() < 10) configBlock->push_back(0xFF);
						}
						configBlock->at(intByteIndex + i) = 0;
					}
				}
				for(uint32_t i = 0; i < binaryValue.size(); i++)
				{
					if(intByteIndex + missingBytes + i > 9)
					{
						intByteIndex = -(missingBytes + i);
						configBlockIndex += 0x10;
						configBlock = &binaryConfig[configBlockIndex].data;
						while(configBlock->size() < 10) configBlock->push_back(0xFF);
					}
					configBlock->at(intByteIndex + missingBytes + i) = binaryValue.at(i);
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
				Output::printError("Error: Error resetting HomeMatic Wired peer 0x" + HelperFunctions::getHexString(_address, 8) + ". Could not write EEPROM.");
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

void HMWiredPeer::addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters)
{
	try
	{
		if(parameters->integers.size() != 3) return;
		if(parameters->strings.size() != 1) return;
		Output::printMessage("addVariableToResetCallback invoked for parameter " + parameters->strings.at(0) + " of device 0x" + HelperFunctions::getHexString(_address) + " with serial number " + _serialNumber + ".", 5);
		Output::printInfo("Parameter " + parameters->strings.at(0) + " of device 0x" + HelperFunctions::getHexString(_address) + " with serial number " + _serialNumber + " will be reset at " + HelperFunctions::getTimeString(parameters->integers.at(2)) + ".");
		std::shared_ptr<VariableToReset> variable(new VariableToReset);
		variable->channel = parameters->integers.at(0);
		int32_t integerValue = parameters->integers.at(1);
		HelperFunctions::memcpyBigEndian(variable->data, integerValue);
		variable->resetTime = parameters->integers.at(2);
		variable->key = parameters->strings.at(0);
		_variablesToResetMutex.lock();
		_variablesToReset.push_back(variable);
		_variablesToResetMutex.unlock();
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

} /* namespace HMWired */
