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

#include "PhilipsHuePeer.h"
#include "LogicalDevices/PhilipsHueCentral.h"
#include "GD.h"

namespace PhilipsHue
{
std::shared_ptr<BaseLib::Systems::Central> PhilipsHuePeer::getCentral()
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

std::shared_ptr<BaseLib::Systems::LogicalDevice> PhilipsHuePeer::getDevice(int32_t address)
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

PhilipsHuePeer::PhilipsHuePeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(GD::bl, parentID, centralFeatures, eventHandler)
{
}

PhilipsHuePeer::PhilipsHuePeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(GD::bl, id, address, serialNumber, parentID, centralFeatures, eventHandler)
{
}

PhilipsHuePeer::~PhilipsHuePeer()
{
	dispose();
}

std::string PhilipsHuePeer::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the indivual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			return stringStream.str();
		}
		return "Unknown command.\n";
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

void PhilipsHuePeer::save(bool savePeer, bool variables, bool centralConfig)
{
	try
	{
		Peer::save(savePeer, variables, centralConfig);
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

void PhilipsHuePeer::loadVariables(BaseLib::Systems::LogicalDevice* device, std::shared_ptr<BaseLib::Database::DataTable> rows)
{
	try
	{
		if(!rows) rows = raiseGetPeerVariables();
		Peer::loadVariables(device, rows);
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

bool PhilipsHuePeer::load(BaseLib::Systems::LogicalDevice* device)
{
	try
	{
		loadVariables(device);

		rpcDevice = GD::rpcDevices.find(_deviceType, _firmwareVersion, -1);
		if(!rpcDevice)
		{
			GD::out.printError("Error loading peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		std::string entry;
		loadConfig();
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

void PhilipsHuePeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		Peer::saveVariables();
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

std::shared_ptr<BaseLib::RPC::ParameterSet> PhilipsHuePeer::getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end())
		{
			GD::out.printDebug("Debug: Parameter set of type " + std::to_string(type) + " not found for channel " + std::to_string(channel));
			return std::shared_ptr<BaseLib::RPC::ParameterSet>();
		}
		return rpcChannel->parameterSets[type];
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

void PhilipsHuePeer::packetReceived(std::shared_ptr<PhilipsHuePacket> packet)
{
	try
	{
		if(!packet) return;
		if(!_centralFeatures || _disposing) return;
		if(packet->senderAddress() != _address) return;
		if(!rpcDevice) return;
		std::shared_ptr<PhilipsHueCentral> central = std::dynamic_pointer_cast<PhilipsHueCentral>(getCentral());
		if(!central) return;
		setLastPacketReceived();
		serviceMessages->endUnreach();

		/*std::shared_ptr<PhilipsHuePacket> sentPacket;
		std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> valueKeys;
		std::map<uint32_t, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>>> rpcValues;
		//Loop through all matching frames
		for(std::vector<FrameValues>::iterator a = frameValues.begin(); a != frameValues.end(); ++a)
		{
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame;
			if(!a->frameID.empty()) frame = rpcDevice->framesByID.at(a->frameID);

			std::vector<FrameValues> sentFrameValues;
			if(packet->messageType() == 0x02) //ACK packet: Check if all values were set correctly. If not set value again
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
						rpcValues[*j].reset(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
					}

					BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
					parameter->data = i->second.value;
					saveParameter(parameter->databaseID, parameter->data);
					if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + i->first + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(i->second.value) + ".");

					 //Process service messages
					if(parameter->rpcParameter && (parameter->rpcParameter->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !i->second.value.empty())
					{
						if(parameter->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeEnum)
						{
							serviceMessages->set(i->first, i->second.value.at(0), *j);
						}
						else if(parameter->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
						{
							serviceMessages->set(i->first, (bool)i->second.value.at(0));
						}
					}

					valueKeys[*j]->push_back(i->first);
					rpcValues[*j]->push_back(rpcDevice->channels.at(*j)->parameterSets.at(a->parameterSetType)->getParameter(i->first)->convertFromPacket(i->second.value, true));
				}
			}
		}

		if(packet->senderAddress() == _address && pendingQueues && !pendingQueues->empty())
		{
			if(packet->destinationAddress() == central->getAddress())
			{
				pendingQueues->front()->setWakeOnRadio(false);

				std::vector<uint8_t> payload;
				payload.push_back(0);
				payload.push_back(0);
				std::shared_ptr<PhilipsHuePacket> wakeUpPacket(new PhilipsHuePacket(packet->messageCounter(), 0x02, 0x00, central->getAddress(), _address, payload, false));
				central->sendPacket(_physicalInterface, wakeUpPacket, false);

				if(packet->messageSubtype() & 2) central->enqueuePendingQueues(_address);
			}
			else if(packet->messageSubtype() & 2) //Wait for the last packet and then enqueue
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(60)); //Wait for the response from the receiving device. 80ms is PhilipsHueimum, 55ms is minimum => With 60ms buffer of 20ms
				central->enqueuePendingQueues(_address);
			}
		}
		else if(packet->messageType() != 0x02 && packet->messageType() != 0xFF && packet->destinationAddress() == central->getAddress()) central->sendOK(packet->messageCounter(), packet->senderAddress());

		//if(!rpcValues.empty() && !resendPacket)
		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::const_iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;
				std::string address(_serialNumber + ":" + std::to_string(j->first));
				raiseEvent(_peerID, j->first, j->second, rpcValues.at(j->first));
				raiseRPCEvent(_peerID, j->first, address, j->second, rpcValues.at(j->first));
			}
		}*/
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

std::string PhilipsHuePeer::getFirmwareVersionString(int32_t firmwareVersion)
{
	try
	{
		return std::to_string(firmwareVersion);
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

//RPC Methods
std::shared_ptr<BaseLib::RPC::RPCVariable> PhilipsHuePeer::getParamsetDescription(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set");

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> PhilipsHuePeer::putParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::RPCVariable> variables, bool onlyPushing)
{
	return BaseLib::RPC::RPCVariable::createError(-32500, "Method not implemented.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> PhilipsHuePeer::getParamset(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcChannel->parameterSets[type];
		if(!parameterSet) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::RPCVariable> variables(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::internal) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::transform))
			{
				GD::out.printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			std::shared_ptr<BaseLib::RPC::RPCVariable> element;
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
			{
				if(!((*i)->operations & BaseLib::RPC::Parameter::Operations::read) && !((*i)->operations & BaseLib::RPC::Parameter::Operations::event)) continue;
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = (*i)->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}

			if(!element) continue;
			if(element->type == BaseLib::RPC::RPCVariableType::rpcVoid) continue;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> PhilipsHuePeer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::RPCVariable> value)
{
	try
	{
		Peer::setValue(channel, valueKey, value); //Ignore result, otherwise setHomegerValue might not be executed
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return BaseLib::RPC::RPCVariable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		if(rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeAction && !value->booleanValue) return BaseLib::RPC::RPCVariable::createError(-5, "Parameter of type action cannot be set to \"false\".");
		BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values(new std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>());
		if((rpcParameter->operations & BaseLib::RPC::Parameter::Operations::read) || (rpcParameter->operations & BaseLib::RPC::Parameter::Operations::event))
		{
			valueKeys->push_back(valueKey);
			values->push_back(value);
		}
		if(rpcParameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::Enum::store)
		{
			parameter->data = rpcParameter->convertToPacket(value);
			saveParameter(parameter->databaseID, parameter->data);
			if(!valueKeys->empty()) raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);
			return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		}
		else if(rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::command) return BaseLib::RPC::RPCVariable::createError(-6, "Parameter is not settable.");
		//_resendCounter[valueKey] = 0;
		if(!rpcParameter->conversion.empty() && rpcParameter->conversion.at(0)->type == BaseLib::RPC::ParameterConversion::Type::Enum::toggle)
		{
			//Handle toggle parameter
			std::string toggleKey = rpcParameter->conversion.at(0)->stringValue;
			if(toggleKey.empty()) return BaseLib::RPC::RPCVariable::createError(-6, "No toggle parameter specified (parameter attribute value is empty).");
			if(valuesCentral[channel].find(toggleKey) == valuesCentral[channel].end()) return BaseLib::RPC::RPCVariable::createError(-5, "Toggle parameter not found.");
			BaseLib::Systems::RPCConfigurationParameter* toggleParam = &valuesCentral[channel][toggleKey];
			std::shared_ptr<BaseLib::RPC::RPCVariable> toggleValue;
			if(toggleParam->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
			{
				toggleValue = toggleParam->rpcParameter->convertFromPacket(toggleParam->data);
				toggleValue->booleanValue = !toggleValue->booleanValue;
			}
			else if(toggleParam->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeInteger ||
					toggleParam->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeFloat)
			{
				int32_t currentToggleValue = (int32_t)toggleParam->data.at(0);
				std::vector<uint8_t> temp({0});
				if(currentToggleValue != toggleParam->rpcParameter->conversion.at(0)->on) temp.at(0) = toggleParam->rpcParameter->conversion.at(0)->on;
				else temp.at(0) = toggleParam->rpcParameter->conversion.at(0)->off;
				toggleValue = toggleParam->rpcParameter->convertFromPacket(temp);
			}
			else return BaseLib::RPC::RPCVariable::createError(-6, "Toggle parameter has to be of type boolean, float or integer.");
			return setValue(channel, toggleKey, toggleValue);
		}
		std::string setRequest = rpcParameter->physicalParameter->setRequest;
		if(setRequest.empty()) return BaseLib::RPC::RPCVariable::createError(-6, "parameter is read only");
		if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return BaseLib::RPC::RPCVariable::createError(-6, "No frame was found for parameter " + valueKey);
		std::shared_ptr<BaseLib::RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];
		std::vector<uint8_t> data = rpcParameter->convertToPacket(value);
		parameter->data = data;
		saveParameter(parameter->databaseID, data);
		if(_bl->debugLevel > 4) GD::out.printDebug("Debug: " + valueKey + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(channel) + " was set to " + BaseLib::HelperFunctions::getHexString(data) + ".");

		std::shared_ptr<PhilipsHueCentral> central = std::dynamic_pointer_cast<PhilipsHueCentral>(getCentral());
		/*std::shared_ptr<PacketQueue> queue(new PacketQueue(_physicalInterface, PacketQueueType::PEER));
		queue->peer = central->getPeer(_peerID);
		queue->noSending = true;

		std::vector<uint8_t> payload;
		if(frame->subtype > -1 && frame->subtypeIndex >= 9)
		{
			while((signed)payload.size() - 1 < frame->subtypeIndex - 9) payload.push_back(0);
			payload.at(frame->subtypeIndex - 9) = (uint8_t)frame->subtype;
		}
		if(frame->channelField >= 9)
		{
			while((signed)payload.size() - 1 < frame->channelField - 9) payload.push_back(0);
			payload.at(frame->channelField - 9) = (uint8_t)channel;
		}
		std::shared_ptr<PhilipsHuePacket> packet(new PhilipsHuePacket(_messageCounter, (uint8_t)frame->type, frame->subtype, getCentral()->physicalAddress(), _address, payload, getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst));

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
			else if(i->param == rpcParameter->physicalParameter->valueID || i->param == rpcParameter->physicalParameter->id)
			{
				std::vector<uint8_t> data = valuesCentral[channel][valueKey].data;
				if(i->mask != 0 && data.size() == 1) data.at(0) &= i->mask;
				packet->setPosition(i->index, i->size, data);
			}
			//Search for all other parameters
			else
			{
				bool paramFound = false;
				for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator j = valuesCentral[channel].begin(); j != valuesCentral[channel].end(); ++j)
				{
					//Only compare id. Till now looking for value_id was not necessary.
					if(i->param == j->second.rpcParameter->physicalParameter->id)
					{
						std::vector<uint8_t> data = j->second.data;
						if(i->mask != 0 && data.size() == 1) data.at(0) &= i->mask;
						packet->setPosition(i->index, i->size, data);
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
				std::shared_ptr<BaseLib::RPC::RPCVariable> logicalDefaultValue = valuesCentral.at(channel).at(*j).rpcParameter->logicalParameter->getDefaultValue();
				std::vector<uint8_t> defaultValue = valuesCentral.at(channel).at(*j).rpcParameter->convertToPacket(logicalDefaultValue);
				if(defaultValue != valuesCentral.at(channel).at(*j).data)
				{
					BaseLib::Systems::RPCConfigurationParameter* tempParam = &valuesCentral.at(channel).at(*j);
					tempParam->data = defaultValue;
					saveParameter(tempParam->databaseID, tempParam->data);
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
		queue->parameterName = valueKey;
		queue->channel = channel;
		queue->push(packet);
		queue->push(central->getMessages()->find(DIRECTIONIN, 0x02, 0x02, std::vector<std::pair<uint32_t, int32_t>>()));
		pendingQueues->removeQueue(valueKey, channel);
		pendingQueues->push(queue);
		if(PhilipsHueDevice::isSwitch(_deviceType)) queue->retries = 12;
		central->enqueuePendingQueues(_address);

		if(!valueKeys->empty()) raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));*/
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. See error log for more details.");
}
//End RPC methods
}
