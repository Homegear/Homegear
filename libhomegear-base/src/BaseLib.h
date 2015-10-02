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
#include "Encoding/Html.h"
#include "Encoding/WebSocket.h"
#include "Managers/SerialDeviceManager.h"
#include "Managers/FileDescriptorManager.h"
#include "HelperFunctions/HelperFunctions.h"
#include "HelperFunctions/Color.h"
#include "HelperFunctions/Math.h"
#include "HelperFunctions/Crypt.h"
#include "HelperFunctions/Base64.h"
#include "HelperFunctions/Net.h"
#include "HelperFunctions/Io.h"
#include "Output/Output.h"
#include "DeviceDescription/Devices.h"
#include "Settings/Settings.h"
#include "Sockets/HTTPClient.h"
#include "Sockets/SocketOperations.h"
#include "Sockets/IWebserverEventSink.h"
#include "Sockets/ServerInfo.h"
#include "Systems/DeviceFamily.h"
#include "Systems/Peer.h"
#include "Systems/SystemFactory.h"
#include "Systems/UpdateInfo.h"
#include "Threads/Threads.h"
#include "SSDP/SSDP.h"
#include "IQueue.h"

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
	 * True when Homegear is still starting. It is set to false, when start up is complete and isOpen() of all interfaces is "true" (plus 30 seconds).
	 */
	bool booting = true;

	/**
	 * True when Homegear received signal 15.
	 */
	bool shuttingDown = false;

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
	 * Port, the non-ssl RPC server listens on.
	 */
	uint32_t rpcPort = 0;

	/**
	 * Object to store information about running updates and to only allow one update at a time.
	 */
	Systems::UpdateInfo deviceUpdateInfo;

	/**
	 * Functions to ease your life for a lot of standard operations.
	 */
	HelperFunctions hf;

	/**
	 * Functions for io operations.
	 */
	Io io;

	/**
	 * The main output object to print text to the standard and error output.
	 */
	Output out;

	/**
	 * Main constructor.
	 *
	 * @param exePath The path to the main executable.
	 * @param errorCallback Callback function which will be called for all error messages. First parameter is the error level (1 = critical, 2 = error, 3 = warning), second parameter is the error string.
	 */
	Obj(std::string exePath, std::function<void(int32_t, std::string)>* errorCallback);

	/**
	 * Destructor.
	 */
	virtual ~Obj();
private:
	Obj(const Obj&);
	Obj& operator=(const Obj&);
};
}
#endif
