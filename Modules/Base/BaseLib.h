#ifndef BASELIB_H_
#define BASELIB_H_

#include "Database/DatabaseTypes.h"
#include "Encoding/XMLRPCDecoder.h"
#include "Encoding/XMLRPCEncoder.h"
#include "Encoding/RPCDecoder.h"
#include "Encoding/RPCEncoder.h"
#include "Encoding/JsonDecoder.h"
#include "Encoding/JsonEncoder.h"
#include "Encoding/HTTP.h"
#include "Managers/SerialDeviceManager.h"
#include "Managers/FileDescriptorManager.h"
#include "HelperFunctions/HelperFunctions.h"
#include "HelperFunctions/Color.h"
#include "HelperFunctions/Math.h"
#include "Output/Output.h"
#include "RPC/Devices.h"
#include "Settings/Settings.h"
#include "Sockets/HTTPClient.h"
#include "Sockets/SocketOperations.h"
#include "Systems/DeviceFamily.h"
#include "Systems/Peer.h"
#include "Systems/SystemFactory.h"
#include "Systems/UpdateInfo.h"
#include "Threads/Threads.h"

namespace BaseLib
{

/**
 * This is the base library main class.
 * It is used to share objects and data between all modules and the main program.
 */
class Obj
{
public:
	/**
	 * The current debug level for logging.
	 */
	int32_t debugLevel = 3;

	/**
	 * The path of the main executable.
	 */
	std::string executablePath;

	/**
	 * The FileDescriptorManager object where all file or socket descriptors should be registered.
	 * This should be done to avoid errors as it can happen, that a closed file descriptor is reopened and suddenly valid again without the object using the old descriptor noticing it.
	 */
	FileDescriptorManager fileDescriptorManager;

	/**
	 * The serial device manager can be used to access one serial device across multiple modules.
	 */
	SerialDeviceManager serialDeviceManager;

	/**
	 * The main.conf settings.
	 */
	Settings settings;

	/**
	 * Object to store information about running updates and to only allow one update at a time.
	 */
	Systems::UpdateInfo deviceUpdateInfo;

	/**
	 * Functions to ease your life for a lot of standard operations.
	 */
	HelperFunctions hf;

	/**
	 * The main output object to print text to the standard and error output.
	 */
	Output out;

	/**
	 * Main constructor.
	 *
	 * @param exePath The path to the main executable.
	 */
	Obj(std::string exePath);

	/**
	 * Destructor.
	 */
	virtual ~Obj();
private:
};
}
#endif
