/* Copyright 2013 Sathya Laufer
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

#include "EventHandler.h"
#include "HelperFunctions.h"
#include "GD.h"

//TODO Thread to reset events to execute timed events
//TODO Save to database (newEvents and lastRaised and lastValue)
//TODO Load from database on startup

EventHandler::EventHandler()
{
}

EventHandler::~EventHandler()
{
	_disposing = true;
	_stopThread = true;
	uint32_t i = 0;
	while(_triggerThreadCount > 0 && i < 300)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); //TODO Don't sleep when we processed event
	}
	if(_mainThread.joinable()) _mainThread.join();
}

void EventHandler::mainThread()
{
	while(!_stopThread && !_disposing)
	{
		try
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(300));



			_mainThreadMutex.lock();
			if(_timedEvents.empty() && _eventsToReset.empty()) _stopThread = true;
			_mainThreadMutex.unlock();
		}
		catch(const std::exception& ex)
		{
			_mainThreadMutex.unlock();
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(Exception& ex)
		{
			_mainThreadMutex.unlock();
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			_mainThreadMutex.unlock();
			HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
}

std::shared_ptr<RPC::RPCVariable> EventHandler::add(std::shared_ptr<RPC::RPCVariable> eventDescription)
{
	try
	{
		if(eventDescription->type != RPC::RPCVariableType::rpcStruct) return RPC::RPCVariable::createError(-5, "Parameter is not of type Struct.");
		if(eventDescription->structValue->find("ID") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No id specified.");
		if(eventDescription->structValue->find("TYPE") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No type specified.");

		std::shared_ptr<Event> event(new Event());
		event->type = (Event::Type::Enum)eventDescription->structValue->at("TYPE")->integerValue;
		event->name = eventDescription->structValue->at("ID")->stringValue;

		if(eventDescription->structValue->find("EVENTMETHOD") == eventDescription->structValue->end() || eventDescription->structValue->at("EVENTMETHOD")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No event method specified.");
		event->eventMethod = eventDescription->structValue->at("EVENTMETHOD")->stringValue;
		if(eventDescription->structValue->find("EVENTMETHODPARAMS") != eventDescription->structValue->end())
		{
			event->eventMethodParameters = eventDescription->structValue->at("EVENTMETHODPARAMS");
		}

		if(event->type == Event::Type::Enum::triggered)
		{
			if(eventDescription->structValue->find("ADDRESS") == eventDescription->structValue->end() || eventDescription->structValue->at("ADDRESS")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No device address specified.");
			event->address = eventDescription->structValue->at("ADDRESS")->stringValue;
			if(eventDescription->structValue->find("VARIABLE") == eventDescription->structValue->end() || eventDescription->structValue->at("VARIABLE")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No variable specified.");
			event->variable = eventDescription->structValue->at("VARIABLE")->stringValue;
			if(eventDescription->structValue->find("TRIGGER") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No trigger specified.");
			event->trigger = (Event::Trigger::Enum)eventDescription->structValue->at("TRIGGER")->integerValue;
			if(eventDescription->structValue->find("TRIGGERVALUE") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No trigger value specified.");
			event->triggerValue = eventDescription->structValue->at("TRIGGERVALUE");

			if(eventDescription->structValue->find("RESETAFTER") != eventDescription->structValue->end())
			{
				if(eventDescription->structValue->find("RESETMETHOD") == eventDescription->structValue->end() || eventDescription->structValue->at("RESETMETHOD")->stringValue.empty()) return RPC::RPCVariable::createError(-5, "No reset method specified.");
				event->resetMethod = eventDescription->structValue->at("RESETMETHOD")->stringValue;
				if(eventDescription->structValue->find("RESETMETHODPARAMS") != eventDescription->structValue->end())
				{
					event->resetMethodParameters = eventDescription->structValue->at("RESETMETHODPARAMS");
				}

				if(eventDescription->structValue->at("RESETAFTER")->type == RPC::RPCVariableType::rpcInteger)
				{
					event->resetAfter = eventDescription->structValue->at("RESETAFTER")->integerValue;
				}
				else if(eventDescription->structValue->at("RESETAFTER")->type == RPC::RPCVariableType::rpcStruct)
				{
					std::shared_ptr<RPC::RPCVariable> resetStruct = eventDescription->structValue->at("RESETAFTER");
					if(resetStruct->structValue->find("INITIALTIME") == resetStruct->structValue->end() || resetStruct->structValue->at("INITIALTIME")->integerValue == 0) return RPC::RPCVariable::createError(-5, "Initial time in reset struct not specified.");
					event->initialTime = resetStruct->structValue->at("INITIALTIME")->integerValue;
					if(resetStruct->structValue->find("OPERATION") != resetStruct->structValue->end())
						event->operation = (Event::Operation::Enum)resetStruct->structValue->at("INITIALTIME")->integerValue;
					if(resetStruct->structValue->find("FACTOR") != resetStruct->structValue->end())
					{
						event->factor = resetStruct->structValue->at("FACTOR")->floatValue;
						if(event->factor <= 0) return RPC::RPCVariable::createError(-5, "Factor is less or equal 0. Please provide a positive value");
					}
					if(resetStruct->structValue->find("LIMIT") != resetStruct->structValue->end())
						event->limit = resetStruct->structValue->at("LIMIT")->integerValue;
					if(resetStruct->structValue->find("RESETAFTER") != resetStruct->structValue->end())
						event->resetAfter = resetStruct->structValue->at("RESETAFTER")->integerValue;
				}
			}
			_eventsMutex.lock();
			_triggeredEvents[event->address][event->variable].push_back(event);
			_eventsMutex.unlock();
		}
		else
		{
			if(eventDescription->structValue->find("EVENTTIME") == eventDescription->structValue->end()) return RPC::RPCVariable::createError(-5, "No time specified.");
			if(eventDescription->structValue->find("RECUREVERY") != eventDescription->structValue->end())
				event->recurEvery = eventDescription->structValue->at("RECUREVERY")->integerValue;
			int32_t nextExecution = getNextExecution(event->eventTime, event->recurEvery);
			_eventsMutex.lock();
			_timedEvents[nextExecution] = event;
			_eventsMutex.unlock();
			_mainThreadMutex.lock();
			if(_stopThread || !_mainThread.joinable())
			{
				_stopThread = false;
				_mainThread = std::thread(&EventHandler::mainThread, this);
			}
			_mainThreadMutex.unlock();
		}
		save(event);
		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
		_eventsMutex.unlock();
		_mainThreadMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_eventsMutex.unlock();
    	_mainThreadMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_eventsMutex.unlock();
    	_mainThreadMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> EventHandler::remove(std::string name)
{
	try
	{
		_eventsMutex.lock();
		//TODO Make sure removing and saving do not occur at the same time:
		//     Remove pointer from maps first. Then wait a little and remove
		//     Event from Database
		_eventsMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_eventsMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_eventsMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_eventsMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

void EventHandler::trigger(std::string& address, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	try
	{
		if(_disposing) return;
		std::thread t(&EventHandler::triggerThreadMultipleVariables, this, address, variables, values);
		HelperFunctions::setThreadPriority(t.native_handle(), 44);
		t.detach();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int32_t EventHandler::getNextExecution(int32_t startTime, int32_t recurEvery)
{
	try
	{
		int32_t time = HelperFunctions::getTimeSeconds();
		if(startTime >= time) return startTime;
		if(recurEvery == 0) return -1;
		int32_t difference = time - startTime;
		return time + (recurEvery - (difference % recurEvery));
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return -1;
}

void EventHandler::trigger(std::string& address, std::string& variable, std::shared_ptr<RPC::RPCVariable>& value)
{
	try
	{
		if(_disposing) return;
		std::thread t(&EventHandler::triggerThread, this, address, variable, value);
		HelperFunctions::setThreadPriority(t.native_handle(), 44);
		t.detach();
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void EventHandler::triggerThreadMultipleVariables(std::string address, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	_triggerThreadCount++;
	try
	{
		for(uint32_t i = 0; i < variables->size(); i++)
		{
			triggerThread(address, variables->at(i), values->at(i));
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
	_triggerThreadCount--;
}

void EventHandler::removeEventToReset(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<int32_t, std::shared_ptr<Event>>::iterator i = _eventsToReset.begin(); i != _eventsToReset.end(); ++i)
		{
			if(i->second->id == id)
			{
				_eventsToReset.erase(i);
				_eventsMutex.unlock();
				return;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _eventsMutex.unlock();
}

void EventHandler::removeTimeToReset(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<int32_t, std::shared_ptr<Event>>::iterator i = _timesToReset.begin(); i != _timesToReset.end(); ++i)
		{
			if(i->second->id == id)
			{
				_timesToReset.erase(i);
				_eventsMutex.unlock();
				return;
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _eventsMutex.unlock();
}

bool EventHandler::eventExists(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<int32_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->id == id)
			{
				_eventsMutex.unlock();
				return true;
			}
		}
		for(std::map<std::string, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator i = _triggeredEvents.begin(); i != _triggeredEvents.end(); ++i)
		{
			for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				for(std::vector<std::shared_ptr<Event>>::iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					if((*k)->id == id)
					{
						_eventsMutex.unlock();
						return true;
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _eventsMutex.unlock();
    return false;
}

void EventHandler::triggerThread(std::string address, std::string variable, std::shared_ptr<RPC::RPCVariable> value)
{
	_triggerThreadCount++;
	try
	{
		if(_triggeredEvents.find(address) == _triggeredEvents.end() || _triggeredEvents.at(address).find(variable) == _triggeredEvents.at(address).end() || !value)
		{
			_triggerThreadCount--;
			return;
		}
		for(std::vector<std::shared_ptr<Event>>::iterator i = _triggeredEvents.at(address).at(variable).begin(); i !=  _triggeredEvents.at(address).at(variable).end(); ++i)
		{
			std::shared_ptr<RPC::RPCVariable> result;
			if((*i)->trigger == Event::Trigger::update)
			{
				result = GD::rpcServers.begin()->second.callMethod((*i)->eventMethod, (*i)->eventMethodParameters);
			}
			else if((*i)->trigger == Event::Trigger::change)
			{
				if(*((*i)->lastValue) != *value)
				{
					result = GD::rpcServers.begin()->second.callMethod((*i)->eventMethod, (*i)->eventMethodParameters);
				}
			}
			else if((*i)->trigger == Event::Trigger::value)
			{
				if(*value == *((*i)->triggerValue))
				{
					result = GD::rpcServers.begin()->second.callMethod((*i)->eventMethod, (*i)->eventMethodParameters);
				}
			}
			else if((*i)->trigger == Event::Trigger::belowThreshold)
			{
				if(*value < *((*i)->triggerValue))
				{
					result = GD::rpcServers.begin()->second.callMethod((*i)->eventMethod, (*i)->eventMethodParameters);
				}
			}
			else if((*i)->trigger == Event::Trigger::aboveThreshold)
			{
				if(*value > *((*i)->triggerValue))
				{
					result = GD::rpcServers.begin()->second.callMethod((*i)->eventMethod, (*i)->eventMethodParameters);
				}
			}

			(*i)->lastRaised = HelperFunctions::getTime();
			(*i)->lastValue = value;
			if(result)
			{
				if(result->errorStruct)
				{
					HelperFunctions::printError("Could not execute RPC method for event from address " + address + " and variable " + variable + ". Error struct:");
					result->print();
				}
				else if((*i)->resetAfter > 0 || (*i)->initialTime > 0)
				{
					removeEventToReset((*i)->id);
					if((*i)->initialTime == 0) //Simple reset
					{
						_eventsMutex.lock();
						_eventsToReset[HelperFunctions::getTimeSeconds() + (*i)->resetAfter] =  *i;
						_eventsMutex.unlock();
					}
					else //Complex reset
					{
						removeTimeToReset((*i)->id);
						_eventsMutex.lock();
						_timesToReset[HelperFunctions::getTimeSeconds() + (*i)->resetAfter] = *i;
						_eventsMutex.unlock();
						if((*i)->currentTime == 0) (*i)->currentTime = (*i)->initialTime;
						if((*i)->factor <= 0)
						{
							HelperFunctions::printWarning("Warning: Factor is less or equal 0. Setting factor to 1. Event from address " + address + " and variable " + variable + ".");
							(*i)->factor = 1;
						}
						_eventsMutex.lock();
						_eventsToReset[HelperFunctions::getTimeSeconds() + (*i)->currentTime] =  *i;
						_eventsMutex.unlock();
						if((*i)->operation == Event::Operation::Enum::addition)
						{
							(*i)->currentTime += (*i)->factor;
							if((*i)->currentTime > (*i)->limit) (*i)->currentTime = (*i)->limit;
						}
						else if((*i)->operation == Event::Operation::Enum::subtraction)
						{
							(*i)->currentTime -= (*i)->factor;
							if((*i)->currentTime < (*i)->limit) (*i)->currentTime = (*i)->limit;
						}
						else if((*i)->operation == Event::Operation::Enum::multiplication)
						{
							(*i)->currentTime *= (*i)->factor;
							if((*i)->currentTime > (*i)->limit) (*i)->currentTime = (*i)->limit;
						}
						else if((*i)->operation == Event::Operation::Enum::division)
						{
							(*i)->currentTime /= (*i)->factor;
							if((*i)->currentTime < (*i)->limit) (*i)->currentTime = (*i)->limit;
						}
					}
					_mainThreadMutex.lock();
					if(_stopThread || !_mainThread.joinable())
					{
						_stopThread = false;
						_mainThread = std::thread(&EventHandler::mainThread, this);
					}
					_mainThreadMutex.unlock();
				}
				save(*i);
			}
		}
	}
	catch(const std::exception& ex)
    {
		_eventsMutex.unlock();
		_mainThreadMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_eventsMutex.unlock();
    	_mainThreadMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_eventsMutex.unlock();
    	_mainThreadMutex.unlock();
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _triggerThreadCount--;
}

void EventHandler::load()
{
	try
	{
		_databaseMutex.lock();
		DataTable rows = GD::db.executeCommand("SELECT * FROM events");
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			std::shared_ptr<Event> event(new Event());
			event->id = row->second.at(0)->intValue;
			event->name = row->second.at(1)->textValue;
			event->type = (Event::Type::Enum)row->second.at(2)->intValue;
			event->address = row->second.at(3)->textValue;
			event->variable = row->second.at(4)->textValue;
			event->trigger = (Event::Trigger::Enum)row->second.at(5)->intValue;
			event->eventMethod = row->second.at(6)->textValue;
			event->eventMethodParameters = _rpcDecoder.decodeResponse(row->second.at(7)->binaryValue);
			event->resetAfter = row->second.at(8)->intValue;
			event->initialTime = row->second.at(9)->intValue;
			event->operation = (Event::Operation::Enum)row->second.at(10)->intValue;
			event->factor = row->second.at(11)->floatValue;
			event->limit = row->second.at(12)->intValue;
			event->resetMethod = row->second.at(13)->textValue;
			event->resetMethodParameters = _rpcDecoder.decodeResponse(row->second.at(14)->binaryValue);
			event->eventTime = row->second.at(15)->intValue;
			event->recurEvery = row->second.at(16)->intValue;
			event->lastValue = _rpcDecoder.decodeResponse(row->second.at(17)->binaryValue);
			event->lastRaised = row->second.at(18)->intValue;
			event->currentTime = row->second.at(19)->intValue;
		}
		//TODO Initialize everything
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void EventHandler::save(std::shared_ptr<Event> event)
{
	try
	{
		//The eventExists is necessary so we don't safe an event that is being deleted
		if(!event || _disposing || (event->id > 0 && !eventExists(event->id))) return;
		_databaseMutex.lock();
		GD::db.executeCommand("SAVEPOINT event" + std::to_string(event->id));
		DataColumnVector data;
		if(event->id > 0) data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->id)));
		else data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->name)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn((int32_t)event->type)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->address)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->variable)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn((int32_t)event->trigger)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->eventMethod)));
		std::shared_ptr<std::vector<char>> value = _rpcEncoder.encodeResponse(event->eventMethodParameters);
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->resetAfter)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->initialTime)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn((int32_t)event->operation)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->factor)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->limit)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->resetMethod)));
		value = _rpcEncoder.encodeResponse(event->resetMethodParameters);
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->eventTime)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->recurEvery)));
		value = _rpcEncoder.encodeResponse(event->lastValue);
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->lastRaised)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(event->currentTime)));
		int32_t result = GD::db.executeWriteCommand("REPLACE INTO events VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)", data);
		if(event->id == 0) event->id = result;
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE event" + std::to_string(event->id));
    _databaseMutex.unlock();
}
