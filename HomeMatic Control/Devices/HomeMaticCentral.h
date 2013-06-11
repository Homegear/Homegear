#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include <memory>
#include <mutex>
#include <string>
#include <cmath>

class BidCoSPacket;

#include "../HomeMaticDevice.h"
#include "../RPC/RPCVariable.h"

class HomeMaticCentral : public HomeMaticDevice
{
public:
	HomeMaticCentral();
	HomeMaticCentral(std::string, int32_t);
	virtual ~HomeMaticCentral();
	void init();
	bool packetReceived(std::shared_ptr<BidCoSPacket> packet);
	void enablePairingMode() { _pairing = true; }
	void disablePairingMode() { _pairing = false; }
	void unpair(int32_t address);
	void setUpBidCoSMessages();
	void setUpConfig() {}
	std::string serialize();
	void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
	void handleCLICommand(std::string command);
	virtual void enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues = false);

	void handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);

	std::shared_ptr<RPC::RPCVariable> listDevices();
	std::shared_ptr<RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	std::shared_ptr<RPC::RPCVariable> getParamsetDescription(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type);
protected:
	std::shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);
private:
	std::shared_ptr<Peer> _currentPeer = nullptr;
};

#endif /* HOMEMATICCENTRAL_H_ */
