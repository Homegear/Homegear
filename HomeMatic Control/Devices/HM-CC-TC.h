#ifndef HM_CC_TC_H
#define HM_CC_TC_H

#include <thread>

#include "../HomeMaticDevice.h"
#include "../Cul.h"
#include "../GD.h"

//TODO Pairing by serial
//TODO Was wird beim Batterien einlegen gesendet?
//TODO Lässt sich beim Pairing über die Zentrale irgendwie der Gerätetyp ermitteln?
class HM_CC_TC : public HomeMaticDevice
{
    public:
        HM_CC_TC();
        HM_CC_TC(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
        HM_CC_TC(std::string, int32_t);
        virtual ~HM_CC_TC();

        void setValveState(int32_t valveState) { _newValveState = valveState; }
        void handleCLICommand(std::string command);
        void stopDutyCycle();

        virtual void setValue(const xmlrpc_c::paramList& paramList);
        virtual xmlrpc_c::value getValue(const xmlrpc_c::paramList& paramList);

        void handlePairingRequest(int32_t messageCounter, BidCoSPacket* packet);
        void handleConfigParamResponse(int32_t messageCounter, BidCoSPacket* packet);
        void handleAck(int32_t messageCounter, BidCoSPacket* packet);
        void handleSetPoint(int32_t messageCounter, BidCoSPacket* packet);
        void handleSetValveState(int32_t messageCounter, BidCoSPacket* packet);
        void handleConfigPeerAdd(int32_t messageCounter, BidCoSPacket* packet);
        std::string serialize();
    protected:
        virtual void setUpBidCoSMessages();
        virtual void init();
    private:
        int32_t _currentDutyCycleDeviceAddress = -1;
        int32_t _temperature = 213;
        int32_t _setPointTemperature = 42;
        int32_t _humidity = 56;
        bool _stopDutyCycleThread = false;
        Peer createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);
        std::thread* _dutyCycleThread = nullptr;
        int32_t _valveState = 0;
        int32_t _newValveState = 0;
        int32_t _dutyCycleCounter  = 0;
        void reset();

        int32_t getNextDutyCycleDeviceAddress();
        int64_t calculateLastDutyCycleEvent();
        int32_t getAdjustmentCommand();

        void sendRequestConfig(int32_t messageCounter, int32_t controlByte, BidCoSPacket* packet);
        void sendConfigParams(int32_t messageCounter, int32_t controlByte);
        void sendDutyCycleBroadcast();
        void sendDutyCyclePacket(uint8_t messageCounter);
        void startDutyCycle(int64_t lastDutyCycleEvent);
        void dutyCycleThread(int64_t lastDutyCycleEvent);
        void setUpConfig();
};

#endif // HM_CC_TC_H
