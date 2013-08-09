#ifndef TICC1100_H_
#define TICC1100_H_

#ifdef TI_CC1100

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <vector>

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "RFDevice.h"

namespace RF
{

class TICC1100 : public RFDevice
{
public:
	TICC1100();
	void init(std::string rfDevice);
	virtual ~TICC1100();

	void startListening();
	void stopListening();
	void sendPacket(std::shared_ptr<BidCoSPacket> packet);
	bool isOpen() { return _fileDescriptor > -1; }
protected:
	int32_t _fileDescriptor = -1;
	struct spi_ioc_transfer _transfer;

	void setupDevice();
	void openDevice();
    void closeDevice();
    void listen();
    void readwrite(std::vector<uint8_t>& data);
};

}

#endif /* TI_CC1100 */

#endif /* TICC1100_H_ */
