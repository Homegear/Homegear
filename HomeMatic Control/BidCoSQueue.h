#ifndef BIDCOSQUEUE_H
#define BIDCOSQUEUE_H

#include "Peer.h"
#include "Cul.h"
#include "BidCoSMessage.h"
#include "Exception.h"

#include <deque>
#include <queue>
#include <thread>
#include <mutex>

class Cul;
class BidCoSMessage;

enum class QueueEntryType { UNDEFINED, MESSAGE, PACKET };

class BidCoSQueueEntry {
protected:
	QueueEntryType _type = QueueEntryType::UNDEFINED;
	shared_ptr<BidCoSMessage> _message;
	BidCoSPacket _packet;
public:
	BidCoSQueueEntry() {}
	virtual ~BidCoSQueueEntry() {}
	QueueEntryType getType() { return _type; }
	BidCoSPacket* getPacket() { return &_packet; }
	void setPacket(const BidCoSPacket& packet, bool setQueueEntryType) { _packet = packet; if(setQueueEntryType) _type = QueueEntryType::PACKET; }
	shared_ptr<BidCoSMessage> getMessage() { return _message; }
	void setMessage(shared_ptr<BidCoSMessage> message) { _message = message; _type = QueueEntryType::MESSAGE; }
};

enum class BidCoSQueueType { EMPTY, DEFAULT, PAIRING, PAIRINGCENTRAL, UNPAIRING };

class BidCoSQueue
{
    protected:
        std::deque<BidCoSQueueEntry> _queue;
        shared_ptr<std::queue<shared_ptr<BidCoSQueue>>> _pendingQueues;
        std::mutex _queueMutex;
        BidCoSQueueType _queueType;
        bool _stopResendThread = false;
        unique_ptr<std::thread> _resendThread;
        std::mutex _resendThreadMutex;
        int32_t resendCounter = 0;
        bool _workingOnPendingQueue = false;

        void pushPendingQueue();
        void sleepAndPushPendingQueue();
    public:
        uint64_t* lastAction = nullptr;
        bool noSending = false;
        HomeMaticDevice* device = nullptr;
        Peer peer;
        BidCoSQueueType getQueueType() { return _queueType; }
        std::deque<BidCoSQueueEntry>* getQueue() { return &_queue; }
        void setQueueType(BidCoSQueueType queueType) {  _queueType = queueType; }

        void push(shared_ptr<BidCoSMessage> message);
        void push(shared_ptr<BidCoSMessage> message, BidCoSPacket* packet);
        void push(const BidCoSPacket& packet);
        void push(shared_ptr<std::queue<shared_ptr<BidCoSQueue>>>& pendingBidCoSQueue);
        BidCoSQueueEntry* front() { return &_queue.front(); }
        void pop();
        bool isEmpty() { return _queue.empty(); }
        void clear();
        void resend();
        void startResendThread();
        void send(BidCoSPacket packet);
        void keepAlive();
        std::string serialize();

        BidCoSQueue();
        BidCoSQueue(std::string serializedObject, HomeMaticDevice* device);
        BidCoSQueue(BidCoSQueueType queueType);
        virtual ~BidCoSQueue();
};

#endif // BIDCOSQUEUE_H
