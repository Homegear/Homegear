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

        std::string handleCLICommand(std::string command);
        void setChannelCount(uint32_t channelCount) { _channelCount = channelCount; }

        std::string serialize();
        void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
    protected:
        std::map<uint32_t, bool> _states;
        uint32_t _channelCount = 2;

        virtual void setUpBidCoSMessages();
        virtual void init();

        virtual void handleStateChangeRemote(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        virtual void handleStateChangeMotionDetector(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);

        void sendStateChangeResponse(std::shared_ptr<BidCoSPacket> receivedPacket, uint32_t channel);
    private:
};

#endif // HM_LC_SWX_FM_H
