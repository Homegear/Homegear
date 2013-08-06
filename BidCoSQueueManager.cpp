#include "BidCoSQueueManager.h"
#include "GD.h"

BidCoSQueueData::BidCoSQueueData()
{
	queue = std::shared_ptr<BidCoSQueue>(new BidCoSQueue());
	lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
		queueData->queue->lastAction = &queueData->lastAction;
		queueData->queue->id = _id++;
		queueData->id = queueData->queue->id;
		queueData->thread = std::shared_ptr<std::thread>(new std::thread(&BidCoSQueueManager::resetQueue, this, address, queueData->id));
		queueData->thread->detach();
		_queueMutex.lock();
		_queues.insert(std::pair<int32_t, std::shared_ptr<BidCoSQueueData>>(address, queueData));
		_queueMutex.unlock();
		return queueData->queue;
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return std::shared_ptr<BidCoSQueue>();
}

void BidCoSQueueManager::resetQueue(int32_t address, uint32_t id)
{
	try
	{
		std::chrono::milliseconds sleepingTime(400);
		while(true)
		{
			_queueMutex.lock();
			if(_queues.find(address) != _queues.end() && _queues.at(address) && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= _queues.at(address)->lastAction + 1000 && !_disposing)
			{
				_queueMutex.unlock();
				std::this_thread::sleep_for(sleepingTime);
			}
			else
			{
				_queueMutex.unlock();
				break;
			}
		}

		_queueMutex.lock();
		if(_queues.find(address) != _queues.end() && _queues.at(address) && _queues.at(address)->id == id)
		{
			if(GD::debugLevel >= 5) std::cout << "Deleting queue " << id << " for 0x" << std::hex << address << std::dec << std::endl;
			std::shared_ptr<BidCoSQueueData> queue = _queues.at(address);
			_queues.erase(address);
			if(!queue->queue->isEmpty())
			{
				std::shared_ptr<Peer> peer = queue->queue->peer;
				if(peer && peer->rpcDevice && !(peer->rpcDevice->rxModes & RPC::Device::RXModes::Enum::wakeUp))
				{
					peer->serviceMessages->setUnreach(true);
				}
			}
		}
		_queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
		_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
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
		_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	_queueMutex.unlock();
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
    }
    return std::shared_ptr<BidCoSQueue>();
}
