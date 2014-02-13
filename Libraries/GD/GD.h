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

namespace PhysicalDevices
{
	class PhysicalDevices;
}
namespace RPC
{
	class ServerSettings;
	class ClientSettings;
}
class EventHandler;

#include "../CLI/CLIServer.h"
#include "../CLI/CLIClient.h"
#include "../LogicalDevices/LogicalDevices.h"
#include "../Database/Database.h"
#include "../Settings/Settings.h"
#include "../Systems/General/DeviceFamily.h"
#include "../Systems/General/DeviceFamilies.h"
#include "../FileDescriptorManager/FileDescriptorManager.h"
#include "../RPC/Server.h"
#include "../RPC/Client.h"
#include "../RPC/Devices.h"

#include <vector>
#include <map>
#include <string>

class GD {
public:
	static std::string configPath;
	static std::string runDir;
	static std::string pidfilePath;
	static std::string socketPath;
	static std::string workingDirectory;
	static std::string executablePath;
	static LogicalDevices devices;
	static std::map<int32_t, RPC::Server> rpcServers;
	static RPC::Client rpcClient;
	static CLI::Server cliServer;
	static CLI::Client cliClient;
	static std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>> deviceFamilies;
	static RPC::Devices rpcDevices;
	static Settings settings;
	static RPC::ServerSettings serverSettings;
	static RPC::ClientSettings clientSettings;
	static Database db;
	static PhysicalDevices::PhysicalDevices physicalDevices;
	static int32_t debugLevel;
	static int32_t rpcLogLevel;
	static EventHandler eventHandler;
	static FileDescriptorManager fileDescriptorManager;

	virtual ~GD() { }
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
