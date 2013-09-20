/* Copyright 2013 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef GD_H_
#define GD_H_

class HomeMaticDevices;

#include <vector>
#include <map>
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
#include "RPC/ServerSettings.h"
#include "RPC/ClientSettings.h"
#include "EventHandler.h"

class GD {
public:
	static std::string configPath;
	static std::string runDir;
	static std::string pidfilePath;
	static std::string socketPath;
	static std::string workingDirectory;
	static std::string executablePath;
	static HomeMaticDevices devices;
	static std::map<int32_t, RPC::Server> rpcServers;
	static RPC::Client rpcClient;
	static CLI::Server cliServer;
	static CLI::Client cliClient;
	static RPC::Devices rpcDevices;
	static Settings settings;
	static RPC::ServerSettings serverSettings;
	static RPC::ClientSettings clientSettings;
	static Database db;
	static std::shared_ptr<RF::RFDevice> rfDevice;
	static int32_t debugLevel;
	static int32_t rpcLogLevel;
	static bool bigEndian;
	static EventHandler eventHandler;

	virtual ~GD() { }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
