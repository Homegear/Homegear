#ifndef BIDCOSMESSAGES_H
#define BIDCOSMESSAGES_H

#include "BidCoSPacket.h"
#include "BidCoSMessage.h"

#include <vector>

class BidCoSMessage;

class BidCoSMessages
{
    public:
        /** Default constructor */
        BidCoSMessages();
        /** Default destructor */
        virtual ~BidCoSMessages();
        void add(BidCoSMessage message);
        BidCoSMessage* find(int32_t direction, BidCoSPacket* packet);
        BidCoSMessage* find(int32_t direction, int32_t messageType, std::vector<std::pair<int32_t, int32_t> > subtypes);
    protected:
    private:
        std::vector<BidCoSMessage> _messages;

};

#endif // BIDCOSMESSAGES_H
