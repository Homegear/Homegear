#ifndef BIDCOSMESSAGES_H
#define BIDCOSMESSAGES_H

class BidCoSPacket;

#include <iostream>
#include <memory>
#include <vector>

#include "BidCoSMessage.h"

class BidCoSMessages
{
    public:
        BidCoSMessages() {}
        virtual ~BidCoSMessages() {}
        void add(std::shared_ptr<BidCoSMessage> message);
        std::shared_ptr<BidCoSMessage> find(int32_t direction, std::shared_ptr<BidCoSPacket> packet);
        std::shared_ptr<BidCoSMessage> find(int32_t direction, int32_t messageType, std::vector<std::pair<uint32_t, int32_t> > subtypes);
    protected:
    private:
        std::vector<std::shared_ptr<BidCoSMessage>> _messages;

};

#endif // BIDCOSMESSAGES_H
