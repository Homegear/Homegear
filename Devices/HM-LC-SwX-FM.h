#ifndef HM_LC_SWX_FM_H
#define HM_LC_SWX_FM_H

#include <thread>
#include <memory>
#include <vector>

class BidCoSQueue;
enum class BidCoSQueueType;

#include "../HomeMaticDevice.h"

class HM_LC_SWX_FM : public HomeMaticDevice
{
    public:
        HM_LC_SWX_FM();
        HM_LC_SWX_FM(std::string, int32_t);
        virtual ~HM_LC_SWX_FM();

        void handleCLICommand(std::string command);
        void setChannelCount(uint32_t channelCount) { _channelCount = channelCount; }

        std::string serialize();
        void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
    protected:
        bool _state = false;
        uint32_t _channelCount = 2;

        virtual void setUpBidCoSMessages();
        virtual void init();

        virtual void handleStateChange(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);

        void sendStateChangeResponse(std::shared_ptr<BidCoSPacket> receivedPacket);
    private:
};

#endif // HM_LC_SWX_FM_H
