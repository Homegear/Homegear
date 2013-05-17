#ifndef HOMEMATICDEVICE_H
#define HOMEMATICDEVICE_H

#include "Cul.h"
#include "BidCoSQueue.h"
#include "Peer.h"
#include "BidCoSPacket.h"
#include "BidCoSMessage.h"
#include "BidCoSMessages.h"


#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include "pthread.h"

class BidCoSMessages;
class Cul;
class BidCoSMessage;
enum class BidCoSQueueType;
class BidCoSQueue;

//TODO Pairing not working when virtual device is put into pairing mode after the real device
//TODO Check if last pairing byte triggers sending of broadcast pairing request
class HomeMaticDevice
{
    public:
        int32_t address();
        std::string serialNumber();
        int32_t firmwareVersion();
        int32_t deviceType();

        HomeMaticDevice();
        HomeMaticDevice(std::string serializedObject);
        HomeMaticDevice(Cul* cul, std::string serialNumber, int32_t address);
        virtual ~HomeMaticDevice();
        virtual void packetReceived(BidCoSPacket*);

        virtual void setLowBattery(bool);
        virtual bool pairDevice(int32_t timeout);
        virtual bool isInPairingMode() { return _pairing; }
        virtual int32_t getCentralAddress();
        virtual std::unordered_map<int32_t, Peer>* getPeers();
        virtual int32_t getAddress();
        virtual BidCoSQueue* getBidCoSQueue() { return _bidCoSQueue.get(); }
        virtual BidCoSMessage* getLastReceivedMessage() { return _lastReceivedMessage; }
        virtual BidCoSPacket* getReceivedPacket() { return &_receivedPacket; }
        virtual std::unordered_map<int32_t, int32_t>* getMessageCounter() { return &_messageCounter; }
        virtual int32_t calculateCycleLength();
        virtual std::string serialize();

        virtual void handleAck(int32_t messageCounter, BidCoSPacket* packet) {}
        virtual void handlePairingRequest(int32_t messageCounter, BidCoSPacket*);
        virtual void handleDutyCyclePacket(int32_t messageCounter, BidCoSPacket*) {}
        virtual void handleConfigStart(int32_t messageCounter, BidCoSPacket*);
        virtual void handleConfigWriteIndex(int32_t messageCounter, BidCoSPacket*);
        virtual void handleConfigPeerAdd(int32_t messageCounter, BidCoSPacket*);
        virtual void handleConfigParamRequest(int32_t messageCounter, BidCoSPacket*);
        virtual void handleConfigParamResponse(int32_t messageCounter, BidCoSPacket*) {};
        virtual void handleConfigRequestPeers(int32_t messageCounter, BidCoSPacket*);
        virtual void handleReset(int32_t messageCounter, BidCoSPacket*);
        virtual void handleConfigEnd(int32_t messageCounter, BidCoSPacket*);
        virtual void handleConfigPeerDelete(int32_t messageCounter, BidCoSPacket*);
        virtual void handleWakeUp(int32_t messageCounter, BidCoSPacket*);
        virtual void handleSetPoint(int32_t messageCounter, BidCoSPacket*) {}
        virtual void handleSetValveState(int32_t messageCounter, BidCoSPacket*) {}

        virtual void sendPairingRequest();
        virtual void sendDirectedPairingRequest(int32_t messageCounter, int32_t controlByte, BidCoSPacket * packet);
        virtual void sendOK(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendOKWithPayload(int32_t messageCounter, int32_t destinationAddress, std::vector<uint8_t> payload, bool isWakeMeUpPacket);
        virtual void sendNOK(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendNOKTargetInvalid(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendConfigParams(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendConfigParamsType2(int32_t messageCounter, int32_t destinationAddress) {}
        virtual void sendPeerList(int32_t messageCounter, int32_t destinationAddress, int32_t channel);
        virtual void sendDutyCycleResponse(int32_t destinationAddress);
        virtual void sendRequestConfig(int32_t messageCounter, int32_t controlByte, BidCoSPacket* packet) {}
    protected:
        Cul* _cul;
        int32_t _address;
        std::string _serialNumber;
        int32_t _firmwareVersion;
        int32_t _deviceType;
        int32_t _deviceClass;
        int32_t _channelMin;
        int32_t _channelMax;
        int32_t _lastPairingByte = 0;
        int32_t _centralAddress = 0;
        int32_t _currentList = 0;
        std::unordered_map<int32_t, std::map<int32_t, int32_t> > _config;
        std::unordered_map<int32_t, Peer> _peers;
        std::mutex _peersMutex;
        std::unordered_map<int32_t, int32_t> _messageCounter;
        std::unordered_map<int32_t, int32_t> _deviceTypeChannels;
        bool _pairing = false;
        bool _justPairedToOrThroughCentral = false;
        auto_ptr<BidCoSQueue> _bidCoSQueue;
        std::thread _resetQueueThread;
        bool _stopResetQueueThread = false;
        BidCoSMessage* _lastReceivedMessage;
        BidCoSPacket _receivedPacket;
        std::mutex _receivedPacketMutex;
        BidCoSMessages* _messages;

        bool _lowBattery = false;

        virtual Peer createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);

        virtual void setUpBidCoSMessages();
        virtual void setUpConfig();
        virtual void newBidCoSQueue(BidCoSQueueType queueType);
        virtual void resetQueue();

        virtual void reset();
    private:
};

#endif // HOMEMATICDEVICE_H
