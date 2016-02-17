/* Copyright 2013-2016 Sathya Laufer
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

#ifndef PHPEVENTS_H_
#define PHPEVENTS_H_

class PhpEvents
{
public:
	class EventData
	{
	public:
		std::string type;
		uint64_t id = 0;
		int32_t channel = -1;
		int32_t hint = -1;
		std::string variable;
		BaseLib::PVariable value;
	};

	static std::mutex eventsMapMutex;
	static std::map<int32_t, std::shared_ptr<PhpEvents>> eventsMap;

	PhpEvents(std::function<void(std::string& output)>& outputCallback, std::function<BaseLib::PVariable(std::string& methodName, BaseLib::PVariable& parameters)>& rpcCallback);
	virtual ~PhpEvents();
	void stop();
	bool enqueue(std::shared_ptr<EventData>& entry);
	std::shared_ptr<EventData> poll();
	void addPeer(uint64_t peerId);
	void removePeer(uint64_t peerId);
	bool peerSubscribed(uint64_t peerId);

	std::function<void(std::string& output)>& getOutputCallback() { return _outputCallback; };
	std::function<BaseLib::PVariable(std::string& methodName, BaseLib::PVariable& parameters)>& getRpcCallback() { return _rpcCallback; };
private:
	std::function<void(std::string& output)> _outputCallback;
	std::function<BaseLib::PVariable(std::string& methodName, BaseLib::PVariable& parameters)> _rpcCallback;

	static const int32_t _bufferSize = 100;
	std::mutex _bufferMutex;
	int32_t _bufferHead = 0;
	int32_t _bufferTail = 0;
	std::shared_ptr<EventData> _buffer[_bufferSize];
	std::mutex _processingThreadMutex;
	bool _processingEntryAvailable = false;
	std::condition_variable _processingConditionVariable;
	std::mutex _peersMutex;
	std::set<uint64_t> _peers;
};
#endif
