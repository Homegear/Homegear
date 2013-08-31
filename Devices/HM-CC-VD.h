#ifndef HM_CC_VD_H
#define HM_CC_VD_H

#include "../HomeMaticDevice.h"
#include "../Database.h"

class HM_CC_VD : public HomeMaticDevice
{
    public:
        HM_CC_VD();
        HM_CC_VD(uint32_t deviceID, std::string, int32_t);
        virtual ~HM_CC_VD();
        void setValveDriveBlocked(bool);
        void setValveDriveLoose(bool);
        void setAdjustingRangeTooSmall(bool);
        std::string handleCLICommand(std::string command);

        void handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void unserialize_0_0_6(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
    protected:
        virtual void setUpBidCoSMessages();
        virtual void init();
        void loadVariables();
        void saveVariables();
    private:
        //In table variables
        int32_t _valveState = 0;
        bool _valveDriveBlocked = false;
        bool _valveDriveLoose = false;
        bool _adjustingRangeTooSmall = false;
        //End

        int32_t* _errorPosition = nullptr;
        int32_t* _offsetPosition = nullptr;

        std::shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>(), bool save = true);
        void reset();

        void handleDutyCyclePacket(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void sendDutyCycleResponse(int32_t destinationAddress, unsigned char oldValveState, unsigned char adjustmentCommand);
        void sendConfigParamsType2(int32_t messageCounter, int32_t destinationAddress);

};

#endif // HM_CC_VD_H
