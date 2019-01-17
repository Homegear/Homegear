/* Copyright 2013-2019 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "GD.h"
#include "../UPnP/UPnP.h"

namespace Homegear
{

std::unique_ptr<BaseLib::SharedObjects> GD::bl;
BaseLib::Output GD::out;
std::string GD::runAsUser = "";
std::string GD::runAsGroup = "";
std::string GD::configPath = "/etc/homegear/";
std::string GD::pidfilePath = "";
std::string GD::workingDirectory = "";
std::string GD::executablePath = "";
std::string GD::executableFile = "";
int64_t GD::startingTime = BaseLib::HelperFunctions::getTime();
std::unique_ptr<FamilyController> GD::familyController;
std::unique_ptr<FamilyServer> GD::familyServer;
std::unique_ptr<LicensingController> GD::licensingController;
std::map<int32_t, std::shared_ptr<Rpc::RpcServer>> GD::rpcServers;
std::unique_ptr<Rpc::Client> GD::rpcClient;
int32_t GD::rpcLogLevel = 1;
BaseLib::Rpc::ServerInfo GD::serverInfo;
Rpc::ClientSettings GD::clientSettings;
std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>> GD::licensingModules;
std::unique_ptr<UPnP> GD::uPnP(new UPnP());
std::unique_ptr<Mqtt> GD::mqtt;
std::unique_ptr<UiController> GD::uiController;
#ifdef EVENTHANDLER
std::unique_ptr<EventHandler> GD::eventHandler;
#endif
#ifndef NO_SCRIPTENGINE
std::unique_ptr<ScriptEngine::ScriptEngineServer> GD::scriptEngineServer;
#endif
std::unique_ptr<IpcServer> GD::ipcServer;
std::unique_ptr<NodeBlue::NodeBlueServer> GD::nodeBlueServer;

}