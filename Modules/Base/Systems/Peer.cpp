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
		rpcDevice.reset();
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
void Peer::raiseCreateSavepoint(std::string name)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onCreateSavepoint(name);
}

void Peer::raiseReleaseSavepoint(std::string name)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onReleaseSavepoint(name);
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

uint64_t Peer::raiseSavePeerParameter(Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IPeerEventSink*)_eventHandler)->onSavePeerParameter(_peerID, data);
}

uint64_t Peer::raiseSavePeerVariable(Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IPeerEventSink*)_eventHandler)->onSavePeerVariable(_peerID, data);
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

void Peer::raiseDeletePeerParameter(Database::DataRow data)
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

uint64_t Peer::raiseSaveServiceMessage(Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IPeerEventSink*)_eventHandler)->onSaveServiceMessage(_peerID, data);
}

void Peer::raiseDeleteServiceMessage(uint64_t databaseID)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onDeleteServiceMessage(databaseID);
}

void Peer::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void Peer::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onRPCUpdateDevice(id, channel, address, hint);
}

void Peer::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onEvent(peerID, channel, variables, values);
}
//End event handling

//ServiceMessages event handling
void Peer::onConfigPending(bool configPending)
{

}

void Peer::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
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
			_bl->out.printWarning("Warning: Could not set parameter " + name + " on channel " + std::to_string(channel) + " for peer " + std::to_string(_peerID) + ". Channel does not exist.");
			return;
		}
		if(valuesCentral.at(channel).find(name) == valuesCentral.at(channel).end())
		{
			_bl->out.printWarning("Warning: Could not set parameter " + name + " on channel " + std::to_string(channel) + " for peer " + std::to_string(_peerID) + ". Parameter does not exist.");
			return;
		}
		RPCConfigurationParameter* parameter = &valuesCentral.at(channel).at(name);
		parameter->data = data;
		saveParameter(parameter->databaseID, RPC::ParameterSet::Type::Enum::values, channel, name, parameter->data);
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

uint64_t Peer::onSaveServiceMessage(Database::DataRow data)
{
	return raiseSaveServiceMessage(data);
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
		if(!(getRXModes() & RPC::Device::RXModes::Enum::always) && !(getRXModes() & RPC::Device::RXModes::Enum::burst)) return;
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

RPC::Device::RXModes::Enum Peer::getRXModes()
{
	try
	{
		if(rpcDevice)
		{
			_rxModes = rpcDevice->rxModes;
			if(configCentral.find(0) != configCentral.end() && configCentral.at(0).find("BURST_RX") != configCentral.at(0).end())
			{
				RPCConfigurationParameter* parameter = &configCentral.at(0).at("BURST_RX");
				if(!parameter->rpcParameter) return _rxModes;
				if(parameter->rpcParameter->convertFromPacket(parameter->data)->booleanValue)
				{
					_rxModes = (RPC::Device::RXModes::Enum)(_rxModes | RPC::Device::RXModes::Enum::burst);
				}
				else
				{
					_rxModes = (RPC::Device::RXModes::Enum)(_rxModes & (~RPC::Device::RXModes::Enum::burst));
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
				std::shared_ptr<Peer> peer = getCentral()->logicalDevice()->getPeer(id);
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

void Peer::save(bool savePeer, bool variables, bool centralConfig)
{
	try
	{
		if(deleting || isTeam()) return;
		raiseCreateSavepoint("peer" + std::to_string(_parentID) + std::to_string(_address));
		if(savePeer)
		{
			_databaseMutex.lock();
			uint64_t result = raiseSavePeer();
			if(_peerID == 0 && result > 0) setID(result);
			_databaseMutex.unlock();
		}
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
    raiseReleaseSavepoint("peer" + std::to_string(_parentID) + std::to_string(_address));
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
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(0)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(address)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(0)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(0)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(std::string(""))));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(value)));
		uint64_t result = raiseSavePeerParameter(data);

		binaryConfig[address].databaseID = result;
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

void Peer::saveParameter(uint32_t parameterID, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t channel, std::string parameterName, std::vector<uint8_t>& value, int32_t remoteAddress, uint32_t remoteChannel)
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
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn((uint32_t)parameterSetType)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(channel)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(remoteAddress)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(remoteChannel)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(parameterName)));
		data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(value)));
		uint64_t result = raiseSavePeerParameter(data);

		if(parameterSetType == RPC::ParameterSet::Type::Enum::master) configCentral[channel][parameterName].databaseID = result;
		else if(parameterSetType == RPC::ParameterSet::Type::Enum::values) valuesCentral[channel][parameterName].databaseID = result;
		else if(parameterSetType == RPC::ParameterSet::Type::Enum::link) linksCentral[channel][remoteAddress][remoteChannel][parameterName].databaseID = result;
		else if(parameterSetType == RPC::ParameterSet::Type::Enum::none) binaryConfig[channel].databaseID = result;
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
			}
		}
		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			uint64_t result = raiseSavePeerVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			uint64_t result = raiseSavePeerVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			uint64_t result = raiseSavePeerVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
			uint64_t result = raiseSavePeerVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
				else saveParameter(0, RPC::ParameterSet::Type::Enum::master, i->first, j->first, j->second.data);
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
				else saveParameter(0, RPC::ParameterSet::Type::Enum::values, i->first, j->first, j->second.data);
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
						else saveParameter(0, RPC::ParameterSet::Type::Enum::link, i->first, l->first, l->second.data, j->first, k->first);
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
			RPC::ParameterSet::Type::Enum parameterSetType = (RPC::ParameterSet::Type::Enum)row->second.at(2)->intValue;

			if(parameterSetType == RPC::ParameterSet::Type::Enum::none)
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
				if(parameterSetType == RPC::ParameterSet::Type::Enum::master) parameter = &configCentral[channel][*parameterName];
				else if(parameterSetType == RPC::ParameterSet::Type::Enum::values) parameter = &valuesCentral[channel][*parameterName];
				else if(parameterSetType == RPC::ParameterSet::Type::Enum::link) parameter = &linksCentral[channel][remoteAddress][remoteChannel][*parameterName];
				parameter->databaseID = databaseID;
				parameter->data.insert(parameter->data.begin(), row->second.at(7)->binaryValue->begin(), row->second.at(7)->binaryValue->end());
				if(!rpcDevice)
				{
					_bl->out.printError("Critical: No xml rpc device found for peer " + std::to_string(_peerID) + ".");
					continue;
				}
				if(rpcDevice->channels.find(channel) != rpcDevice->channels.end() && rpcDevice->channels[channel] && rpcDevice->channels[channel]->parameterSets.find(parameterSetType) != rpcDevice->channels[channel]->parameterSets.end() && rpcDevice->channels[channel]->parameterSets[parameterSetType])
				{
					parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(*parameterName);
				}
				if(!parameter->rpcParameter)
				{
					_bl->out.printError("Error: Deleting parameter " + *parameterName + ", because no corresponding RPC parameter was found. Peer: " + std::to_string(_peerID) + " Channel: " + std::to_string(channel) + " Parameter set type: " + std::to_string((uint32_t)parameterSetType));
					Database::DataRow data;
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn((int32_t)parameterSetType)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(channel)));
					data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(*parameterName)));
					if(parameterSetType == RPC::ParameterSet::Type::Enum::master)
					{
						configCentral[channel].erase(*parameterName);
					}
					else if(parameterSetType == RPC::ParameterSet::Type::Enum::values)
					{
						valuesCentral[channel].erase(*parameterName);
					}
					else if(parameterSetType == RPC::ParameterSet::Type::Enum::link)
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

//RPC methods
std::shared_ptr<RPC::RPCVariable> Peer::getAllValues(bool returnWriteOnly)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> values(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::string type;
		std::shared_ptr<RPC::DeviceType> rpcDeviceType = rpcDevice->getType(_deviceType, _firmwareVersion);
		if(rpcDeviceType) type = rpcDeviceType->id;
		else if(_deviceType.type() == 0) type = "HM-RCV-50"; //Central
		else
		{
			if(!rpcDevice->supportedTypes.empty()) type = rpcDevice->supportedTypes.at(0)->id;
		}

		values->structValue->insert(RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_deviceType.family()))));
		values->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
		values->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));
		values->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));
		std::shared_ptr<RPC::RPCVariable> channels(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
		{
			std::shared_ptr<RPC::RPCVariable> channel(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			channel->structValue->insert(RPC::RPCStructElement("INDEX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(i->first))));

			std::shared_ptr<RPC::RPCVariable> parameters(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
			channel->structValue->insert(RPC::RPCStructElement("PARAMSET", parameters));
			channels->arrayValue->push_back(channel);

			std::shared_ptr<RPC::ParameterSet> parameterSet = getParameterSet(i->first, RPC::ParameterSet::Type::values);
			if(!parameterSet) continue;

			for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator j = parameterSet->parameters.begin(); j != parameterSet->parameters.end(); ++j)
			{
				if((*j)->id.empty() || (*j)->hidden) continue;
				if(!((*j)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*j)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*j)->uiFlags & RPC::Parameter::UIFlags::Enum::internal)  && !((*j)->uiFlags & RPC::Parameter::UIFlags::Enum::transform))
				{
					_bl->out.printDebug("Debug: Omitting parameter " + (*j)->id + " because of it's ui flag.");
					continue;
				}
				bool writeOnly = false;
				if(!((*j)->operations & BaseLib::RPC::Parameter::Operations::read) && !((*j)->operations & BaseLib::RPC::Parameter::Operations::event)) writeOnly = true;
				if(writeOnly && !returnWriteOnly) continue;
				if(valuesCentral.find(i->first) == valuesCentral.end()) continue;
				if(valuesCentral[i->first].find((*j)->id) == valuesCentral[i->first].end()) continue;

				std::shared_ptr<RPC::RPCVariable> element(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
				std::shared_ptr<RPC::RPCVariable> value;
				if(!writeOnly)
				{
					value = ((*j)->convertFromPacket(valuesCentral[i->first][(*j)->id].data));
					if(!value) continue;
					element->structValue->insert(RPC::RPCStructElement("VALUE", value));
				}

				if(returnWriteOnly) element->structValue->insert(RPC::RPCStructElement("READABLE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(!writeOnly))));
				element->structValue->insert(RPC::RPCStructElement("WRITEABLE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(((*j)->operations & 2) == 2))));
				if((*j)->logicalParameter->type == RPC::LogicalParameter::Type::typeBoolean)
				{
					element->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("BOOL")))));
				}
				else if((*j)->logicalParameter->type == RPC::LogicalParameter::Type::typeString)
				{
					element->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("STRING")))));
				}
				else if((*j)->logicalParameter->type == RPC::LogicalParameter::Type::typeAction)
				{
					element->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("ACTION")))));
				}
				else if((*j)->logicalParameter->type == RPC::LogicalParameter::Type::typeInteger)
				{
					RPC::LogicalParameterInteger* parameter = (RPC::LogicalParameterInteger*)(*j)->logicalParameter.get();
					element->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("INTEGER")))));
					element->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
					element->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));

					if(!parameter->specialValues.empty())
					{
						std::shared_ptr<RPC::RPCVariable> specialValues(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
						for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
						{
							std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
							specialElement->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first))));
							specialElement->structValue->insert(RPC::RPCStructElement("VALUE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second))));
							specialValues->arrayValue->push_back(specialElement);
						}
						element->structValue->insert(RPC::RPCStructElement("SPECIAL", specialValues));
					}
				}
				else if((*j)->logicalParameter->type == RPC::LogicalParameter::Type::typeEnum)
				{
					RPC::LogicalParameterEnum* parameter = (RPC::LogicalParameterEnum*)(*j)->logicalParameter.get();
					element->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("ENUM")))));
					element->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
					element->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));

					std::shared_ptr<RPC::RPCVariable> valueList(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					for(std::vector<RPC::ParameterOption>::iterator j = parameter->options.begin(); j != parameter->options.end(); ++j)
					{
						valueList->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->id)));
					}
					element->structValue->insert(RPC::RPCStructElement("VALUE_LIST", valueList));
				}
				else if((*j)->logicalParameter->type == RPC::LogicalParameter::Type::typeFloat)
				{
					RPC::LogicalParameterFloat* parameter = (RPC::LogicalParameterFloat*)(*j)->logicalParameter.get();
					element->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("FLOAT")))));
					element->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
					element->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));

					if(!parameter->specialValues.empty())
					{
						std::shared_ptr<RPC::RPCVariable> specialValues(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
						for(std::unordered_map<std::string, double>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
						{
							std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
							specialElement->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first))));
							specialElement->structValue->insert(RPC::RPCStructElement("VALUE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second))));
							specialValues->arrayValue->push_back(specialElement);
						}
						element->structValue->insert(RPC::RPCStructElement("SPECIAL", specialValues));
					}
				}
				parameters->structValue->insert(RPC::RPCStructElement((*j)->id, element));
			}
		}
		values->structValue->insert(RPC::RPCStructElement("CHANNELS", channels));

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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getDeviceDescription(int32_t channel, std::map<std::string, bool> fields)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::string type;
		std::shared_ptr<RPC::DeviceType> rpcDeviceType = rpcDevice->getType(_deviceType, _firmwareVersion);
		if(rpcDeviceType) type = rpcDeviceType->id;
		else if(_deviceType.type() == 0) type = "HM-RCV-50"; //Central
		else
		{
			if(!rpcDevice->supportedTypes.empty()) type = rpcDevice->supportedTypes.at(0)->id;
		}

		if(channel == -1) //Base device
		{
			if(fields.empty() || fields.find("FAMILY") != fields.end()) description->structValue->insert(RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_deviceType.family()))));
			if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			if(fields.empty() || fields.find("ADDRESS") != fields.end()) description->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			std::shared_ptr<RPC::RPCVariable> variable2 = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			if(fields.empty() || fields.find("CHILDREN") != fields.end()) description->structValue->insert(RPC::RPCStructElement("CHILDREN", variable));
			if(fields.empty() || fields.find("CHANNELS") != fields.end()) description->structValue->insert(RPC::RPCStructElement("CHANNELS", variable2));

			if(fields.empty() || fields.find("CHILDREN") != fields.end() || fields.find("CHANNELS") != fields.end())
			{
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
				{
					if(i->second->hidden) continue;
					if(fields.empty() || fields.find("CHILDREN") != fields.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(i->first))));
					if(fields.empty() || fields.find("CHANNELS") != fields.end()) variable2->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(i->first)));
				}
			}

			if(fields.empty() || fields.find("FIRMWARE") != fields.end())
			{
				if(_firmwareVersion != 0) description->structValue->insert(RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(getFirmwareVersionString(_firmwareVersion)))));
				else description->structValue->insert(RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("?")))));
			}

			if(fields.empty() || fields.find("AVAILABLE_FIRMWARE") != fields.end())
			{
				int32_t newFirmwareVersion = getNewFirmwareVersion();
				if(newFirmwareVersion > _firmwareVersion) description->structValue->insert(RPC::RPCStructElement("AVAILABLE_FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(getFirmwareVersionString(newFirmwareVersion)))));
			}

			if(fields.empty() || fields.find("FLAGS") != fields.end())
			{
				int32_t uiFlags = (int32_t)rpcDevice->uiFlags;
				if(isTeam()) uiFlags |= RPC::Device::UIFlags::dontdelete;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));
			}

			if(fields.empty() || fields.find("INTERFACE") != fields.end()) description->structValue->insert(RPC::RPCStructElement("INTERFACE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(getCentral()->logicalDevice()->getSerialNumber()))));

			if(fields.empty() || fields.find("PARAMSETS") != fields.end())
			{
				variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				description->structValue->insert(RPC::RPCStructElement("PARAMSETS", variable));
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("MASTER")))); //Always MASTER
			}

			if(fields.empty() || fields.find("PARENT") != fields.end()) description->structValue->insert(RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("")))));

			if(fields.empty() || fields.find("PHYSICAL_ADDRESS") != fields.end()) description->structValue->insert(RPC::RPCStructElement("PHYSICAL_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_address))));

			//Compatibility
			if(fields.empty() || fields.find("RF_ADDRESS") != fields.end()) description->structValue->insert(RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_address))));
			//Compatibility
			if(fields.empty() || fields.find("ROAMING") != fields.end()) description->structValue->insert(RPC::RPCStructElement("ROAMING", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(false))));

			if(fields.empty() || fields.find("RX_MODE") != fields.end()) description->structValue->insert(RPC::RPCStructElement("RX_MODE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)rpcDevice->rxModes))));

			if(!type.empty() && (fields.empty() || fields.find("TYPE") != fields.end())) description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			if(fields.empty() || fields.find("VERSION") != fields.end()) description->structValue->insert(RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));

			if(fields.find("WIRELESS") != fields.end()) description->structValue->insert(RPC::RPCStructElement("WIRELESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(wireless()))));
		}
		else
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
			if(rpcChannel->hidden) return description;

			if(fields.empty() || fields.find("FAMILYID") != fields.end()) description->structValue->insert(RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_deviceType.family()))));
			if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			if(fields.empty() || fields.find("CHANNEL") != fields.end()) description->structValue->insert(RPC::RPCStructElement("CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));
			if(fields.empty() || fields.find("ADDRESS") != fields.end()) description->structValue->insert(RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));

			if(fields.empty() || fields.find("AES_ACTIVE") != fields.end())
			{
				int32_t aesActive = 0;
				if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && !configCentral.at(channel).at("AES_ACTIVE").data.empty() && configCentral.at(channel).at("AES_ACTIVE").data.at(0) != 0)
				{
					aesActive = 1;
				}
				description->structValue->insert(RPC::RPCStructElement("AES_ACTIVE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((bool)aesActive))));
			}

			if(fields.empty() || fields.find("DIRECTION") != fields.end() || fields.find("LINK_SOURCE_ROLES") != fields.end() || fields.find("LINK_TARGET_ROLES") != fields.end())
			{
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
				if(fields.empty() || fields.find("DIRECTION") != fields.end()) description->structValue->insert(RPC::RPCStructElement("DIRECTION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(direction))));
				if(fields.empty() || fields.find("LINK_SOURCE_ROLES") != fields.end()) description->structValue->insert(RPC::RPCStructElement("LINK_SOURCE_ROLES", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(linkSourceRoles.str()))));
				if(fields.empty() || fields.find("LINK_TARGET_ROLES") != fields.end()) description->structValue->insert(RPC::RPCStructElement("LINK_TARGET_ROLES", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(linkTargetRoles.str()))));
			}

			if(fields.empty() || fields.find("FLAGS") != fields.end())
			{
				int32_t uiFlags = (int32_t)rpcChannel->uiFlags;
				if(isTeam()) uiFlags |= RPC::DeviceChannel::UIFlags::dontdelete;
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));
			}

			if(fields.empty() || fields.find("GROUP") != fields.end())
			{
				int32_t groupedWith = getChannelGroupedWith(channel);
				if(groupedWith > -1)
				{
					description->structValue->insert(RPC::RPCStructElement("GROUP", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(groupedWith)))));
				}
			}

			if(fields.empty() || fields.find("INDEX") != fields.end()) description->structValue->insert(RPC::RPCStructElement("INDEX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));

			if(fields.empty() || fields.find("PARAMSETS") != fields.end())
			{
				std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				description->structValue->insert(RPC::RPCStructElement("PARAMSETS", variable));
				for(std::map<RPC::ParameterSet::Type::Enum, std::shared_ptr<RPC::ParameterSet>>::iterator j = rpcChannel->parameterSets.begin(); j != rpcChannel->parameterSets.end(); ++j)
				{
					variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second->typeString())));
				}
			}
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::link) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::link)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::master) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::master)->typeString())));
			//if(rpcChannel->parameterSets.find(RPC::ParameterSet::Type::Enum::values) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(RPC::ParameterSet::Type::Enum::values)->typeString())));

			if(fields.empty() || fields.find("PARENT") != fields.end()) description->structValue->insert(RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			if(!type.empty() && (fields.empty() || fields.find("PARENT_TYPE") != fields.end())) description->structValue->insert(RPC::RPCStructElement("PARENT_TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			if(fields.empty() || fields.find("TYPE") != fields.end()) description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->type))));

			if(fields.empty() || fields.find("VERSION") != fields.end()) description->structValue->insert(RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> Peer::getDeviceDescriptions(bool channels, std::map<std::string, bool> fields)
{
	try
	{
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> descriptions(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
		descriptions->push_back(getDeviceDescription(-1, fields));

		if(channels)
		{
			for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				std::shared_ptr<RPC::RPCVariable> description = getDeviceDescription((int32_t)i->first, fields);
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
    return std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>>();
}

std::shared_ptr<RPC::RPCVariable> Peer::getDeviceInfo(std::map<std::string, bool> fields)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> info(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		info->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID))));

		if(fields.empty() || fields.find("NAME") != fields.end()) info->structValue->insert(RPC::RPCStructElement("NAME", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_name))));

		if(wireless())
		{
			if(fields.empty() || fields.find("RSSI") != fields.end())
			{
				if(valuesCentral.find(0) != valuesCentral.end() && valuesCentral.at(0).find("RSSI_DEVICE") != valuesCentral.at(0).end() && valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter)
				{
					info->structValue->insert(RPC::RPCStructElement("RSSI", valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter->convertFromPacket(valuesCentral.at(0).at("RSSI_DEVICE").data)));
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
    return std::shared_ptr<RPC::RPCVariable>();
}

std::shared_ptr<RPC::RPCVariable> Peer::getLink(int32_t channel, int32_t flags, bool avoidDuplicates)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		std::shared_ptr<RPC::RPCVariable> array(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> element;
		bool groupFlag = false;
		if(flags & 0x01) groupFlag = true;
		if(channel > -1 && !groupFlag) //Get link of single channel
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
						if(!(brokenFlags & 2) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel, std::map<std::string, bool>());
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
						if(!(brokenFlags & 1)) description = getDeviceDescription(channel, std::map<std::string, bool>());
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
						if(!(brokenFlags & 2)) description = getDeviceDescription(channel, std::map<std::string, bool>());
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
						if(!(brokenFlags & 1) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel, std::map<std::string, bool>());
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
			if(channel > -1 && groupFlag) //Get links for each grouped channel
			{
				if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
				std::shared_ptr<RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
				if(rpcChannel->paired)
				{
					element = getLink(channel, flags & 0xFFFFFFFE, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());

					int32_t groupedWith = getChannelGroupedWith(channel);
					if(groupedWith > -1)
					{
						element = getLink(groupedWith, flags & 0xFFFFFFFE, avoidDuplicates);
						array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
					}
				}
				else
				{
					element = getLink(channel, flags & 0xFFFFFFFE, avoidDuplicates);
					array->arrayValue->insert(array->arrayValue->end(), element->arrayValue->begin(), element->arrayValue->end());
				}
			}
			else //Get links for all channels
			{
				for(std::map<uint32_t, std::shared_ptr<RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
				{
					element.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					element = getLink(i->first, flags, avoidDuplicates);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLinkInfo(int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel)
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getLinkPeers(int32_t channel, bool returnID)
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
					std::shared_ptr<RPC::RPCVariable> address(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					array->arrayValue->push_back(address);
					address->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)peer->getID())));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetDescription(std::shared_ptr<RPC::ParameterSet> parameterSet)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");

		std::shared_ptr<RPC::RPCVariable> descriptions(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		uint32_t index = 0;
		for(std::vector<std::shared_ptr<RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::internal)  && !((*i)->uiFlags & RPC::Parameter::UIFlags::Enum::transform))
			{
				_bl->out.printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeBoolean)
			{
				RPC::LogicalParameterBoolean* parameter = (RPC::LogicalParameterBoolean*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty()) description->structValue->insert(RPC::RPCStructElement("CONTROL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->control))));
				if(parameter->defaultValueExists) description->structValue->insert(RPC::RPCStructElement("DEFAULT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->defaultValue))));
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->uiFlags))));
				description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->id))));
				description->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));
				description->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->operations))));
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(index))));
				description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("BOOL")))));
				description->structValue->insert(RPC::RPCStructElement("UNIT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->unit))));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeString)
			{
				RPC::LogicalParameterString* parameter = (RPC::LogicalParameterString*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty()) description->structValue->insert(RPC::RPCStructElement("CONTROL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->control))));
				if(parameter->defaultValueExists) description->structValue->insert(RPC::RPCStructElement("DEFAULT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->defaultValue))));
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->uiFlags))));
				description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->id))));
				description->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));
				description->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->operations))));
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(index))));
				description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("STRING")))));
				description->structValue->insert(RPC::RPCStructElement("UNIT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->unit))));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeAction)
			{
				RPC::LogicalParameterAction* parameter = (RPC::LogicalParameterAction*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty()) description->structValue->insert(RPC::RPCStructElement("CONTROL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->control))));
				if(parameter->defaultValueExists) description->structValue->insert(RPC::RPCStructElement("DEFAULT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->defaultValue))));
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->uiFlags))));
				description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->id))));
				description->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));
				description->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->operations))));
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(index))));
				description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("ACTION")))));
				description->structValue->insert(RPC::RPCStructElement("UNIT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->unit))));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeInteger)
			{
				RPC::LogicalParameterInteger* parameter = (RPC::LogicalParameterInteger*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty()) description->structValue->insert(RPC::RPCStructElement("CONTROL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->control))));
				if(parameter->defaultValueExists) description->structValue->insert(RPC::RPCStructElement("DEFAULT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->defaultValue))));
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->uiFlags))));
				description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->id))));
				description->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));
				description->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->operations))));

				if(!parameter->specialValues.empty())
				{
					std::shared_ptr<RPC::RPCVariable> specialValues(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(RPC::RPCStructElement("VALUE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second))));
						specialValues->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(RPC::RPCStructElement("SPECIAL", specialValues));
				}

				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(index))));
				description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("INTEGER")))));
				description->structValue->insert(RPC::RPCStructElement("UNIT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->unit))));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeEnum)
			{
				RPC::LogicalParameterEnum* parameter = (RPC::LogicalParameterEnum*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty()) description->structValue->insert(RPC::RPCStructElement("CONTROL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->control))));
				if(parameter->defaultValueExists) description->structValue->insert(RPC::RPCStructElement("DEFAULT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->defaultValue))));
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->uiFlags))));
				description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->id))));
				description->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));
				description->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->operations))));
				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(index))));
				description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("ENUM")))));
				description->structValue->insert(RPC::RPCStructElement("UNIT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->unit))));

				std::shared_ptr<RPC::RPCVariable> valueList(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				for(std::vector<RPC::ParameterOption>::iterator j = parameter->options.begin(); j != parameter->options.end(); ++j)
				{
					valueList->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->id)));
				}
				description->structValue->insert(RPC::RPCStructElement("VALUE_LIST", valueList));
			}
			else if((*i)->logicalParameter->type == RPC::LogicalParameter::Type::typeFloat)
			{
				RPC::LogicalParameterFloat* parameter = (RPC::LogicalParameterFloat*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty()) description->structValue->insert(RPC::RPCStructElement("CONTROL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->control))));
				if(parameter->defaultValueExists) description->structValue->insert(RPC::RPCStructElement("DEFAULT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->defaultValue))));
				description->structValue->insert(RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->uiFlags))));
				description->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->id))));
				description->structValue->insert(RPC::RPCStructElement("MAX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->max))));
				description->structValue->insert(RPC::RPCStructElement("MIN", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->min))));
				description->structValue->insert(RPC::RPCStructElement("OPERATIONS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)(*i)->operations))));

				if(!parameter->specialValues.empty())
				{
					std::shared_ptr<RPC::RPCVariable> specialValues(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, double>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<RPC::RPCVariable> specialElement(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(RPC::RPCStructElement("VALUE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second))));
						specialValues->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(RPC::RPCStructElement("SPECIAL", specialValues));
				}

				description->structValue->insert(RPC::RPCStructElement("TAB_ORDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(index))));
				description->structValue->insert(RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("FLOAT")))));
				description->structValue->insert(RPC::RPCStructElement("UNIT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(parameter->unit))));
			}

			index++;
			descriptions->structValue->insert(RPC::RPCStructElement((*i)->id, description));
			description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getServiceMessages(bool returnID)
{
	if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
	if(!serviceMessages) return RPC::RPCVariable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get(returnID);
}

std::shared_ptr<RPC::RPCVariable> Peer::getValue(uint32_t channel, std::string valueKey)
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
		if(!(parameter->operations & BaseLib::RPC::Parameter::Operations::read) && !(parameter->operations & BaseLib::RPC::Parameter::Operations::event)) return RPC::RPCVariable::createError(-6, "Parameter is not readable.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::setLinkInfo(int32_t senderChannel, uint64_t receiverID, int32_t receiverChannel, std::string name, std::string description)
{
	try
	{
		if(_peers.find(senderChannel) == _peers.end()) return RPC::RPCVariable::createError(-2, "No peer found for sender channel.");
		std::shared_ptr<BasicPeer> remotePeer = getPeer(senderChannel, receiverID, receiverChannel);
		if(!remotePeer)	return RPC::RPCVariable::createError(-2, "Peer not found.");
		remotePeer->linkDescription = description;
		remotePeer->linkName = name;
		savePeers();
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::setId(uint64_t newPeerId)
{
	try
	{
		if(newPeerId == 0 || newPeerId >= 0x40000000) return RPC::RPCVariable::createError(-100, "New peer ID is invalid.");
		if(!raiseSetPeerID(newPeerId)) return RPC::RPCVariable::createError(-101, "New peer ID is already in use.");
		_peerID = newPeerId;
		if(serviceMessages) serviceMessages->setPeerID(newPeerId);
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. See error log for more details.");
}

std::shared_ptr<RPC::RPCVariable> Peer::setValue(uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		//Nothing to do, return to save ressources
		if(value->stringValue.size() < 3) return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));

		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return RPC::RPCVariable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return BaseLib::RPC::RPCVariable::createError(-5, "Value key is empty.");
		if(channel == 0 && serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = parameter->rpcParameter;
		if(!rpcParameter) return BaseLib::RPC::RPCVariable::createError(-5, "Unknown parameter.");
		//Perform operation on value
		if(value->stringValue.size() > 2 && value->stringValue.at(1) == '='
				&& (value->stringValue.at(0) == '+' || value->stringValue.at(0) == '-' || value->stringValue.at(0) == '*' || value->stringValue.at(0) == '/'))
		{
			if(rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeFloat)
			{
				std::shared_ptr<RPC::RPCVariable> currentValue = rpcParameter->convertFromPacket(parameter->data);
				std::string numberPart = value->stringValue.substr(2);
				double factor = _bl->hf.getDouble(numberPart);
				if(factor == 0) return RPC::RPCVariable::createError(-1, "Factor is \"0\" or no valid number.");
				_bl->out.printError(std::to_string(currentValue->floatValue));
				if(value->stringValue.at(0) == '+') value->floatValue = currentValue->floatValue + factor;
				else if(value->stringValue.at(0) == '-') value->floatValue = currentValue->floatValue - factor;
				else if(value->stringValue.at(0) == '*') value->floatValue = currentValue->floatValue * factor;
				else if(value->stringValue.at(0) == '/') value->floatValue = currentValue->floatValue / factor;
				_bl->out.printError(std::to_string(value->floatValue));
			}
			else if(rpcParameter->logicalParameter->type == RPC::LogicalParameter::Type::Enum::typeInteger)
			{
				std::shared_ptr<RPC::RPCVariable> currentValue = rpcParameter->convertFromPacket(parameter->data);
				std::string numberPart = value->stringValue.substr(2);
				int32_t factor = _bl->hf.getNumber(numberPart);
				if(factor == 0) return RPC::RPCVariable::createError(-1, "Factor is \"0\" or no valid number.");
				if(value->stringValue.at(0) == '+') value->integerValue = currentValue->integerValue + factor;
				else if(value->stringValue.at(0) == '-') value->integerValue = currentValue->integerValue - factor;
				else if(value->stringValue.at(0) == '*') value->integerValue = currentValue->integerValue * factor;
				else if(value->stringValue.at(0) == '/') value->integerValue = currentValue->integerValue / factor;
			}
		}
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. See error log for more details.");
}
//End RPC methods
}
}
