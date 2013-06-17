#include "BidCoSQueueManager.h"
#include "GD.h"

BidCoSQueueData::BidCoSQueueData()
{
	queue = std::shared_ptr<BidCoSQueue>(new BidCoSQueue());
	lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

BidCoSQueueManager::~BidCoSQueueManager()
{
	for(std::unordered_map<int32_t, std::shared_ptr<BidCoSQueueData>>::iterator i = _queues.begin(); i != _queues.end(); ++i)
	{
		i->second->stopThread = true;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(1000)); //Wait for threads to finish
}

std::shared_ptr<BidCoSQueue> BidCoSQueueManager::createQueue(HomeMaticDevice* device, BidCoSQueueType queueType, int32_t address)
{
	try
	{
		if(_queues.find(address) != _queues.end() && _queues[address]->thread) _queues[address]->stopThread = true;
		if(_queues.find(address) != _queues.end()) _queues.erase(_queues.find(address));

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
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _queueMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _queueMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _queueMutex.unlock();
    }
    return std::shared_ptr<BidCoSQueue>();
}

void BidCoSQueueManager::resetQueue(int32_t address, uint32_t id)
{
	try
	{
		std::chrono::milliseconds sleepingTime(300);
		while(_queues.find(address) != _queues.end() && _queues[address] && !_queues[address]->stopThread && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= _queues[address]->lastAction + 1000)
		{
			std::this_thread::sleep_for(sleepingTime);
		}
		_queueMutex.lock();
		if(_queues.find(address) != _queues.end() && _queues[address] && !_queues[address]->stopThread && _queues[address]->id == id)
		{
			if(GD::debugLevel >= 5) std::cout << "Deleting queue " << id << " for 0x" << std::hex << address << std::dec << std::endl;
			_queues.erase(_queues.find(address));
		}
		_queueMutex.unlock();
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _queueMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _queueMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _queueMutex.unlock();
    }
}

std::shared_ptr<BidCoSQueue> BidCoSQueueManager::get(int32_t address)
{
	try
	{
		_queueMutex.lock();
		//Make a copy to make sure, the element exists
		std::shared_ptr<BidCoSQueue> queue((_queues.find(address) != _queues.end()) ? _queues[address]->queue : nullptr);
		_queueMutex.unlock();
		return queue;
	}
	catch(const std::exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _queueMutex.unlock();
    }
    catch(const Exception& ex)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
        _queueMutex.unlock();
    }
    catch(...)
    {
        std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<"." << std::endl;
        _queueMutex.unlock();
    }
    return std::shared_ptr<BidCoSQueue>();
}
