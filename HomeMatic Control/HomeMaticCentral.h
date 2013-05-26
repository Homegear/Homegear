#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include <mutex>

#include "BidCoSPacket.h"

class HomeMaticCentral
{
public:
	HomeMaticCentral() {}
	virtual ~HomeMaticCentral() {}
	void init();
	void packetReceived(BidCoSPacket* packet);
private:
	std::mutex _receivedPacketMutex;
	bool _initialized = false;
};

#endif /* HOMEMATICCENTRAL_H_ */
