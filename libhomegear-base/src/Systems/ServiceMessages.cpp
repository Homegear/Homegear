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

#include "ServiceMessages.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

ServiceMessages::ServiceMessages(BaseLib::Obj* baseLib, uint64_t peerID, std::string peerSerial, IServiceEventSink* eventHandler)
{
	_bl = baseLib;
	_peerID = peerID;
	_peerSerial = peerSerial;
	setEventHandler(eventHandler);
	_configPendingSetTime = _bl->hf.getTime();
}

ServiceMessages::~ServiceMessages()
{

}

//Event handling
void ServiceMessages::raiseConfigPending(bool configPending)
{
	if(_eventHandler) ((IServiceEventSink*)_eventHandler)->onConfigPending(configPending);
}

void ServiceMessages::raiseRPCEvent(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<PVariable>> values)
{
	if(id == 0) return;
	if(_eventHandler) ((IServiceEventSink*)_eventHandler)->onRPCEvent(id, channel, deviceAddress, valueKeys, values);
}

void ServiceMessages::raiseSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data)
{
	if(_eventHandler) ((IServiceEventSink*)_eventHandler)->onSaveParameter(name, channel, data);
}

std::shared_ptr<Database::DataTable> ServiceMessages::raiseGetServiceMessages()
{
	if(!_eventHandler) return std::shared_ptr<Database::DataTable>();
	return ((IServiceEventSink*)_eventHandler)->onGetServiceMessages();
}

void ServiceMessages::raiseSaveServiceMessage(Database::DataRow& data)
{
	if(!_eventHandler) return;
	((IServiceEventSink*)_eventHandler)->onSaveServiceMessage(data);
}

void ServiceMessages::raiseDeleteServiceMessage(uint64_t databaseID)
{
	((IServiceEventSink*)_eventHandler)->onDeleteServiceMessage(databaseID);
}

void ServiceMessages::raiseEnqueuePendingQueues()
{
	if(_eventHandler) ((IServiceEventSink*)_eventHandler)->onEnqueuePendingQueues();
}
//End event handling

void ServiceMessages::load()
{
	try
	{
		std::shared_ptr<BaseLib::Database::DataTable> rows = raiseGetServiceMessages();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			if(row->second.at(2)->intValue < 1000)
			{
				switch(row->second.at(2)->intValue)
				{
				/*case 0:
					_unreach = (bool)row->second.at(3)->intValue;
					break;*/
				case 1:
					_stickyUnreach = (bool)row->second.at(3)->intValue;
					break;
				case 2:
					_configPending = (bool)row->second.at(3)->intValue;
					break;
				case 3:
					_lowbat = (bool)row->second.at(3)->intValue;
					break;
				}
			}
			else
			{
				int32_t channel = row->second.at(3)->intValue;
				std::string id = row->second.at(4)->textValue;
				std::shared_ptr<std::vector<char>> value = row->second.at(5)->binaryValue;
				if(channel < 0 || id.empty() || value->empty()) continue;
				_errorMutex.lock();
				_errors[channel][id] = (uint8_t)value->at(0);
				_errorMutex.unlock();
			}
		}
		_unreach = false; //Always set _unreach to false on start up.

		//Synchronize service message data with peer parameters:
		std::vector<uint8_t> data = { (uint8_t)_unreach };
		raiseSaveParameter("UNREACH", 0, data);
		data = { (uint8_t)_stickyUnreach };
		raiseSaveParameter("STICKY_UNREACH", 0, data);
		data = { (uint8_t)_configPending };
		raiseSaveParameter("CONFIG_PENDING", 0, data);
		data = { (uint8_t)_lowbat };
		raiseSaveParameter("LOWBAT", 0, data);
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

void ServiceMessages::save(uint32_t index, bool value)
{
	try
	{
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		Database::DataRow data;
		if(value || !idIsKnown)
		{
			if(idIsKnown)
			{
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn((int32_t)value)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
				raiseSaveServiceMessage(data);
			}
			else
			{
				if(_peerID == 0) return;
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn((int32_t)value)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn()));
				raiseSaveServiceMessage(data);
			}
		}
		else if(idIsKnown)
		{
			raiseDeleteServiceMessage(_variableDatabaseIDs[index]);
			_variableDatabaseIDs.erase(index);
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

void ServiceMessages::save(int32_t channel, std::string id, uint8_t value)
{
	try
	{
		uint32_t index = 1000;
		for(std::string::iterator i = id.begin(); i != id.end(); ++i)
		{
			index += (int32_t)*i;
		}
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		if(value > 0 || !idIsKnown)
		{
			std::vector<char> binaryValue{ (char)value };
			Database::DataRow data;
			if(idIsKnown)
			{
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(channel)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(id)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_variableDatabaseIDs[index])));
				raiseSaveServiceMessage(data);
			}
			else
			{
				if(_peerID == 0) return;
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(_peerID)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(index)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(channel)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(id)));
				data.push_back(std::shared_ptr<Database::DataColumn>(new Database::DataColumn(binaryValue)));
				raiseSaveServiceMessage(data);
			}
		}
		else if(idIsKnown)
		{
			raiseDeleteServiceMessage(_variableDatabaseIDs[index]);
			_variableDatabaseIDs.erase(index);
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

bool ServiceMessages::set(std::string id, bool value)
{
	try
	{
		if(_disposing) return false;
		if(id == "LOWBAT_REPORTING") id = "LOWBAT"; //HM-TC-IT-WM-W-EU
		if(id == "UNREACH" && value != _unreach)
		{
			if(value && (_bl->booting || _bl->shuttingDown)) return true;
			_unreach = value;
			save(0, value);
		}
		else if(id == "STICKY_UNREACH" && value != _stickyUnreach)
		{
			if(value && (_bl->booting || _bl->shuttingDown)) return true;
			_stickyUnreach = value;
			save(1, value);
		}
		else if(id == "CONFIG_PENDING" && value != _configPending)
		{
			_configPending = value;
			save(2, value);
			if(_configPending) _configPendingSetTime = _bl->hf.getTime();
		}
		else if((id == "LOWBAT") && value != _lowbat)
		{
			_lowbat = value;
			save(3, value);
		}
		else if(!value) //false == 0, a little dirty, but it works
		{
			_errorMutex.lock();
			for(std::map<uint32_t, std::map<std::string, uint8_t>>::iterator i = _errors.begin(); i != _errors.end(); ++i)
			{
				if(i->second.find(id) != i->second.end())
				{
					i->second.at(id) = 0;
					std::vector<uint8_t> data = { (uint8_t)value };
					save(i->first, id, value);
					raiseSaveParameter(id, i->first, data);

					std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({id}));
					std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
					rpcValues->push_back(PVariable(new Variable((int32_t)0)));
					raiseRPCEvent(_peerID, 0, _peerSerial + ":" + std::to_string(i->first), valueKeys, rpcValues);
				}
			}
			_errorMutex.unlock();
			return true;
		}

		std::vector<uint8_t> data = { (uint8_t)value };
		raiseSaveParameter(id, 0, data);

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({id}));
		std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
		rpcValues->push_back(PVariable(new Variable(value)));

		raiseRPCEvent(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
	}
	catch(const std::exception& ex)
    {
		_errorMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_errorMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_errorMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return true;
}

void ServiceMessages::set(std::string id, uint8_t value, uint32_t channel)
{
	try
	{
		if(_disposing) return;
		_errorMutex.lock();
		if(value == 0 && _errors.find(channel) != _errors.end() && _errors[channel].find(id) != _errors[channel].end())
		{
			_errors[channel].erase(id);
			if(_errors[channel].empty()) _errors.erase(channel);
		}
		if(value > 0) _errors[channel][id] = value;
		_errorMutex.unlock();
		save(channel, id, value);
		//RPC Broadcast is done in peer's packetReceived
	}
	catch(const std::exception& ex)
    {
		_errorMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_errorMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_errorMutex.unlock();
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

PVariable ServiceMessages::get(int32_t clientID, bool returnID)
{
	try
	{
		PVariable serviceMessages(new Variable(VariableType::tArray));
		if(returnID && _peerID == 0) return serviceMessages;
		PVariable array;
		if(_unreach)
		{
			array.reset(new Variable(VariableType::tArray));
			if(returnID)
			{
				array->arrayValue->push_back(PVariable(new Variable((int32_t)_peerID)));
				array->arrayValue->push_back(PVariable(new Variable((int32_t)0)));
			}
			else array->arrayValue->push_back(PVariable(new Variable(_peerSerial + ":0")));
			array->arrayValue->push_back(PVariable(new Variable(std::string("UNREACH"))));
			array->arrayValue->push_back(PVariable(new Variable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_stickyUnreach)
		{
			array.reset(new Variable(VariableType::tArray));
			if(returnID)
			{
				array->arrayValue->push_back(PVariable(new Variable((int32_t)_peerID)));
				array->arrayValue->push_back(PVariable(new Variable((int32_t)0)));
			}
			else array->arrayValue->push_back(PVariable(new Variable(_peerSerial + ":0")));
			array->arrayValue->push_back(PVariable(new Variable(std::string("STICKY_UNREACH"))));
			array->arrayValue->push_back(PVariable(new Variable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_configPending)
		{
			array.reset(new Variable(VariableType::tArray));
			if(returnID)
			{
				array->arrayValue->push_back(PVariable(new Variable((int32_t)_peerID)));
				array->arrayValue->push_back(PVariable(new Variable((int32_t)0)));
			}
			else array->arrayValue->push_back(PVariable(new Variable(_peerSerial + ":0")));
			array->arrayValue->push_back(PVariable(new Variable(std::string("CONFIG_PENDING"))));
			array->arrayValue->push_back(PVariable(new Variable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_lowbat)
		{
			array.reset(new Variable(VariableType::tArray));
			if(returnID)
			{
				array->arrayValue->push_back(PVariable(new Variable((int32_t)_peerID)));
				array->arrayValue->push_back(PVariable(new Variable((int32_t)0)));
			}
			else array->arrayValue->push_back(PVariable(new Variable(_peerSerial + ":0")));
			array->arrayValue->push_back(PVariable(new Variable(std::string("LOWBAT"))));
			array->arrayValue->push_back(PVariable(new Variable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		_errorMutex.lock();
		for(std::map<uint32_t, std::map<std::string, uint8_t>>::const_iterator i = _errors.begin(); i != _errors.end(); ++i)
		{
			for(std::map<std::string, uint8_t>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(j->second == 0) continue;
				array.reset(new Variable(VariableType::tArray));
				if(returnID)
				{
					array->arrayValue->push_back(PVariable(new Variable((int32_t)_peerID)));
					array->arrayValue->push_back(PVariable(new Variable(i->first)));
				}
				else array->arrayValue->push_back(PVariable(new Variable(_peerSerial + ":" + std::to_string(i->first))));
				array->arrayValue->push_back(PVariable(new Variable(j->first)));
				array->arrayValue->push_back(PVariable(new Variable((uint32_t)j->second)));
				serviceMessages->arrayValue->push_back(array);
			}
		}
		_errorMutex.unlock();
		return serviceMessages;
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
    _errorMutex.unlock();
    return Variable::createError(-32500, "Unknown application error.");
}

void ServiceMessages::checkUnreach(int32_t cyclicTimeout, uint32_t lastPacketReceived)
{
	try
	{
		if(_bl->booting || _bl->shuttingDown) return;
		uint32_t time = HelperFunctions::getTimeSeconds();
		if(cyclicTimeout > 0 && (time - lastPacketReceived) > (unsigned)cyclicTimeout && !_unreach)
		{
			_unreach = true;
			_stickyUnreach = true;

			std::vector<uint8_t> data = { 1 };
			raiseSaveParameter("UNREACH", 0, data);
			raiseSaveParameter("STICKY_UNREACH", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("UNREACH"), std::string("STICKY_UNREACH")}));
			std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
			rpcValues->push_back(PVariable(new Variable(true)));
			rpcValues->push_back(PVariable(new Variable(true)));

			raiseRPCEvent(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

void ServiceMessages::endUnreach()
{
	try
	{
		if(_unreach == true)
		{
			_unreach = false;
			_unreachResendCounter = 0;

			std::vector<uint8_t> data = { 0 };
			raiseSaveParameter("UNREACH", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("UNREACH")}));
			std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
			rpcValues->push_back(PVariable(new Variable(false)));

			raiseRPCEvent(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

void ServiceMessages::resetConfigPendingSetTime()
{
	try
	{
		_configPendingSetTime = _bl->hf.getTime();
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

void ServiceMessages::setConfigPending(bool value)
{
	try
	{
		if(value != _configPending)
		{
			_configPending = value;
			save(2, value);
			if(_configPending) _configPendingSetTime = _bl->hf.getTime();
			std::vector<uint8_t> data = { (uint8_t)value };
			raiseSaveParameter("CONFIG_PENDING", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("CONFIG_PENDING")}));
			std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
			rpcValues->push_back(PVariable(new Variable(value)));

			raiseRPCEvent(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
			raiseConfigPending(value);
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

void ServiceMessages::setUnreach(bool value, bool requeue)
{
	try
	{
		if(_disposing || (value && (_bl->booting || _bl->shuttingDown))) return;
		if(value != _unreach)
		{
			if(value == true && requeue && _unreachResendCounter < 3)
			{
				raiseEnqueuePendingQueues();
				_unreachResendCounter++;
				return;
			}
			_unreachResendCounter = 0;
			_unreach = value;
			save(0, value);

			if(value) _bl->out.printInfo("Info: Peer " + std::to_string(_peerID) + " is unreachable.");
			std::vector<uint8_t> data = { (uint8_t)value };
			raiseSaveParameter("UNREACH", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("UNREACH")}));
			std::shared_ptr<std::vector<PVariable>> rpcValues(new std::vector<PVariable>());
			rpcValues->push_back(PVariable(new Variable(value)));

			if(value)
			{
				_stickyUnreach = value;
				save(1, value);
				raiseSaveParameter("STICKY_UNREACH", 0, data);

				valueKeys->push_back("STICKY_UNREACH");
				rpcValues->push_back(PVariable(new Variable(true)));
			}

			raiseRPCEvent(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

}
}
