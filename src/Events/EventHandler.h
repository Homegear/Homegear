/* Copyright 2013-2017 Sathya Laufer
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

#ifndef EVENTHANDLER_H_
#define EVENTHANDLER_H_

#include "../../config.h"

#ifdef EVENTHANDLER
#include <homegear-base/BaseLib.h>

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <thread>

namespace Homegear
{

class Event
{
public:
	struct Type
	{
		enum Enum
		{
			triggered = 0,
			timed = 1
		};
	};

	struct Trigger
	{
		enum Enum
		{
			none = 0,
			unchanged = 1,
			changed = 2,
			greater = 3,
			less = 4,
			greaterOrUnchanged = 5,
			lessOrUnchanged = 6,
			updated = 7,
			value = 8,
			notValue = 9,
			greaterThanValue = 10,
			lessThanValue = 11,
			greaterOrEqualValue = 12,
			lessOrEqualValue = 13
		};
	};

	struct Operation
	{
		enum Enum
		{
			none = 0,
			addition = 1,
			subtraction = 2,
			multiplication = 3,
			division = 4
		};
	};

	std::mutex disposingMutex;
	std::atomic_bool disposing;
	uint64_t id = 0;
	Type::Enum type = Type::Enum::triggered;
	std::string name;
	bool enabled = true;
	uint64_t peerID = 0;
	int32_t peerChannel = -1;
	std::string variable;
	Trigger::Enum trigger = Trigger::Enum::none;
	BaseLib::PVariable triggerValue;
	std::string eventMethod;
	BaseLib::PVariable eventMethodParameters;
	uint64_t resetAfter = 0;
	uint64_t initialTime = 0;
	uint64_t currentTime = 0;
	Operation::Enum operation = Operation::Enum::none;
	double factor = 1;
	uint64_t limit = 0;
	std::string resetMethod;
	BaseLib::PVariable resetMethodParameters;
	uint64_t eventTime = 0;
	uint64_t endTime = 0;
	uint64_t recurEvery = 0;
	BaseLib::PVariable lastValue = BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
	uint64_t lastRaised = 0;
	uint64_t lastReset = 0;

	Event() { disposing = false; }

	virtual ~Event() {}
};

class EventHandler : public BaseLib::IQueue
{
public:
	EventHandler();

	virtual ~EventHandler();

	virtual void dispose();

	void init();

	void load();

	BaseLib::PVariable add(BaseLib::PVariable eventDescription);

	BaseLib::PVariable get(std::string name);

	BaseLib::PVariable remove(std::string name);

	BaseLib::PVariable list(int32_t type, uint64_t peerID, int32_t peerChannel, std::string variable);

	BaseLib::PVariable enable(std::string name, bool enabled);

	BaseLib::PVariable abortReset(std::string name);

	BaseLib::PVariable trigger(std::string name);

	void trigger(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<BaseLib::PVariable>> values);

	void trigger(uint64_t peerID, int32_t channel, std::string& variable, BaseLib::PVariable& value);

	/**
	 * Event trigger for system variables.
	 * @param variable The name of the system variable.
	 * @param value The new value of the system variable.
	 */
	void trigger(std::string& variable, BaseLib::PVariable& value);

protected:
	enum class QueueEntryType
	{
		singleVariable,
		multipleVariables,
		rpcCall
	};

	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}

		QueueEntry(QueueEntryType t) : type(t) {}

		QueueEntry(std::string& name, std::string& method, BaseLib::PVariable& methodParameters)
				: type(QueueEntryType::rpcCall), eventName(name), eventMethod(method), eventMethodParameters(methodParameters) {}

		QueueEntry(uint64_t peerId, int32_t channel, std::string& variable, BaseLib::PVariable& value)
				: type(QueueEntryType::singleVariable), peerId(peerId), channel(channel), variable(variable), value(value) {}

		QueueEntry(uint64_t peerId, int32_t channel, std::shared_ptr<std::vector<std::string>>& variables, std::shared_ptr<std::vector<BaseLib::PVariable>>& values)
				: type(QueueEntryType::multipleVariables), peerId(peerId), channel(channel), variables(variables), values(values) {}

		virtual ~QueueEntry() {}

		QueueEntryType type = QueueEntryType::singleVariable;

		// {{{ Triggered event
		uint64_t peerId = 0;
		int32_t channel = -1;

		//Multiple variables
		std::shared_ptr<std::vector<std::string>> variables;
		std::shared_ptr<std::vector<BaseLib::PVariable>> values;

		//One variable
		std::string variable;
		BaseLib::PVariable value;
		// }}}

		// {{{ Timed event
		std::string eventName;
		std::string eventMethod;
		BaseLib::PVariable eventMethodParameters;
		// }}}
	};

	std::atomic_bool _disposing;
	std::mutex _eventsMutex;
	std::map<uint64_t, std::shared_ptr<Event>> _timedEvents;
	std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>> _triggeredEvents;
	std::map<uint64_t, std::shared_ptr<Event>> _eventsToReset;
	std::map<uint64_t, std::shared_ptr<Event>> _timesToReset;
	std::atomic_bool _stopThread;
	std::thread _mainThread;
	std::mutex _mainThreadMutex;
	std::mutex _databaseMutex;
	std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;

	void processTriggerMultipleVariables(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>>& variables, std::shared_ptr<std::vector<BaseLib::PVariable>>& values);

	void processTriggerSingleVariable(uint64_t peerID, int32_t channel, std::string& variable, BaseLib::PVariable& value);

	void processRpcCall(std::string& eventName, std::string& eventMethod, BaseLib::PVariable& eventMethodParameters);

	void mainThread();

	uint64_t getNextExecution(uint64_t startTime, uint64_t recurEvery);

	void removeEventToReset(uint32_t id);

	void removeTimeToReset(uint32_t id);

	void removeTimedEvent(uint32_t id);

	BaseLib::PVariable getEventDescription(std::shared_ptr<Event> event);

	bool eventExists(uint32_t id);

	bool eventExists(std::string name);

	std::shared_ptr<Event> getEvent(std::string name);

	void save(std::shared_ptr<Event>);

	void postTriggerTasks(std::shared_ptr<Event>& event, BaseLib::PVariable& rpcResult, uint64_t currentTime);

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
};

}

#endif
#endif
