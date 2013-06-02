#ifndef HOMEMATICDEVICES_H_
#define HOMEMATICDEVICES_H_

#include <vector>
#include <memory>

#include "HomeMaticDevice.h"

class HomeMaticDevice;

class HomeMaticDevices {
public:
	HomeMaticDevices();
	virtual ~HomeMaticDevices();
	void add(HomeMaticDevice* device);
	bool remove(int32_t address);
	shared_ptr<HomeMaticDevice> get(int32_t address);
	shared_ptr<HomeMaticDevice> get(std::string serialNumber);
	shared_ptr<HomeMaticDevice> getCentral() { return _central; }
	std::vector<shared_ptr<HomeMaticDevice>>* getDevices() { return &_devices; }
	void load();
	void save();
	void stopDutyCycles();
	void stopDutyCycle(shared_ptr<HomeMaticDevice> device);
private:
	std::vector<shared_ptr<HomeMaticDevice>> _devices;
	shared_ptr<HomeMaticDevice> _central = nullptr;
};

#endif /* HOMEMATICDEVICES_H_ */
