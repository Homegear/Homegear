#ifndef HOMEMATICDEVICES_H_
#define HOMEMATICDEVICES_H_

#include <vector>

#include "HomeMaticDevice.h"

class HomeMaticDevice;

class HomeMaticDevices {
public:
	HomeMaticDevices();
	virtual ~HomeMaticDevices();
	void add(HomeMaticDevice* device);
	bool remove(int32_t address);
	HomeMaticDevice* get(int32_t address);
	std::vector<HomeMaticDevice*>* getDevices() { return &_devices; }
	void load();
	void save();
	void stopDutyCycles();
	void stopDutyCycle(HomeMaticDevice* device);
private:
	std::vector<HomeMaticDevice*> _devices;
};

#endif /* HOMEMATICDEVICES_H_ */
