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

#include "IQueue.h"
#include "BaseLib.h"

namespace BaseLib
{

IQueue::IQueue(Obj* baseLib, int32_t threadPriority, int32_t threadPolicy)
{
	_bl = baseLib;
	_stopProcessingThread = false;
	_processingEntryAvailable = false;
	_bufferHead = 0;
	_bufferTail = 0;
	_processingThread = std::thread(&IQueue::process, this);
	Threads::setThreadPriority(_bl, _processingThread.native_handle(), threadPriority, threadPolicy);
}

IQueue::~IQueue()
{
	_stopProcessingThread = true;
	_processingEntryAvailable = true;
	_processingConditionVariable.notify_one();
	if(_processingThread.joinable()) _processingThread.join();
}

bool IQueue::enqueue(std::shared_ptr<IQueueEntry>& entry)
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
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_bufferMutex.unlock();
	}
	catch(BaseLib::Exception& ex)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_bufferMutex.unlock();
	}
	catch(...)
	{
		_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_bufferMutex.unlock();
	}
	return false;
}

void IQueue::process()
{
	while(!_stopProcessingThread)
	{
		std::unique_lock<std::mutex> lock(_processingThreadMutex);
		try
		{
			_bufferMutex.lock();
			if(_bufferHead == _bufferTail) //Only lock, when there is really no packet to process. This check is necessary, because the check of the while loop condition is outside of the mutex
			{
				_bufferMutex.unlock();
				_processingConditionVariable.wait(lock, [&]{ return _processingEntryAvailable; });
			}
			else _bufferMutex.unlock();
			if(_stopProcessingThread)
			{
				lock.unlock();
				return;
			}

			while(_bufferHead != _bufferTail)
			{
				_bufferMutex.lock();
				std::shared_ptr<IQueueEntry> entry = _buffer[_bufferTail];
				_buffer[_bufferTail].reset();
				_bufferTail++;
				if(_bufferTail >= _bufferSize) _bufferTail = 0;
				if(_bufferHead == _bufferTail) _processingEntryAvailable = false; //Set here, because otherwise it might be set to "true" in publish and then set to false again after the while loop
				_bufferMutex.unlock();
				if(entry) processQueueEntry(entry);
			}
		}
		catch(const std::exception& ex)
		{
			_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(const BaseLib::Exception& ex)
		{
			_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		lock.unlock();
	}
}

}
