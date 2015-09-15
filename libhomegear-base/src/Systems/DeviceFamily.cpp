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

#include "DeviceFamily.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{
DeviceFamily::DeviceFamily(BaseLib::Obj* bl, IFamilyEventSink* eventHandler)
{
	_bl = bl;
	_eventHandler = eventHandler;
	if(_eventHandler) setEventHandler(_eventHandler);
}

DeviceFamily::~DeviceFamily()
{
	dispose();
}

//Event handling
void DeviceFamily::raiseCreateSavepointSynchronous(std::string name)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onCreateSavepointSynchronous(name);
}

void DeviceFamily::raiseReleaseSavepointSynchronous(std::string name)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onReleaseSavepointSynchronous(name);
}

void DeviceFamily::raiseCreateSavepointAsynchronous(std::string name)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onCreateSavepointAsynchronous(name);
}

void DeviceFamily::raiseReleaseSavepointAsynchronous(std::string name)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onReleaseSavepointAsynchronous(name);
}

void DeviceFamily::raiseDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onDeleteMetadata(peerID, serialNumber, dataID);
}

void DeviceFamily::raiseDeletePeer(uint64_t id)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onDeletePeer(id);
}

uint64_t DeviceFamily::raiseSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	if(!_eventHandler) return 0;
	return ((IFamilyEventSink*)_eventHandler)->onSavePeer(id, parentID, address, serialNumber);
}

void DeviceFamily::raiseSavePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IFamilyEventSink*)_eventHandler)->onSavePeerParameter(peerID, data);
}

void DeviceFamily::raiseSavePeerVariable(uint64_t peerID, Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IFamilyEventSink*)_eventHandler)->onSavePeerVariable(peerID, data);
}

std::shared_ptr<Database::DataTable> DeviceFamily::raiseGetPeerParameters(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IFamilyEventSink*)_eventHandler)->onGetPeerParameters(peerID);
}

std::shared_ptr<Database::DataTable> DeviceFamily::raiseGetPeerVariables(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IFamilyEventSink*)_eventHandler)->onGetPeerVariables(peerID);
}

void DeviceFamily::raiseDeletePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	return ((IFamilyEventSink*)_eventHandler)->onDeletePeerParameter(peerID, data);
}

bool DeviceFamily::raiseSetPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	if(!_eventHandler) return false;
	return ((IFamilyEventSink*)_eventHandler)->onSetPeerID(oldPeerID, newPeerID);
}

std::shared_ptr<Database::DataTable> DeviceFamily::raiseGetServiceMessages(uint64_t peerID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IFamilyEventSink*)_eventHandler)->onGetServiceMessages(peerID);
}

void DeviceFamily::raiseSaveServiceMessage(uint64_t peerID, Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IFamilyEventSink*)_eventHandler)->onSaveServiceMessage(peerID, data);
}

void DeviceFamily::raiseDeleteServiceMessage(uint64_t databaseID)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onDeleteServiceMessage(databaseID);
}

void DeviceFamily::raiseAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onAddWebserverEventHandler(eventHandler, eventHandlers);
}

void DeviceFamily::raiseRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onRemoveWebserverEventHandler(eventHandlers);
}

std::shared_ptr<Database::DataTable> DeviceFamily::raiseGetDevices()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IFamilyEventSink*)_eventHandler)->onGetDevices((uint32_t)_family);
}

void DeviceFamily::raiseDeleteDevice(uint64_t deviceID)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onDeleteDevice(deviceID);
}

uint64_t DeviceFamily::raiseSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family)
{
	if(!_eventHandler) return 0;
	return ((IFamilyEventSink*)_eventHandler)->onSaveDevice(id, address, serialNumber, type, family);
}

void DeviceFamily::raiseSaveDeviceVariable(Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IFamilyEventSink*)_eventHandler)->onSaveDeviceVariable(data);
}

void DeviceFamily::raiseDeletePeers(int32_t deviceID)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onDeletePeers(deviceID);
}

std::shared_ptr<Database::DataTable> DeviceFamily::raiseGetPeers(uint64_t deviceID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IFamilyEventSink*)_eventHandler)->onGetPeers(deviceID);
}

std::shared_ptr<Database::DataTable> DeviceFamily::raiseGetDeviceVariables(uint64_t deviceID)
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IFamilyEventSink*)_eventHandler)->onGetDeviceVariables(deviceID);
}

void DeviceFamily::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<PVariable>> values)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void DeviceFamily::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onRPCUpdateDevice(id, channel, address, hint);
}

void DeviceFamily::raiseRPCNewDevices(PVariable deviceDescriptions)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onRPCNewDevices(deviceDescriptions);
}

void DeviceFamily::raiseRPCDeleteDevices(PVariable deviceAddresses, PVariable deviceInfo)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onRPCDeleteDevices(deviceAddresses, deviceInfo);
}

void DeviceFamily::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<PVariable>> values)
{
	if(_eventHandler) ((IFamilyEventSink*)_eventHandler)->onEvent(peerID, channel, variables, values);
}

int32_t DeviceFamily::raiseIsAddonClient(int32_t clientID)
{
	if(_eventHandler) return ((IFamilyEventSink*)_eventHandler)->onIsAddonClient(clientID);
	return -1;
}
//End event handling

//Device event handling
void DeviceFamily::onCreateSavepointSynchronous(std::string name)
{
	raiseCreateSavepointSynchronous(name);
}

void DeviceFamily::onReleaseSavepointSynchronous(std::string name)
{
	raiseReleaseSavepointSynchronous(name);
}

void DeviceFamily::onCreateSavepointAsynchronous(std::string name)
{
	raiseCreateSavepointAsynchronous(name);
}

void DeviceFamily::onReleaseSavepointAsynchronous(std::string name)
{
	raiseReleaseSavepointAsynchronous(name);
}

void DeviceFamily::onDeleteMetadata(uint64_t peerID, std::string serialNumber, std::string dataID)
{
	raiseDeleteMetadata(peerID, serialNumber, dataID);
}

void DeviceFamily::onDeletePeer(uint64_t id)
{
	raiseDeletePeer(id);
}

uint64_t DeviceFamily::onSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	return raiseSavePeer(id, parentID, address, serialNumber);
}

void DeviceFamily::onSavePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	raiseSavePeerParameter(peerID, data);
}

void DeviceFamily::onSavePeerVariable(uint64_t peerID, Database::DataRow& data)
{
	raiseSavePeerVariable(peerID, data);
}

std::shared_ptr<BaseLib::Database::DataTable> DeviceFamily::onGetPeerParameters(uint64_t peerID)
{
	return raiseGetPeerParameters(peerID);
}

std::shared_ptr<BaseLib::Database::DataTable> DeviceFamily::onGetPeerVariables(uint64_t peerID)
{
	return raiseGetPeerVariables(peerID);
}

void DeviceFamily::onDeletePeerParameter(uint64_t peerID, Database::DataRow& data)
{
	raiseDeletePeerParameter(peerID, data);
}

bool DeviceFamily::onSetPeerID(uint64_t oldPeerID, uint64_t newPeerID)
{
	return raiseSetPeerID(oldPeerID, newPeerID);
}

std::shared_ptr<BaseLib::Database::DataTable> DeviceFamily::onGetServiceMessages(uint64_t peerID)
{
	return raiseGetServiceMessages(peerID);
}

void DeviceFamily::onSaveServiceMessage(uint64_t peerID, Database::DataRow& data)
{
	raiseSaveServiceMessage(peerID, data);
}

void DeviceFamily::onDeleteServiceMessage(uint64_t databaseID)
{
	raiseDeleteServiceMessage(databaseID);
}

void DeviceFamily::onAddWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler, std::map<int32_t, PEventHandler>& eventHandlers)
{
	raiseAddWebserverEventHandler(eventHandler, eventHandlers);
}

void DeviceFamily::onRemoveWebserverEventHandler(std::map<int32_t, PEventHandler>& eventHandlers)
{
	raiseRemoveWebserverEventHandler(eventHandlers);
}

uint64_t DeviceFamily::onSaveDevice(uint64_t id, int32_t address, std::string serialNumber, uint32_t type, uint32_t family)
{
	return raiseSaveDevice(id, address, serialNumber, type, family);
}

void DeviceFamily::onSaveDeviceVariable(Database::DataRow& data)
{
	raiseSaveDeviceVariable(data);
}

void DeviceFamily::onDeletePeers(int32_t deviceID)
{
	raiseDeletePeers(deviceID);
}

std::shared_ptr<BaseLib::Database::DataTable> DeviceFamily::onGetPeers(uint64_t deviceID)
{
	return raiseGetPeers(deviceID);
}

std::shared_ptr<BaseLib::Database::DataTable> DeviceFamily::onGetDeviceVariables(uint64_t deviceID)
{
	return raiseGetDeviceVariables(deviceID);
}

void DeviceFamily::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<PVariable>> values)
{
	raiseRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void DeviceFamily::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	raiseRPCUpdateDevice(id, channel, address, hint);
}

void DeviceFamily::onRPCNewDevices(PVariable deviceDescriptions)
{
	raiseRPCNewDevices(deviceDescriptions);
}

void DeviceFamily::onRPCDeleteDevices(PVariable deviceAddresses, PVariable deviceInfo)
{
	raiseRPCDeleteDevices(deviceAddresses, deviceInfo);
}

void DeviceFamily::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<PVariable>> values)
{
	raiseEvent(peerID, channel, variables, values);
}

int32_t DeviceFamily::onIsAddonClient(int32_t clientID)
{
	return raiseIsAddonClient(clientID);
}
//End Device event handling

void DeviceFamily::save(bool full)
{
	try
	{
		_bl->out.printMessage("(Shutdown) => Saving devices");
		if(!_devicesMutex.try_lock_for(std::chrono::milliseconds(5000)))
		{
			_bl->out.printError("Error: Could not get device mutex. Saving devices anyway.");
		}
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			_bl->out.printMessage("(Shutdown) => Saving " + getName() + " device " + std::to_string((*i)->getID()));
			(*i)->save(full);
			(*i)->savePeers(full);
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
    _devicesMutex.unlock();
}

void DeviceFamily::add(std::shared_ptr<LogicalDevice> device)
{
	try
	{
		if(!device) return;
		_devicesMutex.lock();
		device->save(true);
		_devices.push_back(device);
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
    _devicesMutex.unlock();
}

std::vector<std::shared_ptr<LogicalDevice>> DeviceFamily::getDevices()
{
	try
	{
		_devicesMutex.lock();
		std::vector<std::shared_ptr<LogicalDevice>> devices;
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			devices.push_back(*i);
		}
		_devicesMutex.unlock();
		return devices;
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
    _devicesMutex.unlock();
	return std::vector<std::shared_ptr<LogicalDevice>>();
}

std::shared_ptr<LogicalDevice> DeviceFamily::get(int32_t address)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getAddress() == address)
			{
				_devicesMutex.unlock();
				return (*i);
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
    _devicesMutex.unlock();
	return std::shared_ptr<LogicalDevice>();
}

std::shared_ptr<LogicalDevice> DeviceFamily::get(uint64_t id)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getID() == id)
			{
				_devicesMutex.unlock();
				return (*i);
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
    _devicesMutex.unlock();
	return std::shared_ptr<LogicalDevice>();
}

std::shared_ptr<LogicalDevice> DeviceFamily::get(std::string serialNumber)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getSerialNumber() == serialNumber)
			{
				_devicesMutex.unlock();
				return (*i);
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
    _devicesMutex.unlock();
	return std::shared_ptr<LogicalDevice>();
}

void DeviceFamily::remove(uint64_t id)
{
	try
	{
		_removeThreadMutex.lock();
		if(_disposed)
		{
			_removeThreadMutex.unlock();
			return;
		}
		if(_removeThread.joinable()) _removeThread.join();
		_removeThread = std::thread(&DeviceFamily::removeThread, this, id);
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
    _removeThreadMutex.unlock();
}

void DeviceFamily::removeThread(uint64_t id)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getID() == id)
			{
				_bl->out.printDebug("Removing device pointer from device array...");
				std::shared_ptr<LogicalDevice> device = *i;
				_devices.erase(i);
				_devicesMutex.unlock();
				_bl->out.printDebug("Disposing device...");
				device->dispose(true);
				_bl->out.printDebug("Deleting peers from database...");
				device->deletePeersFromDatabase();
				_bl->out.printDebug("Deleting database entry...");
				raiseDeleteDevice(id);
				return;
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
    _devicesMutex.unlock();
}

void DeviceFamily::dispose()
{
	try
	{
		if(_disposed) return;
		_disposed = true;
		if(!_devices.empty())
		{
			std::vector<std::shared_ptr<LogicalDevice>> devices;
			_devicesMutex.lock();
			for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
			{
				devices.push_back(*i);
			}
			_devicesMutex.unlock();
			for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = devices.begin(); i != devices.end(); ++i)
			{
				_bl->out.printDebug("Debug: Disposing device " + std::to_string((*i)->getID()));
				(*i)->dispose(false);
			}
		}
		_removeThreadMutex.lock();
		if(_removeThread.joinable()) _removeThread.join();
		_removeThreadMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_devicesMutex.unlock();
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_devicesMutex.unlock();
        _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void DeviceFamily::homegearStarted()
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
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
    _devicesMutex.unlock();
}

void DeviceFamily::homegearShuttingDown()
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
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
    _devicesMutex.unlock();
}

}
}
