#ifndef HM_LC_SWX_FM_H
#define HM_LC_SWX_FM_H

#include <thread>
#include <memory>
#include <vector>

class BidCoSQueue;
enum class BidCoSQueueType;

#include "../HomeMaticDevice.h"
#include "../Database.h"

class HM_LC_SWX_FM : public HomeMaticDevice
{
    public:
        HM_LC_SWX_FM();
        HM_LC_SWX_FM(uint32_t deviceID, std::string, int32_t);
        virtual ~HM_LC_SWX_FM();

        std::string handleCLICommand(std::string command);
        void setChannelCount(uint32_t channelCount) { _channelCount = channelCount; }

        void unserialize_0_0_6(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
    protected:
        //In table variables
        uint32_t _channelCount = 2;
        std::map<uint32_t, bool> _states;
        //End

        virtual void setUpBidCoSMessages();
        virtual void init();
        void loadVariables();
        void saveVariables();
        void saveStates();
        void serializeStates(std::vector<uint8_t>& encodedData);
        void unserializeStates(std::shared_ptr<std::vector<char>> serializedData);

        virtual void handleStateChangeRemote(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        virtual void handleStateChangeMotionDetector(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);

        void sendStateChangeResponse(std::shared_ptr<BidCoSPacket> receivedPacket, uint32_t channel);
    private:
};

#endif // HM_LC_SWX_FM_H
