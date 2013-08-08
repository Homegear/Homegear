#ifndef BIDCOSQUEUEMANAGER_H_
#define BIDCOSQUEUEMANAGER_H_

class HomeMaticDevice;
class BidCoSQueue;
enum class BidCoSQueueType;

#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>

class BidCoSQueueData
{
public:
	uint32_t id = 0;
	std::shared_ptr<BidCoSQueue> queue;
	int64_t lastAction;

	BidCoSQueueData();
	virtual ~BidCoSQueueData() {}
};

class BidCoSQueueManager {
public:
	BidCoSQueueManager() {}
	virtual ~BidCoSQueueManager() {}

	std::shared_ptr<BidCoSQueue> get(int32_t address);
	std::shared_ptr<BidCoSQueue> createQueue(HomeMaticDevice* device, BidCoSQueueType queueType, int32_t address);
	void resetQueue(int32_t address, uint32_t id);
	void dispose(bool wait = true);
protected:
	bool _disposing = false;
	uint32_t _id = 0;
	std::unordered_map<int32_t, std::shared_ptr<BidCoSQueueData>> _queues;
	std::mutex _queueMutex;
};

#endif /* BIDCOSQUEUEMANAGER_H_ */
