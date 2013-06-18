#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include <memory>
#include <mutex>
#include <string>
#include <cmath>

class BidCoSPacket;

#include "HM-CC-TC.h"
#include "../HomeMaticDevice.h"
#include "../RPC/RPCVariable.h"

class HomeMaticCentral : public HomeMaticDevice
{
public:
	HomeMaticCentral();
	HomeMaticCentral(std::string, int32_t);
	virtual ~HomeMaticCentral();
	void init();
	void packetReceived(std::shared_ptr<BidCoSPacket> packet);
	void enablePairingMode() { _pairing = true; }
	void disablePairingMode() { _pairing = false; }
	void unpair(int32_t address, bool defer);
	void reset(int32_t address, bool defer);
	void setUpBidCoSMessages();
	void setUpConfig() {}
	void deletePeer(int32_t address);
	std::string serialize();
	void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
	void handleCLICommand(std::string command);
	virtual void enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues = false);
	int32_t getUniqueAddress(int32_t seed);
	std::string getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber);
	void addHomegearFeatures(std::shared_ptr<Peer> peer);
	void addHomegearFeaturesHMCCVD(std::shared_ptr<Peer> peer);

	void handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleReset(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {}
	void sendEnableAES(int32_t address, int32_t channel);

	std::shared_ptr<RPC::RPCVariable> deleteDevice(std::string serialNumber, int32_t flags);
	std::shared_ptr<RPC::RPCVariable> getDeviceDescription(std::string serialNumber, int32_t channel);
	std::shared_ptr<RPC::RPCVariable> getInstallMode();
	std::shared_ptr<RPC::RPCVariable> getParamsetDescription(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type);
	std::shared_ptr<RPC::RPCVariable> getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type);
	std::shared_ptr<RPC::RPCVariable> getParamset(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type);
	std::shared_ptr<RPC::RPCVariable> getServiceMessages();
	std::shared_ptr<RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	std::shared_ptr<RPC::RPCVariable> listDevices();
	std::shared_ptr<RPC::RPCVariable> listDevices(std::shared_ptr<std::map<std::string, int32_t>> knownDevices);
	std::shared_ptr<RPC::RPCVariable> putParamset(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::shared_ptr<RPC::RPCVariable> paramset);
	std::shared_ptr<RPC::RPCVariable> setInstallMode(bool on);
	std::shared_ptr<RPC::RPCVariable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
protected:
	std::shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);
private:
	//std::shared_ptr<Peer> _dummyHMRCV50;
	std::shared_ptr<Peer> _currentPeer = nullptr;
	uint32_t _timeLeftInPairingMode = 0;
	void pairingModeTimer();
	bool _stopPairingModeThread = false;
	std::thread _pairingModeThread;
	std::map<std::string, std::shared_ptr<RPC::RPCVariable>> _metadata;
};

#endif /* HOMEMATICCENTRAL_H_ */
