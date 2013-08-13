#ifndef RFDEVICE_H_
#define RFDEVICE_H_

#include "../Exception.h"
#include "../HomeMaticDevice.h"

namespace RF
{

class RFDevice
{
public:
	RFDevice();

	virtual void init(std::string rfDevice) {}

	virtual ~RFDevice();

	static std::shared_ptr<RFDevice> create(std::string rfDeviceType);


	virtual void startListening() {}
	virtual void stopListening() {}
	virtual void addHomeMaticDevice(HomeMaticDevice*);
	virtual void removeHomeMaticDevice(HomeMaticDevice*);
	virtual void sendPacket(std::shared_ptr<BidCoSPacket> packet) {}
	virtual bool isOpen() { return false; }
protected:
	std::mutex _homeMaticDevicesMutex;
    std::list<HomeMaticDevice*> _homeMaticDevices;
    std::string _rfDevice;
	std::thread _listenThread;
	std::thread _callbackThread;
	bool _stopCallbackThread;
	std::string _lockfile;
	std::mutex _sendMutex;
	bool _stopped = false;

	virtual void callCallback(std::shared_ptr<BidCoSPacket> packet);
};

}
#endif /* RFDEVICE_H_ */
