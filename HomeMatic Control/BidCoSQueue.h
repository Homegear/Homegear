#ifndef BIDCOSQUEUE_H
#define BIDCOSQUEUE_H

#include "Peer.h"
#include "Cul.h"
#include "BidCoSMessage.h"
#include "Exception.h"

#include <queue>
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
        std::queue<BidCoSQueueEntry> _queue;
        std::mutex _queueMutex;
        BidCoSQueueType _queueType;
        bool _stopResendThread = false;
        unique_ptr<std::thread> _resendThread;
        std::mutex _resendThreadMutex;
        int32_t resendCounter = 0;
    public:
        Peer peer;
        BidCoSQueueType getQueueType() { return _queueType; }
        void setQueueType(BidCoSQueueType queueType) {  _queueType = queueType; }

        void push(BidCoSMessage* message);
        void push(const BidCoSPacket& packet);

        BidCoSQueueEntry* front() { return &_queue.front(); }
        void pop();
        bool isEmpty() { return _queue.empty(); }
        void resend();
        void startResendThread();
        void send(BidCoSPacket packet);

        BidCoSQueue();

        BidCoSQueue(BidCoSQueueType queueType);

        virtual ~BidCoSQueue();
};

#endif // BIDCOSQUEUE_H
