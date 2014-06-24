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

void Peer::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
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

std::shared_ptr<RPC::RPCVariable> Peer::getServiceMessages(bool returnID)
{
	if(_disposing) return RPC::RPCVariable::createError(-32500, "Peer is disposing.");
	if(!serviceMessages) return RPC::RPCVariable::createError(-32500, "Service messages are not initialized.");
	return serviceMessages->get(returnID);
}

}
}
