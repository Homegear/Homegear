#include "BidCoSQueueManager.h"
#include "GD.h"

BidCoSQueueData::BidCoSQueueData()
{
	queue = std::shared_ptr<BidCoSQueue>(new BidCoSQueue());
	lastAction = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

BidCoSQueueManager::~BidCoSQueueManager()
{
	for(std::unordered_map<int32_t, BidCoSQueueData>::iterator i = _queues.begin(); i != _queues.end(); ++i)
	{
		i->second.stopThread = true;
	}
}

BidCoSQueue* BidCoSQueueManager::createQueue(HomeMaticDevice* device, BidCoSQueueType queueType, int32_t address)
{
	if(_queues.find(address) != _queues.end() && _queues[address].thread.get() != nullptr && _queues[address].thread->joinable())
	{
		_queues[address].stopThread = true;
		_queues[address].thread->join();
	}
	if(_queues.find(address) != _queues.end()) _queues.erase(_queues.find(address));

	BidCoSQueueData* queueData = &_queues[address];
	queueData->queue->setQueueType(queueType);
	queueData->queue->device = device;
	queueData->queue->lastAction = &queueData->lastAction;
	queueData->thread = std::shared_ptr<std::thread>(new std::thread(&BidCoSQueueManager::resetQueue, this, address));
	queueData->thread->detach();
	return queueData->queue.get();
}

void BidCoSQueueManager::resetQueue(int32_t address)
{
	std::chrono::milliseconds sleepingTime(300);
	while(_queues.find(address) != _queues.end() && !_queues[address].stopThread && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() <= _queues[address].lastAction + 1000)
	{
		std::this_thread::sleep_for(sleepingTime);
	}
	if(_queues.find(address) != _queues.end() && !_queues[address].stopThread)
	{
		if(GD::debugLevel >= 5) cout << "Deleting queue for 0x" << std::hex << address << std::dec << endl;
		_queues.erase(_queues.find(address));
	}
}

BidCoSQueue* BidCoSQueueManager::get(int32_t address)
{
	if(_queues.find(address) != _queues.end()) return _queues[address].queue.get();
	return nullptr;
}
