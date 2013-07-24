#ifndef HOMEMATICDEVICE_H
#define HOMEMATICDEVICE_H

class Cul;
class BidCoSPacket;
class BidCoSMessages;
enum class BidCoSQueueType;

#include "HMDeviceTypes.h"
#include "BidCoSQueue.h"
#include "Peer.h"
#include "BidCoSMessage.h"
#include "BidCoSMessages.h"
#include "Cul.h"
#include "BidCoSQueueManager.h"
#include "BidCoSPacketManager.h"

#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include "pthread.h"

class HomeMaticDevice
{
    public:
        int32_t address();
        std::string serialNumber();
        int32_t firmwareVersion();
        HMDeviceTypes deviceType();
        std::unordered_map<int32_t, uint8_t>* messageCounter() { return &_messageCounter; }
        virtual int64_t lastDutyCycleEvent() { return _lastDutyCycleEvent; }

        HomeMaticDevice();
        HomeMaticDevice(std::string serialNumber, int32_t address);
        virtual ~HomeMaticDevice();
        virtual bool packetReceived(std::shared_ptr<BidCoSPacket> packet);

        virtual void addPeer(std::shared_ptr<Peer> peer) { if(_peers.find(peer->address) == _peers.end()) _peers[peer->address] = peer; }
        virtual void deletePeersFromDatabase();
        virtual void loadPeersFromDatabase();
        virtual void savePeersToDatabase();
        virtual void setLowBattery(bool);
        virtual bool pairDevice(int32_t timeout);
        virtual bool isInPairingMode() { return _pairing; }
        virtual int32_t getCentralAddress();
        virtual std::unordered_map<int32_t, std::shared_ptr<Peer>>* getPeers();
        virtual int32_t calculateCycleLength(uint8_t messageCounter);
        virtual void stopDutyCycle() {};
        virtual std::string serialize();
        virtual void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
        virtual int32_t getHexInput();
        virtual std::shared_ptr<BidCoSMessages> getMessages() { return _messages; }
        virtual void handleCLICommand(std::string command);
        virtual void sendPacket(std::shared_ptr<BidCoSPacket> packet);
        virtual void sendBurstPacket(std::shared_ptr<BidCoSPacket> packet, int32_t peerAddress, bool useCentralMessageCounter = false, bool isThread = false);
        std::shared_ptr<BidCoSPacket> getSentPacket(int32_t address) { return _sentPackets.get(address); }

        virtual void handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet) {}
        virtual void handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleDutyCyclePacket(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {}
        virtual void handleConfigStart(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleConfigWriteIndex(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleConfigParamRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {};
        virtual void handleConfigRequestPeers(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleReset(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleConfigEnd(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleConfigPeerDelete(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleWakeUp(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
        virtual void handleSetPoint(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {}
        virtual void handleSetValveState(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {}
        virtual void handleTimeRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);

        virtual void sendPairingRequest();
        virtual void sendDirectedPairingRequest(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet);
        virtual void sendOK(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendStealthyOK(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendOKWithPayload(int32_t messageCounter, int32_t destinationAddress, std::vector<uint8_t> payload, bool isWakeMeUpPacket);
        virtual void sendNOK(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendNOKTargetInvalid(int32_t messageCounter, int32_t destinationAddress);
        virtual void sendConfigParams(int32_t messageCounter, int32_t destinationAddress, std::shared_ptr<BidCoSPacket> packet);
        virtual void sendConfigParamsType2(int32_t messageCounter, int32_t destinationAddress) {}
        virtual void sendPeerList(int32_t messageCounter, int32_t destinationAddress, int32_t channel);
        virtual void sendDutyCycleResponse(int32_t destinationAddress);
        virtual void sendRequestConfig(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet) {}
    protected:
        bool _stopWorkerThread = false;
        std::thread _workerThread;
        int32_t _address;
        std::string _serialNumber;
        int32_t _firmwareVersion = 0;
        HMDeviceTypes _deviceType;
        int32_t _deviceClass = 0;
        int32_t _channelMin = 0;
        int32_t _channelMax = 0;
        int32_t _lastPairingByte = 0;
        int32_t _centralAddress = 0;
        int32_t _currentList = 0;
        std::unordered_map<int32_t, std::unordered_map<int32_t, std::map<int32_t, int32_t>>> _config;
        std::unordered_map<int32_t, std::shared_ptr<Peer>> _peers;
        std::unordered_map<std::string, std::shared_ptr<Peer>> _peersBySerial;
        std::mutex _peersMutex;
        std::unordered_map<int32_t, uint8_t> _messageCounter;
        std::unordered_map<int32_t, int32_t> _deviceTypeChannels;
        bool _pairing = false;
        bool _justPairedToOrThroughCentral = false;
        BidCoSQueueManager _bidCoSQueueManager;
        BidCoSPacketManager _receivedPackets;
        BidCoSPacketManager _sentPackets;
        std::shared_ptr<BidCoSMessages> _messages;
        int64_t _lastDutyCycleEvent = 0;
        bool _initialized = false;

        bool _lowBattery = false;

        virtual std::shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, int32_t index23);
        virtual std::shared_ptr<Peer> createTeam(int32_t address, HMDeviceTypes deviceType, std::string serialNumber);
        virtual void worker();
        //virtual void cleanUpMessageCounters();

        virtual void init();
        virtual void setUpBidCoSMessages();
        virtual void setUpConfig();


        virtual void reset();
    private:
};

#endif // HOMEMATICDEVICE_H
