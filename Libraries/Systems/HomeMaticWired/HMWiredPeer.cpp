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
					Output::printError("Error loading HomeMatic Wired peer 0x" + HelperFunctions::getHexString(_address) + ": Device id unknown: 0x" + HelperFunctions::getHexString(row->second.at(3)->intValue) + " Firmware version: " + std::to_string(_firmwareVersion));
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
		Output::printDebug("Loading HomeMatic Wired peer 0x" + HelperFunctions::getHexString(_address, 8));

		loadVariables((HMWiredDevice*)device);

		rpcDevice = GD::rpcDevices.find(_deviceType, _firmwareVersion, -1);
		if(!rpcDevice)
		{
			Output::printError("Error loading HomeMatic Wired peer 0x" + HelperFunctions::getHexString(_address) + ": Device type not found: 0x" + HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
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
			if(channelIndex >= 9 && (signed)packet->payload()->size() > (channelIndex - 9)) channel = packet->payload()->at(channelIndex - 9);
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
		if(!packet) return;
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
					if(GD::debugLevel >= 4) Output::printInfo("Info: " + i->first + " of HomeMatic Wired device 0x" + HelperFunctions::getHexString(_address, 8) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + HelperFunctions::getHexString(i->second.value) + ".");

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

			if(!a->values.empty() && packet->type() == HMWiredPacketType::iMessage && packet->destinationAddress() == getCentral()->getAddress())
			{
				getCentral()->sendOK(packet->senderMessageCounter(), packet->senderAddress());
			}
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

} /* namespace HMWired */
