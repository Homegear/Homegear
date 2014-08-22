#ifndef BASELIB_H_
#define BASELIB_H_

#include "Database/DatabaseTypes.h"
#include "Encoding/XMLRPCDecoder.h"
#include "Encoding/XMLRPCEncoder.h"
#include "Encoding/RPCDecoder.h"
#include "Encoding/RPCEncoder.h"
#include "Managers/SerialDeviceManager.h"
#include "Managers/FileDescriptorManager.h"
#include "HelperFunctions/HelperFunctions.h"
#include "Output/Output.h"
#include "RPC/Devices.h"
#include "Settings/Settings.h"
#include "Sockets/SocketOperations.h"
#include "Systems/DeviceFamily.h"
#include "Systems/Peer.h"
#include "Systems/SystemFactory.h"
#include "Systems/UpdateInfo.h"
#include "Threads/Threads.h"

namespace BaseLib
{

class Obj
{
public:
	int32_t debugLevel;
	std::string executablePath;
	FileDescriptorManager fileDescriptorManager;
	SerialDeviceManager serialDeviceManager;
	Settings settings;
	Systems::UpdateInfo deviceUpdateInfo;
	HelperFunctions hf;
	Output out;

	Obj(std::string exePath);
	virtual ~Obj();
private:
	//Non public constructor
};
}
#endif
