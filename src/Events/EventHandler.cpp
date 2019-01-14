/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "EventHandler.h"

#ifdef EVENTHANDLER
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace Homegear
{

EventHandler::EventHandler() : BaseLib::IQueue(GD::bl.get(), 1, 1000)
{
	_disposing = false;
	_stopThread = false;

	_dummyClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
	_dummyClientInfo->scriptEngineServer = true;
	_dummyClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
	std::vector<uint64_t> groups{5};
	_dummyClientInfo->acls->fromGroups(groups);
	_dummyClientInfo->user = "SYSTEM (5)";
}

EventHandler::~EventHandler()
{
	dispose();
}

void EventHandler::dispose()
{
	if(_disposing) return;
	_disposing = true;
	GD::bl->threadManager.join(_mainThread);
	stopQueue(0);
	_timedEvents.clear();
	_triggeredEvents.clear();
	_eventsToReset.clear();
	_timesToReset.clear();
	_rpcDecoder.reset();
	_rpcEncoder.reset();
}

void EventHandler::init()
{
	_rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get()));
	_rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), false, true));

	startQueue(0, false, GD::bl->settings.eventThreadCount(), GD::bl->settings.eventThreadPriority(), GD::bl->settings.eventThreadPolicy());
}

void EventHandler::mainThread()
{
	while(!_stopThread && !_disposing)
	{
		try
		{
			uint64_t currentTime = BaseLib::HelperFunctions::getTime();
			if(!GD::rpcServers.begin()->second->isRunning())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
				continue;
			}
			_eventsMutex.lock();
			if(!_timedEvents.empty() && _timedEvents.begin()->first <= currentTime)
			{
				std::shared_ptr<Event> event = _timedEvents.begin()->second;
				_eventsMutex.unlock();
				if(event->enabled)
				{
					std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(event->name, event->eventMethod, event->eventMethodParameters));
					enqueue(0, queueEntry);
					event->lastRaised = currentTime;
				}
				save(event);
				if(event->recurEvery == 0 || (event->endTime > 0 && currentTime >= event->endTime))
				{
					GD::out.printInfo("Info: Removing event " + event->name + ", because the end time is reached.");
					remove(event->name);
				}
				else if(event->recurEvery > 0)
				{
					uint64_t nextExecution = getNextExecution(event->eventTime, event->recurEvery);
					_eventsMutex.lock();
					if(event->enabled) GD::out.printInfo("Info: Next execution for event " + event->name + ": " + std::to_string(nextExecution));
					//We don't call removeTimedEvents here, so we don't need to release the lock. Otherwise there is the possibility that the event
					//is recreated after being deleted.
					for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
					{
						if(i->second->id == event->id)
						{
							_timedEvents.erase(i);
							break;
						}
					}
					while(_timedEvents.find(nextExecution) != _timedEvents.end()) nextExecution++;
					_timedEvents[nextExecution] = event;
					_eventsMutex.unlock();
					if(event->enabled) GD::rpcClient->broadcastUpdateEvent(event->name, (int32_t) event->type, event->peerID, event->peerChannel, event->variable);
				}
				else removeTimedEvent(event->id);
			}
			else if(!_eventsToReset.empty() && _eventsToReset.begin()->first <= currentTime)
			{
				std::shared_ptr<Event> event = _eventsToReset.begin()->second;
				_eventsMutex.unlock();

				GD::out.printInfo("Info: Resetting event " + event->name + ".");
				std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(event->name, event->resetMethod, event->resetMethodParameters));
				enqueue(0, queueEntry);
				event->lastReset = currentTime;
				removeEventToReset(event->id);
				save(event);
				GD::rpcClient->broadcastUpdateEvent(event->name, (int32_t) event->type, event->peerID, event->peerChannel, event->variable);
			}
			else if(!_timesToReset.empty() && _timesToReset.begin()->first <= currentTime)
			{
				std::shared_ptr<Event> event = _timesToReset.begin()->second;
				_eventsMutex.unlock();
				GD::out.printInfo("Info: Resetting initial time for event " + event->name + ".");
				removeTimeToReset(event->id);
				event->lastReset = currentTime;
				event->currentTime = 0;
				save(event);
				GD::rpcClient->broadcastUpdateEvent(event->name, (int32_t) event->type, event->peerID, event->peerChannel, event->variable);
			}
			else
			{
				_eventsMutex.unlock();
				std::this_thread::sleep_for(std::chrono::milliseconds(300));
			}

			std::lock_guard<std::mutex> mainThreadGuard(_mainThreadMutex);
			if(_timedEvents.empty() && _eventsToReset.empty() && _timesToReset.empty()) _stopThread = true;
		}
		catch(const std::exception& ex)
		{
			_eventsMutex.unlock();
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(BaseLib::Exception& ex)
		{
			_eventsMutex.unlock();
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			_eventsMutex.unlock();
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
}

BaseLib::PVariable EventHandler::add(BaseLib::PVariable eventDescription)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		if(eventDescription->type != BaseLib::VariableType::tStruct) return BaseLib::Variable::createError(-5, "Parameter is not of type Struct.");
		if(eventDescription->structValue->find("ID") == eventDescription->structValue->end()) return BaseLib::Variable::createError(-5, "No id specified.");
		if(eventDescription->structValue->find("TYPE") == eventDescription->structValue->end()) return BaseLib::Variable::createError(-5, "No type specified.");

		std::shared_ptr<Event> event(new Event());
		event->type = (Event::Type::Enum) eventDescription->structValue->at("TYPE")->integerValue;
		event->name = eventDescription->structValue->at("ID")->stringValue;
		if(event->name.size() > 100) return BaseLib::Variable::createError(-5, "Event ID is too long.");
		bool replace = false;
		if(eventExists(event->name))
		{
			auto descriptionIterator = eventDescription->structValue->find("REPLACE");
			if(descriptionIterator != eventDescription->structValue->end() && descriptionIterator->second->booleanValue) replace = true;
			else return BaseLib::Variable::createError(-5, "An event with this ID already exists.");
		}

		if(eventDescription->structValue->find("EVENTMETHOD") == eventDescription->structValue->end() || eventDescription->structValue->at("EVENTMETHOD")->stringValue.empty()) return BaseLib::Variable::createError(-5, "No event method specified.");
		event->eventMethod = eventDescription->structValue->at("EVENTMETHOD")->stringValue;
		if(event->eventMethod.size() > 100) return BaseLib::Variable::createError(-5, "Event method string is too long.");
		if(eventDescription->structValue->find("EVENTMETHODPARAMS") != eventDescription->structValue->end())
		{
			event->eventMethodParameters = eventDescription->structValue->at("EVENTMETHODPARAMS");
		}

		if(eventDescription->structValue->find("ENABLED") != eventDescription->structValue->end()) event->enabled = eventDescription->structValue->at("ENABLED")->booleanValue;

		if(event->type == Event::Type::Enum::triggered)
		{
			if(eventDescription->structValue->find("PEERID") == eventDescription->structValue->end()) event->peerID = 0;
			else event->peerID = eventDescription->structValue->at("PEERID")->integerValue;
			if(eventDescription->structValue->find("PEERCHANNEL") != eventDescription->structValue->end()) event->peerChannel = eventDescription->structValue->at("PEERCHANNEL")->integerValue;

			if(eventDescription->structValue->find("VARIABLE") == eventDescription->structValue->end() || eventDescription->structValue->at("VARIABLE")->stringValue.empty()) return BaseLib::Variable::createError(-5, "No variable specified.");
			event->variable = eventDescription->structValue->at("VARIABLE")->stringValue;
			if(event->variable.size() > 100) return BaseLib::Variable::createError(-5, "Variable string is too long.");
			if(eventDescription->structValue->find("TRIGGER") == eventDescription->structValue->end()) return BaseLib::Variable::createError(-5, "No trigger specified.");
			event->trigger = (Event::Trigger::Enum) eventDescription->structValue->at("TRIGGER")->integerValue;
			if(eventDescription->structValue->find("TRIGGERVALUE") != eventDescription->structValue->end())
				event->triggerValue = eventDescription->structValue->at("TRIGGERVALUE");
			if((int32_t) event->trigger >= (int32_t) Event::Trigger::value && (!event->triggerValue || event->triggerValue->type == BaseLib::VariableType::tVoid)) return BaseLib::Variable::createError(-5, "No trigger value specified.");

			if(eventDescription->structValue->find("RESETAFTER") != eventDescription->structValue->end() && (eventDescription->structValue->at("RESETAFTER")->integerValue > 0 || eventDescription->structValue->at("RESETAFTER")->type == BaseLib::VariableType::tStruct))
			{
				if(eventDescription->structValue->find("RESETMETHOD") == eventDescription->structValue->end() || eventDescription->structValue->at("RESETMETHOD")->stringValue.empty()) return BaseLib::Variable::createError(-5, "No reset method specified.");
				event->resetMethod = eventDescription->structValue->at("RESETMETHOD")->stringValue;
				if(event->resetMethod.size() > 100) return BaseLib::Variable::createError(-5, "Reset method string is too long.");
				if(eventDescription->structValue->find("RESETMETHODPARAMS") != eventDescription->structValue->end())
				{
					event->resetMethodParameters = eventDescription->structValue->at("RESETMETHODPARAMS");
				}

				if(eventDescription->structValue->at("RESETAFTER")->type == BaseLib::VariableType::tInteger)
				{
					event->resetAfter = ((uint64_t) eventDescription->structValue->at("RESETAFTER")->integerValue) * 1000;
				}
				else if(eventDescription->structValue->at("RESETAFTER")->type == BaseLib::VariableType::tStruct)
				{
					BaseLib::PVariable resetStruct = eventDescription->structValue->at("RESETAFTER");
					if(resetStruct->structValue->find("INITIALTIME") == resetStruct->structValue->end() || resetStruct->structValue->at("INITIALTIME")->integerValue == 0) return BaseLib::Variable::createError(-5, "Initial time in reset struct not specified.");
					event->initialTime = ((uint64_t) resetStruct->structValue->at("INITIALTIME")->integerValue) * 1000;
					if(resetStruct->structValue->find("OPERATION") != resetStruct->structValue->end())
						event->operation = (Event::Operation::Enum) resetStruct->structValue->at("OPERATION")->integerValue;
					if(resetStruct->structValue->find("FACTOR") != resetStruct->structValue->end())
					{
						event->factor = resetStruct->structValue->at("FACTOR")->floatValue;
						if(event->factor <= 0) return BaseLib::Variable::createError(-5, "Factor is less or equal 0. Please provide a positive value");
					}
					if(resetStruct->structValue->find("LIMIT") != resetStruct->structValue->end())
					{
						if(resetStruct->structValue->at("LIMIT")->integerValue < 0) return BaseLib::Variable::createError(-5, "Limit is negative. Please provide a positive value");
						event->limit = ((uint64_t) resetStruct->structValue->at("LIMIT")->integerValue) * 1000;
					}
					if(resetStruct->structValue->find("RESETAFTER") != resetStruct->structValue->end())
						event->resetAfter = ((uint64_t) resetStruct->structValue->at("RESETAFTER")->integerValue) * 1000;
					if(event->resetAfter == 0) return BaseLib::Variable::createError(-5, "RESETAFTER is not specified or 0.");
				}
			}
			if(replace) remove(event->name);
			std::lock_guard<std::mutex> eventsGuard(_eventsMutex);
			_triggeredEvents[event->peerID][event->peerChannel][event->variable].push_back(event);
		}
		else
		{
			if(eventDescription->structValue->find("EVENTTIME") != eventDescription->structValue->end())
				event->eventTime = ((uint64_t) eventDescription->structValue->at("EVENTTIME")->integerValue) * 1000;
			if(event->eventTime == 0) event->eventTime = BaseLib::HelperFunctions::getTimeSeconds();
			if(eventDescription->structValue->find("ENDTIME") != eventDescription->structValue->end())
				event->endTime = ((uint64_t) eventDescription->structValue->at("ENDTIME")->integerValue) * 1000;
			if(eventDescription->structValue->find("RECUREVERY") != eventDescription->structValue->end())
				event->recurEvery = ((uint64_t) eventDescription->structValue->at("RECUREVERY")->integerValue) * 1000;
			uint64_t nextExecution = getNextExecution(event->eventTime, event->recurEvery);

			{
				if(replace) remove(event->name);
				std::lock_guard<std::mutex> eventsGuard(_eventsMutex);
				while(_timedEvents.find(nextExecution) != _timedEvents.end()) nextExecution++;
				_timedEvents[nextExecution] = event;
			}

			std::lock_guard<std::mutex> mainThreadGuard(_mainThreadMutex);
			if(_stopThread || !_mainThread.joinable())
			{
				GD::bl->threadManager.join(_mainThread);
				_stopThread = false;
				GD::bl->threadManager.start(_mainThread, true, &EventHandler::mainThread, this);
			}
		}
		save(event);
		GD::rpcClient->broadcastNewEvent(get(event->name));
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable EventHandler::list(int32_t type, uint64_t peerID, int32_t peerChannel, std::string variable)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		std::vector<std::shared_ptr<Event>> events;
		_eventsMutex.lock();
		//Copy all events first, because listEvents takes very long and we don't want to lock _eventsMutex too long
		if(type == 1 || (type < 0 && peerID == 0))
		{
			for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
			{
				events.push_back(i->second);
			}
		}
		if(type <= 0)
		{
			for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator iChannels = _triggeredEvents.begin(); iChannels != _triggeredEvents.end(); ++iChannels)
			{
				for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator iVariables = iChannels->second.begin(); iVariables != iChannels->second.end(); ++iVariables)
				{
					for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator iEvents = iVariables->second.begin(); iEvents != iVariables->second.end(); ++iEvents)
					{
						for(std::vector<std::shared_ptr<Event>>::iterator iEvent = iEvents->second.begin(); iEvent != iEvents->second.end(); ++iEvent)
						{
							if(peerID == 0 && variable.empty()) events.push_back(*iEvent);
							else if(peerID > 0)
							{
								if((*iEvent)->peerID == peerID && (peerChannel == -1 || (*iEvent)->peerChannel == peerChannel))
								{
									if(variable.empty() || (*iEvent)->variable == variable) events.push_back(*iEvent);
								}
							}
							else if(!variable.empty() && (*iEvent)->variable == variable) events.push_back(*iEvent);
						}
					}
				}
			}
		}
		_eventsMutex.unlock();

		BaseLib::PVariable eventList(new BaseLib::Variable(BaseLib::VariableType::tArray));
		for(std::vector<std::shared_ptr<Event>>::iterator i = events.begin(); i != events.end(); ++i)
		{
			//listEvents really needs a lot of resources, so wait a little bit after each event
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable event = getEventDescription(*i);
			if(event && !event->errorStruct) eventList->arrayValue->push_back(event);
		}
		return eventList;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable EventHandler::get(std::string name)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		std::shared_ptr<Event> event = getEvent(name);
		if(!event) return BaseLib::Variable::createError(-5, "Event not found.");
		return getEventDescription(event);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable EventHandler::getEventDescription(std::shared_ptr<Event> event)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		if(!event) return BaseLib::Variable::createError(-32500, "Event is nullptr.");
		BaseLib::PVariable eventDescription(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		if(event->type == Event::Type::timed)
		{
			eventDescription->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable((int32_t) event->type))));
			eventDescription->structValue->insert(BaseLib::StructElement("ID", BaseLib::PVariable(new BaseLib::Variable(event->name))));
			eventDescription->structValue->insert(BaseLib::StructElement("ENABLED", BaseLib::PVariable(new BaseLib::Variable(event->enabled))));
			eventDescription->structValue->insert(BaseLib::StructElement("EVENTTIME", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->eventTime / 1000)))));
			if(event->recurEvery > 0)
			{
				eventDescription->structValue->insert(BaseLib::StructElement("RECUREVERY", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->recurEvery / 1000)))));
				if(event->endTime > 0) eventDescription->structValue->insert(BaseLib::StructElement("ENDTIME", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->endTime / 1000)))));
			}
			eventDescription->structValue->insert(BaseLib::StructElement("EVENTMETHOD", BaseLib::PVariable(new BaseLib::Variable(event->eventMethod))));
			if(event->eventMethodParameters) eventDescription->structValue->insert(BaseLib::StructElement("EVENTMETHODPARAMS", event->eventMethodParameters));
			eventDescription->structValue->insert(BaseLib::StructElement("LASTRAISED", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->lastRaised / 1000)))));
		}
		else
		{
			eventDescription->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable((int32_t) event->type))));
			eventDescription->structValue->insert(BaseLib::StructElement("ID", BaseLib::PVariable(new BaseLib::Variable(event->name))));
			eventDescription->structValue->insert(BaseLib::StructElement("ENABLED", BaseLib::PVariable(new BaseLib::Variable(event->enabled))));
			eventDescription->structValue->insert(BaseLib::StructElement("PEERID", BaseLib::PVariable(new BaseLib::Variable((uint32_t) event->peerID))));
			if(event->peerChannel > -1) eventDescription->structValue->insert(BaseLib::StructElement("PEERCHANNEL", BaseLib::PVariable(new BaseLib::Variable(event->peerChannel))));
			eventDescription->structValue->insert(BaseLib::StructElement("VARIABLE", BaseLib::PVariable(new BaseLib::Variable(event->variable))));
			eventDescription->structValue->insert(BaseLib::StructElement("TRIGGER", BaseLib::PVariable(new BaseLib::Variable((int32_t) event->trigger))));
			if(event->triggerValue) eventDescription->structValue->insert(BaseLib::StructElement("TRIGGERVALUE", event->triggerValue));
			eventDescription->structValue->insert(BaseLib::StructElement("EVENTMETHOD", BaseLib::PVariable(new BaseLib::Variable(event->eventMethod))));
			if(event->eventMethodParameters) eventDescription->structValue->insert(BaseLib::StructElement("EVENTMETHODPARAMS", event->eventMethodParameters));
			if(event->resetAfter > 0 || event->initialTime > 0)
			{
				if(event->initialTime > 0)
				{
					BaseLib::PVariable resetStruct(new BaseLib::Variable(BaseLib::VariableType::tStruct));

					resetStruct->structValue->insert(BaseLib::StructElement("INITIALTIME", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->initialTime / 1000)))));
					resetStruct->structValue->insert(BaseLib::StructElement("RESETAFTER", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->resetAfter / 1000)))));
					resetStruct->structValue->insert(BaseLib::StructElement("OPERATION", BaseLib::PVariable(new BaseLib::Variable((int32_t) event->operation))));
					resetStruct->structValue->insert(BaseLib::StructElement("FACTOR", BaseLib::PVariable(new BaseLib::Variable(event->factor))));
					resetStruct->structValue->insert(BaseLib::StructElement("LIMIT", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->limit / 1000)))));
					resetStruct->structValue->insert(BaseLib::StructElement("CURRENTTIME", BaseLib::PVariable(new BaseLib::Variable(event->currentTime == 0 ? (uint32_t) (event->initialTime / 1000) : (uint32_t) (event->currentTime / 1000)))));

					eventDescription->structValue->insert(BaseLib::StructElement("RESETAFTER", resetStruct));
				}
				else eventDescription->structValue->insert(BaseLib::StructElement("RESETAFTER", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->resetAfter / 1000)))));
				eventDescription->structValue->insert(BaseLib::StructElement("RESETMETHOD", BaseLib::PVariable(new BaseLib::Variable(event->resetMethod))));
				if(event->eventMethodParameters) eventDescription->structValue->insert(BaseLib::StructElement("RESETMETHODPARAMS", event->resetMethodParameters));
				eventDescription->structValue->insert(BaseLib::StructElement("LASTRESET", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->lastReset / 1000)))));
			}
			eventDescription->structValue->insert(BaseLib::StructElement("LASTVALUE", event->lastValue));
			eventDescription->structValue->insert(BaseLib::StructElement("LASTRAISED", BaseLib::PVariable(new BaseLib::Variable((uint32_t) (event->lastRaised / 1000)))));
		}
		return eventDescription;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable EventHandler::remove(std::string name)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		_eventsMutex.lock();
		std::shared_ptr<Event> event;
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->name == name)
			{
				event = i->second;
				std::lock_guard<std::mutex> disposingGuard(event->disposingMutex);
				event->disposing = true;
				_timedEvents.erase(i);
				break;
			}
		}
		if(!event)
		{
			bool breakLoop = false;
			for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator peerID = _triggeredEvents.begin(); peerID != _triggeredEvents.end(); ++peerID)
			{
				for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator channel = peerID->second.begin(); channel != peerID->second.end(); ++channel)
				{
					for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator variable = channel->second.begin(); variable != channel->second.end(); ++variable)
					{
						for(std::vector<std::shared_ptr<Event>>::iterator currentEvent = variable->second.begin(); currentEvent != variable->second.end(); ++currentEvent)
						{
							if((*currentEvent)->name == name)
							{
								event = *currentEvent;
								_triggeredEvents[peerID->first][channel->first][variable->first].erase(currentEvent);
								if(_triggeredEvents[peerID->first][channel->first][variable->first].empty()) _triggeredEvents[peerID->first][channel->first].erase(variable->first);
								if(_triggeredEvents[peerID->first][channel->first].empty()) _triggeredEvents[peerID->first].erase(channel->first);
								if(_triggeredEvents[peerID->first].empty()) _triggeredEvents.erase(peerID->first);
								std::lock_guard<std::mutex> disposingGuard(event->disposingMutex);
								event->disposing = true;
								breakLoop = true;
								break;
							}
						}
						if(breakLoop) break;
					}
					if(breakLoop) break;
				}
				if(breakLoop) break;
			}
		}
		_eventsMutex.unlock();
		if(!event) return BaseLib::Variable::createError(-5, "Event not found.");
		if(event && event->type == Event::Type::triggered)
		{
			removeEventToReset(event->id);
			removeTimeToReset(event->id);
		}

		_databaseMutex.lock();
		GD::bl->db->deleteEvent(name);
		_databaseMutex.unlock();
		GD::rpcClient->broadcastDeleteEvent(name, (int32_t) event->type, event->peerID, event->peerChannel, event->variable);
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	_databaseMutex.unlock();
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable EventHandler::enable(std::string name, bool enabled)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		_eventsMutex.lock();
		std::shared_ptr<Event> event;
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->name == name)
			{
				event = i->second;
				break;
			}
		}
		if(!event)
		{
			for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator iChannels = _triggeredEvents.begin(); iChannels != _triggeredEvents.end(); ++iChannels)
			{
				for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator iVariables = iChannels->second.begin(); iVariables != iChannels->second.end(); ++iVariables)
				{
					for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator iEvents = iVariables->second.begin(); iEvents != iVariables->second.end(); ++iEvents)
					{
						for(std::vector<std::shared_ptr<Event>>::iterator iEvent = iEvents->second.begin(); iEvent != iEvents->second.end(); ++iEvent)
						{
							if((*iEvent)->name == name)
							{
								event = *iEvent;
								break;
							}
						}
					}
				}
			}
		}
		_eventsMutex.unlock();

		if(!event) return BaseLib::Variable::createError(-5, "Event not found.");
		if(enabled) event->enabled = true;
		else
		{
			event->enabled = false;
			removeEventToReset(event->id);
		}
		save(event);
		GD::rpcClient->broadcastUpdateEvent(name, (int32_t) event->type, event->peerID, event->peerChannel, event->variable);
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	_databaseMutex.unlock();
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable EventHandler::abortReset(std::string name)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		_eventsMutex.lock();
		std::shared_ptr<Event> event;
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->name == name)
			{
				event = i->second;
				break;
			}
		}
		if(!event)
		{
			for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator iChannels = _triggeredEvents.begin(); iChannels != _triggeredEvents.end(); ++iChannels)
			{
				for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator iVariables = iChannels->second.begin(); iVariables != iChannels->second.end(); ++iVariables)
				{
					for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator iEvents = iVariables->second.begin(); iEvents != iVariables->second.end(); ++iEvents)
					{
						for(std::vector<std::shared_ptr<Event>>::iterator iEvent = iEvents->second.begin(); iEvent != iEvents->second.end(); ++iEvent)
						{
							if((*iEvent)->name == name)
							{
								event = *iEvent;
								break;
							}
						}
					}
				}
			}
		}
		_eventsMutex.unlock();

		removeEventToReset(event->id);
		save(event);
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	_databaseMutex.unlock();
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void EventHandler::trigger(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values)
{
	try
	{
		if(_disposing) return;
		std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(peerID, channel, variables, values));
		enqueue(0, queueEntry);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

uint64_t EventHandler::getNextExecution(uint64_t startTime, uint64_t recurEvery)
{
	try
	{
		uint64_t time = BaseLib::HelperFunctions::getTime();
		if(startTime >= time) return startTime;
		if(recurEvery == 0) return -1;
		uint64_t difference = time - startTime;
		return time + (recurEvery - (difference % recurEvery));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return -1;
}

void EventHandler::trigger(std::string& variable, BaseLib::PVariable& value)
{
	try
	{
		if(_disposing) return;
		std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(0, -1, variable, value));
		enqueue(0, queueEntry);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::trigger(uint64_t peerID, int32_t channel, std::string& variable, BaseLib::PVariable& value)
{
	try
	{
		if(_disposing) return;
		std::shared_ptr<BaseLib::IQueueEntry> queueEntry(new QueueEntry(peerID, channel, variable, value));
		enqueue(0, queueEntry);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::processTriggerMultipleVariables(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>>& variables, std::shared_ptr<std::vector<BaseLib::PVariable>>& values)
{
	try
	{
		for(uint32_t i = 0; i < variables->size(); i++)
		{
			processTriggerSingleVariable(peerID, channel, variables->at(i), values->at(i));
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::removeEventToReset(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _eventsToReset.begin(); i != _eventsToReset.end(); ++i)
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
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
}

void EventHandler::removeTimeToReset(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timesToReset.begin(); i != _timesToReset.end(); ++i)
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
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
}

void EventHandler::removeTimedEvent(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->id == id)
			{
				std::string name = i->second->name;
				int32_t type = (int32_t) i->second->type;
				uint64_t peerID = i->second->peerID;
				int32_t peerChannel = i->second->peerChannel;
				std::string variable = i->second->variable;
				_timedEvents.erase(i); //erase invalidates the iterator.
				_eventsMutex.unlock();
				GD::rpcClient->broadcastDeleteEvent(name, type, peerID, peerChannel, variable);
				return;
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
}

bool EventHandler::eventExists(uint32_t id)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->id == id)
			{
				_eventsMutex.unlock();
				return true;
			}
		}
		for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator peerID = _triggeredEvents.begin(); peerID != _triggeredEvents.end(); ++peerID)
		{
			for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator channel = peerID->second.begin(); channel != peerID->second.end(); ++channel)
			{
				for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator variable = channel->second.begin(); variable != channel->second.end(); ++variable)
				{
					for(std::vector<std::shared_ptr<Event>>::iterator event = variable->second.begin(); event != variable->second.end(); ++event)
					{
						if((*event)->id == id)
						{
							_eventsMutex.unlock();
							return true;
						}
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	return false;
}

bool EventHandler::eventExists(std::string name)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->name == name)
			{
				_eventsMutex.unlock();
				return true;
			}
		}
		for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator peerID = _triggeredEvents.begin(); peerID != _triggeredEvents.end(); ++peerID)
		{
			for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator channel = peerID->second.begin(); channel != peerID->second.end(); ++channel)
			{
				for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator variable = channel->second.begin(); variable != channel->second.end(); ++variable)
				{
					for(std::vector<std::shared_ptr<Event>>::iterator event = variable->second.begin(); event != variable->second.end(); ++event)
					{
						if((*event)->name == name)
						{
							_eventsMutex.unlock();
							return true;
						}
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	return false;
}

std::shared_ptr<Event> EventHandler::getEvent(std::string name)
{
	_eventsMutex.lock();
	try
	{
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->name == name)
			{
				_eventsMutex.unlock();
				return i->second;
			}
		}
		for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator peerID = _triggeredEvents.begin(); peerID != _triggeredEvents.end(); ++peerID)
		{
			for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator channel = peerID->second.begin(); channel != peerID->second.end(); ++channel)
			{
				for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator variable = channel->second.begin(); variable != channel->second.end(); ++variable)
				{
					for(std::vector<std::shared_ptr<Event>>::iterator event = variable->second.begin(); event != variable->second.end(); ++event)
					{
						if((*event)->name == name)
						{
							_eventsMutex.unlock();
							return (*event);
						}
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	return std::shared_ptr<Event>();
}

BaseLib::PVariable EventHandler::trigger(std::string name)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Event handler is shutting down.");
		_eventsMutex.lock();
		std::shared_ptr<Event> event;
		for(std::map<uint64_t, std::shared_ptr<Event>>::iterator i = _timedEvents.begin(); i != _timedEvents.end(); ++i)
		{
			if(i->second->name == name)
			{
				event = i->second;
				break;
			}
		}
		if(!event)
		{
			for(std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator peerID = _triggeredEvents.begin(); peerID != _triggeredEvents.end(); ++peerID)
			{
				for(std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator channel = peerID->second.begin(); channel != peerID->second.end(); ++channel)
				{
					for(std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator variable = channel->second.begin(); variable != channel->second.end(); ++variable)
					{
						for(std::vector<std::shared_ptr<Event>>::iterator currentEvent = variable->second.begin(); currentEvent != variable->second.end(); ++currentEvent)
						{
							if((*currentEvent)->name == name)
							{
								event = *currentEvent;
								break;
							}
						}
					}
				}
			}
		}
		_eventsMutex.unlock();

		if(!event) return BaseLib::Variable::createError(-5, "Event not found.");
		uint64_t currentTime = BaseLib::HelperFunctions::getTime();
		event->lastRaised = currentTime;
		GD::out.printInfo("Info: Event \"" + event->name + "\" raised for peer with id " + std::to_string(event->peerID) + ", channel " + std::to_string(event->peerChannel) + " and variable \"" + event->variable + "\". Trigger: \"manual\"");
		BaseLib::PVariable eventMethodParameters(new BaseLib::Variable());
		*eventMethodParameters = *event->eventMethodParameters;
		BaseLib::PVariable result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, event->eventMethod, eventMethodParameters);

		postTriggerTasks(event, result, currentTime);

		GD::rpcClient->broadcastUpdateEvent(name, (int32_t) event->type, event->peerID, event->peerChannel, event->variable);
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	_eventsMutex.unlock();
	_databaseMutex.unlock();
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void EventHandler::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry)
{
	try
	{
		std::shared_ptr<QueueEntry> queueEntry;
		queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
		if(!queueEntry) return;
		if(queueEntry->type == QueueEntryType::singleVariable)
		{
			processTriggerSingleVariable(queueEntry->peerId, queueEntry->channel, queueEntry->variable, queueEntry->value);
		}
		else if(queueEntry->type == QueueEntryType::multipleVariables)
		{
			processTriggerMultipleVariables(queueEntry->peerId, queueEntry->channel, queueEntry->variables, queueEntry->values);
		}
		else if(queueEntry->type == QueueEntryType::rpcCall)
		{
			processRpcCall(queueEntry->eventName, queueEntry->eventMethod, queueEntry->eventMethodParameters);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::processRpcCall(std::string& eventName, std::string& eventMethod, BaseLib::PVariable& eventMethodParameters)
{
	try
	{
		BaseLib::PVariable eventMethodParametersCopy(new BaseLib::Variable());
		if(eventMethodParameters) *eventMethodParametersCopy = *eventMethodParameters;
		BaseLib::PVariable result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, eventMethod, eventMethodParametersCopy);
		if(result && result->errorStruct && !GD::bl->shuttingDown)
		{
			GD::out.printError("Could not execute RPC method \"" + eventMethod + "\" for event \"" + eventName + "\". Error struct:");
			result->print(true, false);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::processTriggerSingleVariable(uint64_t peerID, int32_t channel, std::string& variable, BaseLib::PVariable& value)
{
	try
	{
		if(!value) return;
		uint64_t currentTime = BaseLib::HelperFunctions::getTime();
		std::vector<std::shared_ptr<Event>> triggeredEvents;

		{
			std::lock_guard<std::mutex> eventsGuard(_eventsMutex);
			std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>>::iterator peerIterator = _triggeredEvents.find(peerID);
			if(peerIterator == _triggeredEvents.end()) return;
			std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>::iterator channelIterator = peerIterator->second.find(channel);
			if(channelIterator == peerIterator->second.end()) return;
			std::map<std::string, std::vector<std::shared_ptr<Event>>>::iterator variableIterator = channelIterator->second.find(variable);
			if(variableIterator == channelIterator->second.end()) return;
			for(std::vector<std::shared_ptr<Event>>::iterator i = variableIterator->second.begin(); i != variableIterator->second.end(); ++i)
			{
				//Don't raise the same event multiple times
				if(!(*i)->enabled || ((*i)->lastValue && *((*i)->lastValue) == *value && currentTime - (*i)->lastRaised < 220)) continue;
				triggeredEvents.push_back(*i);
			}
		}

		for(std::vector<std::shared_ptr<Event>>::iterator i = triggeredEvents.begin(); i != triggeredEvents.end(); ++i)
		{
			BaseLib::PVariable eventMethodParameters(new BaseLib::Variable());
			*eventMethodParameters = *(*i)->eventMethodParameters;
			BaseLib::PVariable result;
			BaseLib::PVariable lastValue;

			{
				std::lock_guard<std::mutex> eventsGuard(_eventsMutex);
				lastValue = (*i)->lastValue;
			}

			if(((int32_t) (*i)->trigger) < 8)
			{
				//Comparison with previous value
				if((*i)->trigger == Event::Trigger::updated)
				{
					GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"updated\"");
					(*i)->lastRaised = currentTime;
					result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
				}
				else if((*i)->trigger == Event::Trigger::unchanged)
				{
					if(*lastValue == *value)
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"unchanged\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::changed)
				{
					if(*lastValue != *value)
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"changed\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::greater)
				{
					if(*lastValue > *value)
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"greater\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::less)
				{
					if(*lastValue < *value)
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"less\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::greaterOrUnchanged)
				{
					if(*lastValue >= *value)
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"greaterOrUnchanged\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::lessOrUnchanged)
				{
					if(*lastValue <= *value)
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"lessOrUnchanged\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
			}
			else if((*i)->triggerValue)
			{
				//Comparison with trigger value
				if((*i)->trigger == Event::Trigger::value)
				{
					if(*value == *((*i)->triggerValue))
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"value\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::notValue)
				{
					if(*value != *((*i)->triggerValue))
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"notValue\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::greaterThanValue)
				{
					if(*value > *((*i)->triggerValue))
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"greaterThanValue\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::lessThanValue)
				{
					if(*value < *((*i)->triggerValue))
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"lessThanValue\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::greaterOrEqualValue)
				{
					if(*value >= *((*i)->triggerValue))
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"greaterOrEqualValue\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
				else if((*i)->trigger == Event::Trigger::lessOrEqualValue)
				{
					if(*value <= *((*i)->triggerValue))
					{
						GD::out.printInfo("Info: Event \"" + (*i)->name + "\" raised for peer with id " + std::to_string(peerID) + ", channel " + std::to_string(channel) + " and variable \"" + variable + "\". Trigger: \"lessOrEqualValue\"");
						(*i)->lastRaised = currentTime;
						result = GD::rpcServers.begin()->second->callMethod(_dummyClientInfo, (*i)->eventMethod, eventMethodParameters);
					}
				}
			}

			{
				std::lock_guard<std::mutex> eventsGuard(_eventsMutex);
				(*i)->lastValue = value;
			}

			postTriggerTasks(*i, result, currentTime);

			GD::rpcClient->broadcastUpdateEvent((*i)->name, (int32_t) (*i)->type, (*i)->peerID, (*i)->peerChannel, (*i)->variable);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::postTriggerTasks(std::shared_ptr<Event>& event, BaseLib::PVariable& rpcResult, uint64_t currentTime)
{
	try
	{
		if(!event)
		{
			GD::out.printError("Error: Could not execute post trigger tasks. event was nullptr.");
			return;
		}
		if(rpcResult && rpcResult->errorStruct && !GD::bl->shuttingDown)
		{
			GD::out.printError("Error: Could not execute RPC method for event from peer with id " + std::to_string(event->peerID) + ", channel " + std::to_string(event->peerChannel) + " and variable " + event->variable + ". Error struct:");
			rpcResult->print(true, true);
		}
		if(event->lastRaised >= currentTime && (event->resetAfter > 0 || event->initialTime > 0))
		{
			try
			{
				removeEventToReset(event->id);
				uint64_t resetTime = currentTime + event->resetAfter;
				if(event->initialTime == 0) //Simple reset
				{
					GD::out.printInfo("Info: Event \"" + event->name + "\" for peer with id " + std::to_string(event->peerID) + ", channel " + std::to_string(event->peerChannel) + " and variable \"" + event->variable + "\" will be reset in " + std::to_string(event->resetAfter / 1000) + " seconds.");

					_eventsMutex.lock();
					while(_eventsToReset.find(resetTime) != _eventsToReset.end()) resetTime++;
					_eventsToReset[resetTime] = event;
					_eventsMutex.unlock();
				}
				else //Complex reset
				{
					removeTimeToReset(event->id);
					GD::out.printInfo("Info: INITIALTIME for event \"" + event->name + "\" will be reset in " + std::to_string(event->resetAfter / 1000) + " seconds.");
					_eventsMutex.lock();
					while(_timesToReset.find(resetTime) != _timesToReset.end()) resetTime++;
					_timesToReset[resetTime] = event;
					_eventsMutex.unlock();
					if(event->currentTime == 0) event->currentTime = event->initialTime;
					if(event->factor <= 0)
					{
						GD::out.printWarning("Warning: Factor is less or equal 0. Setting factor to 1. Event from peer with id " + std::to_string(event->peerID) + ", channel " + std::to_string(event->peerChannel) + " and variable " + event->variable + ".");
						event->factor = 1;
					}
					resetTime = currentTime + event->currentTime;
					_eventsMutex.lock();
					while(_eventsToReset.find(resetTime) != _eventsToReset.end()) resetTime++;
					_eventsToReset[resetTime] = event;
					_eventsMutex.unlock();
					GD::out.printInfo("Info: Event \"" + event->name + "\" will be reset in " + std::to_string(event->currentTime / 1000) + " seconds.");
					if(event->operation == Event::Operation::Enum::addition)
					{
						event->currentTime += event->factor;
						if(event->currentTime > event->limit) event->currentTime = event->limit;
					}
					else if(event->operation == Event::Operation::Enum::subtraction)
					{
						event->currentTime -= event->factor;
						if(event->currentTime < event->limit) event->currentTime = event->limit;
					}
					else if(event->operation == Event::Operation::Enum::multiplication)
					{
						event->currentTime *= event->factor;
						if(event->currentTime > event->limit) event->currentTime = event->limit;
					}
					else if(event->operation == Event::Operation::Enum::division)
					{
						event->currentTime /= event->factor;
						if(event->currentTime < event->limit) event->currentTime = event->limit;
					}
				}
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}

			try
			{
				std::lock_guard<std::mutex> mainThreadGuard(_mainThreadMutex);
				if(_stopThread || !_mainThread.joinable())
				{
					GD::bl->threadManager.join(_mainThread);
					_stopThread = false;
					GD::bl->threadManager.start(_mainThread, true, &EventHandler::mainThread, this);
				}
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		save(event);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::load()
{
	try
	{
		if(!GD::bl || !_rpcDecoder)
		{
			GD::out.printError("Error: Tried to load events, but event handler is not initialized.");
			return;
		}

		_databaseMutex.lock();
		std::shared_ptr<BaseLib::Database::DataTable> rows = GD::bl->db->getEvents();
        if(!rows->empty()) GD::out.printWarning("Warning: Homegear's event handler is deprecated and will be removed in future versions. Please use Node-BLUE.");
		_databaseMutex.unlock();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			std::shared_ptr<Event> event(new Event());
			event->id = row->second.at(0)->intValue;
			event->name = row->second.at(1)->textValue;
			event->type = (Event::Type::Enum) row->second.at(2)->intValue;
			event->peerID = row->second.at(3)->intValue;
			event->peerChannel = row->second.at(4)->intValue;
			event->variable = row->second.at(5)->textValue;
			event->trigger = (Event::Trigger::Enum) row->second.at(6)->intValue;
			event->triggerValue = _rpcDecoder->decodeResponse(*row->second.at(7)->binaryValue);
			event->eventMethod = row->second.at(8)->textValue;
			event->eventMethodParameters = _rpcDecoder->decodeResponse(*row->second.at(9)->binaryValue);
			event->resetAfter = row->second.at(10)->intValue;
			event->initialTime = row->second.at(11)->intValue;
			event->operation = (Event::Operation::Enum) row->second.at(12)->intValue;
			event->factor = row->second.at(13)->floatValue;
			event->limit = row->second.at(14)->intValue;
			event->resetMethod = row->second.at(15)->textValue;
			event->resetMethodParameters = _rpcDecoder->decodeResponse(*row->second.at(16)->binaryValue);
			event->eventTime = row->second.at(17)->intValue;
			event->endTime = row->second.at(18)->intValue;
			event->recurEvery = row->second.at(19)->intValue;
			event->lastValue = _rpcDecoder->decodeResponse(*row->second.at(20)->binaryValue);
			event->lastRaised = row->second.at(21)->intValue;
			event->lastReset = row->second.at(22)->intValue;
			event->currentTime = row->second.at(23)->intValue;
			event->enabled = row->second.at(24)->intValue;
			std::lock_guard<std::mutex> eventsGuard(_eventsMutex);
			if(event->eventTime > 0)
			{
				uint64_t nextExecution = getNextExecution(event->eventTime, event->recurEvery);
				while(_timedEvents.find(nextExecution) != _timedEvents.end()) nextExecution++;
				_timedEvents[nextExecution] = event;
			}
			else
			{
				_triggeredEvents[event->peerID][event->peerChannel][event->variable].push_back(event);
				if(event->resetAfter > 0 && event->lastReset <= event->lastRaised)
				{
					if(event->initialTime > 0)
					{
						_eventsToReset[event->lastRaised + event->currentTime] = event;
						_timesToReset[event->lastRaised + event->resetAfter] = event;
					}
					else _eventsToReset[event->lastRaised + event->resetAfter] = event;
				}
				else if(event->initialTime > 0) event->currentTime = 0;
			}
		}
		std::lock_guard<std::mutex> mainThreadGuard(_mainThreadMutex);
		if(!_timedEvents.empty() || !_eventsToReset.empty())
		{
			_stopThread = false;
			GD::bl->threadManager.start(_mainThread, true, &EventHandler::mainThread, this);
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void EventHandler::save(std::shared_ptr<Event> event)
{
	std::string eventID = std::to_string(event->id);
	try
	{
		//The eventExists is necessary so we don't safe an event that is being deleted
		if(!event || _disposing) return;
		std::lock_guard<std::mutex> databaseGuard(_databaseMutex);
		if(event->id > 0 && !eventExists(event->id)) return;
		std::lock_guard<std::mutex> disposingGuard(event->disposingMutex);
		if(event->disposing) return;
		BaseLib::Database::DataRow data;
		if(event->id > 0) data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->id)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->name)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((int32_t) event->type)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->peerID)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->peerChannel)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->variable)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((int32_t) event->trigger)));
		std::vector<char> value;
		_rpcEncoder->encodeResponse(event->triggerValue, value);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->eventMethod)));
		_rpcEncoder->encodeResponse(event->eventMethodParameters, value);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->resetAfter)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->initialTime)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn((int32_t) event->operation)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->factor)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->limit)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->resetMethod)));
		_rpcEncoder->encodeResponse(event->resetMethodParameters, value);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->eventTime)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->endTime)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->recurEvery)));
		_rpcEncoder->encodeResponse(event->lastValue, value);
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(value)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->lastRaised)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->lastReset)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->currentTime)));
		data.push_back(std::shared_ptr<BaseLib::Database::DataColumn>(new BaseLib::Database::DataColumn(event->enabled)));
		GD::bl->db->saveEventAsynchronous(data);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

}

#endif
