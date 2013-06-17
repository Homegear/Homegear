#ifndef BIDCOSPACKETMANAGER_H_
#define BIDCOSPACKETMANAGER_H_

class BidCoSPacket;

#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>

class BidCoSPacketInfo
{
public:
	BidCoSPacketInfo();
	virtual ~BidCoSPacketInfo() {}

	uint32_t id = 0;
	int64_t time;
	std::shared_ptr<BidCoSPacket> packet;
	std::shared_ptr<std::thread> thread;
	bool stopThread = false;
};

class BidCoSPacketManager
{
public:
	BidCoSPacketManager() {}
	virtual ~BidCoSPacketManager();

	std::shared_ptr<BidCoSPacket> get(int32_t address);
	std::shared_ptr<BidCoSPacketInfo> getInfo(int32_t address);
	void set(int32_t address, std::shared_ptr<BidCoSPacket>& packet);
	void deletePacket(int32_t address, uint32_t id);
	void keepAlive(int32_t address);
protected:
	uint32_t _id = 0;
	std::unordered_map<int32_t, std::shared_ptr<BidCoSPacketInfo>> _packets;
};

#endif /* BIDCOSPACKETMANAGER_H_ */
