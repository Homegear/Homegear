#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include <memory>
#include <mutex>
#include <string>

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
	void packetReceived(shared_ptr<BidCoSPacket> packet);
	void enablePairingMode() { _pairing = true; }
	void disablePairingMode() { _pairing = false; }
	void unpair(int32_t address);
	void setUpBidCoSMessages();
	void setUpConfig() {}
	std::string serialize();
	void unserialize(std::string serializedObject, uint8_t dutyCycleMessageCounter, int64_t lastDutyCycleEvent);
	void handleCLICommand(std::string command);

	void handlePairingRequest(int32_t messageCounter, shared_ptr<BidCoSPacket>);
	void handleAck(int32_t messageCounter, shared_ptr<BidCoSPacket>);
	void handleConfigParamResponse(int32_t messageCounter, shared_ptr<BidCoSPacket>);

	shared_ptr<RPC::RPCVariable> listDevices();
protected:
	shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);
private:
	shared_ptr<Peer> _currentPeer = nullptr;
};

#endif /* HOMEMATICCENTRAL_H_ */
