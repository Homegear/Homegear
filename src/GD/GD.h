/* Copyright 2013-2015 Sathya Laufer
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

#include "../../config.h"
#ifdef SCRIPTENGINE
#include "../ScriptEngine/ScriptEngine.h"
#endif
#include "../CLI/CLIServer.h"
#include "../CLI/CLIClient.h"
#include "../Events/EventHandler.h"
#include "../Systems/FamilyController.h"
#include "../Systems/PhysicalInterfaces.h"
#include "../Systems/DatabaseController.h"
#include "homegear-base/BaseLib.h"
#include "../RPC/Server.h"
#include "../RPC/Client.h"

#include <vector>
#include <map>
#include <string>
#include <memory>

class UPnP;
class MQTT;

class GD
{
public:
	static std::unique_ptr<BaseLib::Obj> bl;
	static BaseLib::Output out;
	static DatabaseController db;
	static std::string configPath;
	static std::string runDir;
	static std::string pidfilePath;
	static std::string socketPath;
	static std::string workingDirectory;
	static std::string executablePath;
	static FamilyController familyController;
	//We can work with rpcServers without Mutex, because elements are never deleted and iterators are not invalidated upon insertion of new elements.
	static std::map<int32_t, RPC::Server> rpcServers;
	static RPC::Client rpcClient;
	static CLI::Server cliServer;
	static CLI::Client cliClient;
	static BaseLib::Rpc::ServerInfo serverInfo;
	static RPC::ClientSettings clientSettings;
	static int32_t rpcLogLevel;
	static PhysicalInterfaces physicalInterfaces;
	static std::map<int32_t, std::unique_ptr<BaseLib::Systems::DeviceFamily>> deviceFamilies;
	static std::map<std::string, int32_t> deviceFamiliesByName;
	static std::unique_ptr<UPnP> uPnP;
	static std::unique_ptr<MQTT> mqtt;
#ifdef EVENTHANDLER
	static EventHandler eventHandler;
#endif
#ifdef SCRIPTENGINE
	static ScriptEngine scriptEngine;
#endif

	virtual ~GD() {}
private:
	//Non public constructor
	GD();
};

#endif /* GD_H_ */
