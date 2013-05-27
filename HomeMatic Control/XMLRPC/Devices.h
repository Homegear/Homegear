#ifndef DEVICES_H_
#define DEVICES_H_

#include <vector>

#include "Device.h"
#include "../BidCoSPacket.h"
#include "../HMDeviceTypes.h"

namespace XMLRPC {

class Devices {
public:
	Devices();
	virtual ~Devices() {}
	void load();
	Device* find(BidCoSPacket* packet);
	Device* find(HMDeviceTypes deviceType);
protected:
	std::vector<Device> _devices;
};

} /* namespace XMLRPC */
#endif /* DEVICES_H_ */
