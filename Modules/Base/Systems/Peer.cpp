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
		if(centralFeatures)
		{
			serviceMessages.reset(new ServiceMessages(baseLib, 0, "", this));
		}
		_lastPacketReceived = HelperFunctions::getTimeSeconds();
		rpcDevice.reset();
		setEventHandler(eventHandler);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
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
    catch(Exception& ex)
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
	_rpcCache.clear();
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

void Peer::raiseDeleteMetadata(std::string objectID, std::string dataID)
{
	if(_eventHandler) ((IPeerEventSink*)_eventHandler)->onDeleteMetadata(objectID, dataID);
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
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
    catch(Exception& ex)
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
	catch(Exception& ex)
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
		raiseDeleteMetadata(_serialNumber);
		raiseDeleteMetadata(std::to_string(_peerID));
		if(rpcDevice)
		{
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
			{
				raiseDeleteMetadata(_serialNumber + ':' + std::to_string(i->first));
				raiseDeleteMetadata(std::to_string(_peerID) + ':' + std::to_string(i->first));
			}
		}
		raiseDeletePeer();
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
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
			if(_peerID == 0) _peerID = result;
			_databaseMutex.unlock();
		}
		if(variables)
		{
			saveVariables();
		}
		else
		{
			//Service messages are not saved when set.
			saveServiceMessages();
		}
		if(centralConfig) saveConfig();
	}
	catch(const std::exception& ex)
    {
		_databaseMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
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
    catch(Exception& ex)
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
    catch(Exception& ex)
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
    catch(Exception& ex)
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
	_databaseMutex.unlock();
}

void Peer::saveVariables()
{
	try
	{
		if(_peerID == 0 || isTeam()) return;
		saveVariable(1000, _name);
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
    catch(Exception& ex)
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
    catch(Exception& ex)
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
    catch(Exception& ex)
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
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveServiceMessages()
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
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
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
    catch(Exception& ex)
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

				RPCConfigurationParameter* parameter;
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
    catch(Exception& ex)
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
std::shared_ptr<RPC::RPCVariable> Peer::getDeviceDescription(int32_t channel, std::map<std::string, bool> fields)
{
	try
	{
		if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		std::shared_ptr<RPC::RPCVariable> description(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));

		std::string type;
		std::shared_ptr<BaseLib::RPC::DeviceType> rpcDeviceType = rpcDevice->getType(_deviceType, _firmwareVersion);
		if(rpcDeviceType) type = rpcDeviceType->id;
		else if(_deviceType.type() == 0) type = "HM-RCV-50"; //Central
		else
		{
			if(!rpcDevice->supportedTypes.empty()) type = rpcDevice->supportedTypes.at(0)->id;
		}

		if(channel == -1) //Base device
		{
			if(fields.empty() || fields.find("FAMILYID") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_deviceType.family()))));
			if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			if(fields.empty() || fields.find("ADDRESS") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			std::shared_ptr<RPC::RPCVariable> variable2 = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			if(fields.empty() || fields.find("CHILDREN") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("CHILDREN", variable));
			if(fields.empty() || fields.find("CHANNELS") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("CHANNELS", variable2));

			if(fields.empty() || fields.find("CHILDREN") != fields.end() || fields.find("CHANNELS") != fields.end())
			{
				for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
				{
					if(i->second->hidden) continue;
					if(fields.empty() || fields.find("CHILDREN") != fields.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(i->first))));
					if(fields.empty() || fields.find("CHANNELS") != fields.end()) variable2->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(i->first)));
				}
			}

			if(fields.empty() || fields.find("FIRMWARE") != fields.end())
			{
				if(_firmwareVersion != 0) description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(HelperFunctions::getHexString(_firmwareVersion >> 8) + "." + HelperFunctions::getHexString(_firmwareVersion & 0xFF)))));
				else description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("?")))));
			}

			if(fields.empty() || fields.find("AVAILABLE_FIRMWARE") != fields.end())
			{
				int32_t newFirmwareVersion = getNewFirmwareVersion();
				if(newFirmwareVersion > _firmwareVersion) description->structValue->insert(BaseLib::RPC::RPCStructElement("AVAILABLE_FIRMWARE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(HelperFunctions::getHexString(newFirmwareVersion >> 8) + "." + HelperFunctions::getHexString(newFirmwareVersion & 0xFF)))));
			}

			if(fields.empty() || fields.find("FLAGS") != fields.end())
			{
				int32_t uiFlags = (int32_t)rpcDevice->uiFlags;
				if(isTeam()) uiFlags |= BaseLib::RPC::Device::UIFlags::dontdelete;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));
			}

			if(fields.empty() || fields.find("INTERFACE") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("INTERFACE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(getCentral()->logicalDevice()->getSerialNumber()))));

			if(fields.empty() || fields.find("PARAMSETS") != fields.end())
			{
				variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				description->structValue->insert(BaseLib::RPC::RPCStructElement("PARAMSETS", variable));
				variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("MASTER")))); //Always MASTER
			}

			if(fields.empty() || fields.find("PARENT") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("")))));

			if(fields.empty() || fields.find("PHYSICAL_ADDRESS") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("PHYSICAL_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_address))));

			//Compatibility
			if(fields.empty() || fields.find("RF_ADDRESS") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("RF_ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_address))));
			//Compatibility
			if(fields.empty() || fields.find("ROAMING") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("ROAMING", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(false))));

			if(fields.empty() || fields.find("RX_MODE") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("RX_MODE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)rpcDevice->rxModes))));

			if(!type.empty() && (fields.empty() || fields.find("TYPE") != fields.end())) description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			if(fields.empty() || fields.find("VERSION") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));

			if(fields.find("WIRELESS") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("WIRELESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(wireless()))));
		}
		else
		{
			if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return RPC::RPCVariable::createError(-2, "Unknown channel.");
			std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
			if(rpcChannel->hidden) return description;

			if(fields.empty() || fields.find("FAMILYID") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("FAMILY", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_deviceType.family()))));
			if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)_peerID))));
			if(fields.empty() || fields.find("ADDRESS") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("ADDRESS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));

			if(fields.empty() || fields.find("AES_ACTIVE") != fields.end())
			{
				int32_t aesActive = 0;
				if(configCentral.find(channel) != configCentral.end() && configCentral.at(channel).find("AES_ACTIVE") != configCentral.at(channel).end() && !configCentral.at(channel).at("AES_ACTIVE").data.empty() && configCentral.at(channel).at("AES_ACTIVE").data.at(0) != 0)
				{
					aesActive = 1;
				}
				description->structValue->insert(BaseLib::RPC::RPCStructElement("AES_ACTIVE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((bool)aesActive))));
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
				if(rpcChannel->direction != BaseLib::RPC::DeviceChannel::Direction::Enum::none) direction = (int32_t)rpcChannel->direction;
				if(fields.empty() || fields.find("DIRECTION") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("DIRECTION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(direction))));
				if(fields.empty() || fields.find("LINK_SOURCE_ROLES") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("LINK_SOURCE_ROLES", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(linkSourceRoles.str()))));
				if(fields.empty() || fields.find("LINK_TARGET_ROLES") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("LINK_TARGET_ROLES", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(linkTargetRoles.str()))));
			}

			if(fields.empty() || fields.find("FLAGS") != fields.end())
			{
				int32_t uiFlags = (int32_t)rpcChannel->uiFlags;
				if(isTeam()) uiFlags |= BaseLib::RPC::DeviceChannel::UIFlags::dontdelete;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(uiFlags))));
			}

			if(fields.empty() || fields.find("GROUP") != fields.end())
			{
				int32_t groupedWith = getChannelGroupedWith(channel);
				if(groupedWith > -1)
				{
					description->structValue->insert(BaseLib::RPC::RPCStructElement("GROUP", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(groupedWith)))));
				}
			}

			if(fields.empty() || fields.find("INDEX") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("INDEX", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));

			if(fields.empty() || fields.find("PARAMSETS") != fields.end())
			{
				std::shared_ptr<RPC::RPCVariable> variable = std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				description->structValue->insert(BaseLib::RPC::RPCStructElement("PARAMSETS", variable));
				for(std::map<BaseLib::RPC::ParameterSet::Type::Enum, std::shared_ptr<BaseLib::RPC::ParameterSet>>::iterator j = rpcChannel->parameterSets.begin(); j != rpcChannel->parameterSets.end(); ++j)
				{
					variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->second->typeString())));
				}
			}
			//if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::link) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::link)->typeString())));
			//if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::master) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::master)->typeString())));
			//if(rpcChannel->parameterSets.find(BaseLib::RPC::ParameterSet::Type::Enum::values) != rpcChannel->parameterSets.end()) variable->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->parameterSets.at(BaseLib::RPC::ParameterSet::Type::Enum::values)->typeString())));

			if(fields.empty() || fields.find("PARENT") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber))));

			if(!type.empty() && (fields.empty() || fields.find("PARENT_TYPE") != fields.end())) description->structValue->insert(BaseLib::RPC::RPCStructElement("PARENT_TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(type))));

			if(fields.empty() || fields.find("TYPE") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcChannel->type))));

			if(fields.empty() || fields.find("VERSION") != fields.end()) description->structValue->insert(BaseLib::RPC::RPCStructElement("VERSION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(rpcDevice->version))));
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
			for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
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

		if(fields.empty() || fields.find("NAME") != fields.end()) info->structValue->insert(BaseLib::RPC::RPCStructElement("NAME", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_name))));

		if(wireless())
		{
			if(fields.empty() || fields.find("RSSI") != fields.end())
			{
				if(valuesCentral.find(0) != valuesCentral.end() && valuesCentral.at(0).find("RSSI_DEVICE") != valuesCentral.at(0).end() && valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter)
				{
					info->structValue->insert(BaseLib::RPC::RPCStructElement("RSSI", valuesCentral.at(0).at("RSSI_DEVICE").rpcParameter->convertFromPacket(valuesCentral.at(0).at("RSSI_DEVICE").data)));
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
			std::shared_ptr<BaseLib::RPC::LinkRole> linkRoles = rpcDevice->channels.at(channel)->linkRoles;
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
				element->structValue->insert(BaseLib::RPC::RPCStructElement("DESCRIPTION", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->linkDescription))));
				element->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(brokenFlags))));
				element->structValue->insert(BaseLib::RPC::RPCStructElement("NAME", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->linkName))));
				if(isSender)
				{
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peerSerialNumber + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)remotePeer->getID()))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->channel))));
					if(flags & 4)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, BaseLib::RPC::ParameterSet::Type::Enum::link, _peerID, channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 2) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel, std::map<std::string, bool>());
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));
					if(flags & 2)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1)) paramset = getParamset(channel, BaseLib::RPC::ParameterSet::Type::Enum::link, peerID, (*i)->channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 1)) description = getDeviceDescription(channel, std::map<std::string, bool>());
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_DESCRIPTION", description));
					}
				}
				else //When sender is broken
				{
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_serialNumber + ":" + std::to_string(channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(channel))));
					if(flags & 4)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 2) && remotePeer) paramset = getParamset(channel, BaseLib::RPC::ParameterSet::Type::Enum::link, peerID, (*i)->channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_PARAMSET", paramset));
					}
					if(flags & 16)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 2)) description = getDeviceDescription(channel, std::map<std::string, bool>());
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("RECEIVER_DESCRIPTION", description));
					}
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(peerSerialNumber + ":" + std::to_string((*i)->channel)))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_ID", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)remotePeer->getID()))));
					element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_CHANNEL", std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((*i)->channel))));
					if(flags & 2)
					{
						std::shared_ptr<RPC::RPCVariable> paramset;
						if(!(brokenFlags & 1) && remotePeer) paramset = remotePeer->getParamset((*i)->channel, BaseLib::RPC::ParameterSet::Type::Enum::link, _peerID, channel);
						else paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(paramset->errorStruct) paramset.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_PARAMSET", paramset));
					}
					if(flags & 8)
					{
						std::shared_ptr<RPC::RPCVariable> description;
						if(!(brokenFlags & 1) && remotePeer) description = remotePeer->getDeviceDescription((*i)->channel, std::map<std::string, bool>());
						else description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						if(description->errorStruct) description.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
						element->structValue->insert(BaseLib::RPC::RPCStructElement("SENDER_DESCRIPTION", description));
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
				std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
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
				flags &= 7; //Remove flag 8 and 16. Both are not processed, when getting links for all devices.
				for(std::map<uint32_t, std::shared_ptr<BaseLib::RPC::DeviceChannel>>::iterator i = rpcDevice->channels.begin(); i != rpcDevice->channels.end(); ++i)
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

std::shared_ptr<RPC::RPCVariable> Peer::getParamsetDescription(std::shared_ptr<RPC::ParameterSet> parameterSet)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");

		std::shared_ptr<BaseLib::RPC::RPCVariable> descriptions(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<BaseLib::RPC::RPCVariable> description(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		std::shared_ptr<BaseLib::RPC::RPCVariable> element;
		uint32_t index = 0;
		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::internal)  && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::transform))
			{
				_bl->out.printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeBoolean)
			{
				BaseLib::RPC::LogicalParameterBoolean* parameter = (BaseLib::RPC::LogicalParameterBoolean*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
					element->booleanValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "BOOL";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeString)
			{
				BaseLib::RPC::LogicalParameterString* parameter = (BaseLib::RPC::LogicalParameterString*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "STRING";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeAction)
			{
				BaseLib::RPC::LogicalParameterAction* parameter = (BaseLib::RPC::LogicalParameterAction*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
					element->booleanValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcBoolean));
				element->booleanValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "ACTION";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeInteger)
			{
				BaseLib::RPC::LogicalParameterInteger* parameter = (BaseLib::RPC::LogicalParameterInteger*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
					element->integerValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				if(!parameter->specialValues.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, int32_t>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> specialElement(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("VALUE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->second))));
						element->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(BaseLib::RPC::RPCStructElement("SPECIAL", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "INTEGER";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeEnum)
			{
				BaseLib::RPC::LogicalParameterEnum* parameter = (BaseLib::RPC::LogicalParameterEnum*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
					element->integerValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "ENUM";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
				for(std::vector<BaseLib::RPC::ParameterOption>::iterator j = parameter->options.begin(); j != parameter->options.end(); ++j)
				{
					element->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->id)));
				}
				description->structValue->insert(BaseLib::RPC::RPCStructElement("VALUE_LIST", element));
			}
			else if((*i)->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeFloat)
			{
				BaseLib::RPC::LogicalParameterFloat* parameter = (BaseLib::RPC::LogicalParameterFloat*)(*i)->logicalParameter.get();

				if(!(*i)->control.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
					element->stringValue = (*i)->control;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("CONTROL", element));
				}

				if(parameter->defaultValueExists)
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcFloat));
					element->floatValue = parameter->defaultValue;
					description->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->uiFlags;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FLAGS", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = (*i)->id;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("ID", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcFloat));
				element->floatValue = parameter->max;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MAX", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcFloat));
				element->floatValue = parameter->min;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("MIN", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = (*i)->operations;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("OPERATIONS", element));

				if(!parameter->specialValues.empty())
				{
					element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
					for(std::unordered_map<std::string, double>::iterator j = parameter->specialValues.begin(); j != parameter->specialValues.end(); ++j)
					{
						std::shared_ptr<BaseLib::RPC::RPCVariable> specialElement(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("ID", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->first))));
						specialElement->structValue->insert(BaseLib::RPC::RPCStructElement("VALUE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(j->second))));
						element->arrayValue->push_back(specialElement);
					}
					description->structValue->insert(BaseLib::RPC::RPCStructElement("SPECIAL", element));
				}

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
				element->integerValue = index;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TAB_ORDER", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = "FLOAT";
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", element));

				element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
				element->stringValue = parameter->unit;
				description->structValue->insert(BaseLib::RPC::RPCStructElement("UNIT", element));
			}

			index++;
			descriptions->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, description));
			description.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> Peer::getParamsetId(uint32_t channel, RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::RPCVariable::createError(-32500, "Peer is disposing.");
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown channel.");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::RPCVariable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::Systems::BasicPeer> remotePeer;
		if(type == BaseLib::RPC::ParameterSet::Type::link && remoteID > 0)
		{
			remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return BaseLib::RPC::RPCVariable::createError(-2, "Unknown remote peer.");
		}

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(rpcDevice->channels[channel]->parameterSets[type]->id));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Peer::getServiceMessages(bool returnID)
{
	if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
	if(!serviceMessages) return RPC::RPCVariable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get(returnID);
}
//End RPC methods
}
}
