#ifndef DEVICES_H_
#define DEVICES_H_

#include <vector>
#include <memory>

#include "Device.h"
#include "../BidCoSPacket.h"
#include "../HMDeviceTypes.h"

namespace XMLRPC {

class Devices {
public:
	Devices();
	virtual ~Devices() {}
	void load();
	shared_ptr<Device> find(shared_ptr<BidCoSPacket> packet);
	shared_ptr<Device> find(HMDeviceTypes deviceType, uint32_t firmwareVersion);
protected:
	std::vector<shared_ptr<Device>> _devices;
};

} /* namespace XMLRPC */
#endif /* DEVICES_H_ */
