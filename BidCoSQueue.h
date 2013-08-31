#ifndef BIDCOSQUEUE_H
#define BIDCOSQUEUE_H

class Peer;
class BidCoSMessage;
class BidCoSPacket;
class HomeMaticDevice;
class ServiceMessages;

#include <iostream>
#include <string>
#include <list>
#include <queue>
#include <thread>
#include <mutex>

#include "delegate.hpp"
#include "Exception.h"
#include "PendingBidCoSQueues.h"

enum class QueueEntryType { UNDEFINED, MESSAGE, PACKET };

class CallbackFunctionParameter
{
public:
	std::vector<int64_t> integers;
	std::vector<std::string> strings;

	CallbackFunctionParameter() {}
	virtual ~CallbackFunctionParameter() {}
};

class BidCoSQueueEntry {
protected:
	QueueEntryType _type = QueueEntryType::UNDEFINED;
	std::shared_ptr<BidCoSMessage> _message;
	std::shared_ptr<BidCoSPacket> _packet;
public:
	bool stealthy = false;
	bool forceResend= false;

	BidCoSQueueEntry() {}
	virtual ~BidCoSQueueEntry() {}
	QueueEntryType getType() { return _type; }
	void setType(QueueEntryType type) { _type = type; }
	std::shared_ptr<BidCoSPacket> getPacket() { return _packet; }
	void setPacket(std::shared_ptr<BidCoSPacket> packet, bool setQueueEntryType) { _packet = packet; if(setQueueEntryType) _type = QueueEntryType::PACKET; }
	std::shared_ptr<BidCoSMessage> getMessage() { return _message; }
	void setMessage(std::shared_ptr<BidCoSMessage> message, bool setQueueEntryType) { _message = message; if(setQueueEntryType) _type = QueueEntryType::MESSAGE; }
};

enum class BidCoSQueueType { EMPTY, DEFAULT, CONFIG, PAIRING, PAIRINGCENTRAL, UNPAIRING, PEER };

class BidCoSQueue
{
    protected:
		bool _disposing = false;
		//I'm using list, so iterators are not invalidated
        std::list<BidCoSQueueEntry> _queue;
        std::shared_ptr<PendingBidCoSQueues> _pendingQueues;
        std::mutex _queueMutex;
        std::mutex _sendThreadMutex;
        BidCoSQueueType _queueType;
        bool _stopResendThread = false;
        std::thread _resendThread;
        int32_t _resendCounter = 0;
        uint32_t _resendThreadId = 0;
        bool _stopPopWaitThread = false;
        uint32_t _popWaitThreadId = 0;
        std::thread _popWaitThread;
        std::thread _sendThread;
        std::thread _startResendThread;
        std::thread _pushPendingQueueThread;
        bool _workingOnPendingQueue = false;
        int64_t _lastPop = 0;
        void (HomeMaticDevice::*_queueProcessed)() = nullptr;
        void pushPendingQueue();
        void sleepAndPushPendingQueue();
        void resend(uint32_t threadId, bool burst);
        void startResendThread(bool force);
        void stopResendThread();
        void popWaitThread(uint32_t threadId, uint32_t waitingTime);
        void stopPopWaitThread();
        void nextQueueEntry();
    public:
        uint32_t retries = 4;
        uint32_t id = 0;
        std::shared_ptr<int64_t> lastAction;
        bool noSending = false;
        HomeMaticDevice* device = nullptr;
        std::shared_ptr<Peer> peer;
        std::shared_ptr<CallbackFunctionParameter> callbackParameter;
        delegate<void (std::shared_ptr<CallbackFunctionParameter>)> queueEmptyCallback;
        BidCoSQueueType getQueueType() { return _queueType; }
        std::list<BidCoSQueueEntry>* getQueue() { return &_queue; }
        void setQueueType(BidCoSQueueType queueType) {  _queueType = queueType; }

        void push(std::shared_ptr<BidCoSMessage> message, bool forceResend = false);
        void push(std::shared_ptr<BidCoSMessage> message, std::shared_ptr<BidCoSPacket> packet, bool forceResend = false);
        void pushFront(std::shared_ptr<BidCoSPacket> packet, bool stealthy = false, bool popBeforePushing = false, bool forceResend = false);
        void push(std::shared_ptr<BidCoSPacket> packet, bool forceResend = false, bool stealthy = false);
        void push(std::shared_ptr<PendingBidCoSQueues>& pendingBidCoSQueues);
        void push(std::shared_ptr<BidCoSQueue> pendingBidCoSQueue, bool popImmediately, bool clearPendingQueues);
        BidCoSQueueEntry* front() { return &_queue.front(); }
        void pop();
        void popWait(uint32_t waitingTime);
        bool isEmpty() { return _queue.empty() && (!_pendingQueues || _pendingQueues->empty()); }
        bool pendingQueuesEmpty() { return (!_pendingQueues || _pendingQueues->empty()); }
        void clear();
        void send(std::shared_ptr<BidCoSPacket> packet, bool stealthy);
        void keepAlive();
        void longKeepAlive();
        void dispose();
        void serialize(std::vector<uint8_t>& encodedData);
        void unserialize(std::shared_ptr<std::vector<char>> serializedData, HomeMaticDevice* device, uint32_t position = 0);
        void unserialize_0_0_6(std::string serializedObject, HomeMaticDevice* device);

        BidCoSQueue();
        BidCoSQueue(BidCoSQueueType queueType);
        virtual ~BidCoSQueue();
};

#endif // BIDCOSQUEUE_H
