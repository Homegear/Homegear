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
protected:
	std::vector<Device> _devices;
};

} /* namespace XMLRPC */
#endif /* DEVICES_H_ */
