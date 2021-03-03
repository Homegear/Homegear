/* Copyright 2013-2020 Homegear GmbH
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

#ifndef GD_H_
#define GD_H_

#include "../ScriptEngine/ScriptEngineServer.h"
#include "../Node-BLUE/NodeBlueServer.h"
#include "../IPC/IpcServer.h"
#include "../Licensing/LicensingController.h"
#include "../FamilyModules/FamilyController.h"
#include "../FamilyModules/FamilyServer.h"
#include "../Database/DatabaseController.h"
#include "../UI/UiController.h"
#include "../VariableProfiles/VariableProfileManager.h"
#include "../RPC/RpcServer.h"
#include "../RPC/Client.h"
#include "../MQTT/Mqtt.h"
#include "../IpcLogger.h"
#include "../Database/SystemVariableController.h"
#include <homegear-base/BaseLib.h>

#include <vector>
#include <map>
#include <string>
#include <memory>

namespace Homegear {

class UPnP;
class Mqtt;

class GD {
 public:
  GD() = delete;

  static std::unique_ptr<BaseLib::SharedObjects> bl;
  static BaseLib::Output out;
  static const std::string baseLibVersion;
  static const std::string nodeLibVersion;
  static const std::string ipcLibVersion;
  static std::string runAsUser;
  static std::string runAsGroup;
  static std::string configPath;
  static std::string pidfilePath;
  static std::string workingDirectory;
  static std::string executablePath;
  static std::string executableFile;
  static int64_t startingTime;
  static std::unique_ptr<FamilyController> familyController;
  static std::unique_ptr<FamilyServer> familyServer;
  static std::unique_ptr<LicensingController> licensingController;
  //We can work with rpcServers without Mutex, because elements are never deleted and iterators are not invalidated upon insertion of new elements.
  static std::map<int32_t, std::shared_ptr<Rpc::RpcServer>> rpcServers;
  static std::unique_ptr<Rpc::Client> rpcClient;
#ifndef NO_SCRIPTENGINE
  static std::unique_ptr<ScriptEngine::ScriptEngineServer> scriptEngineServer;
#endif
  static std::unique_ptr<IpcServer> ipcServer;
  static std::unique_ptr<NodeBlue::NodeBlueServer> nodeBlueServer;
  static BaseLib::Rpc::ServerInfo serverInfo;
  static Rpc::ClientSettings clientSettings;
  static int32_t rpcLogLevel;
  static std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>> licensingModules;
  static std::unique_ptr<UPnP> uPnP;
  static std::unique_ptr<Mqtt> mqtt;
  static std::unique_ptr<UiController> uiController;
  static std::unique_ptr<VariableProfileManager> variableProfileManager;
  static std::unique_ptr<SystemVariableController> systemVariableController;
  static std::unique_ptr<IpcLogger> ipcLogger;
};

}

#endif /* GD_H_ */
