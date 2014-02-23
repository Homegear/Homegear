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

#ifndef EVENTHANDLER_H_
#define EVENTHANDLER_H_

#include "../HelperFunctions/HelperFunctions.h"
#include "../Encoding/RPCEncoder.h"
#include "../Encoding/RPCDecoder.h"
#include "../RPC/Server.h"

#include <memory>
#include <string>
#include <map>
#include <mutex>
#include <thread>

class Event
{
public:
	struct Type
	{
		enum Enum { triggered = 0, timed = 1 };
	};

	struct Trigger
	{
		enum Enum { none = 0, unchanged = 1, changed = 2, greater = 3, less = 4, greaterOrUnchanged = 5, lessOrUnchanged = 6, updated = 7, value = 8, notValue = 9, greaterThanValue = 10, lessThanValue = 11, greaterOrEqualValue = 12, lessOrEqualValue = 13 };
	};

	struct Operation
	{
		enum Enum { none = 0, addition = 1, subtraction = 2, multiplication = 3, division = 4 };
	};

	uint32_t id = 0;
	Type::Enum type = Type::Enum::triggered;
	std::string name;
	bool enabled = true;
	uint64_t peerID = 0;
	int32_t peerChannel = -1;
	std::string variable;
	Trigger::Enum trigger = Trigger::Enum::none;
	std::shared_ptr<RPC::RPCVariable> triggerValue;
	std::string eventMethod;
	std::shared_ptr<RPC::RPCVariable> eventMethodParameters;
	uint64_t resetAfter = 0;
	uint64_t initialTime = 0;
	uint64_t currentTime = 0;
	Operation::Enum operation = Operation::Enum::none;
	double factor = 1;
	uint64_t limit = 0;
	std::string resetMethod;
	std::shared_ptr<RPC::RPCVariable> resetMethodParameters;
	uint64_t eventTime = 0;
	uint64_t endTime = 0;
	uint64_t recurEvery = 0;
	std::shared_ptr<RPC::RPCVariable> lastValue;
	uint64_t lastRaised = 0;
	uint64_t lastReset = 0;

	Event() {}
	virtual ~Event() {}
};

class EventHandler
{
public:
	EventHandler();
	virtual ~EventHandler();

	void load();
	std::shared_ptr<RPC::RPCVariable> add(std::shared_ptr<RPC::RPCVariable> eventDescription);
	std::shared_ptr<RPC::RPCVariable> remove(std::string name);
	std::shared_ptr<RPC::RPCVariable> list(int32_t type, uint64_t peerID, int32_t peerChannel, std::string variable);
	std::shared_ptr<RPC::RPCVariable> enable(std::string name, bool enabled);
	std::shared_ptr<RPC::RPCVariable> abortReset(std::string name);
	std::shared_ptr<RPC::RPCVariable> trigger(std::string name);
	void trigger(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	void trigger(uint64_t peerID, int32_t channel, std::string& variable, std::shared_ptr<RPC::RPCVariable>& value);
protected:
	bool _disposing = false;
	int32_t _triggerThreadCount = 0;
	std::mutex _eventsMutex;
	std::map<uint64_t, std::shared_ptr<Event>> _timedEvents;
	std::map<uint64_t, std::map<int32_t, std::map<std::string, std::vector<std::shared_ptr<Event>>>>> _triggeredEvents;
	std::map<uint64_t, std::shared_ptr<Event>> _eventsToReset;
	std::map<uint64_t, std::shared_ptr<Event>> _timesToReset;
	bool _stopThread = false;
	std::thread _mainThread;
	std::mutex _mainThreadMutex;
	std::mutex _databaseMutex;
	RPC::RPCEncoder _rpcEncoder;
	RPC::RPCDecoder _rpcDecoder;

	void triggerThreadMultipleVariables(uint64_t peerID, int32_t channel, std::shared_ptr<std::vector<std::string>> variables, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values);
	void triggerThread(uint64_t peerID, int32_t channel, std::string variable, std::shared_ptr<RPC::RPCVariable> value);
	void mainThread();
	uint64_t getNextExecution(uint64_t startTime, uint64_t recurEvery);
	void removeEventToReset(uint32_t id);
	void removeTimeToReset(uint32_t id);
	void removeTimedEvent(uint32_t id);
	bool eventExists(uint32_t id);
	bool eventExists(std::string name);
	void save(std::shared_ptr<Event>);
	void postTriggerTasks(std::shared_ptr<Event>& event, std::shared_ptr<RPC::RPCVariable>& rpcResult, uint64_t currentTime);
};
#endif /* EVENTHANDLER_H_ */
