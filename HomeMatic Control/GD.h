#ifndef GD_H_
#define GD_H_

class Cul;
class Database;
class HomeMaticDevices;

#include <vector>
#include <string>

#include "Database.h"
#include "Cul.h"
#include "HomeMaticDevices.h"
#include "RPC/Server.h"
#include "RPC/Devices.h"

class GD {
public:
	static std::string workingDirectory;
	static std::string executablePath;
	static HomeMaticDevices devices;
	static RPC::Server rpcServer;
	static RPC::Devices rpcDevices;
	static Database db;
	static Cul cul;
	static int32_t debugLevel;
	static int32_t rpcLogLevel;

	virtual ~GD() { }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
