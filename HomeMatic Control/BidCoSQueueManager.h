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
	std::shared_ptr<BidCoSQueue> queue;
	std::shared_ptr<std::thread> thread;
	bool stopThread = false;
	int64_t lastAction;

	BidCoSQueueData();
	virtual ~BidCoSQueueData() {}
};

class BidCoSQueueManager {
public:
	BidCoSQueueManager() {}
	virtual ~BidCoSQueueManager();

	BidCoSQueue* get(int32_t address);
	BidCoSQueue* createQueue(HomeMaticDevice* device, BidCoSQueueType queueType, int32_t address);
	void resetQueue(int32_t address);
protected:
	std::unordered_map<int32_t, BidCoSQueueData> _queues;
};

#endif /* BIDCOSQUEUEMANAGER_H_ */
