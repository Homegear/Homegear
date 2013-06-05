#ifndef BIDCOSMESSAGE_H
#define BIDCOSMESSAGE_H

#include <vector>
#include <functional>
#include <map>

#include "BidCoSPacket.h"
#include "BidCoSQueue.h"
#include "HomeMaticDevice.h"

class HomeMaticDevice;
class BidCoSQueue;

enum MessageAccess { NOACCESS = 0x00, ACCESSPAIREDTOSENDER = 0x01, ACCESSDESTISME = 0x02, ACCESSCENTRAL = 0x04, ACCESSUNPAIRING = 0x08, FULLACCESS = 0x80 };
enum MessageDirection { DIRECTIONIN, DIRECTIONOUT };

class BidCoSMessage
{
    public:
        BidCoSMessage();
        /** Default constructor */
        BidCoSMessage(int32_t messageType, HomeMaticDevice* device, int32_t access, void (HomeMaticDevice::*messageHandlerIncoming)(int32_t, shared_ptr<BidCoSPacket>));
        BidCoSMessage(int32_t messageType, HomeMaticDevice* device, int32_t access, int32_t accessPairing, void (HomeMaticDevice::*messageHandlerIncoming)(int32_t, shared_ptr<BidCoSPacket>));
        BidCoSMessage(int32_t messageType, int32_t controlByte, HomeMaticDevice* device, void (HomeMaticDevice::*messageHandlerOutgoing)(int32_t, int32_t, shared_ptr<BidCoSPacket>));
        /** Default destructor */
        virtual ~BidCoSMessage();

        MessageDirection getDirection() { return _direction; }
        void setDirection(MessageDirection direction) { _direction = direction; }
        int32_t getControlByte() { return _controlByte; }
        void setControlByte(int32_t controlByte) { _controlByte = controlByte; }
        int32_t getMessageType() { return _messageType; }
        void setMessageType(int32_t messageType) { _messageType = messageType; }
        int32_t getMessageAccess() { return _access; }
        void setMessageAccess(int32_t access) { _access = access; }
        int32_t getMessageAccessPairing() { return _accessPairing; }
        void setMessageAccessPairing(int32_t accessPairing) { _accessPairing = accessPairing; }
        HomeMaticDevice* getDevice() { return _device; }
        void invokeMessageHandlerIncoming(shared_ptr<BidCoSPacket> packet);
        void invokeMessageHandlerOutgoing(shared_ptr<BidCoSPacket> packet);
        bool checkAccess(shared_ptr<BidCoSPacket> packet, BidCoSQueue* queue);
        std::vector<std::pair<uint32_t, int32_t>>* getSubtypes() { return &_subtypes; }
        void addSubtype(int32_t subtypePosition, int32_t subtype) { _subtypes.push_back(std::pair<uint32_t, int32_t>(subtypePosition, subtype)); };
        uint32_t subtypeCount() { return _subtypes.size(); }
        void setMessageCounter(shared_ptr<BidCoSPacket> packet);
        bool typeIsEqual(shared_ptr<BidCoSPacket> packet);
        bool typeIsEqual(shared_ptr<BidCoSMessage> message, shared_ptr<BidCoSPacket> packet);
        bool typeIsEqual(shared_ptr<BidCoSMessage> message);
        bool typeIsEqual(int32_t messageType, std::vector<std::pair<uint32_t, int32_t>>* subtypes);
    protected:
        MessageDirection _direction = DIRECTIONIN;
        int32_t _messageType = -1;
        int32_t _controlByte = 0;
        HomeMaticDevice* _device = nullptr;
        int32_t _access = 0;
        int32_t _accessPairing = 0;
        std::vector<std::pair<uint32_t, int32_t>> _subtypes;
        void (HomeMaticDevice::*_messageHandlerIncoming)(int32_t, shared_ptr<BidCoSPacket>) = nullptr;
        void (HomeMaticDevice::*_messageHandlerOutgoing)(int32_t, int32_t, shared_ptr<BidCoSPacket>) = nullptr;
    private:
};

#endif // BIDCOSMESSAGE_H
