#ifndef HM_CC_VD_H
#define HM_CC_VD_H


using namespace std;

#include "HomeMaticDevice.h"
#include "Cul.h"

//TODO Pairing by serial
//TODO Was wird beim Batterien einlegen gesendet?
//TODO Don't allow pairing to two TCs
class HM_CC_VD : public HomeMaticDevice
{
    public:
        HM_CC_VD();
        /** Default constructor */
        HM_CC_VD(std::string, int32_t);
        /** Default destructor */
        virtual ~HM_CC_VD();
        void setValveDriveBlocked(bool);
        void setValveDriveLoose(bool);
        void setAdjustingRangeTooSmall(bool);

        void handleConfigPeerAdd(int32_t messageCounter, BidCoSPacket* packet);
    protected:
    private:
        int32_t _valveState = 0;
        int32_t* _errorPosition = nullptr;
        int32_t* _offsetPosition = nullptr;
        bool _valveDriveBlocked = false;
        bool _valveDriveLoose = false;
        bool _adjustingRangeTooSmall = false;

        Peer createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);
        void reset();

        void handleDutyCyclePacket(int32_t messageCounter, BidCoSPacket* packet);
        void sendDutyCycleResponse(int32_t destinationAddress, unsigned char oldValveState, unsigned char adjustmentCommand);
        void sendConfigParamsType2(int32_t messageCounter, int32_t destinationAddress);

};

#endif // HM_CC_VD_H
