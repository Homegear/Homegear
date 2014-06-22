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
	addEventHandler(eventHandler);
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

//Event handling
void LogicalDevice::raiseCreateSavepoint(std::string name)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onCreateSavepoint(name);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseReleaseSavepoint(std::string name)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onReleaseSavepoint(name);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseDeleteMetadata(std::string objectID, std::string dataID)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onDeleteMetadata(objectID, dataID);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseDeletePeer(uint64_t id)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onDeletePeer(id);
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
    _eventHandlerMutex.unlock();
}

uint64_t LogicalDevice::raiseSavePeer(uint64_t id, uint32_t parentID, int32_t address, std::string serialNumber)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				uint64_t result = ((IDeviceEventSink*)*i)->onSavePeer(id, parentID, address, serialNumber);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return 0;
}

uint64_t LogicalDevice::raiseSavePeerParameter(uint64_t peerID, Database::DataRow data)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				uint64_t result = ((IDeviceEventSink*)*i)->onSavePeerParameter(peerID, data);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return 0;
}

uint64_t LogicalDevice::raiseSavePeerVariable(uint64_t peerID, Database::DataRow data)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				uint64_t result = ((IDeviceEventSink*)*i)->onSavePeerVariable(peerID, data);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return 0;
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetPeerParameters(uint64_t peerID)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				std::shared_ptr<Database::DataTable> result = ((IDeviceEventSink*)*i)->onGetPeerParameters(peerID);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return std::shared_ptr<Database::DataTable>();
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetPeerVariables(uint64_t peerID)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				std::shared_ptr<Database::DataTable> result = ((IDeviceEventSink*)*i)->onGetPeerVariables(peerID);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return std::shared_ptr<Database::DataTable>();
}

void LogicalDevice::raiseDeletePeerParameter(uint64_t peerID, Database::DataRow data)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onDeletePeerParameter(peerID, data);
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
    _eventHandlerMutex.unlock();
}

uint64_t LogicalDevice::raiseSaveDevice()
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				uint64_t result = ((IDeviceEventSink*)*i)->onSaveDevice(_deviceID, _address, _serialNumber, _deviceType, (uint32_t)_deviceFamily);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return 0;
}

uint64_t LogicalDevice::raiseSaveDeviceVariable(Database::DataRow data)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				int32_t result = ((IDeviceEventSink*)*i)->onSaveDeviceVariable(data);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return 0;
}

void LogicalDevice::raiseDeletePeers(int32_t deviceID)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onDeletePeers(deviceID);
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
    _eventHandlerMutex.unlock();
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetPeers()
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				std::shared_ptr<Database::DataTable> result = ((IDeviceEventSink*)*i)->onGetPeers(_deviceID);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return std::shared_ptr<Database::DataTable>();
}

std::shared_ptr<Database::DataTable> LogicalDevice::raiseGetDeviceVariables()
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i)
			{
				std::shared_ptr<Database::DataTable> result = ((IDeviceEventSink*)*i)->onGetDeviceVariables(_deviceID);
				_eventHandlerMutex.unlock();
				return result;
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
    _eventHandlerMutex.unlock();
    return std::shared_ptr<Database::DataTable>();
}

void LogicalDevice::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onRPCUpdateDevice(id, channel, address, hint);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onRPCNewDevices(deviceDescriptions);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onRPCDeleteDevices(deviceAddresses, deviceInfo);
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
    _eventHandlerMutex.unlock();
}

void LogicalDevice::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::forward_list<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IDeviceEventSink*)*i)->onEvent(peerID, channel, variables, values);
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
    _eventHandlerMutex.unlock();
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
