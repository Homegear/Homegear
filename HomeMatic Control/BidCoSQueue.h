#ifndef BIDCOSQUEUE_H
#define BIDCOSQUEUE_H

#include "Peer.h"
#include "Cul.h"
#include "BidCoSMessage.h"
#include "Exception.h"

#include <deque>
#include <thread>
#include <mutex>

class Cul;
class BidCoSMessage;

enum class QueueEntryType { UNDEFINED, MESSAGE, PACKET };

class BidCoSQueueEntry {
protected:
	QueueEntryType _type = QueueEntryType::UNDEFINED;
	BidCoSMessage* _message = nullptr;
	BidCoSPacket _packet;
public:
	BidCoSQueueEntry() {}
	virtual ~BidCoSQueueEntry() {}
	QueueEntryType getType() { return _type; }
	BidCoSPacket* getPacket() { return &_packet; }
	void setPacket(const BidCoSPacket& packet) { _packet = packet; _type = QueueEntryType::PACKET; }
	BidCoSMessage* getMessage() { return _message; }
	void setMessage(BidCoSMessage* message) { _message = message; _type = QueueEntryType::MESSAGE; }
};

enum class BidCoSQueueType { EMPTY, DEFAULT, PAIRING, PAIRINGCENTRAL, UNPAIRING };

class BidCoSQueue
{
    protected:
        std::deque<BidCoSQueueEntry> _queue;
        std::mutex _queueMutex;
        BidCoSQueueType _queueType;
		shared_ptr<BidCoSQueue> _pendingBidCoSQueue;
        bool _stopResendThread = false;
        unique_ptr<std::thread> _resendThread;
        std::mutex _resendThreadMutex;
        int32_t resendCounter = 0;
    public:
        bool noSending = false;
        HomeMaticDevice* device = nullptr;
        Peer peer;
        BidCoSQueueType getQueueType() { return _queueType; }
        std::deque<BidCoSQueueEntry>* getQueue() { return &_queue; }
        void setQueueType(BidCoSQueueType queueType) {  _queueType = queueType; }

        void push(BidCoSMessage* message);
        void push(const BidCoSPacket& packet);
        void push(shared_ptr<BidCoSQueue>& pendingBidCoSQueue);
        BidCoSQueueEntry* front() { return &_queue.front(); }
        void pop();
        bool isEmpty() { return _queue.empty(); }
        void clear();
        void resend();
        void startResendThread();
        void send(BidCoSPacket packet);

        BidCoSQueue();

        BidCoSQueue(BidCoSQueueType queueType);

        virtual ~BidCoSQueue();
};

#endif // BIDCOSQUEUE_H
