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
	virtual ~PendingBidCoSQueues() {}
	void serialize(std::vector<uint8_t>& encodedData);
	void unserialize(std::shared_ptr<std::vector<char>> serializedData, Peer* peer, HomeMaticDevice* device);
	void unserialize_0_0_6(std::string serializedObject, Peer* peer, HomeMaticDevice* device);

	void push(std::shared_ptr<BidCoSQueue> queue);
	void pop();
	bool empty();
	uint32_t size();
	std::shared_ptr<BidCoSQueue> front();
	void clear();
private:
	std::mutex _queuesMutex;
	//Cannot be unique_ptr because class must be copyable
    std::deque<std::shared_ptr<BidCoSQueue>> _queues;
};

#endif /* PENDINGBIDCOSQUEUES_H_ */
