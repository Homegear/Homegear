#ifndef PENDINGBIDCOSQUEUES_H_
#define PENDINGBIDCOSQUEUES_H_

class BidCoSQueue;
class Peer;
class HomeMaticDevice;

#include <string>
#include <iostream>
#include <memory>
#include <queue>
#include <mutex>

#include "Exception.h"

class PendingBidCoSQueues {
public:
	PendingBidCoSQueues();
	PendingBidCoSQueues(std::string serializedObject, Peer* peer, HomeMaticDevice* device);
	virtual ~PendingBidCoSQueues() {}

	void push(std::shared_ptr<BidCoSQueue> queue);
	void pop();
	bool empty();
	uint32_t size();
	std::shared_ptr<BidCoSQueue> front();
	void clear();
	std::string serialize();
private:
	std::mutex _queuesMutex;
	//Cannot be unique_ptr because class must be copyable
    std::deque<std::shared_ptr<BidCoSQueue>> _queues;
};

#endif /* PENDINGBIDCOSQUEUES_H_ */
