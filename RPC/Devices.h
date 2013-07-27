#ifndef DEVICES_H_
#define DEVICES_H_

class BidCoSPacket;

#include <vector>
#include <memory>

#include "Device.h"
#include "../HMDeviceTypes.h"

namespace RPC {

class Devices {
public:
	Devices();
	virtual ~Devices() {}
	void load();
	std::shared_ptr<Device> find(std::shared_ptr<BidCoSPacket> packet);
	std::shared_ptr<Device> find(std::string typeID, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>());
	std::shared_ptr<Device> find(HMDeviceTypes deviceType, uint32_t firmwareVersion, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>());
	std::shared_ptr<Device> find(HMDeviceTypes deviceType, uint32_t firmwareVersion, int32_t countFromSysinfo);
protected:
	std::vector<std::shared_ptr<Device>> _devices;
};

} /* namespace XMLRPC */
#endif /* DEVICES_H_ */
