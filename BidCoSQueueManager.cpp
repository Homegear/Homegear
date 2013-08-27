#include "BidCoSQueueManager.h"
#include "GD.h"
#include "HelperFunctions.h"

BidCoSQueueData::BidCoSQueueData()
{
	queue = std::shared_ptr<BidCoSQueue>(new BidCoSQueue());
	lastAction.reset(new int64_t);
	*lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void BidCoSQueueManager::dispose(bool wait)
{
	_disposing = true;
	if(wait) std::this_thread::sleep_for(std::chrono::milliseconds(1500)); //Wait for threads to finish
}

std::shared_ptr<BidCoSQueue> BidCoSQueueManager::createQueue(HomeMaticDevice* device, BidCoSQueueType queueType, int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSQueue>();
		_queueMutex.lock();
		if(_queues.find(address) != _queues.end()) _queues.erase(_queues.find(address));
		_queueMutex.unlock();

		std::shared_ptr<BidCoSQueueData> queueData(new BidCoSQueueData());
		queueData->queue->setQueueType(queueType);
		queueData->queue->device = device;
		queueData->queue->lastAction = queueData->lastAction;
		queueData->queue->id = _id++;
		queueData->id = queueData->queue->id;
		std::thread t(&BidCoSQueueManager::resetQueue, this, address, queueData->id);
		t.detach();
		_queueMutex.lock();
		_queues.insert(std::pair<int32_t, std::shared_ptr<BidCoSQueueData>>(address, queueData));
		_queueMutex.unlock();
		return queueData->queue;
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
    _queueMutex.unlock();
    return std::shared_ptr<BidCoSQueue>();
}

void BidCoSQueueManager::resetQueue(int32_t address, uint32_t id)
{
	try
	{
		std::chrono::milliseconds sleepingTime(400);
		while(true)
		{
			if(_disposing) return;
			_queueMutex.lock();
			if(_queues.find(address) != _queues.end() && _queues.at(address) && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= *_queues.at(address)->lastAction + 1000)
			{
				_queueMutex.unlock();
				std::this_thread::sleep_for(sleepingTime);
				if(_disposing) return;
			}
			else if(_disposing)
			{
				_queueMutex.unlock();
				return;
			}
			else
			{
				_queueMutex.unlock();
				break;
			}
		}

		std::shared_ptr<BidCoSQueueData> queue;
		std::shared_ptr<Peer> peer;
		bool setUnreach = false;
		_queueMutex.lock();
		if(_queues.find(address) != _queues.end() && _queues.at(address) && _queues.at(address)->id == id)
		{
			HelperFunctions::printDebug("Debug: Deleting queue " + std::to_string(id) + " for 0x" + HelperFunctions::getHexString(address));
			queue = _queues.at(address);
			_queues.erase(address);
			if(!queue->queue->isEmpty() && queue->queue->getQueueType() != BidCoSQueueType::PAIRING)
			{
				peer = queue->queue->peer;
				if(peer && peer->rpcDevice && ((peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::always) || (peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::burst)))
				{
					setUnreach = true;
				}
			}
			queue->queue->dispose();
		}
		//setUnreach calls enqueuePendingQueues, which calls BidCoSQueueManger::get => deadlock,
		//so we need to unlock first
		_queueMutex.unlock();
		if(setUnreach) peer->serviceMessages->setUnreach(true);
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_queueMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_queueMutex.unlock();
        HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BidCoSQueue> BidCoSQueueManager::get(int32_t address)
{
	try
	{
		if(_disposing) return std::shared_ptr<BidCoSQueue>();
		_queueMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSQueue> queue((_queues.find(address) != _queues.end()) ? _queues[address]->queue : std::shared_ptr<BidCoSQueue>());
		_queueMutex.unlock();
		return queue;
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
    _queueMutex.unlock();
    return std::shared_ptr<BidCoSQueue>();
}
