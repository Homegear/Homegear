#ifndef GD_H_
#define GD_H_

class Database;
class HomeMaticDevices;

#include <vector>
#include <string>

#include "Database.h"
#include "RFDevices/RFDevice.h"
#include "HomeMaticDevices.h"
#include "RPC/Server.h"
#include "RPC/Client.h"
#include "CLI/CLIServer.h"
#include "CLI/CLIClient.h"
#include "RPC/Devices.h"
#include "Settings.h"

class GD {
public:
	static std::string configPath;
	static std::string runDir;
	static std::string pidfilePath;
	static std::string socketPath;
	static std::string workingDirectory;
	static std::string executablePath;
	static HomeMaticDevices devices;
	static RPC::Server rpcServer;
	static RPC::Client rpcClient;
	static CLI::Server cliServer;
	static CLI::Client cliClient;
	static RPC::Devices rpcDevices;
	static Settings settings;
	static Database db;
	static std::shared_ptr<RF::RFDevice> rfDevice;
	static int32_t debugLevel;
	static int32_t rpcLogLevel;
	static bool bigEndian;

	virtual ~GD() { }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
