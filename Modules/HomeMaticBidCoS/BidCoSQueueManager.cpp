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

#include "BidCoSQueueManager.h"
#include "../Base/BaseLib.h"
#include "GD.h"

namespace BidCoS
{
BidCoSQueueData::BidCoSQueueData(std::shared_ptr<IBidCoSInterface> physicalInterface)
{
	if(!physicalInterface) physicalInterface = GD::defaultPhysicalInterface;
	queue = std::shared_ptr<BidCoSQueue>(new BidCoSQueue(physicalInterface));
	lastAction.reset(new int64_t);
	*lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

BidCoSQueueManager::BidCoSQueueManager()
{
}

BidCoSQueueManager::~BidCoSQueueManager()
{
	try
	{
		if(!_disposing) dispose();
		_workerThreadMutex.lock();
		if(_workerThread.joinable()) _workerThread.join();
		if(_resetQueueThread.joinable()) _resetQueueThread.join(); //After waiting for worker thread!
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
	_workerThreadMutex.unlock();
}

void BidCoSQueueManager::dispose(bool wait)
{
	_disposing = true;
	_stopWorkerThread = true;
}

void BidCoSQueueManager::worker()
{
	try
	{
		std::chrono::milliseconds sleepingTime(100);
		int32_t lastQueue;
		lastQueue = 0;
		while(!_stopWorkerThread)
		{
			try
			{
				std::this_thread::sleep_for(sleepingTime);
				if(_stopWorkerThread) return;
				_queueMutex.lock();
				if(!_queues.empty())
				{
					if(!_queues.empty())
					{
						std::unordered_map<int32_t, std::shared_ptr<BidCoSQueueData>>::iterator nextQueue = _queues.find(lastQueue);
						if(nextQueue != _queues.end())
						{
							nextQueue++;
							if(nextQueue == _queues.end()) nextQueue = _queues.begin();
						}
						else nextQueue = _queues.begin();
						lastQueue = nextQueue->first;
					}
				}
				std::shared_ptr<BidCoSQueueData> queue;
				if(_queues.find(lastQueue) != _queues.end()) queue = _queues.at(lastQueue);
				_queueMutex.unlock();
				if(queue)
				{
					if(_resetQueueThread.joinable()) _resetQueueThread.join();
					//Has to be called in a thread as resetQueue might cause queuing (retrying in setUnreach) and therefore a deadlock
					_resetQueueThread = std::thread(&BidCoSQueueManager::resetQueue, this, lastQueue, queue->id);
					_resetQueueThread.detach();
				}
			}
			catch(const std::exception& ex)
			{
				_queueMutex.unlock();
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(BaseLib::Exception& ex)
			{
				_queueMutex.unlock();
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				_queueMutex.unlock();
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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
}

std::shared_ptr<BidCoSQueue> BidCoSQueueManager::createQueue(HomeMaticDevice* device, std::shared_ptr<IBidCoSInterface> physicalInterface, BidCoSQueueType queueType, int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSQueue>();
		if(!physicalInterface) physicalInterface = GD::defaultPhysicalInterface;
		_queueMutex.lock();
		if(_stopWorkerThread)
		{
			_queueMutex.unlock();
			_workerThreadMutex.lock();
			if(_stopWorkerThread)
			{
				try //Catch "Resource deadlock avoided". Error should be fixed, but just in case.
				{
					if(_workerThread.joinable()) _workerThread.join();
					_stopWorkerThread = false;
					_workerThread = std::thread(&BidCoSQueueManager::worker, this);
					BaseLib::Threads::setThreadPriority(GD::bl, _workerThread.native_handle(), 19);
				}
				catch(const std::exception& ex)
				{
					GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
				}
				catch(...)
				{
					GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
				}
			}
			_workerThreadMutex.unlock();
		}
		else if(_queues.find(address) != _queues.end())
		{
			_queues.erase(_queues.find(address));
			_queueMutex.unlock();
		}
		else _queueMutex.unlock();

		std::shared_ptr<BidCoSQueueData> queueData(new BidCoSQueueData(physicalInterface));
		queueData->queue->setQueueType(queueType);
		queueData->queue->device = device;
		queueData->queue->lastAction = queueData->lastAction;
		queueData->queue->id = _id++;
		queueData->id = queueData->queue->id;
		_queueMutex.lock();
		_queues.insert(std::pair<int32_t, std::shared_ptr<BidCoSQueueData>>(address, queueData));
		_queueMutex.unlock();
		GD::out.printDebug("Creating SAVEPOINT BidCoSQueue" + std::to_string(address) + "_" + std::to_string(queueData->id));
		raiseCreateSavepoint("BidCoSQueue" + std::to_string(address) + "_" + std::to_string(queueData->id));
		return queueData->queue;
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
    _queueMutex.unlock();
    _workerThreadMutex.unlock();
    return std::shared_ptr<BidCoSQueue>();
}

void BidCoSQueueManager::resetQueue(int32_t address, uint32_t id)
{
	try
	{
		if(_disposing) return;
		_queueMutex.lock();
		if(_queues.find(address) != _queues.end() && _queues.at(address) && _queues.at(address)->queue && !_queues.at(address)->queue->isEmpty() && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= *_queues.at(address)->lastAction + 2000)
		{
			if(_queues.empty()) _stopWorkerThread = true;
			_queueMutex.unlock();
			return;
		}

		std::shared_ptr<BidCoSQueueData> queue;
		std::shared_ptr<BidCoSPeer> peer;
		bool setUnreach = false;
		if(_queues.find(address) != _queues.end() && _queues.at(address) && _queues.at(address)->id == id)
		{
			GD::out.printDebug("Debug: Deleting queue " + std::to_string(id) + " for BidCoS peer with address 0x" + BaseLib::HelperFunctions::getHexString(address));
			queue = _queues.at(address);
			_queues.erase(address);
			if(!queue->queue->isEmpty() && queue->queue->getQueueType() != BidCoSQueueType::PAIRING)
			{
				peer = queue->queue->peer;
				if(peer && peer->rpcDevice && ((peer->getRXModes() & BaseLib::RPC::Device::RXModes::Enum::always) || (peer->getRXModes() & BaseLib::RPC::Device::RXModes::Enum::burst)))
				{
					setUnreach = true;
				}
			}
			queue->queue->dispose();
		}
		//setUnreach calls enqueuePendingQueues, which calls BidCoSQueueManger::get => deadlock,
		//so we need to unlock first
		if(_queues.empty()) _stopWorkerThread = true;
		_queueMutex.unlock();
		if(setUnreach) peer->serviceMessages->setUnreach(true);
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_queueMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::out.printDebug("Releasing SAVEPOINT BidCoSQueue" + std::to_string(address) + "_" + std::to_string(id));
    raiseReleaseSavepoint("BidCoSQueue" + std::to_string(address) + "_" + std::to_string(id));
}

std::shared_ptr<BidCoSQueue> BidCoSQueueManager::get(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSQueue>();
		_queueMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSQueue> queue((_queues.find(address) != _queues.end()) ? _queues[address]->queue : std::shared_ptr<BidCoSQueue>());
		if(queue) queue->keepAlive(); //Don't delete queue in the next second
		_queueMutex.unlock();
		return queue;
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
    _queueMutex.unlock();
    return std::shared_ptr<BidCoSQueue>();
}

//Event handling
void BidCoSQueueManager::raiseCreateSavepoint(std::string name)
{
	if(_eventHandler) ((IBidCoSQueueManagerEventSink*)_eventHandler)->onQueueCreateSavepoint(name);
}

void BidCoSQueueManager::raiseReleaseSavepoint(std::string name)
{
	if(_eventHandler) ((IBidCoSQueueManagerEventSink*)_eventHandler)->onQueueReleaseSavepoint(name);
}
//End event hantlind
}
