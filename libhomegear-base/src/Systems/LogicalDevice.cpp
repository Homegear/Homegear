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

#include "LogicalDevice.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

LogicalDevice::LogicalDevice(DeviceFamilies deviceFamily, BaseLib::Obj* baseLib, IDeviceEventSink* eventHandler)
{
	_bl = baseLib;
	_deviceFamily = deviceFamily;
	setEventHandler(eventHandler);
}

LogicalDevice::LogicalDevice(DeviceFamilies deviceFamily, BaseLib::Obj* baseLib, uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : LogicalDevice(deviceFamily, baseLib, eventHandler)
{
	_deviceID = deviceID;
	_serialNumber = serialNumber;
	_address = address;
}

LogicalDevice::~LogicalDevice()
{
}

void LogicalDevice::dispose(bool wait)
{
	_disposing = true;
	_central.reset();
	_currentPeer.reset();
	_peers.clear();
	_peersBySerial.clear();
	_peersByID.clear();
}

//Event handling
void LogicalDevice::raiseCreateSavepointSynchronous(std::string name)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onCreateSavepointSynchronous(name);
}

void LogicalDevice::raiseReleaseSavepointSynchronous(std::string name)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onReleaseSavepointSynchronous(name);
}

void LogicalDevice::raiseCreateSavepointAsynchronous(std::string name)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onCreateSavepointAsynchronous(name);
}

void LogicalDevice::raiseReleaseSavepointAsynchronous(std::string name)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onReleaseSavepointAsynchronous(name);
}

void LogicalDevice::raiseDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeleteMetadata(peerID, serialNumber, dataID);
}

void LogicalDevice::raiseDeletePeer(uint64_t id)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeletePeer(id);
}

uint64_t LogicalDevice::raiseSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSavePeer(id, parentID, address, serialNumber);
}

void LogicalDevice::raiseSavePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IDeviceEventSink*)_eventHandler)->onSavePeerParameter(peerID, data);
}

void LogicalDevice::raiseSavePeerVariable(uint64_t peerID, Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IDeviceEventSink*)_eventHandler)->onSavePeerVariable(peerID, data);
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetPeerParameters(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IDeviceEventSink*)_eventHandler)->onGetPeerParameters(peerID);
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetPeerVariables(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IDeviceEventSink*)_eventHandler)->onGetPeerVariables(peerID);
}

void LogicalDevice::raiseDeletePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeletePeerParameter(peerID, data);
}

bool LogicalDevice::raiseSetPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	if(!_eventHandler) return false;
	return ((IDeviceEventSink*)_eventHandler)->onSetPeerID(oldPeerID, newPeerID);
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetServiceMessages(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IDeviceEventSink*)_eventHandler)->onGetServiceMessages(peerID);
}

void LogicalDevice::raiseSaveServiceMessage(uint64_t peerID, Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IDeviceEventSink*)_eventHandler)->onSaveServiceMessage(peerID, data);
}

void LogicalDevice::raiseDeleteServiceMessage(uint64_t databaseID)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeleteServiceMessage(databaseID);
}

void LogicalDevice::raiseAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onAddWebserverEventHandler(eventHandler, eventHandlers);
}

void LogicalDevice::raiseRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRemoveWebserverEventHandler(eventHandlers);
}

uint64_t LogicalDevice::raiseSaveDevice()
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSaveDevice(_deviceID, _address, _serialNumber, _deviceType, (uint32_t)_deviceFamily);
}

void LogicalDevice::raiseSaveDeviceVariable(Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IDeviceEventSink*)_eventHandler)->onSaveDeviceVariable(data);
}

void LogicalDevice::raiseDeletePeers(int32_t deviceID)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeletePeers(deviceID);
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetPeers()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IDeviceEventSink*)_eventHandler)->onGetPeers(_deviceID);
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetDeviceVariables()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IDeviceEventSink*)_eventHandler)->onGetDeviceVariables(_deviceID);
}

void LogicalDevice::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<PVariable>> values)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void LogicalDevice::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCUpdateDevice(id, channel, address, hint);
}

void LogicalDevice::raiseRPCNewDevices(PVariable deviceDescriptions)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCNewDevices(deviceDescriptions);
}

void LogicalDevice::raiseRPCDeleteDevices(PVariable deviceAddresses, PVariable deviceInfo)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCDeleteDevices(deviceAddresses, deviceInfo);
}

void LogicalDevice::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<PVariable>> values)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onEvent(peerID, channel, variables, values);
}

int32_t LogicalDevice::raiseIsAddonClient(int32_t clientID)
{
	if(_eventHandler) return ((IDeviceEventSink*)_eventHandler)->onIsAddonClient(clientID);
	return -1;
}
//End event handling

//Peer event handling
void LogicalDevice::onCreateSavepointSynchronous(std::string name)
{
	raiseCreateSavepointSynchronous(name);
}

void LogicalDevice::onReleaseSavepointSynchronous(std::string name)
{
	raiseReleaseSavepointSynchronous(name);
}

void LogicalDevice::onCreateSavepointAsynchronous(std::string name)
{
	raiseCreateSavepointAsynchronous(name);
}

void LogicalDevice::onReleaseSavepointAsynchronous(std::string name)
{
	raiseReleaseSavepointAsynchronous(name);
}

void LogicalDevice::onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID)
{
	raiseDeleteMetadata(peerID, serialNumber, dataID);
}

void LogicalDevice::onDeletePeer(uint64_t id)
{
	raiseDeletePeer(id);
}

uint64_t LogicalDevice::onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	return raiseSavePeer(id, parentID, address, serialNumber);
}

void LogicalDevice::onSavePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	raiseSavePeerParameter(peerID, data);
}

void LogicalDevice::onSavePeerVariable(uint64_t peerID, Database::DataRow& data)
{
	raiseSavePeerVariable(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> LogicalDevice::onGetPeerParameters(uint64_t peerID)
{
	return raiseGetPeerParameters(peerID);
}

std::shared_ptr<BaseLib::Database::DataTable> LogicalDevice::onGetPeerVariables(uint64_t peerID)
{
	return raiseGetPeerVariables(peerID);
}

void LogicalDevice::onDeletePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	raiseDeletePeerParameter(peerID, data);
}

bool LogicalDevice::onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	return raiseSetPeerID(oldPeerID, newPeerID);
}

std::shared_ptr<BaseLib::Database::DataTable> LogicalDevice::onGetServiceMessages(uint64_t peerID)
{
	return raiseGetServiceMessages(peerID);
}

void LogicalDevice::onSaveServiceMessage(uint64_t peerID, Database::DataRow& data)
{
	raiseSaveServiceMessage(peerID, data);
}

void LogicalDevice::onDeleteServiceMessage(uint64_t databaseID)
{
	raiseDeleteServiceMessage(databaseID);
}

void LogicalDevice::onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers)
{
	raiseAddWebserverEventHandler(eventHandler, eventHandlers);
}

void LogicalDevice::onRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers)
{
	raiseRemoveWebserverEventHandler(eventHandlers);
}

void LogicalDevice::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<PVariable>> values)
{
	raiseRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void LogicalDevice::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	raiseRPCUpdateDevice(id, channel, address, hint);
}

void LogicalDevice::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<PVariable>> values)
{
	raiseEvent(peerID, channel, variables, values);
}

int32_t LogicalDevice::onIsAddonClient(int32_t clientID)
{
	return raiseIsAddonClient(clientID);
}
//End Peer event handling

void LogicalDevice::getPeers(std::vector<std::shared_ptr<Peer>>& peers, std::shared_ptr<std::set<uint64_t>> knownDevices)
{
	try
	{
		_peersMutex.lock();
		for(std::map<uint64_t, std::shared_ptr<Peer>>::iterator i = _peersByID.begin(); i != _peersByID.end(); ++i)
		{
			if(knownDevices && knownDevices->find(i->first) != knownDevices->end()) continue; //only add unknown devices
			peers.push_back(i->second);
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
    _peersMutex.unlock();
}

std::shared_ptr<Peer> LogicalDevice::getPeer(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end())
		{
			std::shared_ptr<Peer> peer(_peers.at(address));
			_peersMutex.unlock();
			return peer;
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
    _peersMutex.unlock();
    return std::shared_ptr<Peer>();
}

std::shared_ptr<Peer> LogicalDevice::getPeer(uint64_t id)
{
	try
	{
		_peersMutex.lock();
		if(_peersByID.find(id) != _peersByID.end())
		{
			std::shared_ptr<Peer> peer(_peersByID.at(id));
			_peersMutex.unlock();
			return peer;
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
    _peersMutex.unlock();
    return std::shared_ptr<Peer>();
}

std::shared_ptr<Peer> LogicalDevice::getPeer(std::string serialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end())
		{
			std::shared_ptr<Peer> peer(_peersBySerial.at(serialNumber));
			_peersMutex.unlock();
			return peer;
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
    _peersMutex.unlock();
    return std::shared_ptr<Peer>();
}

bool LogicalDevice::peerExists(int32_t address)
{
	try
	{
		_peersMutex.lock();
		if(_peers.find(address) != _peers.end())
		{
			_peersMutex.unlock();
			return true;
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
    _peersMutex.unlock();
    return false;
}

bool LogicalDevice::peerExists(uint64_t id)
{
	try
	{
		_peersMutex.lock();
		if(_peersByID.find(id) != _peersByID.end())
		{
			_peersMutex.unlock();
			return true;
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
    _peersMutex.unlock();
    return false;
}

bool LogicalDevice::peerExists(std::string serialNumber)
{
	try
	{
		_peersMutex.lock();
		if(_peersBySerial.find(serialNumber) != _peersBySerial.end())
		{
			_peersMutex.unlock();
			return true;
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
    _peersMutex.unlock();
    return false;
}

void LogicalDevice::setPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	try
	{
		std::shared_ptr<Peer> peer = getPeer(oldPeerID);
		if(!peer) return;
		_peersMutex.lock();
		if(_peersByID.find(oldPeerID) != _peersByID.end()) _peersByID.erase(oldPeerID);
		_peersByID[newPeerID] = peer;
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
    _peersMutex.unlock();
}

DeviceFamilies LogicalDevice::deviceFamily()
{
	return _deviceFamily;
}

void LogicalDevice::homegearStarted()
{
	try
	{
		std::vector<std::shared_ptr<Peer>> peers;
		getPeers(peers);
		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			(*i)->homegearStarted();
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

void LogicalDevice::homegearShuttingDown()
{
	try
	{
		std::vector<std::shared_ptr<Peer>> peers;
		getPeers(peers);
		for(std::vector<std::shared_ptr<Peer>>::iterator i = peers.begin(); i != peers.end(); ++i)
		{
			(*i)->homegearShuttingDown();
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

void LogicalDevice::load()
{
	try
	{
		loadVariables();
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

void LogicalDevice::save(bool saveDevice)
{
	try
	{
		if(saveDevice)
		{
			uint64_t result = raiseSaveDevice();
			if(_deviceID == 0) _deviceID = result;
		}
		saveVariables();
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

void LogicalDevice::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSaveDeviceVariable(data);
		}
		else
		{
			if(_deviceID == 0) return;
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			raiseSaveDeviceVariable(data);
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

void LogicalDevice::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSaveDeviceVariable(data);
		}
		else
		{
			if(_deviceID == 0) return;
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			raiseSaveDeviceVariable(data);
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

void LogicalDevice::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
			raiseSaveDeviceVariable(data);
		}
		else
		{
			if(_deviceID == 0) return;
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
			raiseSaveDeviceVariable(data);
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

void LogicalDevice::deletePeersFromDatabase()
{
	try
	{
		raiseDeletePeers(_deviceID);
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

}
}
