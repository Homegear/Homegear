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

#include "DeviceFamily.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

DeviceFamily::DeviceFamily(std::shared_ptr<Obj> baseLib, IFamilyEventSink* eventHandler)
{
	Obj::ins = baseLib;
	Output::setDebugLevel(baseLib->debugLevel);
	addEventHandler(eventHandler);
}

DeviceFamily::~DeviceFamily()
{
	try
	{
		if(_removeThread.joinable()) _removeThread.join();
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

//Event handling
void DeviceFamily::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IFamilyEventSink*)*i)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
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
    _eventHandlerMutex.unlock();
}

void DeviceFamily::raiseRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IFamilyEventSink*)*i)->onRPCUpdateDevice(id, channel, address, hint);
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
    _eventHandlerMutex.unlock();
}

void DeviceFamily::raiseRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IFamilyEventSink*)*i)->onRPCNewDevices(deviceDescriptions);
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
    _eventHandlerMutex.unlock();
}

void DeviceFamily::raiseRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IFamilyEventSink*)*i)->onRPCDeleteDevices(deviceAddresses, deviceInfo);
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
    _eventHandlerMutex.unlock();
}

void DeviceFamily::raiseEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IFamilyEventSink*)*i)->onEvent(peerID, channel, variables, values);
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
    _eventHandlerMutex.unlock();
}
//End event handling

//Device event handling
void DeviceFamily::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	try
	{
		raiseRPCEvent(id, channel, deviceAddress, valueKeys, values);
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

void DeviceFamily::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
{
	try
	{
		raiseRPCUpdateDevice(id, channel, address, hint);
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

void DeviceFamily::onRPCNewDevices(std::shared_ptr<RPC::RPCVariable> deviceDescriptions)
{
	try
	{
		raiseRPCNewDevices(deviceDescriptions);
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

void DeviceFamily::onRPCDeleteDevices(std::shared_ptr<RPC::RPCVariable> deviceAddresses, std::shared_ptr<RPC::RPCVariable> deviceInfo)
{
	try
	{
		raiseRPCDeleteDevices(deviceAddresses, deviceInfo);
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

void DeviceFamily::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
{
	try
	{
		raiseEvent(peerID, channel, variables, values);
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
//End Device event handling

bool DeviceFamily::available()
{
	if(!Obj::ins->physicalDevices.get(_family)) return false;
	return Obj::ins->physicalDevices.get(_family)->isOpen();
}

void DeviceFamily::save(bool full)
{
	try
	{
		Output::printMessage("(Shutdown) => Saving devices");
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			Output::printMessage("(Shutdown) => Saving " + getName() + " device " + std::to_string((*i)->getID()));
			(*i)->save(full);
			(*i)->savePeers(full);
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
    _devicesMutex.unlock();
	return std::shared_ptr<LogicalDevice>();
}

void DeviceFamily::remove(uint64_t id)
{
	try
	{
		if(_removeThread.joinable()) _removeThread.join();
		_removeThread = std::thread(&DeviceFamily::removeThread, this, id);
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

void DeviceFamily::removeThread(uint64_t id)
{
	try
	{
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			if((*i)->getID() == id)
			{
				Output::printDebug("Removing device pointer from device array...");
				std::shared_ptr<LogicalDevice> device = *i;
				_devices.erase(i);
				_devicesMutex.unlock();
				Output::printDebug("Disposing device...");
				device->dispose(true);
				Output::printDebug("Deleting peers from database...");
				device->deletePeersFromDatabase();
				Output::printDebug("Deleting database entry...");
				DataColumnVector data;
				data.push_back(std::shared_ptr<DataColumn>(new DataColumn(id)));
				Obj::ins->db.executeCommand("DELETE FROM devices WHERE deviceID=?", data);
				return;
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
    _devicesMutex.unlock();
}

void DeviceFamily::dispose()
{
	try
	{
		if(_devices.empty()) return;
		std::vector<std::shared_ptr<LogicalDevice>> devices;
		_devicesMutex.lock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			devices.push_back(*i);
		}
		_devicesMutex.unlock();
		for(std::vector<std::shared_ptr<LogicalDevice>>::iterator i = devices.begin(); i != devices.end(); ++i)
		{
			Output::printDebug("Debug: Disposing device " + std::to_string((*i)->getID()));
			(*i)->dispose(false);
		}
	}
	catch(const std::exception& ex)
    {
		_devicesMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_devicesMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_devicesMutex.unlock();
        Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

}
}
