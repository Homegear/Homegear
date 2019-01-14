/* Copyright 2013-2019 Homegear GmbH
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

#ifndef NO_SCRIPTENGINE

#include "../GD/GD.h"
#include "PhpEvents.h"

namespace Homegear
{

std::mutex PhpEvents::eventsMapMutex;
std::map<int32_t, std::shared_ptr<PhpEvents>> PhpEvents::eventsMap;

PhpEvents::PhpEvents(std::string& token, std::function<void(std::string output, bool error)>& outputCallback, std::function<BaseLib::PVariable(std::string methodName, BaseLib::PVariable parameters, bool wait)>& rpcCallback)
{
	_stopProcessing = false;
	_bufferCount = 0;
	_token = token;
	_outputCallback = outputCallback;
	_rpcCallback = rpcCallback;
}

PhpEvents::~PhpEvents()
{
	stop();
}

void PhpEvents::stop()
{
	if(_stopProcessing) return;
	_stopProcessing = true;
	_processingConditionVariable.notify_all();
}

bool PhpEvents::enqueue(std::shared_ptr<EventData>& entry)
{
	try
	{
		if(!entry || _stopProcessing) return false;
		std::unique_lock<std::mutex> lock(_queueMutex);
		if(_stopProcessing) return true;

		if(_bufferCount >= _bufferSize) return false;

		{
			std::lock_guard<std::mutex> bufferGuard(_bufferMutex);
			_buffer[_bufferTail] = entry;
			_bufferTail = (_bufferTail + 1) % _bufferSize;
			++_bufferCount;
		}

		_processingConditionVariable.notify_one();
		return true;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

std::shared_ptr<PhpEvents::EventData> PhpEvents::poll(int32_t timeout)
{
	if(timeout < 1) timeout = 10000;
	std::shared_ptr<EventData> eventData;
	if(_stopProcessing) return eventData;
	try
	{
		{
			std::unique_lock<std::mutex> lock(_queueMutex);

			if(!_processingConditionVariable.wait_for(lock, std::chrono::milliseconds(timeout), [&] { return _bufferCount > 0 || _stopProcessing; })) return eventData;
			if(_stopProcessing) return eventData;

			std::lock_guard<std::mutex> bufferGuard(_bufferMutex);
			eventData = _buffer[_bufferHead];
			_buffer[_bufferHead].reset();
			_bufferHead = (_bufferHead + 1) % _bufferSize;
			--_bufferCount;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return eventData;
}

void PhpEvents::addPeer(uint64_t peerId, int32_t channel, std::string& variable)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		if(channel > -1 && !variable.empty()) _peers[peerId][channel].insert(variable);
		else _peers.emplace(std::make_pair(peerId, std::map<int32_t, std::set<std::string>>()));
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void PhpEvents::removePeer(uint64_t peerId, int32_t channel, std::string& variable)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		if(channel > -1 && !variable.empty())
		{
			_peers[peerId][channel].erase(variable);
			if(_peers[peerId][channel].empty()) _peers[peerId].erase(channel);
		}
		else _peers.erase(peerId);
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

bool PhpEvents::peerSubscribed(uint64_t peerId, int32_t channel, std::string& variable)
{
	try
	{
		std::lock_guard<std::mutex> peersGuard(_peersMutex);
		auto peerIterator = _peers.find(peerId);
		if(peerIterator != _peers.end())
		{
			if(!peerIterator->second.empty() && channel > -1 && !variable.empty())
			{
				auto channelIterator = peerIterator->second.find(channel);
				if(channelIterator == peerIterator->second.end()) return false;
				return channelIterator->second.find(variable) != channelIterator->second.end();
			}
			else return true;
		}
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(const BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return false;
}

}

#endif
