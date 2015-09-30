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

#include "Peer.h"
#include "ServiceMessages.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

Peer::Peer(BaseLib::Obj* baseLib, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler)
{
	try
	{
		_bl = baseLib;
		_parentID = parentID;
		_centralFeatures = centralFeatures;
		if(centralFeatures) serviceMessages.reset(new ServiceMessages(baseLib, 0, "", this));
		_lastPacketReceived = HelperFunctions::getTimeSeconds();
		_rpcDevice.reset();
		setEventHandler(eventHandler);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::Peer(BaseLib::Obj* baseLib, int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : Peer(baseLib, parentID, centralFeatures, eventHandler)
{
	try
	{
		_peerID = id;
		_address = address;
		_serialNumber = serialNumber;
		if(serviceMessages)
		{
			serviceMessages->setPeerID(id);
			serviceMessages->setPeerSerial(serialNumber);
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::~Peer()
{
	serviceMessages->resetEventHandler();
}

void Peer::dispose()
{
	_disposing = true;
	_central.reset();
	_peers.clear();
}


//Event handling
void Peer::raiseCreateSavepointSynchronous(std::string name)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onCreateSavepointSynchronous(name);
}

void Peer::raiseReleaseSavepointSynchronous(std::string name)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onReleaseSavepointSynchronous(name);
}

void Peer::raiseCreateSavepointAsynchronous(std::string name)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onCreateSavepointAsynchronous(name);
}

void Peer::raiseReleaseSavepointAsynchronous(std::string name)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onReleaseSavepointAsynchronous(name);
}

void Peer::raiseDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onDeleteMetadata(peerID, serialNumber, dataID);
}

void Peer::raiseDeletePeer()
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onDeletePeer(_peerID);
}

uint64_t Peer::raiseSavePeer()
{
	if(!_eventHandler) return 0;
	return ((IPeerEventSink*)_eventHandler)->onSavePeer(_peerID, _parentID, _address, _serialNumber);
}

void Peer::raiseSavePeerParameter(Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IPeerEventSink*)_eventHandler)->onSavePeerParameter(_peerID, data);
}

void Peer::raiseSavePeerVariable(Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IPeerEventSink*)_eventHandler)->onSavePeerVariable(_peerID, data);
}

std::shared_ptr<Database::DataTable> Peer::raiseGetPeerParameters()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IPeerEventSink*)_eventHandler)->onGetPeerParameters(_peerID);
}

std::shared_ptr<Database::DataTable> Peer::raiseGetPeerVariables()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IPeerEventSink*)_eventHandler)->onGetPeerVariables(_peerID);
}

void Peer::raiseDeletePeerParameter(Database::DataRow& data)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onDeletePeerParameter(_peerID, data);
}

bool Peer::raiseSetPeerID(uint64_t newPeerID)
{
	if(!_eventHandler) return false;
	return ((IPeerEventSink*)_eventHandler)->onSetPeerID(_peerID, newPeerID);
}

std::shared_ptr<Database::DataTable> Peer::raiseGetServiceMessages()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IPeerEventSink*)_eventHandler)->onGetServiceMessages(_peerID);
}

void Peer::raiseSaveServiceMessage(Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IPeerEventSink*)_eventHandler)->onSaveServiceMessage(_peerID, data);
}

void Peer::raiseDeleteServiceMessage(uint64_t databaseID)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onDeleteServiceMessage(databaseID);
}

void Peer::raiseAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onAddWebserverEventHandler(eventHandler, _webserverEventHandlers);
}

void Peer::raiseRemoveWebserverEventHandler()
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onRemoveWebserverEventHandler(_webserverEventHandlers);
}

void Peer::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void Peer::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onRPCUpdateDevice(id, channel, address, hint);
}

void Peer::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onEvent(peerID, channel, variables, values);
}

int32_t Peer::raiseIsAddonClient(int32_t clientID)
{
	if(_eventHandler) return ((IPeerEventSink*)_eventHandler)->onIsAddonClient(clientID);
	return -1;
}
//End event handling

//ServiceMessages event handling
void Peer::onConfigPending(bool configPending)
{

}

void Peer::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<Variable>>> values)
{
	raiseRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void Peer::onSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data)
{
	try
	{
		if(_peerID == 0) return; //Peer not saved yet
		if(valuesCentral.find(channel) == valuesCentral.end())
		{
			//Service message variables sometimes just don't exist. So only output a debug message.
			if(channel != 0) _bl->out.printWarning("Warning: Could not set parameter " + name + " on channel " + std::to_string(channel) + " for peer " + std::to_string(_peerID) + ". Channel does not exist.");
			else _bl->out.printDebug("Debug: Could not set parameter " + name + " on channel " + std::to_string(channel) + " for peer " + std::to_string(_peerID) + ". Channel does not exist.");
			return;
		}
		if(valuesCentral.at(channel).find(name) == valuesCentral.at(channel).end())
		{
			if(_bl->debugLevel >= 5) _bl->out.printDebug("Debug: Could not set parameter " + name + " on channel " + std::to_string(channel) + " for peer " + std::to_string(_peerID) + ". Parameter does not exist.");
			return;
		}
		RPCConfigurationParameter* parameter = &valuesCentral.at(channel).at(name);
		if(parameter->data == data) return;
		parameter->data = data;
		saveParameter(parameter->databaseID, ParameterGroup::Type::Enum::variables, channel, name, parameter->data);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<Database::DataTable> Peer::onGetServiceMessages()
{
	return raiseGetServiceMessages();
}

void Peer::onSaveServiceMessage(Database::DataRow& data)
{
	raiseSaveServiceMessage(data);
}

void Peer::onDeleteServiceMessage(uint64_t databaseID)
{
	raiseDeleteServiceMessage(databaseID);
}

void Peer::onEnqueuePendingQueues()
{
	try
	{
		if(pendingQueuesEmpty()) return;
		if(!(getRXModes() & HomegearDevice::ReceiveModes::Enum::always) && !(getRXModes() & HomegearDevice::ReceiveModes::Enum::wakeOnRadio)) return;
		enqueuePendingQueues();
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
//End ServiceMessages event handling

void Peer::setID(uint64_t id)
{
	if(_peerID == 0)
	{
		_peerID = id;
		if(serviceMessages) serviceMessages->setPeerID(id);
	}
	else _bl->out.printError("Cannot reset peer ID");
}

void Peer::setSerialNumber(std::string serialNumber)
{
	if(serialNumber.length() > 20) return;
	_serialNumber = serialNumber;
	if(serviceMessages) serviceMessages->setPeerSerial(serialNumber);
	if(_peerID > 0) save(true, false, false);
}

HomegearDevice::ReceiveModes::Enum Peer::getRXModes()
{
	try
	{
		if(_rpcDevice)
		{
			_rxModes = _rpcDevice->receiveModes;
			if(configCentral.find(0) != configCentral.end())
			{
				std::unordered_map<std::string, RPCConfigurationParameter>::iterator parameterIterator = configCentral.at(0).find("WAKE_ON_RADIO");
				if(parameterIterator != configCentral.at(0).end())
				{
					if(!parameterIterator->second.rpcParameter) return _rxModes;
					if(parameterIterator->second.rpcParameter->convertFromPacket(parameterIterator->second.data)->booleanValue)
					{
						_rxModes = (HomegearDevice::ReceiveModes::Enum)(_rxModes | HomegearDevice::ReceiveModes::Enum::wakeOnRadio);
					}
					else
					{
						_rxModes = (HomegearDevice::ReceiveModes::Enum)(_rxModes & (~HomegearDevice::ReceiveModes::Enum::wakeOnRadio));
					}
				}
			}
		}
		return _rxModes;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return _rxModes;
}

void Peer::setLastPacketReceived()
{
	_lastPacketReceived = HelperFunctions::getTimeSeconds();
}

std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, std::string serialNumber, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			if((*i)->serialNumber.empty())
			{
				std::shared_ptr<Central> central(getCentral());
				if(central)
				{
					std::shared_ptr<Peer> peer(central->logicalDevice()->getPeer((*i)->address));
					if(peer) (*i)->serialNumber = peer->getSerialNumber();
				}
			}
			if((*i)->serialNumber == serialNumber && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, int32_t address, int32_t remoteChannel)
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

std::shared_ptr<BasicPeer> Peer::getPeer(int32_t channel, uint64_t id, int32_t remoteChannel)
{
	try
	{
		if(_peers.find(channel) == _peers.end()) return std::shared_ptr<BasicPeer>();

		for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers[channel].begin(); i != _peers[channel].end(); ++i)
		{
			//TODO: Remove a few versions after ID is saved
			if((*i)->id == 0)
			{
				std::shared_ptr<Peer> peer = getCentral()->logicalDevice()->getPeer((*i)->serialNumber);
				if(peer) (*i)->id = peer->getID();
				else if((*i)->hidden && (*i)->address == getCentral()->logicalDevice()->getAddress())
				{
					(*i)->id = 0xFFFFFFFFFFFFFFFF;
				}
			}
			if((*i)->id == id && (remoteChannel < 0 || remoteChannel == (*i)->channel)) return *i;
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	return std::shared_ptr<BasicPeer>();
}

void Peer::deleteFromDatabase()
{
	try
	{
		deleting = true;
		raiseDeleteMetadata(_peerID, _serialNumber);
		raiseDeletePeer();
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::initializeCentralConfig()
{
	std::string savepointName("PeerConfig" + std::to_string(_peerID));
	try
	{
		if(!_rpcDevice)
		{
			_bl->out.printWarning("Warning: Tried to initialize peer's central config without rpcDevice being set.");
			return;
		}
		raiseCreateSavepointAsynchronous(savepointName);
		for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
		{
			initializeMasterSet(i->first, i->second->configParameters);
			initializeValueSet(i->first, i->second->variables);
			if(i->second->alternativeFunction)
			{
				initializeMasterSet(i->first, i->second->alternativeFunction->configParameters);
				initializeValueSet(i->first, i->second->alternativeFunction->variables);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    raiseReleaseSavepointAsynchronous(savepointName);
}

void Peer::initializeMasterSet(int32_t channel, PConfigParameters masterSet)
{
	try
	{
		if(!masterSet || masterSet->parameters.empty()) return;
		BaseLib::Systems::RPCConfigurationParameter parameter;
		for(Parameters::iterator j = masterSet->parameters.begin(); j != masterSet->parameters.end(); ++j)
		{
			if(!j->second) continue;
			if(!j->second->id.empty() && configCentral[channel].find(j->second->id) == configCentral[channel].end())
			{
				parameter = BaseLib::Systems::RPCConfigurationParameter();
				parameter.rpcParameter = j->second;
				setDefaultValue(&parameter);
				configCentral[channel][j->second->id] = parameter;
				saveParameter(0, ParameterGroup::Type::config, channel, j->second->id, parameter.data);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::initializeValueSet(int32_t channel, PVariables valueSet)
{
	try
	{
		if(!valueSet || valueSet->parameters.empty()) return;
		BaseLib::Systems::RPCConfigurationParameter parameter;
		for(Parameters::iterator j = valueSet->parameters.begin(); j != valueSet->parameters.end(); ++j)
		{
			if(!j->second) continue;
			if(!j->second->id.empty() && valuesCentral[channel].find(j->second->id) == valuesCentral[channel].end())
			{
				parameter = BaseLib::Systems::RPCConfigurationParameter();
				parameter.rpcParameter = j->second;
				setDefaultValue(&parameter);
				valuesCentral[channel][j->second->id] = parameter;
				saveParameter(0, ParameterGroup::Type::variables, channel, j->second->id, parameter.data);
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::setDefaultValue(RPCConfigurationParameter* parameter)
{
	try
	{
		//parameter cannot be nullptr at this point.
		parameter->rpcParameter->convertToPacket(parameter->rpcParameter->logical->getDefaultValue(), parameter->data);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::save(bool savePeer, bool variables, bool centralConfig)
{
	std::string savepointName("peer_54" + std::to_string(_parentID) + std::to_string(_peerID));
	try
	{
		if(deleting || isTeam()) return;

		if(savePeer)
		{
			_databaseMutex.lock();
			uint64_t result = raiseSavePeer();
			if(_peerID == 0 && result > 0) setID(result);
			_databaseMutex.unlock();
		}
		if(variables || centralConfig) raiseCreateSavepointAsynchronous(savepointName);
		if(variables) saveVariables();
		if(centralConfig) saveConfig();
	}
	catch(const std::exception& ex)
    {
		_databaseMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_databaseMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_databaseMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    if(variables || centralConfig) raiseReleaseSavepointAsynchronous(savepointName);
}

void Peer::saveParameter(uint32_t parameterID, std::vector<uint8_t>& value)
{
	try
	{
		if(parameterID == 0)
		{
			if(!isTeam()) _bl->out.printError("Error: Peer " + std::to_string(_peerID) + ": Tried to save parameter without parameterID");
			return;
		}
		_databaseMutex.lock();
		Database::DataRow data;
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(value)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(parameterID)));
		raiseSavePeerParameter(data);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveParameter(uint32_t parameterID, uint32_t address, std::vector<uint8_t>& value)
{
	try
	{
		if(parameterID > 0)
		{
			saveParameter(parameterID, value);
			return;
		}
		if(_peerID == 0 || isTeam()) return;
		//Creates a new entry for parameter in database
		_databaseMutex.lock();
		Database::DataRow data;
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(0)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(address)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(0)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(0)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(std::string(""))));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(value)));
		raiseSavePeerParameter(data);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveParameter(uint32_t parameterID, ParameterGroup::Type::Enum parameterSetType, uint32_t channel, std::string parameterName, std::vector<uint8_t>& value, int32_t remoteAddress, uint32_t remoteChannel)
{
	try
	{
		if(parameterID > 0)
		{
			saveParameter(parameterID, value);
			return;
		}
		if(_peerID == 0 || isTeam()) return;
		//Creates a new entry for parameter in database
		_databaseMutex.lock();
		Database::DataRow data;
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn((uint32_t)parameterSetType)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(channel)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(remoteAddress)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(remoteChannel)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(parameterName)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(value)));
		raiseSavePeerParameter(data);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::loadVariables(BaseLib::Systems::LogicalDevice* device, std::shared_ptr<BaseLib::Database::DataTable> rows)
{
	try
	{
		if(!rows) return;
		_databaseMutex.lock();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1000:
				_name = row->second.at(4)->textValue;
				break;
			case 1001:
				_firmwareVersion = row->second.at(3)->intValue;
				break;
			case 1002:
				_deviceType = BaseLib::Systems::LogicalDeviceType(device->deviceFamily(), row->second.at(3)->intValue);
				if(_deviceType.type() == (uint32_t)0xFFFFFFFF)
				{
					_bl->out.printError("Error loading peer " + std::to_string(_peerID) + ": Device type unknown: 0x" + BaseLib::HelperFunctions::getHexString(row->second.at(3)->intValue) + " Firmware version: " + std::to_string(_firmwareVersion));
				}
				break;
			case 1003:
				_firmwareVersionString = row->second.at(4)->textValue;
				break;
			case 1004:
				_ip = row->second.at(4)->textValue;
				break;
			case 1005:
				_idString = row->second.at(4)->textValue;
				break;
			case 1006:
				_typeString = row->second.at(4)->textValue;
				break;
			}
		}
		_databaseMutex.unlock();
		return;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveVariables()
{
	try
	{
		if(_peerID == 0 || isTeam()) return;
		saveVariable(1000, _name);
		saveVariable(1001, _firmwareVersion);
		saveVariable(1002, (int32_t)_deviceType.type());
		saveVariable(1003, _firmwareVersionString);
		saveVariable(1004, _ip);
		saveVariable(1005, _idString);
		saveVariable(1006, _typeString);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::saveVariable(uint32_t index, int32_t intValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSavePeerVariable(data);
		}
		else
		{
			if(_peerID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			raiseSavePeerVariable(data);
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSavePeerVariable(data);
		}
		else
		{
			if(_peerID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			raiseSavePeerVariable(data);
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSavePeerVariable(data);
		}
		else
		{
			if(_peerID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			raiseSavePeerVariable(data);
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSavePeerVariable(data);
		}
		else
		{
			if(_peerID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
			raiseSavePeerVariable(data);
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveConfig()
{
	try
	{
		if(_peerID == 0 || isTeam()) return;
		for(std::unordered_map<uint32_t, ConfigDataBlock>::iterator i = binaryConfig.begin(); i != binaryConfig.end(); ++i)
		{
			std::string emptyString;
			if(i->second.databaseID > 0) saveParameter(i->second.databaseID, i->second.data);
			else saveParameter(0, i->first, i->second.data);
		}
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(j->first.empty())
				{
					_bl->out.printError("Error: Parameter has no id.");
					continue;
				}
				if(j->second.databaseID > 0) saveParameter(j->second.databaseID, j->second.data);
				else saveParameter(0, ParameterGroup::Type::Enum::config, i->first, j->first, j->second.data);
			}
		}
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(j->first.empty())
				{
					_bl->out.printError("Error: Parameter has no id.");
					continue;
				}
				if(j->second.databaseID > 0) saveParameter(j->second.databaseID, j->second.data);
				else saveParameter(0, ParameterGroup::Type::Enum::variables, i->first, j->first, j->second.data);
			}
		}
		for(std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>::iterator i = linksCentral.begin(); i != linksCentral.end(); ++i)
		{
			for(std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator l = k->second.begin(); l != k->second.end(); ++l)
					{
						if(l->first.empty())
						{
							_bl->out.printError("Error: Parameter has no id.");
							continue;
						}
						if(l->second.databaseID > 0) saveParameter(l->second.databaseID, l->second.data);
						else saveParameter(0, ParameterGroup::Type::Enum::link, i->first, l->first, l->second.data, j->first, k->first);
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::loadConfig()
{
	try
	{
		_databaseMutex.lock();
		Database::DataRow data;
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetPeerParameters();
		for(Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			uint32_t databaseID = row->second.at(0)->intValue;
			ParameterGroup::Type::Enum parameterGroupType = (ParameterGroup::Type::Enum)row->second.at(2)->intValue;

			if(parameterGroupType == ParameterGroup::Type::Enum::none)
			{
				uint32_t index = row->second.at(3)->intValue;
				ConfigDataBlock* config = &binaryConfig[index];
				config->databaseID = databaseID;
				config->data.insert(config->data.begin(), row->second.at(7)->binaryValue->begin(), row->second.at(7)->binaryValue->end());
			}
			else
			{
				uint32_t channel = row->second.at(3)->intValue;
				int32_t remoteAddress = row->second.at(4)->intValue;
				int32_t remoteChannel = row->second.at(5)->intValue;
				std::string* parameterName = &row->second.at(6)->textValue;
				if(parameterName->empty())
				{
					_bl->out.printCritical("Critical: Added central config parameter without id. Device: " + std::to_string(_peerID) + " Channel: " + std::to_string(channel));
					_databaseMutex.lock();
					data.clear();
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(std::string(""))));
					raiseDeletePeerParameter(data);
					_databaseMutex.unlock();
					continue;
				}

				RPCConfigurationParameter* parameter = nullptr;
				if(parameterGroupType == ParameterGroup::Type::Enum::config) parameter = &configCentral[channel][*parameterName];
				else if(parameterGroupType == ParameterGroup::Type::Enum::variables) parameter = &valuesCentral[channel][*parameterName];
				else if(parameterGroupType == ParameterGroup::Type::Enum::link) parameter = &linksCentral[channel][remoteAddress][remoteChannel][*parameterName];
				parameter->databaseID = databaseID;
				parameter->data.insert(parameter->data.begin(), row->second.at(7)->binaryValue->begin(), row->second.at(7)->binaryValue->end());
				if(!_rpcDevice)
				{
					_bl->out.printError("Critical: No xml rpc device found for peer " + std::to_string(_peerID) + ".");
					continue;
				}
				if(_rpcDevice->functions.find(channel) != _rpcDevice->functions.end() && _rpcDevice->functions[channel])
				{
					if(parameterGroupType == ParameterGroup::Type::Enum::config) parameter->rpcParameter = _rpcDevice->functions[channel]->configParameters->parameters[*parameterName];
					else if(parameterGroupType == ParameterGroup::Type::Enum::variables) parameter->rpcParameter = _rpcDevice->functions[channel]->variables->parameters[*parameterName];
					else if(parameterGroupType == ParameterGroup::Type::Enum::link) parameter->rpcParameter = _rpcDevice->functions[channel]->linkParameters->parameters[*parameterName];
					if(!parameter->rpcParameter && _rpcDevice->functions[channel]->alternativeFunction)
					{
						if(parameterGroupType == ParameterGroup::Type::Enum::config) parameter->rpcParameter = _rpcDevice->functions[channel]->alternativeFunction->configParameters->parameters[*parameterName];
						else if(parameterGroupType == ParameterGroup::Type::Enum::variables) parameter->rpcParameter = _rpcDevice->functions[channel]->alternativeFunction->variables->parameters[*parameterName];
						else if(parameterGroupType == ParameterGroup::Type::Enum::link) parameter->rpcParameter = _rpcDevice->functions[channel]->alternativeFunction->linkParameters->parameters[*parameterName];
					}
				}
				if(!parameter->rpcParameter)
				{
					_bl->out.printError("Error: Deleting parameter " + *parameterName + ", because no corresponding RPC parameter was found. Peer: " + std::to_string(_peerID) + " Channel: " + std::to_string(channel) + " Parameter set type: " + std::to_string((uint32_t)parameterGroupType));
					Database::DataRow data;
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn((int32_t)parameterGroupType)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(channel)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(*parameterName)));
					if(parameterGroupType == ParameterGroup::Type::Enum::config)
					{
						configCentral[channel].erase(*parameterName);
					}
					else if(parameterGroupType == ParameterGroup::Type::Enum::variables)
					{
						valuesCentral[channel].erase(*parameterName);
					}
					else if(parameterGroupType == ParameterGroup::Type::Enum::link)
					{
						linksCentral[channel][remoteAddress][remoteChannel].erase(*parameterName);
						data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(remoteAddress)));
						data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(remoteChannel)));
					}
					raiseDeletePeerParameter(data);
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::initializeTypeString()
{
	try
	{
		if(!_rpcDevice) return;
		if(!_typeString.empty())
		{
			_rpcTypeString = _typeString;
			return;
		}
		PSupportedDevice rpcDeviceType = _rpcDevice->getType(_deviceType, _firmwareVersion);
		if(rpcDeviceType) _rpcTypeString = rpcDeviceType->id;
		else if(_deviceType.type() == 0) _rpcTypeString = "HM-RCV-50"; //Central
		else if(!_rpcDevice->supportedDevices.empty()) _rpcTypeString = _rpcDevice->supportedDevices.at(0)->id;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

//RPC methods
std::shared_ptr<Variable> Peer::getAllValues(int32_t clientID, bool returnWriteOnly)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<Variable> values(new Variable(VariableType::tStruct));

		values->structValue->insert(StructElement("FAMILY", std::shared_ptr<Variable>(new Variable((uint32_t)_deviceType.family()))));
		values->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable((uint32_t)_peerID))));
		values->structValue->insert(StructElement("ADDRESS", std::shared_ptr<Variable>(new Variable(_serialNumber))));
		values->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(_rpcTypeString))));
		values->structValue->insert(StructElement("TYPE_ID", std::shared_ptr<Variable>(new Variable(_deviceType.type()))));
		values->structValue->insert(StructElement("NAME", std::shared_ptr<Variable>(new Variable(_name))));
		std::shared_ptr<Variable> channels(new Variable(VariableType::tArray));
		for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
		{
			if(!i->second) continue;
			if(!i->second->countFromVariable.empty() && configCentral[0].find(i->second->countFromVariable) != configCentral[0].end() && configCentral[0][i->second->countFromVariable].data.size() > 0 && i->first >= i->second->channel + configCentral[0][i->second->countFromVariable].data.at(configCentral[0][i->second->countFromVariable].data.size() - 1)) continue;
			std::shared_ptr<Variable> channel(new Variable(VariableType::tStruct));
			channel->structValue->insert(StructElement("INDEX", std::shared_ptr<Variable>(new Variable(i->first))));
			channel->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(i->second->type))));

			std::shared_ptr<Variable> parameters(new Variable(VariableType::tStruct));
			channel->structValue->insert(StructElement("PARAMSET", parameters));
			channels->arrayValue->push_back(channel);

			PParameterGroup parameterGroup = getParameterSet(i->first, ParameterGroup::Type::variables);
			if(!parameterGroup) continue;

			for(Parameters::iterator j = parameterGroup->parameters.begin(); j != parameterGroup->parameters.end(); ++j)
			{
				if(!j->second || j->second->id.empty() || !j->second->visible) continue;
				if(!j->second->visible && !j->second->service && !j->second->internal  && !j->second->transform)
				{
					_bl->out.printDebug("Debug: Omitting parameter " + j->second->id + " because of it's ui flag.");
					continue;
				}
				if(!j->second->readable && !returnWriteOnly) continue;
				if(valuesCentral.find(i->first) == valuesCentral.end()) continue;
				if(valuesCentral[i->first].find(j->second->id) == valuesCentral[i->first].end()) continue;

				std::shared_ptr<Variable> element(new Variable(VariableType::tStruct));
				std::shared_ptr<Variable> value;
				if(j->second->readable)
				{
					value = (j->second->convertFromPacket(valuesCentral[i->first][j->second->id].data));
					if(!value) continue;
					element->structValue->insert(StructElement("VALUE", value));
				}

				if(returnWriteOnly) element->structValue->insert(StructElement("READABLE", std::shared_ptr<Variable>(new Variable(j->second->readable))));
				element->structValue->insert(StructElement("WRITEABLE", std::shared_ptr<Variable>(new Variable(j->second->writeable))));
				if(j->second->logical->type == ILogical::Type::tBoolean)
				{
					element->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("BOOL")))));
				}
				else if(j->second->logical->type == ILogical::Type::tString)
				{
					element->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("STRING")))));
				}
				else if(j->second->logical->type == ILogical::Type::tAction)
				{
					element->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("ACTION")))));
				}
				else if(j->second->logical->type == ILogical::Type::tInteger)
				{
					LogicalInteger* parameter = (LogicalInteger*)j->second->logical.get();
					element->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("INTEGER")))));
					element->structValue->insert(StructElement("MIN", std::shared_ptr<Variable>(new Variable(parameter->minimumValue))));
					element->structValue->insert(StructElement("MAX", std::shared_ptr<Variable>(new Variable(parameter->maximumValue))));

					if(!parameter->specialValuesStringMap.empty())
					{
						std::shared_ptr<Variable> specialValues(new Variable(VariableType::tArray));
						for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValuesStringMap.begin(); j != parameter->specialValuesStringMap.end(); ++j)
						{
							std::shared_ptr<Variable> specialElement(new Variable(VariableType::tStruct));
							specialElement->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(j->first))));
							specialElement->structValue->insert(StructElement("VALUE", std::shared_ptr<Variable>(new Variable(j->second))));
							specialValues->arrayValue->push_back(specialElement);
						}
						element->structValue->insert(StructElement("SPECIAL", specialValues));
					}
				}
				else if(j->second->logical->type == ILogical::Type::tEnum)
				{
					LogicalEnumeration* parameter = (LogicalEnumeration*)j->second->logical.get();
					element->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("ENUM")))));
					element->structValue->insert(StructElement("MIN", std::shared_ptr<Variable>(new Variable(parameter->minimumValue))));
					element->structValue->insert(StructElement("MAX", std::shared_ptr<Variable>(new Variable(parameter->maximumValue))));

					std::shared_ptr<Variable> valueList(new Variable(VariableType::tArray));
					for(std::vector<EnumerationValue>::iterator j = parameter->values.begin(); j != parameter->values.end(); ++j)
					{
						valueList->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(j->id)));
					}
					element->structValue->insert(StructElement("VALUE_LIST", valueList));
				}
				else if(j->second->logical->type == ILogical::Type::tFloat)
				{
					LogicalDecimal* parameter = (LogicalDecimal*)j->second->logical.get();
					element->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("FLOAT")))));
					element->structValue->insert(StructElement("MIN", std::shared_ptr<Variable>(new Variable(parameter->minimumValue))));
					element->structValue->insert(StructElement("MAX", std::shared_ptr<Variable>(new Variable(parameter->maximumValue))));

					if(!parameter->specialValuesStringMap.empty())
					{
						std::shared_ptr<Variable> specialValues(new Variable(VariableType::tArray));
						for(std::unordered_map<std::string, double>::iterator j = parameter->specialValuesStringMap.begin(); j != parameter->specialValuesStringMap.end(); ++j)
						{
							std::shared_ptr<Variable> specialElement(new Variable(VariableType::tStruct));
							specialElement->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(j->first))));
							specialElement->structValue->insert(StructElement("VALUE", std::shared_ptr<Variable>(new Variable(j->second))));
							specialValues->arrayValue->push_back(specialElement);
						}
						element->structValue->insert(StructElement("SPECIAL", specialValues));
					}
				}
				parameters->structValue->insert(StructElement(j->second->id, element));
			}
		}
		values->structValue->insert(StructElement("CHANNELS", channels));

		return values;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::getDeviceDescription(int32_t clientID, int32_t channel, std::map<std::string, bool> fields)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<Variable> description(new Variable(VariableType::tStruct));

		if(channel == -1) //Base device
		{
			if(fields.empty() || fields.find("FAMILY") != fields.end()) description->structValue->insert(StructElement("FAMILY", std::shared_ptr<Variable>(new Variable((uint32_t)_deviceType.family()))));
			if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable((uint32_t)_peerID))));
			if(fields.empty() || fields.find("ADDRESS") != fields.end()) description->structValue->insert(StructElement("ADDRESS", std::shared_ptr<Variable>(new Variable(_serialNumber))));

			std::shared_ptr<Variable> variable = std::shared_ptr<Variable>(new Variable(VariableType::tArray));
			std::shared_ptr<Variable> variable2 = std::shared_ptr<Variable>(new Variable(VariableType::tArray));
			if(fields.empty() || fields.find("CHILDREN") != fields.end()) description->structValue->insert(StructElement("CHILDREN", variable));
			if(fields.empty() || fields.find("CHANNELS") != fields.end()) description->structValue->insert(StructElement("CHANNELS", variable2));

			if(fields.empty() || fields.find("CHILDREN") != fields.end() || fields.find("CHANNELS") != fields.end())
			{
				for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
				{
					if(!i->second->visible) continue;
					if(!i->second->countFromVariable.empty() && configCentral[0].find(i->second->countFromVariable) != configCentral[0].end() && configCentral[0][i->second->countFromVariable].data.size() > 0 && i->first >= i->second->channel + configCentral[0][i->second->countFromVariable].data.at(configCentral[0][i->second->countFromVariable].data.size() - 1)) continue;
					if(fields.empty() || fields.find("CHILDREN") != fields.end()) variable->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(_serialNumber + ":" + std::to_string(i->first))));
					if(fields.empty() || fields.find("CHANNELS") != fields.end()) variable2->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(i->first)));
				}
			}

			if(fields.empty() || fields.find("FIRMWARE") != fields.end())
			{
				if(_firmwareVersion != 0) description->structValue->insert(StructElement("FIRMWARE", std::shared_ptr<Variable>(new Variable(getFirmwareVersionString(_firmwareVersion)))));
				else if(!_firmwareVersionString.empty()) description->structValue->insert(StructElement("FIRMWARE", std::shared_ptr<Variable>(new Variable(_firmwareVersionString))));
				else description->structValue->insert(StructElement("FIRMWARE", std::shared_ptr<Variable>(new Variable(std::string("?")))));
			}

			if(fields.empty() || fields.find("AVAILABLE_FIRMWARE") != fields.end())
			{
				int32_t newFirmwareVersion = getNewFirmwareVersion();
				if(newFirmwareVersion > _firmwareVersion) description->structValue->insert(StructElement("AVAILABLE_FIRMWARE", std::shared_ptr<Variable>(new Variable(getFirmwareVersionString(newFirmwareVersion)))));
			}

			if(fields.empty() || fields.find("FLAGS") != fields.end())
			{
				int32_t uiFlags = 0;
				if(_rpcDevice->visible) uiFlags += 1;
				if(_rpcDevice->internal) uiFlags += 2;
				if(!_rpcDevice->deletable || isTeam()) uiFlags += 8;
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
			}

			if(fields.empty() || fields.find("INTERFACE") != fields.end()) description->structValue->insert(StructElement("INTERFACE", std::shared_ptr<Variable>(new Variable(getCentral()->logicalDevice()->getSerialNumber()))));

			if(fields.empty() || fields.find("PARAMSETS") != fields.end())
			{
				variable = std::shared_ptr<Variable>(new Variable(VariableType::tArray));
				description->structValue->insert(StructElement("PARAMSETS", variable));
				variable->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(std::string("MASTER")))); //Always MASTER
			}

			if(fields.empty() || fields.find("PARENT") != fields.end()) description->structValue->insert(StructElement("PARENT", std::shared_ptr<Variable>(new Variable(std::string("")))));

			if(!_ip.empty() && (fields.empty() || fields.find("IP_ADDRESS") != fields.end())) description->structValue->insert(StructElement("IP_ADDRESS", std::shared_ptr<Variable>(new Variable(_ip))));

			if(fields.empty() || fields.find("PHYSICAL_ADDRESS") != fields.end()) description->structValue->insert(StructElement("PHYSICAL_ADDRESS", std::shared_ptr<Variable>(new Variable(_address))));

			//Compatibility
			if(fields.empty() || fields.find("RF_ADDRESS") != fields.end()) description->structValue->insert(StructElement("RF_ADDRESS", std::shared_ptr<Variable>(new Variable(_address))));
			//Compatibility
			if(fields.empty() || fields.find("ROAMING") != fields.end()) description->structValue->insert(StructElement("ROAMING", std::shared_ptr<Variable>(new Variable((int32_t)0))));

			if(fields.empty() || fields.find("RX_MODE") != fields.end()) description->structValue->insert(StructElement("RX_MODE", std::shared_ptr<Variable>(new Variable((int32_t)_rpcDevice->receiveModes))));

			if(!_rpcTypeString.empty() && (fields.empty() || fields.find("TYPE") != fields.end())) description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(_rpcTypeString))));

			if(fields.empty() || fields.find("TYPE_ID") != fields.end()) description->structValue->insert(StructElement("TYPE_ID", std::shared_ptr<Variable>(new Variable(_deviceType.type()))));

			if(fields.empty() || fields.find("VERSION") != fields.end()) description->structValue->insert(StructElement("VERSION", std::shared_ptr<Variable>(new Variable(_rpcDevice->version))));

			if(fields.find("WIRELESS") != fields.end()) description->structValue->insert(StructElement("WIRELESS", std::shared_ptr<Variable>(new Variable(wireless()))));
		}
		else
		{
			if(_rpcDevice->functions.find(channel) == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
			PFunction rpcFunction = _rpcDevice->functions.at(channel);
			if(!rpcFunction->countFromVariable.empty() && configCentral[0].find(rpcFunction->countFromVariable) != configCentral[0].end() && configCentral[0][rpcFunction->countFromVariable].data.size() > 0 && channel >= (int32_t)rpcFunction->channel + configCentral[0][rpcFunction->countFromVariable].data.at(configCentral[0][rpcFunction->countFromVariable].data.size() - 1)) return Variable::createError(-2, "Channel index larger than defined.");
			if(!rpcFunction->visible) return description;

			if(fields.empty() || fields.find("FAMILYID") != fields.end()) description->structValue->insert(StructElement("FAMILY", std::shared_ptr<Variable>(new Variable((uint32_t)_deviceType.family()))));
			if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable((uint32_t)_peerID))));
			if(fields.empty() || fields.find("CHANNEL") != fields.end()) description->structValue->insert(StructElement("CHANNEL", std::shared_ptr<Variable>(new Variable(channel))));
			if(fields.empty() || fields.find("ADDRESS") != fields.end()) description->structValue->insert(StructElement("ADDRESS", std::shared_ptr<Variable>(new Variable(_serialNumber + ":" + std::to_string(channel)))));

			if(fields.empty() || fields.find("AES_ACTIVE") != fields.end())
			{
				int32_t aesActive = 0;
				if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && !configCentral.at(channel).at("AES_ACTIVE").data.empty() && configCentral.at(channel).at("AES_ACTIVE").data.at(0) != 0)
				{
					aesActive = 1;
				}
				//Integer for compatability
				description->structValue->insert(StructElement("AES_ACTIVE", std::shared_ptr<Variable>(new Variable(aesActive))));
			}

			if(fields.empty() || fields.find("DIRECTION") != fields.end() || fields.find("LINK_SOURCE_ROLES") != fields.end() || fields.find("LINK_TARGET_ROLES") != fields.end())
			{
				int32_t direction = 0;
				std::ostringstream linkSourceRoles;
				std::ostringstream linkTargetRoles;
				for(LinkFunctionTypes::iterator k = rpcFunction->linkSenderFunctionTypes.begin(); k != rpcFunction->linkSenderFunctionTypes.end(); ++k)
				{
					//Probably only one direction is supported, but just in case I use the "or"
					if(!k->empty())
					{
						if(direction & 1) linkSourceRoles << " ";
						linkSourceRoles << *k;
						direction |= 1;
					}
				}
				for(LinkFunctionTypes::iterator k = rpcFunction->linkReceiverFunctionTypes.begin(); k != rpcFunction->linkReceiverFunctionTypes.end(); ++k)
				{
					//Probably only one direction is supported, but just in case I use the "or"
					if(!k->empty())
					{
						if(direction & 2) linkTargetRoles << " ";
						linkTargetRoles << *k;
						direction |= 2;
					}
				}

				//Overwrite direction when manually set
				if(rpcFunction->direction != Function::Direction::Enum::none) direction = (int32_t)rpcFunction->direction;
				if(fields.empty() || fields.find("DIRECTION") != fields.end()) description->structValue->insert(StructElement("DIRECTION", std::shared_ptr<Variable>(new Variable(direction))));
				if(fields.empty() || fields.find("LINK_SOURCE_ROLES") != fields.end()) description->structValue->insert(StructElement("LINK_SOURCE_ROLES", std::shared_ptr<Variable>(new Variable(linkSourceRoles.str()))));
				if(fields.empty() || fields.find("LINK_TARGET_ROLES") != fields.end()) description->structValue->insert(StructElement("LINK_TARGET_ROLES", std::shared_ptr<Variable>(new Variable(linkTargetRoles.str()))));
			}

			if(fields.empty() || fields.find("FLAGS") != fields.end())
			{
				int32_t uiFlags = 0;
				if(rpcFunction->visible) uiFlags += 1;
				if(rpcFunction->internal) uiFlags += 2;
				if(rpcFunction->deletable || isTeam()) uiFlags += 8;
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
			}

			if(fields.empty() || fields.find("GROUP") != fields.end())
			{
				int32_t groupedWith = getChannelGroupedWith(channel);
				if(groupedWith > -1)
				{
					description->structValue->insert(StructElement("GROUP", std::shared_ptr<Variable>(new Variable(_serialNumber + ":" + std::to_string(groupedWith)))));
				}
			}

			if(fields.empty() || fields.find("INDEX") != fields.end()) description->structValue->insert(StructElement("INDEX", std::shared_ptr<Variable>(new Variable(channel))));

			if(fields.empty() || fields.find("PARAMSETS") != fields.end())
			{
				std::shared_ptr<Variable> variable = std::shared_ptr<Variable>(new Variable(VariableType::tArray));
				description->structValue->insert(StructElement("PARAMSETS", variable));
				if(!rpcFunction->configParameters->parameters.empty()) variable->arrayValue->push_back(PVariable(new Variable(std::string("MASTER"))));
				if(!rpcFunction->variables->parameters.empty()) variable->arrayValue->push_back(PVariable(new Variable(std::string("VALUES"))));
				if(!rpcFunction->linkParameters->parameters.empty()) variable->arrayValue->push_back(PVariable(new Variable(std::string("LINK"))));
			}
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::master) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::master)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::values) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::values)->typeString())));

			if(fields.empty() || fields.find("PARENT") != fields.end()) description->structValue->insert(StructElement("PARENT", std::shared_ptr<Variable>(new Variable(_serialNumber))));

			if(!_rpcTypeString.empty() && (fields.empty() || fields.find("PARENT_TYPE") != fields.end())) description->structValue->insert(StructElement("PARENT_TYPE", std::shared_ptr<Variable>(new Variable(_rpcTypeString))));

			if(fields.empty() || fields.find("TYPE") != fields.end()) description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(rpcFunction->type))));

			if(fields.empty() || fields.find("VERSION") != fields.end()) description->structValue->insert(StructElement("VERSION", std::shared_ptr<Variable>(new Variable(_rpcDevice->version))));
		}
		return description;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<std::vector<std::shared_ptr<Variable>>> Peer::getDeviceDescriptions(int32_t clientID, bool channels, std::map<std::string, bool> fields)
{
	try
	{
		std::shared_ptr<std::vector<std::shared_ptr<Variable>>> descriptions(new std::vector<std::shared_ptr<Variable>>());
		descriptions->push_back(getDeviceDescription(clientID, -1, fields));

		if(channels)
		{
			for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
			{
				if(!i->second->countFromVariable.empty() && configCentral[0].find(i->second->countFromVariable) != configCentral[0].end() && configCentral[0][i->second->countFromVariable].data.size() > 0 && i->first >= i->second->channel + configCentral[0][i->second->countFromVariable].data.at(configCentral[0][i->second->countFromVariable].data.size() - 1)) continue;
				std::shared_ptr<Variable> description = getDeviceDescription(clientID, (int32_t)i->first, fields);
				if(!description->structValue->empty()) descriptions->push_back(description);
			}
		}

		return descriptions;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<std::vector<std::shared_ptr<Variable>>>();
}

std::shared_ptr<Variable> Peer::getDeviceInfo(int32_t clientID, std::map<std::string, bool> fields)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<Variable> info(new Variable(VariableType::tStruct));

		info->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable((int32_t)_peerID))));

		if(fields.empty() || fields.find("NAME") != fields.end()) info->structValue->insert(StructElement("NAME", std::shared_ptr<Variable>(new Variable(_name))));

		if(wireless())
		{
			if(fields.empty() || fields.find("RSSI") != fields.end())
			{
				if(valuesCentral.find(0) != valuesCentral.end() && valuesCentral.at(0).find("RSSI_DEVICE") != valuesCentral.at(0).end() && valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter)
				{
					info->structValue->insert(StructElement("RSSI", valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter->convertFromPacket(valuesCentral.at(0).at("RSSI_DEVICE").data)));
				}
			}
		}

		return info;
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<Variable>();
}

std::shared_ptr<Variable> Peer::getLink(int32_t clientID, int32_t channel, int32_t flags, bool avoidDuplicates)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return Variable::createError(-2, "Not a central peer.");
		std::shared_ptr<Variable> array(new Variable(VariableType::tArray));
		std::shared_ptr<Variable> element;
		bool groupFlag = false;
		if(flags & 0x01) groupFlag = true;
		if(channel > -1 && !groupFlag) //Get link of single channel
		{
			if(_rpcDevice->functions.find(channel) == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			bool isSender = false;
			//Return if there are no link roles defined
			PFunction rpcFunction = _rpcDevice->functions.at(channel);
			if(!rpcFunction->linkSenderFunctionTypes.empty()) isSender = true;
			else if(rpcFunction->linkReceiverFunctionTypes.empty()) return array;
			std::shared_ptr<Central> central = getCentral();
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<Peer> remotePeer(central->logicalDevice()->getPeer((*i)->address));
				if(!remotePeer)
				{
					_bl->out.printDebug("Debug: Can't return link description for peer with address 0x" + BaseLib::HelperFunctions::getHexString((*i)->address, 6) + ". The peer is not paired to Homegear.");
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
						peerSerialNumber = remotePeer->getSerialNumber();
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
				if(peerID == _peerID && _rpcDevice->functions.find((*i)->channel) == _rpcDevice->functions.end())
				{
					if(isSender) brokenFlags = 2 | 4; //LINK_FLAG_RECEIVER_BROKEN | PEER_IS_ME
					else brokenFlags = 1 | 4; //LINK_FLAG_SENDER_BROKEN | PEER_IS_ME
				}
				if(brokenFlags == 0 && remotePeer && remotePeer->serviceMessages->getUnreach()) brokenFlags = 2;
				if(serviceMessages->getUnreach()) brokenFlags |= 1;
				element.reset(new Variable(VariableType::tStruct));
				element->structValue->insert(StructElement("DESCRIPTION", std::shared_ptr<Variable>(new Variable((*i)->linkDescription))));
				element->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(brokenFlags))));
				element->structValue->insert(StructElement("NAME", std::shared_ptr<Variable>(new Variable((*i)->linkName))));
				if(isSender)
				{
					element->structValue->insert(StructElement("RECEIVER", std::shared_ptr<Variable>(new Variable(peerSerialNumber + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(StructElement("RECEIVER_ID", std::shared_ptr<Variable>(new Variable((int32_t)remotePeer->getID()))));
					element->structValue->insert(StructElement("RECEIVER_CHANNEL", std::shared_ptr<Variable>(new Variable((*i)->channel))));
					if(flags & 4)
					{
						std::shared_ptr<Variable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = remotePeer->getParamset(clientID, (*i)->channel, ParameterGroup::Type::Enum::link, _peerID, channel);
						else paramset.reset(new Variable(VariableType::tStruct));
						if(paramset->errorStruct) paramset.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<Variable> description;
						if(!(brokenFlags & 2) && remotePeer) description = remotePeer->getDeviceDescription(clientID, (*i)->channel, std::map<std::string, bool>());
						else description.reset(new Variable(VariableType::tStruct));
						if(description->errorStruct) description.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(StructElement("SENDER", std::shared_ptr<Variable>(new Variable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(StructElement("SENDER_ID", std::shared_ptr<Variable>(new Variable((int32_t)_peerID))));
					element->structValue->insert(StructElement("SENDER_CHANNEL", std::shared_ptr<Variable>(new Variable(channel))));
					if(flags & 2)
					{
						std::shared_ptr<Variable> paramset;
						if(!(brokenFlags & 1)) paramset = getParamset(clientID, channel, ParameterGroup::Type::Enum::link, peerID, (*i)->channel);
						else paramset.reset(new Variable(VariableType::tStruct));
						if(paramset->errorStruct) paramset.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<Variable> description;
						if(!(brokenFlags & 1)) description = getDeviceDescription(clientID, channel, std::map<std::string, bool>());
						else description.reset(new Variable(VariableType::tStruct));
						if(description->errorStruct) description.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("SENDER_DESCRIPTION", description));
					}
				}
				else //When sender is broken
				{
					element->structValue->insert(StructElement("RECEIVER", std::shared_ptr<Variable>(new Variable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(StructElement("RECEIVER_ID", std::shared_ptr<Variable>(new Variable((int32_t)_peerID))));
					element->structValue->insert(StructElement("RECEIVER_CHANNEL", std::shared_ptr<Variable>(new Variable(channel))));
					if(flags & 4)
					{
						std::shared_ptr<Variable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = getParamset(clientID, channel, ParameterGroup::Type::Enum::link, peerID, (*i)->channel);
						else paramset.reset(new Variable(VariableType::tStruct));
						if(paramset->errorStruct) paramset.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<Variable> description;
						if(!(brokenFlags & 2)) description = getDeviceDescription(clientID, channel, std::map<std::string, bool>());
						else description.reset(new Variable(VariableType::tStruct));
						if(description->errorStruct) description.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(StructElement("SENDER", std::shared_ptr<Variable>(new Variable(peerSerialNumber + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(StructElement("SENDER_ID", std::shared_ptr<Variable>(new Variable((int32_t)remotePeer->getID()))));
					element->structValue->insert(StructElement("SENDER_CHANNEL", std::shared_ptr<Variable>(new Variable((*i)->channel))));
					if(flags & 2)
					{
						std::shared_ptr<Variable> paramset;
						if(!(brokenFlags & 1) && remotePeer) paramset = remotePeer->getParamset(clientID, (*i)->channel, ParameterGroup::Type::Enum::link, _peerID, channel);
						else paramset.reset(new Variable(VariableType::tStruct));
						if(paramset->errorStruct) paramset.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<Variable> description;
						if(!(brokenFlags & 1) && remotePeer) description = remotePeer->getDeviceDescription(clientID, (*i)->channel, std::map<std::string, bool>());
						else description.reset(new Variable(VariableType::tStruct));
						if(description->errorStruct) description.reset(new Variable(VariableType::tStruct));
						element->structValue->insert(StructElement("SENDER_DESCRIPTION", description));
					}
				}
				array->arrayValue->push_back(element);
			}
		}
		else
		{
			if(channel > -1 && groupFlag) //Get links for each grouped channel
			{
				if(_rpcDevice->functions.find(channel) == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
				PFunction rpcFunction = _rpcDevice->functions.at(channel);
				if(rpcFunction->grouped)
				{
					element = getLink(clientID, channel, flags & 0xFFFFFFFE, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());

					int32_t groupedWith = getChannelGroupedWith(channel);
					if(groupedWith > -1)
					{
						element = getLink(clientID, groupedWith, flags & 0xFFFFFFFE, avoidDuplicates);
						array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
					}
				}
				else
				{
					element = getLink(clientID, channel, flags & 0xFFFFFFFE, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
			else //Get links for all channels
			{
				for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
				{
					element.reset(new Variable(VariableType::tArray));
					element = getLink(clientID, i->first, flags, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::getLinkInfo(int32_t clientID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(_peers.find(senderChannel) == _peers.end()) return Variable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverID, receiverChannel);
		if(!remotePeer) return Variable::createError(-2, "Peer not found.");
		std::shared_ptr<Variable> response(new Variable(VariableType::tStruct));
		response->structValue->insert(StructElement("DESCRIPTION", std::shared_ptr<Variable>(new Variable(remotePeer->linkDescription))));
		response->structValue->insert(StructElement("NAME", std::shared_ptr<Variable>(new Variable(remotePeer->linkName))));
		return response;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::getLinkPeers(int32_t clientID, int32_t channel, bool returnID)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<Variable> array(new Variable(VariableType::tArray));
		if(channel > -1)
		{
			if(_rpcDevice->functions.find(channel) == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
			//Return if no peers are paired to the channel
			if(_peers.find(channel) == _peers.end() || _peers.at(channel).empty()) return array;
			PFunction rpcFunction = _rpcDevice->functions.at(channel);
			//Return if there are no link roles defined
			if(rpcFunction->linkSenderFunctionTypes.empty() || rpcFunction->linkReceiverFunctionTypes.empty()) return array;
			std::shared_ptr<Central> central(getCentral());
			if(!central) return array; //central actually should always be set at this point
			for(std::vector<std::shared_ptr<BaseLib::Systems::BasicPeer>>::iterator i = _peers.at(channel).begin(); i != _peers.at(channel).end(); ++i)
			{
				if((*i)->hidden) continue;
				std::shared_ptr<Peer> peer(central->logicalDevice()->getPeer((*i)->address));
				if(returnID && !peer) continue;
				bool peerKnowsMe = false;
				if(peer && peer->getPeer(channel, _peerID)) peerKnowsMe = true;

				std::string peerSerial = (*i)->serialNumber;
				if((*i)->serialNumber.empty() || (*i)->id == 0)
				{
					if(peerKnowsMe || (*i)->address == _address)
					{
						(*i)->serialNumber = peer->getSerialNumber();
						(*i)->id = peer->getID();
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
					std::shared_ptr<Variable> address(new Variable(VariableType::tArray));
					array->arrayValue->push_back(address);
					address->arrayValue->push_back(std::shared_ptr<Variable>(new Variable((int32_t)peer->getID())));
					address->arrayValue->push_back(std::shared_ptr<Variable>(new Variable((*i)->channel)));
				}
				else array->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(peerSerial + ":" + std::to_string((*i)->channel))));
			}
		}
		else
		{
			for(Functions::iterator i = _rpcDevice->functions.begin(); i != _rpcDevice->functions.end(); ++i)
			{
				std::shared_ptr<Variable> linkPeers = getLinkPeers(clientID, i->first, returnID);
				array->arrayValue->insert(array->arrayValue->end(), linkPeers->arrayValue->begin(), linkPeers->arrayValue->end());
			}
		}
		return array;
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::getParamsetDescription(int32_t clientID, PParameterGroup parameterGroup)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");

		std::shared_ptr<Variable> descriptions(new Variable(VariableType::tStruct));
		std::shared_ptr<Variable> description(new Variable(VariableType::tStruct));
		uint32_t index = 0;
		for(Parameters::iterator i = parameterGroup->parameters.begin(); i != parameterGroup->parameters.end(); ++i)
		{
			if(!i->second || i->second->id.empty() || !i->second->visible) continue;
			if(!i->second->visible && !i->second->service && !i->second->internal  && !i->second->transform)
			{
				_bl->out.printDebug("Debug: Omitting parameter " + i->second->id + " because of it's ui flag.");
				continue;
			}
			int32_t operations = 0;
			if(i->second->readable) operations += 5;
			if(i->second->writeable) operations += 2;
			int32_t uiFlags = 0;
			if(i->second->visible) uiFlags += 1;
			if(i->second->internal) uiFlags += 2;
			if(i->second->transform) uiFlags += 4;
			if(i->second->service) uiFlags += 8;
			if(i->second->sticky) uiFlags += 0x10;
			if(i->second->logical->type == ILogical::Type::tBoolean)
			{
				LogicalBoolean* parameter = (LogicalBoolean*)i->second->logical.get();

				if(!i->second->control.empty()) description->structValue->insert(StructElement("CONTROL", std::shared_ptr<Variable>(new Variable(i->second->control))));
				if(parameter->defaultValueExists) description->structValue->insert(StructElement("DEFAULT", std::shared_ptr<Variable>(new Variable(parameter->defaultValue))));
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
				description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(i->second->id))));
				description->structValue->insert(StructElement("OPERATIONS", std::shared_ptr<Variable>(new Variable(operations))));
				description->structValue->insert(StructElement("TAB_ORDER", std::shared_ptr<Variable>(new Variable(index))));
				description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("BOOL")))));
				description->structValue->insert(StructElement("UNIT", std::shared_ptr<Variable>(new Variable(i->second->unit))));
			}
			else if(i->second->logical->type == ILogical::Type::tString)
			{
				LogicalString* parameter = (LogicalString*)i->second->logical.get();

				if(!i->second->control.empty()) description->structValue->insert(StructElement("CONTROL", std::shared_ptr<Variable>(new Variable(i->second->control))));
				if(parameter->defaultValueExists) description->structValue->insert(StructElement("DEFAULT", std::shared_ptr<Variable>(new Variable(parameter->defaultValue))));
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
				description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(i->second->id))));
				description->structValue->insert(StructElement("OPERATIONS", std::shared_ptr<Variable>(new Variable(operations))));
				description->structValue->insert(StructElement("TAB_ORDER", std::shared_ptr<Variable>(new Variable(index))));
				description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("STRING")))));
				description->structValue->insert(StructElement("UNIT", std::shared_ptr<Variable>(new Variable(i->second->unit))));
			}
			else if(i->second->logical->type == ILogical::Type::tAction)
			{
				if(!i->second->control.empty()) description->structValue->insert(StructElement("CONTROL", std::shared_ptr<Variable>(new Variable(i->second->control))));
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
				description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(i->second->id))));
				description->structValue->insert(StructElement("OPERATIONS", std::shared_ptr<Variable>(new Variable(operations))));
				description->structValue->insert(StructElement("TAB_ORDER", std::shared_ptr<Variable>(new Variable(index))));
				description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("ACTION")))));
				description->structValue->insert(StructElement("UNIT", std::shared_ptr<Variable>(new Variable(i->second->unit))));
			}
			else if(i->second->logical->type == ILogical::Type::tInteger)
			{
				LogicalInteger* parameter = (LogicalInteger*)i->second->logical.get();

				if(!i->second->control.empty()) description->structValue->insert(StructElement("CONTROL", std::shared_ptr<Variable>(new Variable(i->second->control))));
				if(parameter->defaultValueExists) description->structValue->insert(StructElement("DEFAULT", std::shared_ptr<Variable>(new Variable(parameter->defaultValue))));
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
				description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(i->second->id))));
				description->structValue->insert(StructElement("MAX", std::shared_ptr<Variable>(new Variable(parameter->maximumValue))));
				description->structValue->insert(StructElement("MIN", std::shared_ptr<Variable>(new Variable(parameter->minimumValue))));
				description->structValue->insert(StructElement("OPERATIONS", std::shared_ptr<Variable>(new Variable(operations))));

				if(!parameter->specialValuesStringMap.empty())
				{
					std::shared_ptr<Variable> specialValues(new Variable(VariableType::tArray));
					for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValuesStringMap.begin(); j != parameter->specialValuesStringMap.end(); ++j)
					{
						std::shared_ptr<Variable> specialElement(new Variable(VariableType::tStruct));
						specialElement->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(j->first))));
						specialElement->structValue->insert(StructElement("VALUE", std::shared_ptr<Variable>(new Variable(j->second))));
						specialValues->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(StructElement("SPECIAL", specialValues));
				}

				description->structValue->insert(StructElement("TAB_ORDER", std::shared_ptr<Variable>(new Variable(index))));
				description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("INTEGER")))));
				description->structValue->insert(StructElement("UNIT", std::shared_ptr<Variable>(new Variable(i->second->unit))));
			}
			else if(i->second->logical->type == ILogical::Type::tEnum)
			{
				LogicalEnumeration* parameter = (LogicalEnumeration*)i->second->logical.get();

				if(!i->second->control.empty()) description->structValue->insert(StructElement("CONTROL", std::shared_ptr<Variable>(new Variable(i->second->control))));
				description->structValue->insert(StructElement("DEFAULT", std::shared_ptr<Variable>(new Variable(parameter->defaultValueExists ? parameter->defaultValue : 0))));
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
				description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(i->second->id))));
				description->structValue->insert(StructElement("MAX", std::shared_ptr<Variable>(new Variable(parameter->maximumValue))));
				description->structValue->insert(StructElement("MIN", std::shared_ptr<Variable>(new Variable(parameter->minimumValue))));
				description->structValue->insert(StructElement("OPERATIONS", std::shared_ptr<Variable>(new Variable(operations))));
				description->structValue->insert(StructElement("TAB_ORDER", std::shared_ptr<Variable>(new Variable(index))));
				description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("ENUM")))));
				description->structValue->insert(StructElement("UNIT", std::shared_ptr<Variable>(new Variable(i->second->unit))));

				std::shared_ptr<Variable> valueList(new Variable(VariableType::tArray));
				for(std::vector<EnumerationValue>::iterator j = parameter->values.begin(); j != parameter->values.end(); ++j)
				{
					valueList->arrayValue->push_back(std::shared_ptr<Variable>(new Variable(j->id)));
				}
				description->structValue->insert(StructElement("VALUE_LIST", valueList));
			}
			else if(i->second->logical->type == ILogical::Type::tFloat)
			{
				LogicalDecimal* parameter = (LogicalDecimal*)i->second->logical.get();

				if(!i->second->control.empty()) description->structValue->insert(StructElement("CONTROL", std::shared_ptr<Variable>(new Variable(i->second->control))));
				if(parameter->defaultValueExists) description->structValue->insert(StructElement("DEFAULT", std::shared_ptr<Variable>(new Variable(parameter->defaultValue))));
				description->structValue->insert(StructElement("FLAGS", std::shared_ptr<Variable>(new Variable(uiFlags))));
				description->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(i->second->id))));
				description->structValue->insert(StructElement("MAX", std::shared_ptr<Variable>(new Variable(parameter->maximumValue))));
				description->structValue->insert(StructElement("MIN", std::shared_ptr<Variable>(new Variable(parameter->minimumValue))));
				description->structValue->insert(StructElement("OPERATIONS", std::shared_ptr<Variable>(new Variable(operations))));

				if(!parameter->specialValuesStringMap.empty())
				{
					std::shared_ptr<Variable> specialValues(new Variable(VariableType::tArray));
					for(std::unordered_map<std::string, double>::iterator j = parameter->specialValuesStringMap.begin(); j != parameter->specialValuesStringMap.end(); ++j)
					{
						std::shared_ptr<Variable> specialElement(new Variable(VariableType::tStruct));
						specialElement->structValue->insert(StructElement("ID", std::shared_ptr<Variable>(new Variable(j->first))));
						specialElement->structValue->insert(StructElement("VALUE", std::shared_ptr<Variable>(new Variable(j->second))));
						specialValues->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(StructElement("SPECIAL", specialValues));
				}

				description->structValue->insert(StructElement("TAB_ORDER", std::shared_ptr<Variable>(new Variable(index))));
				description->structValue->insert(StructElement("TYPE", std::shared_ptr<Variable>(new Variable(std::string("FLOAT")))));
				description->structValue->insert(StructElement("UNIT", std::shared_ptr<Variable>(new Variable(i->second->unit))));
			}

			index++;
			descriptions->structValue->insert(StructElement(i->second->id, description));
			description.reset(new Variable(VariableType::tStruct));
		}
		return descriptions;
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::getParamsetId(int32_t clientID, uint32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(_rpcDevice->functions.find(channel) == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
		PFunction rpcFunction = _rpcDevice->functions.at(channel);
		std::shared_ptr<BasicPeer> remotePeer;
		if(type == ParameterGroup::Type::link && remoteID > 0)
		{
			remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return Variable::createError(-2, "Unknown remote peer.");
		}

		std::string id;
		if(type == ParameterGroup::Type::Enum::config) id = rpcFunction->configParameters->id;
		else if(type == ParameterGroup::Type::Enum::variables) id = rpcFunction->variables->id;
		else if(type == ParameterGroup::Type::Enum::link) id = rpcFunction->linkParameters->id;
		return std::shared_ptr<Variable>(new Variable(id));
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::getServiceMessages(int32_t clientID, bool returnID)
{
	if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
	if(!serviceMessages) return Variable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get(clientID, returnID);
}

std::shared_ptr<Variable> Peer::getValue(int32_t clientID, uint32_t channel, std::string valueKey, bool requestFromDevice, bool asynchronous)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(!_rpcDevice) return Variable::createError(-32500, "Unknown application error.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return Variable::createError(-5, "Unknown parameter.");
		if(_rpcDevice->functions.find(channel) == _rpcDevice->functions.end()) return Variable::createError(-2, "Unknown channel.");
		PFunction rpcFunction = _rpcDevice->functions.at(channel);
		PParameterGroup parameterGroup = getParameterSet(channel, ParameterGroup::Type::Enum::variables);
		PParameter parameter = parameterGroup->parameters.at(valueKey);
		if(!parameter) return Variable::createError(-5, "Unknown parameter.");
		if(!parameter->readable) return Variable::createError(-6, "Parameter is not readable.");
		if(requestFromDevice)
		{
			std::shared_ptr<Variable> variable = getValueFromDevice(parameter, channel, asynchronous);
			if((!asynchronous && variable->type != VariableType::tVoid) || variable->errorStruct) return variable;
		}
		return parameter->convertFromPacket(valuesCentral[channel][valueKey].data);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::reportValueUsage(int32_t clientID)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		return std::shared_ptr<Variable>(new Variable(!serviceMessages->getConfigPending()));
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::rssiInfo(int32_t clientID)
{
	try
	{
		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(!wireless()) return Variable::createError(-100, "Peer is not a wireless peer.");
		if(valuesCentral.find(0) == valuesCentral.end() || valuesCentral.at(0).find("RSSI_DEVICE") == valuesCentral.at(0).end() || !valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter)
		{
			return Variable::createError(-101, "Peer has no rssi information.");
		}
		std::shared_ptr<Variable> response(new Variable(VariableType::tStruct));
		std::shared_ptr<Variable> rpcArray(new Variable(VariableType::tArray));

		std::shared_ptr<Variable> element;
		if(valuesCentral.at(0).find("RSSI_PEER") != valuesCentral.at(0).end() && valuesCentral.at(0).at("RSSI_PEER").rpcParameter)
		{
			element = valuesCentral.at(0).at("RSSI_PEER").rpcParameter->convertFromPacket(valuesCentral.at(0).at("RSSI_PEER").data);
			if(element->integerValue == 0) element->integerValue = 65536;
			rpcArray->arrayValue->push_back(element);
		}
		else
		{
			element = std::shared_ptr<Variable>(new Variable(65536));
			rpcArray->arrayValue->push_back(element);
		}

		element = valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter->convertFromPacket(valuesCentral.at(0).at("RSSI_DEVICE").data);
		if(element->integerValue == 0) element->integerValue = 65536;
		rpcArray->arrayValue->push_back(element);

		std::shared_ptr<Central> central = getCentral();
		if(!central) return Variable::createError(-32500, "Central is nullptr.");
		response->structValue->insert(StructElement(central->logicalDevice()->getSerialNumber(), rpcArray));
		return response;
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::setLinkInfo(int32_t clientID, int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(_peers.find(senderChannel) == _peers.end()) return Variable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverID, receiverChannel);
		if(!remotePeer)	return Variable::createError(-2, "Peer not found.");
		remotePeer->linkDescription = description;
		remotePeer->linkName = name;
		savePeers();
		return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));
	}
	catch(const std::exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<Variable> Peer::setId(int32_t clientID, uint64_t newPeerId)
{
	try
	{
		if(newPeerId == 0 || newPeerId >= 0x40000000) return Variable::createError(-100, "New peer ID is invalid.");
		if(!raiseSetPeerID(newPeerId)) return Variable::createError(-101, "New peer ID is already in use.");
		_peerID = newPeerId;
		if(serviceMessages) serviceMessages->setPeerID(newPeerId);
		return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

std::shared_ptr<Variable> Peer::setValue(int32_t clientID, uint32_t channel, std::string valueKey, std::shared_ptr<Variable> value)
{
	try
	{
		//Nothing to do, return to save ressources
		if(value->stringValue.size() < 3) return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));

		if(_disposing) return Variable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return Variable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return Variable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return Variable::createError(-5, "Unknown parameter.");
		RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		PParameter rpcParameter = parameter->rpcParameter;
		if(!rpcParameter) return Variable::createError(-5, "Unknown parameter.");
		//Perform operation on value
		if(value->stringValue.size() > 2 && value->stringValue.at(1) == '='
				&& (value->stringValue.at(0) == '+' || value->stringValue.at(0) == '-' || value->stringValue.at(0) == '*' || value->stringValue.at(0) == '/'))
		{
			if(rpcParameter->logical->type == ILogical::Type::Enum::tFloat)
			{
				std::shared_ptr<Variable> currentValue = rpcParameter->convertFromPacket(parameter->data);
				std::string numberPart = value->stringValue.substr(2);
				double factor = Math::getDouble(numberPart);
				if(factor == 0) return Variable::createError(-1, "Factor is \"0\" or no valid number.");
				if(value->stringValue.at(0) == '+') value->floatValue = currentValue->floatValue + factor;
				else if(value->stringValue.at(0) == '-') value->floatValue = currentValue->floatValue - factor;
				else if(value->stringValue.at(0) == '*') value->floatValue = currentValue->floatValue * factor;
				else if(value->stringValue.at(0) == '/') value->floatValue = currentValue->floatValue / factor;
				value->type = VariableType::tFloat;
				value->stringValue.clear();
			}
			else if(rpcParameter->logical->type == ILogical::Type::Enum::tInteger)
			{
				std::shared_ptr<Variable> currentValue = rpcParameter->convertFromPacket(parameter->data);
				std::string numberPart = value->stringValue.substr(2);
				int32_t factor = Math::getNumber(numberPart);
				if(factor == 0) return Variable::createError(-1, "Factor is \"0\" or no valid number.");
				if(value->stringValue.at(0) == '+') value->integerValue = currentValue->integerValue + factor;
				else if(value->stringValue.at(0) == '-') value->integerValue = currentValue->integerValue - factor;
				else if(value->stringValue.at(0) == '*') value->integerValue = currentValue->integerValue * factor;
				else if(value->stringValue.at(0) == '/') value->integerValue = currentValue->integerValue / factor;
				value->type = VariableType::tInteger;
				value->stringValue.clear();
			}
		}
		return std::shared_ptr<Variable>(new Variable(VariableType::tVoid));
	}
	catch(const std::exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Variable::createError(-32500, "Unknown application error. See error log for more details.");
}
//End RPC methods
}
}
