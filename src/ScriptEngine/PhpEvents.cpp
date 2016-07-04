/* Copyright 2013-2016 Sathya Laufer
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


#include "../GD/GD.h"
#include "PhpEvents.h"

std::mutex PhpEvents::eventsMapMutex;
std::map<int32_t, std::shared_ptr<PhpEvents>> PhpEvents::eventsMap;

PhpEvents::PhpEvents(std::string& token, std::function<void(std::string& output)>& outputCallback, std::function<BaseLib::PVariable(std::string& methodName, BaseLib::PVariable& parameters)>& rpcCallback)
{
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
	_processingEntryAvailable = true;
	_processingConditionVariable.notify_one();
}

bool PhpEvents::enqueue(std::shared_ptr<EventData>& entry)
{
	try
	{
		_bufferMutex.lock();
		int32_t tempHead = _bufferHead + 1;
		if(tempHead >= _bufferSize) tempHead = 0;
		if(tempHead == _bufferTail)
		{
			_bufferMutex.unlock();
			return false;
		}

		_buffer[_bufferHead] = entry;
		_bufferHead++;
		if(_bufferHead >= _bufferSize)
		{
			_bufferHead = 0;
		}
		_processingEntryAvailable = true;
		_bufferMutex.unlock();

		_processingConditionVariable.notify_one();
		return true;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_bufferMutex.unlock();
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_bufferMutex.unlock();
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_bufferMutex.unlock();
	}
	return false;
}

std::shared_ptr<PhpEvents::EventData> PhpEvents::poll(int32_t timeout)
{
	std::shared_ptr<EventData> eventData;
	std::unique_lock<std::mutex> lock(_processingThreadMutex);
	try
	{
		{
			BaseLib::DisposableLockGuard bufferGuard(_bufferMutex);
			if(_bufferHead == _bufferTail)
			{
				bufferGuard.dispose();
				if(timeout > 0)	_processingConditionVariable.wait_for(lock, std::chrono::milliseconds(timeout), [&]{ return _processingEntryAvailable; });
				else _processingConditionVariable.wait(lock, [&]{ return _processingEntryAvailable; });
			}
		}
		if(GD::bl->shuttingDown)
		{
			lock.unlock();
			return eventData;
		}

		_bufferMutex.lock();
		eventData = _buffer[_bufferTail];
		_buffer[_bufferTail].reset();
		_bufferTail++;
		if(_bufferTail >= _bufferSize) _bufferTail = 0;
		if(_bufferHead == _bufferTail) _processingEntryAvailable = false;
		_bufferMutex.unlock();
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
	lock.unlock();
	return eventData;
}

void PhpEvents::addPeer(uint64_t peerId)
{
	_peersMutex.lock();
	try
	{
		_peers.insert(peerId);
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
	_peersMutex.unlock();
}

void PhpEvents::removePeer(uint64_t peerId)
{
	_peersMutex.lock();
	try
	{
		_peers.erase(peerId);
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
	_peersMutex.unlock();
}

bool PhpEvents::peerSubscribed(uint64_t peerId)
{
	_peersMutex.lock();
	try
	{
		if(_peers.find(peerId) != _peers.end())
		{
			_peersMutex.unlock();
			return true;
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
	_peersMutex.unlock();
	return false;
}
