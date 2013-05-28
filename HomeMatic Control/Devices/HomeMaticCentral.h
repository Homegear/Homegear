#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include <mutex>
#include <string>

#include "../HomeMaticDevice.h"

class BidCoSPacket;

class HomeMaticCentral : public HomeMaticDevice
{
public:
	HomeMaticCentral();
	HomeMaticCentral(std::string serializedObject);
	HomeMaticCentral(std::string, int32_t);
	virtual ~HomeMaticCentral();
	void init();
	void packetReceived(BidCoSPacket* packet);
	void enablePairingMode() { _pairing = true; }
	void disablePairingMode() { _pairing = false; }
	void setUpBidCoSMessages();
	void setUpConfig() {}
	std::string serialize();
	void handleCLICommand(std::string command);

	void handlePairingRequest(int32_t messageCounter, BidCoSPacket*);
	void handleAck(int32_t messageCounter, BidCoSPacket*);
	void handleConfigParamResponse(int32_t messageCounter, BidCoSPacket*);
protected:
	virtual Peer createPeer(int32_t address, int32_t firmwareVersion, HMDeviceTypes deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter);
private:

};

#endif /* HOMEMATICCENTRAL_H_ */
