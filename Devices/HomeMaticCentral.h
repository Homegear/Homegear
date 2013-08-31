#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include <memory>
#include <mutex>
#include <string>
#include <cmath>

class BidCoSPacket;

#include "HM-CC-TC.h"
#include "HM-LC-SwX-FM.h"
#include "../HomeMaticDevice.h"
#include "../RPC/RPCVariable.h"

class HomeMaticCentral : public HomeMaticDevice
{
public:
	HomeMaticCentral();
	HomeMaticCentral(uint32_t deviceID, std::string, int32_t);
	virtual ~HomeMaticCentral();
	void init();
	bool packetReceived(std::shared_ptr<BidCoSPacket> packet);
	void enablePairingMode() { _pairing = true; }
	void disablePairingMode() { _pairing = false; }
	void unpair(int32_t address, bool defer);
	void reset(int32_t address, bool defer);
	void setUpBidCoSMessages();
	void setUpConfig() {}
	void deletePeer(int32_t address);
	void addPeerToTeam(std::shared_ptr<Peer> peer, int32_t channel, uint32_t teamChannel, std::string teamSerialNumber);
	void addPeerToTeam(std::shared_ptr<Peer> peer, int32_t channel, int32_t address, uint32_t teamChannel);
	void removePeerFromTeam(std::shared_ptr<Peer> peer);
	void resetTeam(std::shared_ptr<Peer> peer, uint32_t channel);
	void unserialize_0_0_6(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
	std::string handleCLICommand(std::string command);
	virtual void enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues = false);
	virtual void enqueuePendingQueues(int32_t deviceAddress);
	int32_t getUniqueAddress(int32_t seed);
	std::string getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber);
	void addPeersToVirtualDevices();
	void addHomegearFeatures(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues);
	void addHomegearFeaturesHMCCVD(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues);
	void addHomegearFeaturesRemote(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues);
	void addHomegearFeaturesMotionDetector(std::shared_ptr<Peer> peer, int32_t channel, bool pushPendingBidCoSQueues);

	void handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleReset(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {}
	void sendEnableAES(int32_t address, int32_t channel);
	void sendRequestConfig(int32_t address, uint8_t localChannel, uint8_t list = 0, int32_t remoteAddress = 0, uint8_t remoteChannel = 0);

	std::shared_ptr<RPC::RPCVariable> addDevice(std::string serialNumber);
	std::shared_ptr<RPC::RPCVariable> addLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	std::shared_ptr<RPC::RPCVariable> removeLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	std::shared_ptr<RPC::RPCVariable> deleteDevice(std::string serialNumber, int32_t flags);
	std::shared_ptr<RPC::RPCVariable> deleteMetadata(std::string objectID, std::string dataID = "");
	std::shared_ptr<RPC::RPCVariable> getDeviceDescription(std::string serialNumber, int32_t channel);
	std::shared_ptr<RPC::RPCVariable> getInstallMode();
	std::shared_ptr<RPC::RPCVariable> getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	std::shared_ptr<RPC::RPCVariable> setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	std::shared_ptr<RPC::RPCVariable> getLinkPeers(std::string serialNumber, int32_t channel);
	std::shared_ptr<RPC::RPCVariable> getLinks(std::string serialNumber, int32_t channel, int32_t flags);
	std::shared_ptr<RPC::RPCVariable> getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	std::shared_ptr<RPC::RPCVariable> getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	std::shared_ptr<RPC::RPCVariable> getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	std::shared_ptr<RPC::RPCVariable> getServiceMessages();
	std::shared_ptr<RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	std::shared_ptr<RPC::RPCVariable> listDevices();
	std::shared_ptr<RPC::RPCVariable> listDevices(std::shared_ptr<std::map<std::string, int32_t>> knownDevices);
	std::shared_ptr<RPC::RPCVariable> listTeams();
	std::shared_ptr<RPC::RPCVariable> setTeam(std::string serialNumber, int32_t channel, std::string teamSerialNumber, int32_t teamChannel, bool force = false, bool burst = true);
	std::shared_ptr<RPC::RPCVariable> putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset);
	std::shared_ptr<RPC::RPCVariable> setInstallMode(bool on, int32_t duration = 60, bool debugOutput = true);
	std::shared_ptr<RPC::RPCVariable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
protected:
	std::shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>(), bool save = true);
	virtual void worker();
private:
	std::shared_ptr<Peer> _currentPeer = nullptr;
	uint32_t _timeLeftInPairingMode = 0;
	void pairingModeTimer(int32_t duration, bool debugOutput = true);
	bool _stopPairingModeThread = false;
	std::thread _pairingModeThread;
	std::map<std::string, std::shared_ptr<RPC::RPCVariable>> _metadata;
};

#endif /* HOMEMATICCENTRAL_H_ */
