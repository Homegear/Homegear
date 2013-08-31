#ifndef HOMEMATICDEVICES_H_
#define HOMEMATICDEVICES_H_

class HomeMaticDevice;
class HomeMaticCentral;

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include "Devices/HomeMaticCentral.h"

class HomeMaticDevices {
public:
	HomeMaticDevices();
	virtual ~HomeMaticDevices();
	void add(HomeMaticDevice* device);
	bool remove(int32_t address);
	std::shared_ptr<HomeMaticDevice> get(int32_t address);
	std::shared_ptr<HomeMaticDevice> get(std::string serialNumber);
	std::shared_ptr<HomeMaticCentral> getCentral();
	std::vector<std::shared_ptr<HomeMaticDevice>> getDevices();
	void convertDatabase();
	void load();
	void save(bool full, bool crash = false);
	void dispose();
	std::string handleCLICommand(std::string& command);

private:
	std::shared_ptr<HomeMaticDevice> _currentDevice;
	std::mutex _devicesMutex;
	std::vector<std::shared_ptr<HomeMaticDevice>> _devices;
	std::shared_ptr<HomeMaticCentral> _central;

	void initializeDatabase();
	void loadDevicesFromDatabase_0_0_6();
	void loadDevicesFromDatabase();
	int32_t getUniqueAddress(uint8_t firstByte);
	std::string getUniqueSerialNumber(std::string seedPrefix);
	void createCentral();
	void createSpyDevice();
};

#endif /* HOMEMATICDEVICES_H_ */
