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

LogicalDevice::LogicalDevice(IDeviceEventSink* eventHandler)
{
	addEventHandler(eventHandler);
}

LogicalDevice::LogicalDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler) : LogicalDevice(eventHandler)
{
	_deviceID = deviceID;
	_serialNumber = serialNumber;
	_address = address;
}

LogicalDevice::~LogicalDevice()
{
}

//Event handling
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

//Peer event handling
void LogicalDevice::onRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
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

void LogicalDevice::onRPCUpdateDevice(uint64_t id, int32_t channel, std::string address, int32_t hint)
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

void LogicalDevice::onEvent(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> values)
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
//End Peer event handling

DeviceFamilies LogicalDevice::deviceFamily()
{
	return Obj::family->getFamily();
}

void LogicalDevice::load()
{
	try
	{
		loadVariables();
	}
    catch(const std::exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void LogicalDevice::save(bool saveDevice)
{
	try
	{
		if(saveDevice)
		{
			_databaseMutex.lock();
			DataColumnVector data;
			if(_deviceID > 0) data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			else data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_address)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_serialNumber)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceType)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn((uint32_t)Obj::family->getFamily())));
			int32_t result = Obj::ins->db.executeWriteCommand("REPLACE INTO devices VALUES(?, ?, ?, ?, ?)", data);
			if(_deviceID == 0) _deviceID = result;
			_databaseMutex.unlock();
		}
		saveVariables();
	}
    catch(const std::exception& ex)
    {
    	_databaseMutex.unlock();
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_databaseMutex.unlock();
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_databaseMutex.unlock();
        BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void LogicalDevice::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET integerValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void LogicalDevice::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(stringValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET stringValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(stringValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void LogicalDevice::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		_databaseMutex.lock();
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			Obj::ins->db.executeWriteCommand("UPDATE deviceVariables SET binaryValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_deviceID == 0)
			{
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_deviceID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(binaryValue)));
			int32_t result = Obj::ins->db.executeWriteCommand("REPLACE INTO deviceVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void LogicalDevice::deletePeersFromDatabase()
{
	try
	{
		_databaseMutex.lock();
		std::ostringstream command;
		command << "DELETE FROM peers WHERE parent=" << std::dec << _deviceID;
		Obj::ins->db.executeCommand(command.str());
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

}
}
