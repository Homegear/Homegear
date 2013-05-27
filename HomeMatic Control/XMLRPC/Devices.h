#ifndef DEVICES_H_
#define DEVICES_H_

#include <vector>

#include "Device.h"

namespace XMLRPC {

class Devices {
public:
	Devices();
	virtual ~Devices() {}
	void load();
	Device* find(uint32_t deviceType, uint32_t firmwareVersion);
protected:
	std::vector<Device> _devices;
};

} /* namespace XMLRPC */
#endif /* DEVICES_H_ */
