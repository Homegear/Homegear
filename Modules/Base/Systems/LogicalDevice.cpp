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
void LogicalDevice::raiseCreateSavepoint(std::string name)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onCreateSavepoint(name);
}

void LogicalDevice::raiseReleaseSavepoint(std::string name)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onReleaseSavepoint(name);
}

void LogicalDevice::raiseDeleteMetadata(std::string objectID, std::string dataID)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeleteMetadata(objectID, dataID);
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

uint64_t LogicalDevice::raiseSavePeerParameter(uint64_t peerID, Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSavePeerParameter(peerID, data);
}

uint64_t LogicalDevice::raiseSavePeerVariable(uint64_t peerID, Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSavePeerVariable(peerID, data);
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

void LogicalDevice::raiseDeletePeerParameter(uint64_t peerID, Database::DataRow data)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeletePeerParameter(peerID, data);
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetServiceMessages(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IDeviceEventSink*)_eventHandler)->onGetServiceMessages(peerID);
}

uint64_t LogicalDevice::raiseSaveServiceMessage(uint64_t peerID, Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSaveServiceMessage(peerID, data);
}

void LogicalDevice::raiseDeleteServiceMessage(uint64_t databaseID)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onDeleteServiceMessage(databaseID);
}

uint64_t LogicalDevice::raiseSaveDevice()
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSaveDevice(_deviceID, _address, _serialNumber, _deviceType, (uint32_t)_deviceFamily);
}

uint64_t LogicalDevice::raiseSaveDeviceVariable(Database::DataRow data)
{
	if(!_eventHandler) return 0;
	return ((IDeviceEventSink*)_eventHandler)->onSaveDeviceVariable(data);
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

void LogicalDevice::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void LogicalDevice::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCUpdateDevice(id, channel, address, hint);
}

void LogicalDevice::raiseRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCNewDevices(deviceDescriptions);
}

void LogicalDevice::raiseRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onRPCDeleteDevices(deviceAddresses, deviceInfo);
}

void LogicalDevice::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	if(_eventHandler) ((IDeviceEventSink*)_eventHandler)->onEvent(peerID, channel, variables, values);
}
//End event handling

//Peer event handling
void LogicalDevice::onCreateSavepoint(std::string name)
{
	raiseCreateSavepoint(name);
}

void LogicalDevice::onReleaseSavepoint(std::string name)
{
	raiseReleaseSavepoint(name);
}

void LogicalDevice::onDeleteMetadata(std::string objectID, std::string dataID)
{
	raiseDeleteMetadata(objectID, dataID);
}

void LogicalDevice::onDeletePeer(uint64_t id)
{
	raiseDeletePeer(id);
}

uint64_t LogicalDevice::onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	return raiseSavePeer(id, parentID, address, serialNumber);
}

uint64_t LogicalDevice::onSavePeerParameter(uint64_t peerID, Database::DataRow data)
{
	return raiseSavePeerParameter(peerID, data);
}

uint64_t LogicalDevice::onSavePeerVariable(uint64_t peerID, Database::DataRow data)
{
	return raiseSavePeerVariable(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> LogicalDevice::onGetPeerParameters(uint64_t peerID)
{
	return raiseGetPeerParameters(peerID);
}

std::shared_ptr<BaseLib::Database::DataTable> LogicalDevice::onGetPeerVariables(uint64_t peerID)
{
	return raiseGetPeerVariables(peerID);
}

void LogicalDevice::onDeletePeerParameter(uint64_t peerID, Database::DataRow data)
{
	raiseDeletePeerParameter(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> LogicalDevice::onGetServiceMessages(uint64_t peerID)
{
	return raiseGetServiceMessages(peerID);
}

uint64_t LogicalDevice::onSaveServiceMessage(uint64_t peerID, Database::DataRow data)
{
	return raiseSaveServiceMessage(peerID, data);
}

void LogicalDevice::onDeleteServiceMessage(uint64_t databaseID)
{
	raiseDeleteServiceMessage(databaseID);
}

void LogicalDevice::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	raiseRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void LogicalDevice::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	raiseRPCUpdateDevice(id, channel, address, hint);
}

void LogicalDevice::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	raiseEvent(peerID, channel, variables, values);
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

DeviceFamilies LogicalDevice::deviceFamily()
{
	return _deviceFamily;
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(intValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			uint64_t result = raiseSaveDeviceVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(stringValue)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			uint64_t result = raiseSaveDeviceVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
			data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
			uint64_t result = raiseSaveDeviceVariable(data);
			if(result) _variableDatabaseIDs[index] = result;
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
