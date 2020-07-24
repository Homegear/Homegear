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

#ifndef NO_SCRIPTENGINE

#include "ScriptEngineServer.h"
#include "../GD/GD.h"
#include "../RPC/RpcMethods/BuildingRpcMethods.h"
#include "../RPC/RpcMethods/UiRpcMethods.h"
#include "../RPC/RpcMethods/VariableProfileRpcMethods.h"
#include "../RPC/RpcMethods/NodeBlueRpcMethods.h"

#include <homegear-base/BaseLib.h>
#include <homegear-base/Managers/ProcessManager.h>

namespace Homegear {

namespace ScriptEngine {

ScriptEngineServer::ScriptEngineServer() : IQueue(GD::bl.get(), 3, 100000) {
  _out.init(GD::bl.get());
  _out.setPrefix("Script Engine Server: ");

  _shuttingDown = false;
  _stopServer = false;

  _lifetick1.first = 0;
  _lifetick1.second = true;
  _lifetick2.first = 0;
  _lifetick2.second = true;

  _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, true));
  _rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), true, true));
  _scriptEngineClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
  _scriptEngineClientInfo->scriptEngineServer = true;
  _scriptEngineClientInfo->initInterfaceId = "scriptEngine";
  _scriptEngineClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  std::vector<uint64_t> groups{2};
  _scriptEngineClientInfo->acls->fromGroups(groups);
  _scriptEngineClientInfo->user = "SYSTEM (2)";

  _rpcMethods.emplace("devTest", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDevTest()));
  _rpcMethods.emplace("system.getCapabilities", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemGetCapabilities()));
  _rpcMethods.emplace("system.listMethods", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemListMethods()));
  _rpcMethods.emplace("system.methodHelp", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodHelp()));
  _rpcMethods.emplace("system.methodSignature", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodSignature()));
  _rpcMethods.emplace("system.multicall", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMulticall()));
  _rpcMethods.emplace("acknowledgeGlobalServiceMessage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAcknowledgeGlobalServiceMessage()));
  _rpcMethods.emplace("activateLinkParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCActivateLinkParamset()));
  _rpcMethods.emplace("abortEventReset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerEvent()));
  _rpcMethods.emplace("addDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDevice()));
  _rpcMethods.emplace("addEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddEvent()));
  _rpcMethods.emplace("addLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddLink()));
  _rpcMethods.emplace("checkServiceAccess", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCheckServiceAccess()));
  _rpcMethods.emplace("copyConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCopyConfig()));
  _rpcMethods.emplace("clientServerInitialized", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCClientServerInitialized()));
  _rpcMethods.emplace("createDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateDevice()));
  _rpcMethods.emplace("deleteData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteData()));
  _rpcMethods.emplace("deleteDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteDevice()));
  _rpcMethods.emplace("deleteMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteMetadata()));
  _rpcMethods.emplace("deleteNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteNodeData()));
  _rpcMethods.emplace("deleteSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteSystemVariable()));
  _rpcMethods.emplace("enableEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCEnableEvent()));
  _rpcMethods.emplace("executeMiscellaneousDeviceMethod", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCExecuteMiscellaneousDeviceMethod()));
  _rpcMethods.emplace("familyExists", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCFamilyExists()));
  _rpcMethods.emplace("forceConfigUpdate", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCForceConfigUpdate()));
  _rpcMethods.emplace("getAllConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllConfig()));
  _rpcMethods.emplace("getAllMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllMetadata()));
  _rpcMethods.emplace("getAllScripts", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllScripts()));
  _rpcMethods.emplace("getAllSystemVariables", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllSystemVariables()));
  _rpcMethods.emplace("getAllValues", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllValues()));
  _rpcMethods.emplace("getConfigParameter", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetConfigParameter()));
  _rpcMethods.emplace("getData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetData()));
  _rpcMethods.emplace("getDeviceDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDeviceDescription()));
  _rpcMethods.emplace("getDeviceInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDeviceInfo()));
  _rpcMethods.emplace("getEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetEvent()));
  _rpcMethods.emplace("getInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetInstallMode()));
  _rpcMethods.emplace("getKeyMismatchDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetKeyMismatchDevice()));
  _rpcMethods.emplace("getLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkInfo()));
  _rpcMethods.emplace("getLinkPeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkPeers()));
  _rpcMethods.emplace("getLinks", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinks()));
  _rpcMethods.emplace("getMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetMetadata()));
  _rpcMethods.emplace("getName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetName()));
  _rpcMethods.emplace("getNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetNodeData()));
  _rpcMethods.emplace("getFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetFlowData()));
  _rpcMethods.emplace("getGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetGlobalData()));
  _rpcMethods.emplace("getNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetNodeVariable()));
  _rpcMethods.emplace("getPairingInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPairingInfo()));
  _rpcMethods.emplace("getPairingState", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPairingState()));
  _rpcMethods.emplace("getParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamset()));
  _rpcMethods.emplace("getParamsetDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamsetDescription()));
  _rpcMethods.emplace("getParamsetId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamsetId()));
  _rpcMethods.emplace("getPeerId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPeerId()));
  _rpcMethods.emplace("getServiceMessages", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetServiceMessages()));
  _rpcMethods.emplace("getSniffedDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSniffedDevices()));
  _rpcMethods.emplace("getSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariable()));
  _rpcMethods.emplace("getSystemVariableFlags", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariableFlags()));
  _rpcMethods.emplace("getUpdateStatus", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetUpdateStatus()));
  _rpcMethods.emplace("getValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetValue()));
  _rpcMethods.emplace("getVariableDescription", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariableDescription()));
  _rpcMethods.emplace("getVersion", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVersion()));
  _rpcMethods.emplace("init", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCInit()));
  _rpcMethods.emplace("invokeFamilyMethod", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCInvokeFamilyMethod()));
  _rpcMethods.emplace("lifetick", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCLifetick()));
  _rpcMethods.emplace("listBidcosInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListBidcosInterfaces()));
  _rpcMethods.emplace("listClientServers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListClientServers()));
  _rpcMethods.emplace("listDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListDevices()));
  _rpcMethods.emplace("listEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListEvents()));
  _rpcMethods.emplace("listFamilies", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListFamilies()));
  _rpcMethods.emplace("listInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListInterfaces()));
  _rpcMethods.emplace("listKnownDeviceTypes", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListKnownDeviceTypes()));
  _rpcMethods.emplace("listTeams", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListTeams()));
  _rpcMethods.emplace("logLevel", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCLogLevel()));
  _rpcMethods.emplace("peerExists", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPeerExists()));
  _rpcMethods.emplace("ping", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPing()));
  _rpcMethods.emplace("putParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPutParamset()));
  _rpcMethods.emplace("removeEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveEvent()));
  _rpcMethods.emplace("removeLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveLink()));
  _rpcMethods.emplace("reportValueUsage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCReportValueUsage()));
  _rpcMethods.emplace("rssiInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRssiInfo()));
  _rpcMethods.emplace("runScript", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRunScript()));
  _rpcMethods.emplace("searchDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSearchDevices()));
  _rpcMethods.emplace("searchInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSearchInterfaces()));
  _rpcMethods.emplace("setData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetData()));
  _rpcMethods.emplace("setGlobalServiceMessage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetGlobalServiceMessage()));
  _rpcMethods.emplace("setId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetId()));
  _rpcMethods.emplace("setInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetInstallMode()));
  _rpcMethods.emplace("setInterface", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetInterface()));
  _rpcMethods.emplace("setLanguage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetLanguage()));
  _rpcMethods.emplace("setLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetLinkInfo()));
  _rpcMethods.emplace("setMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetMetadata()));
  _rpcMethods.emplace("setName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetName()));
  _rpcMethods.emplace("setNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetNodeData()));
  _rpcMethods.emplace("setFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetFlowData()));
  _rpcMethods.emplace("setGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetGlobalData()));
  _rpcMethods.emplace("setNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetNodeVariable()));
  _rpcMethods.emplace("setSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetSystemVariable()));
  _rpcMethods.emplace("setTeam", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetTeam()));
  _rpcMethods.emplace("setValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetValue()));
  _rpcMethods.emplace("startSniffing", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCStartSniffing()));
  _rpcMethods.emplace("stopSniffing", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCStopSniffing()));
  _rpcMethods.emplace("subscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSubscribePeers()));
  _rpcMethods.emplace("triggerEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerEvent()));
  _rpcMethods.emplace("triggerRpcEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerRpcEvent()));
  _rpcMethods.emplace("unsubscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUnsubscribePeers()));
  _rpcMethods.emplace("updateFirmware", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateFirmware()));
  _rpcMethods.emplace("writeLog", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCWriteLog()));

  { // Node-BLUE
    _rpcMethods.emplace("addNodesToFlow", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCAddNodesToFlow()));
    _rpcMethods.emplace("flowHasTag", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCFlowHasTag()));
    _rpcMethods.emplace("removeNodesFromFlow", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRemoveNodesFromFlow()));
  }

  { // Buildings
    _rpcMethods.emplace("addStoryToBuilding", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCAddStoryToBuilding()));
    _rpcMethods.emplace("createBuilding", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCCreateBuilding()));
    _rpcMethods.emplace("deleteBuilding", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCDeleteBuilding()));
    _rpcMethods.emplace("getStoriesInBuilding", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetStoriesInBuilding()));
    _rpcMethods.emplace("getBuildingMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetBuildingMetadata()));
    _rpcMethods.emplace("getBuildings", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetBuildings()));
    _rpcMethods.emplace("removeStoryFromBuilding", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRemoveStoryFromBuilding()));
    _rpcMethods.emplace("setBuildingMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetBuildingMetadata()));
    _rpcMethods.emplace("updateBuilding", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCUpdateBuilding()));
  }

  { // Stories
    _rpcMethods.emplace("addRoomToStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddRoomToStory()));
    _rpcMethods.emplace("createStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateStory()));
    _rpcMethods.emplace("deleteStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteStory()));
    _rpcMethods.emplace("getRoomsInStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRoomsInStory()));
    _rpcMethods.emplace("getStoryMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetStoryMetadata()));
    _rpcMethods.emplace("getStories", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetStories()));
    _rpcMethods.emplace("removeRoomFromStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveRoomFromStory()));
    _rpcMethods.emplace("setStoryMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetStoryMetadata()));
    _rpcMethods.emplace("updateStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateStory()));
  }

  { // Rooms
    _rpcMethods.emplace("addChannelToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddChannelToRoom()));
    _rpcMethods.emplace("addDeviceToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDeviceToRoom()));
    _rpcMethods.emplace("addSystemVariableToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddSystemVariableToRoom()));
    _rpcMethods.emplace("addVariableToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddVariableToRoom()));
    _rpcMethods.emplace("createRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateRoom()));
    _rpcMethods.emplace("deleteRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteRoom()));
    _rpcMethods.emplace("getChannelsInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetChannelsInRoom()));
    _rpcMethods.emplace("getDevicesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDevicesInRoom()));
    _rpcMethods.emplace("getRoomMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRoomMetadata()));
    _rpcMethods.emplace("getSystemVariablesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariablesInRoom()));
    _rpcMethods.emplace("getVariablesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariablesInRoom()));
    _rpcMethods.emplace("getRooms", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRooms()));
    _rpcMethods.emplace("removeChannelFromRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveChannelFromRoom()));
    _rpcMethods.emplace("removeDeviceFromRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveDeviceFromRoom()));
    _rpcMethods.emplace("removeSystemVariableFromRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveSystemVariableFromRoom()));
    _rpcMethods.emplace("removeVariableFromRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveVariableFromRoom()));
    _rpcMethods.emplace("setRoomMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetRoomMetadata()));
    _rpcMethods.emplace("updateRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateRoom()));
  }

  { // Categories
    _rpcMethods.emplace("addCategoryToChannel", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToChannel()));
    _rpcMethods.emplace("addCategoryToDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToDevice()));
    _rpcMethods.emplace("addCategoryToSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToSystemVariable()));
    _rpcMethods.emplace("addCategoryToVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToVariable()));
    _rpcMethods.emplace("createCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateCategory()));
    _rpcMethods.emplace("deleteCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteCategory()));
    _rpcMethods.emplace("getCategories", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetCategories()));
    _rpcMethods.emplace("getCategoryMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetCategoryMetadata()));
    _rpcMethods.emplace("getChannelsInCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetChannelsInCategory()));
    _rpcMethods.emplace("getDevicesInCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDevicesInCategory()));
    _rpcMethods.emplace("getSystemVariablesInCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariablesInCategory()));
    _rpcMethods.emplace("getVariablesInCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariablesInCategory()));
    _rpcMethods.emplace("removeCategoryFromChannel", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromChannel()));
    _rpcMethods.emplace("removeCategoryFromDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromDevice()));
    _rpcMethods.emplace("removeCategoryFromSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromSystemVariable()));
    _rpcMethods.emplace("removeCategoryFromVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromVariable()));
    _rpcMethods.emplace("setCategoryMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetCategoryMetadata()));
    _rpcMethods.emplace("updateCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateCategory()));
  }

  { // System variables
    _rpcMethods.emplace("addRoleToSystemVariable", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCAddRoleToSystemVariable>()));
    _rpcMethods.emplace("getSystemVariablesInRole", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetSystemVariablesInRole>()));
    _rpcMethods.emplace("removeRoleFromSystemVariable", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCRemoveRoleFromSystemVariable>()));
  }

  { // Roles
    _rpcMethods.emplace("addRoleToVariable", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCAddRoleToVariable>()));
    _rpcMethods.emplace("aggregateRoles", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCAggregateRoles>()));
    _rpcMethods.emplace("createRole", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCCreateRole>()));
    _rpcMethods.emplace("deleteRole", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCDeleteRole>()));
    _rpcMethods.emplace("getRoles", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRoles>()));
    _rpcMethods.emplace("getRolesInDevice", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRolesInDevice>()));
    _rpcMethods.emplace("getRolesInRoom", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRolesInRoom>()));
    _rpcMethods.emplace("getRoleMetadata", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRoleMetadata>()));
    _rpcMethods.emplace("getVariablesInRole", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetVariablesInRole>()));
    _rpcMethods.emplace("removeRoleFromVariable", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCRemoveRoleFromVariable>()));
    _rpcMethods.emplace("setRoleMetadata", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCSetRoleMetadata>()));
    _rpcMethods.emplace("updateRole", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCUpdateRole>()));
  }

  { // UI
    _rpcMethods.emplace("addUiElement", std::make_shared<RpcMethods::RpcAddUiElement>());
    _rpcMethods.emplace("checkUiElementSimpleCreation", std::make_shared<RpcMethods::RpcCheckUiElementSimpleCreation>());
    _rpcMethods.emplace("getAllUiElements", std::make_shared<RpcMethods::RpcGetAllUiElements>());
    _rpcMethods.emplace("getAvailableUiElements", std::make_shared<RpcMethods::RpcGetAvailableUiElements>());
    _rpcMethods.emplace("getCategoryUiElements", std::make_shared<RpcMethods::RpcGetCategoryUiElements>());
    _rpcMethods.emplace("getRoomUiElements", std::make_shared<RpcMethods::RpcGetRoomUiElements>());
    _rpcMethods.emplace("getUiElement", std::make_shared<RpcMethods::RpcGetUiElement>());
    _rpcMethods.emplace("getUiElementMetadata", std::make_shared<RpcMethods::RpcGetUiElementMetadata>());
    _rpcMethods.emplace("getUiElementsWithVariable", std::make_shared<RpcMethods::RpcGetUiElementsWithVariable>());
    _rpcMethods.emplace("requestUiRefresh", std::make_shared<RpcMethods::RpcRequestUiRefresh>());
    _rpcMethods.emplace("removeUiElement", std::make_shared<RpcMethods::RpcRemoveUiElement>());
    _rpcMethods.emplace("setUiElementMetadata", std::make_shared<RpcMethods::RpcSetUiElementMetadata>());
  }

  _localRpcMethods.emplace("scriptOutput", std::bind(&ScriptEngineServer::scriptOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("scriptHeaders", std::bind(&ScriptEngineServer::scriptHeaders, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("peerExists", std::bind(&ScriptEngineServer::peerExists, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  _localRpcMethods.emplace("listRpcClients", std::bind(&ScriptEngineServer::listRpcClients, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("raiseDeleteDevice", std::bind(&ScriptEngineServer::raiseDeleteDevice, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getFamilySetting", std::bind(&ScriptEngineServer::getFamilySetting, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("setFamilySetting", std::bind(&ScriptEngineServer::setFamilySetting, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("deleteFamilySetting", std::bind(&ScriptEngineServer::deleteFamilySetting, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  _localRpcMethods.emplace("mqttPublish", std::bind(&ScriptEngineServer::mqttPublish, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  //{{{ Users
  _localRpcMethods.emplace("auth", std::bind(&ScriptEngineServer::auth, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("createUser", std::bind(&ScriptEngineServer::createUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("deleteUser", std::bind(&ScriptEngineServer::deleteUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getUserMetadata", std::bind(&ScriptEngineServer::getUserMetadata, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("listUsers", std::bind(&ScriptEngineServer::listUsers, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("setUserMetadata", std::bind(&ScriptEngineServer::setUserMetadata, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getUsersGroups", std::bind(&ScriptEngineServer::getUsersGroups, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("updateUser", std::bind(&ScriptEngineServer::updateUser, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("userExists", std::bind(&ScriptEngineServer::userExists, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  _localRpcMethods.emplace("createOauthKeys", std::bind(&ScriptEngineServer::createOauthKeys, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("refreshOauthKey", std::bind(&ScriptEngineServer::refreshOauthKey, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("verifyOauthKey", std::bind(&ScriptEngineServer::verifyOauthKey, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  //}}}

  //{{{ User data
  _rpcMethods.emplace("deleteUserData", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCDeleteUserData>()));
  _rpcMethods.emplace("getUserData", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetUserData>()));
  _rpcMethods.emplace("setUserData", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCSetUserData>()));
  //}}}

  //{{{ Variable profiles
  _rpcMethods.emplace("activateVariableProfile", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcActivateVariableProfile>()));
  _rpcMethods.emplace("addVariableProfile", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcAddVariableProfile>()));
  _rpcMethods.emplace("deleteVariableProfile", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcDeleteVariableProfile>()));
  _rpcMethods.emplace("getAllVariableProfiles", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcGetAllVariableProfiles>()));
  _rpcMethods.emplace("getVariableProfile", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcGetVariableProfile>()));
  _rpcMethods.emplace("updateVariableProfile", std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcUpdateVariableProfile>()));
  //}}}

  //{{{ Groups
  _localRpcMethods.emplace("createGroup", std::bind(&ScriptEngineServer::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("deleteGroup", std::bind(&ScriptEngineServer::deleteGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getGroup", std::bind(&ScriptEngineServer::getGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getGroups", std::bind(&ScriptEngineServer::getGroups, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("groupExists", std::bind(&ScriptEngineServer::groupExists, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("updateGroup", std::bind(&ScriptEngineServer::updateGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  //}}}

  //{{{ Modules
  _localRpcMethods.emplace("listModules", std::bind(&ScriptEngineServer::listModules, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("loadModule", std::bind(&ScriptEngineServer::loadModule, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("unloadModule", std::bind(&ScriptEngineServer::unloadModule, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("reloadModule", std::bind(&ScriptEngineServer::reloadModule, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  //}}}

  //{{{ License modules
  _localRpcMethods.emplace("checkLicense", std::bind(&ScriptEngineServer::checkLicense, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("removeLicense", std::bind(&ScriptEngineServer::removeLicense, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getLicenseStates", std::bind(&ScriptEngineServer::getLicenseStates, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("getTrialStartTime", std::bind(&ScriptEngineServer::getTrialStartTime, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  //}}}

  //{{{ Node-BLUE
  _localRpcMethods.emplace("nodeEvent", std::bind(&ScriptEngineServer::nodeEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("nodeOutput", std::bind(&ScriptEngineServer::nodeOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  _localRpcMethods.emplace("executePhpNodeBaseMethod", std::bind(&ScriptEngineServer::executePhpNodeBaseMethod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  //}}}
}

ScriptEngineServer::~ScriptEngineServer() {
  if (!_stopServer) stop();
  GD::bl->threadManager.join(_scriptFinishedThread);
}

bool ScriptEngineServer::lifetick() {
  try {
    {
      std::lock_guard<std::mutex> lifetick1Guard(_lifetick1Mutex);
      if (!_lifetick1.second && BaseLib::HelperFunctions::getTime() - _lifetick1.first > 120000) {
        GD::out.printCritical("Critical: RPC server's lifetick 1 was not updated for more than 120 seconds.");
        return false;
      }
    }

    {
      std::lock_guard<std::mutex> lifetick2Guard(_lifetick2Mutex);
      if (!_lifetick2.second && BaseLib::HelperFunctions::getTime() - _lifetick2.first > 120000) {
        GD::out.printCritical("Critical: RPC server's lifetick 2 was not updated for more than 120 seconds.");
        return false;
      }
    }

    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      auto result = sendRequest(*i, "lifetick", std::make_shared<BaseLib::Array>(), true);
      if (result->errorStruct || !result->booleanValue) return false;
    }

    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

void ScriptEngineServer::collectGarbage() {
  try {
    _lastGargabeCollection = GD::bl->hf.getTime();
    std::vector<PScriptEngineProcess> processesToShutdown;
    {
      std::lock_guard<std::mutex> processGuard(_processMutex);
      bool emptyProcess = false;
      bool emptyNodeProcess = false;
      for (std::map<pid_t, PScriptEngineProcess>::iterator i = _processes.begin(); i != _processes.end(); ++i) {
        if (i->second->scriptCount() == 0 && BaseLib::HelperFunctions::getTime() - i->second->lastExecution > 10000 && i->second->getClientData() && !i->second->getClientData()->closed) {
          if (i->second->isNodeProcess()) {
            if (emptyNodeProcess) closeClientConnection(i->second->getClientData());
            else emptyNodeProcess = true;
          } else {
            if (emptyProcess) closeClientConnection(i->second->getClientData());
            else emptyProcess = true;
          }
        }
      }
    }

    std::vector<PScriptEngineClientData> clientsToRemove;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      try {
        for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
          if (i->second->closed) clientsToRemove.push_back(i->second);
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
    }
    for (std::vector<PScriptEngineClientData>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i) {
      {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        _clients.erase((*i)->id);
      }
      _out.printDebug("Debug: Script engine client " + std::to_string((*i)->id) + " removed.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool ScriptEngineServer::start() {
  try {
    stop();
    _processCallbackHandlerId = BaseLib::ProcessManager::registerCallbackHandler(std::function<void(pid_t pid, int exitCode, int signal, bool coreDumped)>(std::bind(&ScriptEngineServer::processKilled,
                                                                                                                                                                     this,
                                                                                                                                                                     std::placeholders::_1,
                                                                                                                                                                     std::placeholders::_2,
                                                                                                                                                                     std::placeholders::_3,
                                                                                                                                                                     std::placeholders::_4)));
    _socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
    _shuttingDown = false;
    _stopServer = false;
    if (!getFileDescriptor(true)) return false;
    uint32_t scriptEngineThreadCount = GD::bl->settings.scriptEngineThreadCount();
    if (scriptEngineThreadCount < 5) scriptEngineThreadCount = 5;
    startQueue(0, false, scriptEngineThreadCount, 0, SCHED_OTHER);
    startQueue(1, false, scriptEngineThreadCount, 0, SCHED_OTHER);
    startQueue(2, false, scriptEngineThreadCount, 0, SCHED_OTHER);
    GD::bl->threadManager.start(_mainThread, true, &ScriptEngineServer::mainThread, this);
    return true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

void ScriptEngineServer::stop() {
  try {
    _shuttingDown = true;
    _out.printDebug("Debug: Waiting for script engine server's client threads to finish.");
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        clients.push_back(i->second);
        closeClientConnection(i->second);
        std::lock_guard<std::mutex> processGuard(_processMutex);
        std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(i->second->pid);
        if (processIterator != _processes.end()) {
          processIterator->second->invokeScriptFinished(-32501);
          _processes.erase(processIterator);
        }
      }
    }

    _stopServer = true;
    GD::bl->threadManager.join(_mainThread);
    while (_clients.size() > 0) {
      for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
        std::unique_lock<std::mutex> waitLock((*i)->waitMutex);
        waitLock.unlock();
        (*i)->requestConditionVariable.notify_all();
      }
      collectGarbage();
      if (_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    stopQueue(0);
    stopQueue(1);
    stopQueue(2);
    unlink(_socketPath.c_str());
    BaseLib::ProcessManager::unregisterCallbackHandler(_processCallbackHandlerId);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::homegearShuttingDown() {
  try {
    _shuttingDown = true;
    stopDevices();
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }
    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      sendRequest(*i, "shutdown", parameters, false);
    }

    int32_t i = 0;
    while (_clients.size() > 0 && i < 60) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      collectGarbage();
      i++;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::homegearReloading() {
  try {
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }
    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(*i, "reload", parameters, false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::processKilled(pid_t pid, int exitCode, int signal, bool coreDumped) {
  try {
    std::shared_ptr<ScriptEngineProcess> process;

    {
      std::lock_guard<std::mutex> processGuard(_processMutex);
      std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(pid);
      if (processIterator != _processes.end()) {
        process = processIterator->second;
        closeClientConnection(process->getClientData());
        _processes.erase(processIterator);
      }
    }

    if (process) {
      if (signal != -1 && signal != 15) _out.printCritical("Critical: Client process with pid " + std::to_string(pid) + " was killed with signal " + std::to_string(signal) + '.');
      else if (exitCode != 0) _out.printError("Error: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + '.');
      else _out.printInfo("Info: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + '.');

      if (signal != -1 && signal != 15) exitCode = -32500;

      process->setExited(true);
      process->requestConditionVariable.notify_all();

      {
        std::lock_guard<std::mutex> scriptFinishedGuard(_scriptFinishedThreadMutex);
        GD::bl->threadManager.start(_scriptFinishedThread, true, &ScriptEngineServer::invokeScriptFinished, this, process, -1, exitCode);
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::devTestClient() {
  try {
    if (_shuttingDown) return;
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    BaseLib::Array parameterArray{std::make_shared<BaseLib::Variable>(6)};
    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>(std::move(parameterArray));
    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PVariable response = sendRequest(*i, "devTest", parameters, true);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

uint32_t ScriptEngineServer::scriptCount() {
  try {
    if (_shuttingDown) return 0;
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    uint32_t count = 0;
    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PVariable response = sendRequest(*i, "scriptCount", parameters, true);
      count += response->integerValue;
    }
    return count;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return 0;
}

std::vector<std::tuple<int32_t, uint64_t, std::string, int32_t, std::string>> ScriptEngineServer::getRunningScripts() {
  try {
    if (_shuttingDown) return std::vector<std::tuple<int32_t, uint64_t, std::string, int32_t, std::string>>();
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    std::vector<std::tuple<int32_t, uint64_t, std::string, int32_t, std::string>> runningScripts;
    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PVariable response = sendRequest(*i, "getRunningScripts", parameters, true);
      if (runningScripts.capacity() <= runningScripts.size() + response->arrayValue->size()) runningScripts.reserve(runningScripts.capacity() + response->arrayValue->size() + 100);
      for (auto &script : *(response->arrayValue)) {
        runningScripts.push_back(std::tuple<int32_t, uint64_t, std::string, int32_t, std::string>((int32_t)(*i)->pid,
                                                                                                  script->arrayValue->at(0)->integerValue64,
                                                                                                  script->arrayValue->at(1)->stringValue,
                                                                                                  script->arrayValue->at(2)->integerValue,
                                                                                                  script->arrayValue->at(3)->stringValue));
      }
    }
    return runningScripts;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::vector<std::tuple<int32_t, uint64_t, std::string, int32_t, std::string>>();
}

BaseLib::PVariable ScriptEngineServer::executePhpNodeMethod(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects exactly three parameters.");

    PScriptEngineClientData clientData;
    int32_t clientId = 0;
    {
      std::lock_guard<std::mutex> nodeClientIdMapGuard(_nodeClientIdMapMutex);
      auto nodeClientIdIterator = _nodeClientIdMap.find(parameters->at(0)->stringValue);
      if (nodeClientIdIterator == _nodeClientIdMap.end()) return BaseLib::Variable::createError(-1, "Unknown node.");
      clientId = nodeClientIdIterator->second;
    }
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) return BaseLib::Variable::createError(-32501, "Node process not found.");
      clientData = clientIterator->second;
    }

    BaseLib::PVariable result = sendRequest(clientData, "executePhpNodeMethod", parameters, true);
    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::executeDeviceMethod(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects exactly three parameters.");

    PScriptEngineClientData clientData;
    int32_t clientId = 0;
    {
      std::lock_guard<std::mutex> deviceClientIdMapGuard(_deviceClientIdMapMutex);
      auto deviceClientIdIterator = _deviceClientIdMap.find(parameters->at(0)->integerValue64);
      if (deviceClientIdIterator == _deviceClientIdMap.end()) return BaseLib::Variable::createError(-1, "Unknown device.");
      clientId = deviceClientIdIterator->second;
    }
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) return BaseLib::Variable::createError(-32501, "Device's process not found.");
      clientData = clientIterator->second;
    }

    BaseLib::PVariable result = sendRequest(clientData, "executeDeviceMethod", parameters, true);
    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineServer::broadcastEvent(std::string &source, uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> &variables, BaseLib::PArray &values) {
  try {
    if (_shuttingDown) return;
    if (!_scriptEngineClientInfo->acls->checkEventServerMethodAccess("event")) return;

    bool checkAcls = _scriptEngineClientInfo->acls->variablesRoomsCategoriesRolesDevicesReadSet();
    std::shared_ptr<BaseLib::Systems::Peer> peer;
    if (checkAcls) {
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
        if (central) peer = central->getPeer(id);
        if (peer) break;
      }

      if (!peer) return;

      std::shared_ptr<std::vector<std::string>> newVariables;
      BaseLib::PArray newValues;
      newVariables->reserve(variables->size());
      newValues->reserve(values->size());
      for (int32_t i = 0; i < (int32_t)variables->size(); i++) {
        if (id == 0) {
          if (_scriptEngineClientInfo->acls->variablesRoomsCategoriesRolesReadSet()) {
            auto systemVariable = GD::systemVariableController->getInternal(variables->at(i));
            if (systemVariable && _scriptEngineClientInfo->acls->checkSystemVariableReadAccess(systemVariable)) {
              newVariables->push_back(variables->at(i));
              newValues->push_back(values->at(i));
            }
          }
        } else if (peer && _scriptEngineClientInfo->acls->checkVariableReadAccess(peer, channel, variables->at(i))) {
          newVariables->push_back(variables->at(i));
          newValues->push_back(values->at(i));
        }
      }
      variables = newVariables;
      values = newValues;
    }

    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(5);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(source));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(id));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(channel));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(*variables));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(values));
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastEvent", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastEvent\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::broadcastNewDevices(std::vector<uint64_t> &ids, BaseLib::PVariable deviceDescriptions) {
  try {
    if (_shuttingDown) return;

    if (!_scriptEngineClientInfo->acls->checkEventServerMethodAccess("newDevices")) return;
    if (_scriptEngineClientInfo->acls->roomsCategoriesRolesDevicesReadSet()) {
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
        if (central && central->peerExists((uint64_t)ids.front())) //All ids are from the same family
        {
          for (auto id : ids) {
            auto peer = central->getPeer((uint64_t)id);
            if (!peer || !_scriptEngineClientInfo->acls->checkDeviceReadAccess(peer)) return;
          }
        }
      }
    }

    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PArray parameters(new BaseLib::Array{deviceDescriptions});
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastNewDevices", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastNewDevices\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::broadcastDeleteDevices(BaseLib::PVariable deviceInfo) {
  try {
    if (_shuttingDown) return;
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PArray parameters(new BaseLib::Array{deviceInfo});
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastDeleteDevices", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastDeleteDevices\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint) {
  try {
    if (_shuttingDown) return;

    if (!_scriptEngineClientInfo->acls->checkEventServerMethodAccess("updateDevice")) return;
    if (_scriptEngineClientInfo->acls->roomsCategoriesRolesDevicesReadSet()) {
      std::shared_ptr<BaseLib::Systems::Peer> peer;
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
        if (central) peer = central->getPeer(id);
        if (!peer || !_scriptEngineClientInfo->acls->checkDeviceReadAccess(peer)) return;
      }
    }

    std::vector<PScriptEngineClientData> clients;

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id)), BaseLib::PVariable(new BaseLib::Variable(channel)), BaseLib::PVariable(new BaseLib::Variable(hint))});
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastUpdateDevice", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastUpdateDevice\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void ScriptEngineServer::broadcastVariableProfileStateChanged(uint64_t profileId, bool state) {
  try {
    if (_shuttingDown) return;

    if (!_scriptEngineClientInfo->acls->checkEventServerMethodAccess("variableProfileStateChanged")) return;

    std::vector<PScriptEngineClientData> clients;

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PScriptEngineClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(2);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(profileId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(state));

      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastVariableProfileStateChanged", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastVariableProfileStateChanged\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void ScriptEngineServer::closeClientConnection(PScriptEngineClientData client) {
  try {
    if (!client) return;
    GD::bl->fileDescriptorManager.shutdown(client->fileDescriptor);
    client->closed = true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::stopDevices() {
  try {
    _out.printInfo("Info: Stopping devices.");
    std::vector<PScriptEngineClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "stopDevices", parameters, true);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  try {
    std::shared_ptr<QueueEntry> queueEntry;
    queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
    if (!queueEntry || queueEntry->clientData->closed) return;

    if (index == 0) //Request
    {
      if (queueEntry->parameters->size() != 4) {
        _out.printError("Error: Wrong parameter count while calling method " + queueEntry->methodName);
        return;
      }

      PClientScriptInfo scriptInfo = std::make_shared<ClientScriptInfo>();
      auto scriptId = queueEntry->parameters->at(0)->structValue->at("scriptId");
      scriptInfo->scriptId = queueEntry->parameters->at(0)->structValue->at("scriptId")->integerValue;
      std::string user = queueEntry->parameters->at(0)->structValue->at("user")->stringValue;
      std::string language = queueEntry->parameters->at(0)->structValue->at("language")->stringValue;

      scriptInfo->clientInfo = std::make_shared<BaseLib::RpcClientInfo>();
      *scriptInfo->clientInfo = *_scriptEngineClientInfo;

      scriptInfo->clientInfo->peerId = queueEntry->parameters->at(0)->structValue->at("peerId")->integerValue64;
      if (scriptInfo->clientInfo->peerId != 0) scriptInfo->clientInfo->addon = true;

      if (!user.empty()) {
        scriptInfo->clientInfo->user = user;
        scriptInfo->clientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), scriptInfo->scriptId);
        if (!scriptInfo->clientInfo->acls->fromUser(user)) {
          _out.printError("Error: Could not get ACLs for user \"" + user + "\".");
          auto result = BaseLib::Variable::createError(-32603, "Unauthorized.");
          sendResponse(queueEntry->clientData, scriptId, queueEntry->parameters->at(1), result);
        }
      }

      if (!language.empty()) scriptInfo->clientInfo->language = language;

      auto localMethodIterator = _localRpcMethods.find(queueEntry->methodName);
      if (localMethodIterator != _localRpcMethods.end()) {
        if (GD::bl->debugLevel >= 5) {
          _out.printInfo("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + queueEntry->methodName);
          for (BaseLib::Array::iterator i = queueEntry->parameters->at(3)->arrayValue->begin(); i != queueEntry->parameters->at(3)->arrayValue->end(); ++i) {
            (*i)->print(true, false);
          }
        }
        BaseLib::PVariable result = localMethodIterator->second(queueEntry->clientData, scriptInfo, queueEntry->parameters->at(3)->arrayValue);
        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Response: ");
          result->print(true, false);
        }

        if (queueEntry->parameters->at(2)->booleanValue) sendResponse(queueEntry->clientData, scriptId, queueEntry->parameters->at(1), result);
        return;
      }

      std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>>::iterator methodIterator = _rpcMethods.find(queueEntry->methodName);
      if (methodIterator == _rpcMethods.end()) {
        BaseLib::PVariable result = GD::ipcServer->callRpcMethod(scriptInfo->clientInfo, queueEntry->methodName, queueEntry->parameters->at(3)->arrayValue);
        if (queueEntry->parameters->at(2)->booleanValue) sendResponse(queueEntry->clientData, scriptId, queueEntry->parameters->at(1), result);
        return;
      }

      if (GD::bl->debugLevel >= 5) {
        _out.printInfo("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + queueEntry->methodName);
        for (std::vector<BaseLib::PVariable>::iterator i = queueEntry->parameters->at(3)->arrayValue->begin(); i != queueEntry->parameters->at(3)->arrayValue->end(); ++i) {
          (*i)->print(true, false);
        }
      }

      BaseLib::PVariable result = _rpcMethods.at(queueEntry->methodName)->invoke(scriptInfo->clientInfo, queueEntry->parameters->at(3)->arrayValue);
      if (GD::bl->debugLevel >= 5) {
        _out.printDebug("Response: ");
        result->print(true, false);
      }

      if (queueEntry->parameters->at(2)->booleanValue) sendResponse(queueEntry->clientData, scriptId, queueEntry->parameters->at(1), result);
    } else if (index == 1) //Response
    {
      BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
      int32_t packetId = response->arrayValue->at(0)->integerValue;

      {
        std::lock_guard<std::mutex> responseGuard(queueEntry->clientData->rpcResponsesMutex);
        auto responseIterator = queueEntry->clientData->rpcResponses.find(packetId);
        if (responseIterator != queueEntry->clientData->rpcResponses.end()) {
          PScriptEngineResponse element = responseIterator->second;
          if (element) {
            element->response = response;
            element->packetId = packetId;
            element->finished = true;
          }
        }
      }
      std::unique_lock<std::mutex> waitLock(queueEntry->clientData->waitMutex);
      waitLock.unlock();
      queueEntry->clientData->requestConditionVariable.notify_all();
    } else if (index == 2) //Second queue for sending packets. Response is processed by first queue
    {
      sendRequest(queueEntry->clientData, queueEntry->methodName, queueEntry->parameters, false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

BaseLib::PVariable ScriptEngineServer::send(PScriptEngineClientData &clientData, std::vector<char> &data) {
  try {
    int32_t totallySentBytes = 0;
    std::lock_guard<std::mutex> sendGuard(clientData->sendMutex);
    while (totallySentBytes < (signed)data.size()) {
      int32_t sentBytes = ::send(clientData->fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL);
      if (sentBytes <= 0) {
        if (errno == EAGAIN) continue;
        if (clientData->fileDescriptor->descriptor != -1) GD::out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
        return BaseLib::Variable::createError(-32500, "Unknown application error.");
      }
      totallySentBytes += sentBytes;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::PVariable(new BaseLib::Variable());
}

BaseLib::PVariable ScriptEngineServer::sendRequest(PScriptEngineClientData &clientData, std::string methodName, const BaseLib::PArray &parameters, bool wait) {
  try {
    {
      std::lock_guard<std::mutex> lifetick1Guard(_lifetick1Mutex);
      _lifetick1.second = false;
      _lifetick1.first = BaseLib::HelperFunctions::getTime();
    }

    int32_t packetId;
    {
      std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
      packetId = _currentPacketId++;
    }
    BaseLib::PArray array = std::make_shared<BaseLib::Array>();
    array->reserve(3);
    array->push_back(std::make_shared<BaseLib::Variable>(packetId));
    array->push_back(std::make_shared<BaseLib::Variable>(wait));
    array->push_back(std::make_shared<BaseLib::Variable>(parameters));
    std::vector<char> data;
    _rpcEncoder->encodeRequest(methodName, array, data);

    PScriptEngineResponse response;
    if (wait) {
      {
        std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
        auto result = clientData->rpcResponses.emplace(packetId, std::make_shared<ScriptEngineResponse>());
        if (result.second) response = result.first->second;
      }
      if (!response) {
        _out.printError("Critical: Could not insert response struct into map.");
        return BaseLib::Variable::createError(-32500, "Unknown application error.");
      }
    }

    if (GD::ipcLogger->enabled()) GD::ipcLogger->log(IpcModule::scriptEngine, packetId, clientData->pid, IpcLoggerPacketDirection::toClient, data);

    std::unique_lock<std::mutex> waitLock(clientData->waitMutex);
    BaseLib::PVariable result = send(clientData, data);
    if (result->errorStruct || !wait) {
      std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
      clientData->rpcResponses.erase(packetId);
      if (!wait) return std::make_shared<BaseLib::Variable>();
      else return result;
    }

    int32_t i = 0;
    while (!clientData->requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
      return response->finished || clientData->closed || _stopServer;
    })) {
      i++;
      if (i == 60) {
        _out.printError("Error: Script engine client with process ID " + std::to_string(clientData->pid) + " is not responding...");
        break;
      }
    }

    if (!response->finished || (response->response && response->response->arrayValue->size() != 2) || response->packetId != packetId) {
      _out.printError("Error: No or invalid response received to RPC request. Method: " + methodName + "." + (response->response ? " Response was: " + response->response->print(false, false, true) : ""));
      result = BaseLib::Variable::createError(-1, "No response received.");
    } else result = response->response->arrayValue->at(1);

    {
      std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
      clientData->rpcResponses.erase(packetId);
    }

    {
      std::lock_guard<std::mutex> lifetick1Guard(_lifetick1Mutex);
      _lifetick1.second = true;
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }

  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineServer::sendResponse(PScriptEngineClientData &clientData, BaseLib::PVariable &scriptId, BaseLib::PVariable &packetId, BaseLib::PVariable &variable) {
  try {
    {
      std::lock_guard<std::mutex> lifetick2Guard(_lifetick2Mutex);
      _lifetick2.second = false;
      _lifetick2.first = BaseLib::HelperFunctions::getTime();
    }

    BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{scriptId, packetId, variable})));
    std::vector<char> data;
    _rpcEncoder->encodeResponse(array, data);
    if (GD::ipcLogger->enabled()) GD::ipcLogger->log(IpcModule::scriptEngine, packetId->integerValue, clientData->pid, IpcLoggerPacketDirection::toClient, data);
    send(clientData, data);

    {
      std::lock_guard<std::mutex> lifetick2Guard(_lifetick2Mutex);
      _lifetick2.second = true;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::mainThread() {
  try {
    int32_t result = 0;
    std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor;
    while (!_stopServer) {
      if (!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        getFileDescriptor();
        continue;
      }

      timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      fd_set readFileDescriptor;
      int32_t maxfd = 0;
      FD_ZERO(&readFileDescriptor);
      {
        std::vector<PScriptEngineClientData> clients;
        {
          std::lock_guard<std::mutex> stateGuard(_stateMutex);
          clients.reserve(_clients.size());
          for (auto &client : _clients) {
            if (client.second->closed) continue;
            if (client.second->fileDescriptor->descriptor == -1) {
              client.second->closed = true;
              continue;
            }
            clients.push_back(client.second);
          }
        }

        auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
        fileDescriptorGuard.lock();
        maxfd = _serverFileDescriptor->descriptor;
        FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);

        for (auto &client : clients) {
          int32_t descriptor = client->fileDescriptor->descriptor;
          if (descriptor == -1) continue; //Never pass -1 to FD_SET => undefined behaviour. This is just a safety precaution, because the file descriptor manager is locked and descriptors shouldn't be modified.
          FD_SET(descriptor, &readFileDescriptor);
          if (descriptor > maxfd) maxfd = descriptor;
        }
      }

      result = select(maxfd + 1, &readFileDescriptor, NULL, NULL, &timeout);
      if (result == 0) {
        if (GD::bl->hf.getTime() - _lastGargabeCollection > 10000 || _clients.size() > GD::bl->settings.scriptEngineServerMaxConnections() * 100 / 112) collectGarbage();
        continue;
      } else if (result == -1) {
        if (errno == EINTR) continue;
        _out.printError("Error: select returned -1: " + std::string(strerror(errno)));
        continue;
      }

      if (FD_ISSET(_serverFileDescriptor->descriptor, &readFileDescriptor) && !_shuttingDown) {
        sockaddr_un clientAddress;
        socklen_t addressSize = sizeof(addressSize);
        std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr *)&clientAddress, &addressSize));
        if (!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;
        _out.printInfo("Info: Connection accepted. Client number: " + std::to_string(clientFileDescriptor->id));

        if (_clients.size() > GD::bl->settings.scriptEngineServerMaxConnections()) {
          collectGarbage();
          if (_clients.size() > GD::bl->settings.scriptEngineServerMaxConnections()) {
            _out.printError("Error: There are too many clients connected to me. Closing connection. You can increase the number of allowed connections in main.conf.");
            GD::bl->fileDescriptorManager.close(clientFileDescriptor);
            continue;
          }
        }

        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        if (_shuttingDown) {
          GD::bl->fileDescriptorManager.close(clientFileDescriptor);
          continue;
        }
        PScriptEngineClientData clientData = PScriptEngineClientData(new ScriptEngineClientData(clientFileDescriptor));
        clientData->id = _currentClientId++;
        _clients[clientData->id] = clientData;
        continue;
      }

      PScriptEngineClientData clientData;
      {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
          if (i->second->fileDescriptor->descriptor == -1) {
            i->second->closed = true;
            continue;
          }
          if (FD_ISSET(i->second->fileDescriptor->descriptor, &readFileDescriptor)) {
            clientData = i->second;
            break;
          }
        }
      }

      if (clientData) readClient(clientData);
    }
    GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

PScriptEngineProcess ScriptEngineServer::getFreeProcess(bool nodeProcess, uint32_t maxThreadCount) {
  try {
    std::lock_guard<std::mutex> processGuard(_newProcessMutex);

    if (nodeProcess && GD::bl->settings.maxNodeThreadsPerProcess() != -1 && maxThreadCount + 1 > (unsigned)GD::bl->settings.maxNodeThreadsPerProcess()) {
      GD::out.printError("Error: Could not get script process for node, because maximum number of threads in node is greater than the number of threads allowed per process.");
      return std::shared_ptr<ScriptEngineProcess>();
    }

    {
      std::lock_guard<std::mutex> processGuard(_processMutex);
      for (std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator i = _processes.begin(); i != _processes.end(); ++i) {
        if (i->second->getClientData()->closed) continue;
        if (nodeProcess && i->second->isNodeProcess() && (GD::bl->settings.maxNodeThreadsPerProcess() == -1 || i->second->nodeThreadCount() + maxThreadCount + 1 <= (unsigned)GD::bl->settings.maxNodeThreadsPerProcess())) {
          i->second->lastExecution = BaseLib::HelperFunctions::getTime();
          return i->second;
        } else if (!nodeProcess && !i->second->isNodeProcess() && i->second->scriptCount() < GD::bl->threadManager.getMaxThreadCount() / GD::bl->settings.scriptEngineMaxThreadsPerScript()
            && (GD::bl->settings.scriptEngineMaxScriptsPerProcess() == -1 || i->second->scriptCount() < (unsigned)GD::bl->settings.scriptEngineMaxScriptsPerProcess())) {
          i->second->lastExecution = BaseLib::HelperFunctions::getTime();
          return i->second;
        }
      }
    }
    _out.printInfo("Info: Spawning new script engine process.");
    std::shared_ptr<ScriptEngineProcess> process(new ScriptEngineProcess(nodeProcess));
    std::vector<std::string> arguments{"-c", GD::configPath, "-rse"};
    if (GD::bl->settings.scriptEngineManualClientStart()) {
      pid_t currentProcessId = _manualClientCurrentProcessId++;
      process->setPid(currentProcessId);
      {
        std::lock_guard<std::mutex> unconnectedProcessesGuard(_unconnectedProcessesMutex);
        _unconnectedProcesses.push(currentProcessId);
      }
    } else {
      process->setPid(BaseLib::ProcessManager::system(GD::executablePath + "/" + GD::executableFile, arguments, _bl->fileDescriptorManager.getMax()));
    }
    if (process->getPid() != -1) {
      std::unique_lock<std::mutex> processGuard(_processMutex);
      _processes[process->getPid()] = process;
      process->requestConditionVariable.wait_for(processGuard, std::chrono::milliseconds(120000), [&] { return (bool)(process->getClientData() || process->getExited()); });

      if (!process->getClientData()) {
        _processes.erase(process->getPid());
        _out.printError("Error: Could not start new script engine process.");
        return std::shared_ptr<ScriptEngineProcess>();
      }
      process->setUnregisterNode(std::function<void(std::string)>(std::bind(&ScriptEngineServer::unregisterNode, this, std::placeholders::_1)));
      process->setUnregisterDevice(std::function<void(uint64_t)>(std::bind(&ScriptEngineServer::unregisterDevice, this, std::placeholders::_1)));
      _out.printInfo("Info: Script engine process successfully spawned. Process id is " + std::to_string(process->getPid()) + ". Client id is: " + std::to_string(process->getClientData()->id) + ".");
      return process;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::shared_ptr<ScriptEngineProcess>();
}

void ScriptEngineServer::readClient(PScriptEngineClientData &clientData) {
  try {
    int32_t processedBytes = 0;
    int32_t bytesRead = 0;
    bytesRead = read(clientData->fileDescriptor->descriptor, clientData->buffer.data(), clientData->buffer.size());
    if (bytesRead <= 0) //read returns 0, when connection is disrupted.
    {
      _out.printInfo("Info: Connection to script server's client number " + std::to_string(clientData->fileDescriptor->id) + " closed.");
      closeClientConnection(clientData);
      return;
    }

    if (bytesRead > (signed)clientData->buffer.size()) bytesRead = clientData->buffer.size();

    try {
      processedBytes = 0;
      while (processedBytes < bytesRead) {
        processedBytes += clientData->binaryRpc->process(&(clientData->buffer[processedBytes]), bytesRead - processedBytes);
        if (clientData->binaryRpc->isFinished()) {
          if (GD::ipcLogger->enabled()) {
            if (clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request) {
              std::string methodName;
              BaseLib::PArray request = _rpcDecoder->decodeRequest(clientData->binaryRpc->getData(), methodName);
              GD::ipcLogger->log(IpcModule::scriptEngine, request->at(1)->integerValue, clientData->pid, IpcLoggerPacketDirection::toServer, clientData->binaryRpc->getData());
            } else {
              BaseLib::PVariable response = _rpcDecoder->decodeResponse(clientData->binaryRpc->getData());
              GD::ipcLogger->log(IpcModule::scriptEngine, response->arrayValue->at(0)->integerValue, clientData->pid, IpcLoggerPacketDirection::toServer, clientData->binaryRpc->getData());
            }
          }

          if (clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request) {
            std::string methodName;
            BaseLib::PArray parameters = _rpcDecoder->decodeRequest(clientData->binaryRpc->getData(), methodName);

            if (methodName == "registerScriptEngineClient" && parameters->size() == 4) {
              BaseLib::PVariable result = registerScriptEngineClient(clientData, parameters->at(3)->arrayValue);
              sendResponse(clientData, parameters->at(0)->structValue->at("scriptId"), parameters->at(1), result);
            } else if (methodName == "scriptFinished" && parameters->size() == 4) {
              scriptFinished(clientData, parameters->at(0)->structValue->at("scriptId")->integerValue, parameters->at(3)->arrayValue);
            } else {
              std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(clientData, methodName, parameters);
              if (!enqueue(0, queueEntry)) printQueueFullError(_out, "Error: Could not queue incoming RPC method call \"" + methodName + "\". Queue is full.");
            }
          } else //Response
          {
            std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(clientData, clientData->binaryRpc->getData());
            if (!enqueue(1, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC response. Queue is full.");
          }
          clientData->binaryRpc->reset();
        }
      }
    }
    catch (BaseLib::Rpc::BinaryRpcException &ex) {
      _out.printError("Error processing packet: " + std::string(ex.what()));
      clientData->binaryRpc->reset();
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool ScriptEngineServer::getFileDescriptor(bool deleteOldSocket) {
  try {
    if (!BaseLib::Io::directoryExists(GD::bl->settings.socketPath().c_str())) {
      if (errno == ENOENT) _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise the script engine won't work.");
      else _out.printCritical("Critical: Error reading information of directory " + GD::bl->settings.socketPath() + ". The script engine won't work: " + strerror(errno));
      _stopServer = true;
      return false;
    }
    bool isDirectory = false;
    int32_t result = BaseLib::Io::isDirectory(GD::bl->settings.socketPath(), isDirectory);
    if (result != 0 || !isDirectory) {
      _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise the script engine interface won't work.");
      _stopServer = true;
      return false;
    }
    if (deleteOldSocket) {
      if (unlink(_socketPath.c_str()) == -1 && errno != ENOENT) {
        _out.printCritical("Critical: Couldn't delete existing socket: " + _socketPath + ". Please delete it manually. The script engine won't work. Error: " + strerror(errno));
        return false;
      }
    } else if (BaseLib::Io::fileExists(_socketPath)) return false;

    _serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0));
    if (_serverFileDescriptor->descriptor == -1) {
      _out.printCritical("Critical: Couldn't create socket: " + _socketPath + ". The script engine won't work. Error: " + strerror(errno));
      return false;
    }
    int32_t reuseAddress = 1;
    if (setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseAddress, sizeof(int32_t)) == -1) {
      GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
      _out.printCritical("Critical: Couldn't set socket options: " + _socketPath + ". The script engine won't work correctly. Error: " + strerror(errno));
      return false;
    }
    sockaddr_un serverAddress;
    serverAddress.sun_family = AF_LOCAL;
    //104 is the size on BSD systems - slightly smaller than in Linux
    if (_socketPath.length() > 104) {
      //Check for buffer overflow
      _out.printCritical("Critical: Socket path is too long.");
      return false;
    }
    strncpy(serverAddress.sun_path, _socketPath.c_str(), 104);
    serverAddress.sun_path[103] = 0; //Just to make sure the string is null terminated.
    bool bound = (bind(_serverFileDescriptor->descriptor.load(), (sockaddr *)&serverAddress, strlen(serverAddress.sun_path) + 1 + sizeof(serverAddress.sun_family)) != -1);
    if (_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1) {
      GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
      _out.printCritical("Critical: Script engine server could not start listening. Error: " + std::string(strerror(errno)));
    }
    if (chmod(_socketPath.c_str(), S_IRWXU | S_IRWXG) == -1) {
      _out.printError("Error: chmod failed on unix socket \"" + _socketPath + "\".");
    }
    return true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
  return false;
}

std::string ScriptEngineServer::checkSessionId(const std::string &sessionId) {
  try {
    if (_shuttingDown) return "";
    PScriptEngineClientData client;

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (std::map<int32_t, PScriptEngineClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        client = i->second;
        break;
      }
    }
    if (!client) {
      PScriptEngineProcess process = getFreeProcess(false, 0);
      if (!process) {
        GD::out.printError("Error: Could not check session ID. Could not get free process.");
        return "";
      }
      client = process->getClientData();
    }
    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    parameters->push_back(std::make_shared<BaseLib::Variable>(sessionId));
    BaseLib::PVariable result = sendRequest(client, "checkSessionId", parameters, true);
    if (result->errorStruct) {
      GD::out.printError("Error: checkSessionId returned: " + result->structValue->at("faultString")->stringValue);
      return "";
    }
    return result->stringValue;
  }
  catch (const std::exception &ex) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return "";
}

void ScriptEngineServer::invokeScriptFinished(PScriptEngineProcess process, int32_t id, int32_t exitCode) {
  try {
    if (id == -1) process->invokeScriptFinished(exitCode);
    else {
      process->invokeScriptFinished(id, exitCode);
      process->unregisterScript(id);
    }
    if (exitCode != 0 && process->isNodeProcess() && !_shuttingDown && !GD::bl->shuttingDown) GD::nodeBlueServer->restartFlows();
  }
  catch (const std::exception &ex) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::invokeScriptFinishedEarly(PScriptInfo scriptInfo, int32_t exitCode) {
  try {
    if (scriptInfo->scriptFinishedCallback) scriptInfo->scriptFinishedCallback(scriptInfo, exitCode);
  }
  catch (const std::exception &ex) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::executeScript(PScriptInfo &scriptInfo, bool wait) {
  try {
    std::unique_lock<std::mutex> executeScriptGuard(_executeScriptMutex);
    if (_shuttingDown || GD::bl->shuttingDown) return;
    PScriptEngineProcess process = getFreeProcess(scriptInfo->getType() == BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode, scriptInfo->maxThreadCount);
    if (!process || _shuttingDown || GD::bl->shuttingDown) {
      if (!_shuttingDown && !GD::bl->shuttingDown) {
        _out.printError("Error: Could not get free process. Not executing script.");
        if (scriptInfo->returnOutput) scriptInfo->output.append("Error: Could not get free process. Not executing script.\n");
        scriptInfo->exitCode = -1;
      } else {
        _out.printInfo("Info: Not executing script, because I'm shutting down.");
        scriptInfo->exitCode = 0;
      }

      scriptInfo->finished = true;

      {
        std::lock_guard<std::mutex> scriptFinishedGuard(_scriptFinishedThreadMutex);
        GD::bl->threadManager.start(_scriptFinishedThread, true, &ScriptEngineServer::invokeScriptFinishedEarly, this, scriptInfo, -1);
      }

      return;
    }

    {
      std::lock_guard<std::mutex> scriptIdGuard(_currentScriptIdMutex);
      while (scriptInfo->id == 0) scriptInfo->id = _currentScriptId++;
    }

    if (GD::bl->debugLevel >= 5 || (scriptInfo->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode && scriptInfo->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::simpleNode)) {
      _out.printInfo("Info: Starting script \"" + scriptInfo->fullPath + "\" with id " + std::to_string(scriptInfo->id) + ".");
    }
    process->registerScript(scriptInfo->id, scriptInfo);

    PScriptEngineClientData clientData = process->getClientData();
    PScriptFinishedInfo scriptFinishedInfo;
    std::unique_lock<std::mutex> scriptFinishedLock;
    if (wait) {
      scriptFinishedInfo = process->getScriptFinishedInfo(scriptInfo->id);
      scriptFinishedLock = std::move(std::unique_lock<std::mutex>(scriptFinishedInfo->mutex));
    }

    BaseLib::PArray parameters;
    ScriptInfo::ScriptType scriptType = scriptInfo->getType();
    if (scriptType == ScriptInfo::ScriptType::cli) {
      parameters = std::move(BaseLib::PArray(new BaseLib::Array{
          std::make_shared<BaseLib::Variable>(scriptInfo->id),
          std::make_shared<BaseLib::Variable>((int32_t)scriptInfo->getType()),
          std::make_shared<BaseLib::Variable>(scriptInfo->fullPath),
          std::make_shared<BaseLib::Variable>(scriptInfo->relativePath),
          std::make_shared<BaseLib::Variable>(scriptInfo->script),
          std::make_shared<BaseLib::Variable>(scriptInfo->arguments),
          std::make_shared<BaseLib::Variable>((bool)scriptInfo->scriptOutputCallback || scriptInfo->returnOutput)}));
    } else if (scriptType == ScriptInfo::ScriptType::web) {
      parameters = std::move(BaseLib::PArray(new BaseLib::Array{
          std::make_shared<BaseLib::Variable>(scriptInfo->id),
          std::make_shared<BaseLib::Variable>((int32_t)scriptInfo->getType()),
          std::make_shared<BaseLib::Variable>(scriptInfo->contentPath),
          std::make_shared<BaseLib::Variable>(scriptInfo->fullPath),
          std::make_shared<BaseLib::Variable>(scriptInfo->relativePath),
          scriptInfo->http.serialize(),
          scriptInfo->serverInfo->serialize(),
          scriptInfo->clientInfo->serialize()}));
    } else if (scriptType == ScriptInfo::ScriptType::device || scriptType == ScriptInfo::ScriptType::device2) {
      parameters = std::move(BaseLib::PArray(new BaseLib::Array{
          std::make_shared<BaseLib::Variable>(scriptInfo->id),
          std::make_shared<BaseLib::Variable>((int32_t)scriptInfo->getType()),
          std::make_shared<BaseLib::Variable>(scriptInfo->fullPath),
          std::make_shared<BaseLib::Variable>(scriptInfo->relativePath),
          std::make_shared<BaseLib::Variable>(scriptInfo->script),
          std::make_shared<BaseLib::Variable>(scriptInfo->arguments),
          std::make_shared<BaseLib::Variable>(scriptInfo->peerId)}));

      if (scriptType == ScriptInfo::ScriptType::device2) {
        std::lock_guard<std::mutex> deviceClientIdMapGuard(_deviceClientIdMapMutex);
        _deviceClientIdMap.erase(scriptInfo->peerId);
        _deviceClientIdMap.emplace(scriptInfo->peerId, clientData->id);
      }
    } else if (scriptType == ScriptInfo::ScriptType::simpleNode) {
      parameters = std::move(BaseLib::PArray(new BaseLib::Array{
          std::make_shared<BaseLib::Variable>(scriptInfo->id),
          std::make_shared<BaseLib::Variable>((int32_t)scriptInfo->getType()),
          scriptInfo->nodeInfo,
          std::make_shared<BaseLib::Variable>(scriptInfo->fullPath),
          std::make_shared<BaseLib::Variable>(scriptInfo->relativePath),
          std::make_shared<BaseLib::Variable>(scriptInfo->inputPort),
          scriptInfo->message}));
    } else if (scriptType == ScriptInfo::ScriptType::statefulNode) {
      parameters = std::move(BaseLib::PArray(new BaseLib::Array{
          std::make_shared<BaseLib::Variable>(scriptInfo->id),
          std::make_shared<BaseLib::Variable>((int32_t)scriptInfo->getType()),
          scriptInfo->nodeInfo,
          std::make_shared<BaseLib::Variable>(scriptInfo->fullPath),
          std::make_shared<BaseLib::Variable>(scriptInfo->relativePath),
          std::make_shared<BaseLib::Variable>(scriptInfo->maxThreadCount)}));

      {
        std::lock_guard<std::mutex> nodeClientIdMapGuard(_nodeClientIdMapMutex);
        _nodeClientIdMap.erase(scriptInfo->nodeInfo->structValue->at("id")->stringValue);
        _nodeClientIdMap.emplace(scriptInfo->nodeInfo->structValue->at("id")->stringValue, clientData->id);
      }
    }

    BaseLib::PVariable result = sendRequest(clientData, "executeScript", parameters, true);
    if (result->errorStruct) {
      _out.printError("Error: Could not execute script: " + result->structValue->at("faultString")->stringValue);
      if (scriptInfo->returnOutput) scriptInfo->output.append("Error: Could not execute script: " + result->structValue->at("faultString")->stringValue + '\n');

      {
        std::lock_guard<std::mutex> scriptFinishedGuard(_scriptFinishedThreadMutex);
        GD::bl->threadManager.start(_scriptFinishedThread, true, &ScriptEngineServer::invokeScriptFinished, this, process, scriptInfo->id, result->structValue->at("faultCode")->integerValue);
      }

      if (scriptType == ScriptInfo::ScriptType::statefulNode) {
        std::lock_guard<std::mutex> nodeClientIdMapGuard(_nodeClientIdMapMutex);
        _nodeClientIdMap.erase(scriptInfo->nodeInfo->structValue->at("id")->stringValue);
      } else if (scriptType == ScriptInfo::ScriptType::device2) {
        std::lock_guard<std::mutex> deviceClientIdMapGuard(_deviceClientIdMapMutex);
        _deviceClientIdMap.erase(scriptInfo->peerId);
      }

      return;
    }
    scriptInfo->started = true;
    if (wait) {
      executeScriptGuard.unlock();
      while (!scriptFinishedInfo->conditionVariable.wait_for(scriptFinishedLock, std::chrono::milliseconds(10000), [&] { return scriptFinishedInfo->finished || clientData->closed || _stopServer; }));
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

BaseLib::PVariable ScriptEngineServer::getAllScripts() {
  try {
    BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));

    std::vector<std::string> scripts = GD::bl->io.getFiles(GD::bl->settings.scriptPath(), true);
    for (std::vector<std::string>::iterator i = scripts.begin(); i != scripts.end(); ++i) {
      array->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(*i)));
    }

    return array;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineServer::unregisterNode(std::string nodeId) {
  try {
    std::lock_guard<std::mutex> nodeClientIdMapGuard(_nodeClientIdMapMutex);
    _nodeClientIdMap.erase(nodeId);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineServer::unregisterDevice(uint64_t peerId) {
  try {
    std::lock_guard<std::mutex> deviceClientIdMapGuard(_deviceClientIdMapMutex);
    _deviceClientIdMap.erase(peerId);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

// {{{ RPC methods
BaseLib::PVariable ScriptEngineServer::registerScriptEngineClient(PScriptEngineClientData &clientData, BaseLib::PArray &parameters) {
  try {
    pid_t pid = parameters->at(0)->integerValue;
    if (GD::bl->settings.scriptEngineManualClientStart()) {
      std::lock_guard<std::mutex> unconnectedProcessesGuard(_unconnectedProcessesMutex);
      if (!_unconnectedProcesses.empty()) {
        pid = _unconnectedProcesses.front();
        _unconnectedProcesses.pop();
      }
    }
    std::unique_lock<std::mutex> processGuard(_processMutex);
    std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(pid);
    if (processIterator == _processes.end()) {
      _out.printError("Error: Cannot register client. No process with pid " + std::to_string(pid) + " found.");
      return BaseLib::Variable::createError(-1, "No matching process found.");
    }
    if (processIterator->second->getClientData()) {
      _out.printError("Error: Cannot register client, because it already is registered.");
      return BaseLib::PVariable(new BaseLib::Variable());
    }
    clientData->pid = pid;
    processIterator->second->setClientData(clientData);
    processGuard.unlock();
    processIterator->second->requestConditionVariable.notify_all();
    _out.printInfo("Info: Client with pid " + std::to_string(pid) + " successfully registered.");
    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::scriptFinished(PScriptEngineClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    int32_t exitCode = parameters->at(0)->integerValue;
    std::lock_guard<std::mutex> processGuard(_processMutex);
    std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(clientData->pid);
    if (processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
    processIterator->second->invokeScriptFinished(scriptId, exitCode);
    processIterator->second->unregisterScript(scriptId);
    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::scriptHeaders(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("scriptHeaders")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() < 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> processGuard(_processMutex);
    std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(clientData->pid);
    if (processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
    processIterator->second->invokeScriptHeaders(scriptInfo->scriptId, parameters->at(0));
    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::scriptOutput(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 2) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> processGuard(_processMutex);
    std::map<pid_t, std::shared_ptr<ScriptEngineProcess>>::iterator processIterator = _processes.find(clientData->pid);
    if (processIterator == _processes.end()) return BaseLib::Variable::createError(-1, "No matching process found.");
    processIterator->second->invokeScriptOutput(scriptInfo->scriptId, parameters->at(0)->stringValue, parameters->at(1)->booleanValue);
    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::peerExists(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("peerExists")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter is not of type integer.");

    if (scriptInfo->clientInfo->acls->roomsCategoriesRolesDevicesReadSet()) {
      auto families = GD::familyController->getFamilies();
      for (auto &family : families) {
        auto central = family.second->getCentral();
        if (central && central->peerExists((uint64_t)parameters->at(0)->integerValue64)) {
          auto peer = central->getPeer((uint64_t)parameters->at(0)->integerValue64);
          if (!peer || !scriptInfo->clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
        }
      }
    }

    return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->peerExists((uint64_t)parameters->at(0)->integerValue64)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::listRpcClients(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("listRpcClients")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 0) return BaseLib::Variable::createError(-1, "Method doesn't expect any parameters.");

    BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
    for (auto &server : GD::rpcServers) {
      const std::vector<BaseLib::PRpcClientInfo> clients = server.second->getClientInfo();
      for (std::vector<BaseLib::PRpcClientInfo>::const_iterator j = clients.begin(); j != clients.end(); ++j) {
        BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));

        element->structValue->insert(BaseLib::StructElement("CLIENT_ID", BaseLib::PVariable(new BaseLib::Variable((*j)->id))));
        element->structValue->insert(BaseLib::StructElement("IP_ADDRESS", BaseLib::PVariable(new BaseLib::Variable((*j)->address))));
        element->structValue->insert(BaseLib::StructElement("INIT_URL", BaseLib::PVariable(new BaseLib::Variable((*j)->initUrl))));
        element->structValue->insert(BaseLib::StructElement("INIT_INTERFACE_ID", BaseLib::PVariable(new BaseLib::Variable((*j)->initInterfaceId))));
        element->structValue->insert(BaseLib::StructElement("XML_RPC", BaseLib::PVariable(new BaseLib::Variable((*j)->rpcType == BaseLib::RpcType::xml))));
        element->structValue->insert(BaseLib::StructElement("BINARY_RPC", BaseLib::PVariable(new BaseLib::Variable((*j)->rpcType == BaseLib::RpcType::binary))));
        element->structValue->insert(BaseLib::StructElement("JSON_RPC", BaseLib::PVariable(new BaseLib::Variable((*j)->rpcType == BaseLib::RpcType::json))));
        element->structValue->insert(BaseLib::StructElement("WEBSOCKET", BaseLib::PVariable(new BaseLib::Variable((*j)->rpcType == BaseLib::RpcType::websocket))));
        element->structValue->insert(BaseLib::StructElement("INIT_KEEP_ALIVE", BaseLib::PVariable(new BaseLib::Variable((*j)->initKeepAlive))));
        element->structValue->insert(BaseLib::StructElement("INIT_BINARY_MODE", BaseLib::PVariable(new BaseLib::Variable((*j)->initBinaryMode))));
        element->structValue->insert(BaseLib::StructElement("INIT_NEW_FORMAT", BaseLib::PVariable(new BaseLib::Variable((*j)->initNewFormat))));
        element->structValue->insert(BaseLib::StructElement("INIT_SUBSCRIBE_PEERS", BaseLib::PVariable(new BaseLib::Variable((*j)->initSubscribePeers))));
        element->structValue->insert(BaseLib::StructElement("INIT_JSON_MODE", BaseLib::PVariable(new BaseLib::Variable((*j)->initJsonMode))));

        result->arrayValue->push_back(element);
      }
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::raiseDeleteDevice(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("raiseDeleteDevice")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1 || parameters->at(0)->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-1, "Method expects device address array as parameter.");

    std::vector<uint64_t> newIds{};
    GD::rpcClient->broadcastDeleteDevices(newIds, parameters->at(0), BaseLib::PVariable(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{0}))));

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getFamilySetting(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getFamilySetting")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2 || parameters->at(0)->type != BaseLib::VariableType::tInteger || parameters->at(1)->type != BaseLib::VariableType::tString)
      return BaseLib::Variable::createError(-1,
                                            "Method expects family ID and parameter name as parameters.");

    std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
    if (!family) return BaseLib::Variable::createError(-2, "Device family not found.");

    BaseLib::Systems::FamilySettings::PFamilySetting setting = family->getFamilySetting(parameters->at(1)->stringValue);
    if (!setting) return BaseLib::Variable::createError(-3, "Setting not found.");

    if (!setting->stringValue.empty()) return BaseLib::PVariable(new BaseLib::Variable(setting->stringValue));
    else if (!setting->binaryValue.empty()) {
      if (setting->binaryValue.size() > 3 && setting->binaryValue[0] == 'B' && setting->binaryValue[1] == 'i' && setting->binaryValue[2] == 'n') {
        try {
          return _rpcDecoder->decodeResponse(setting->binaryValue);
        }
        catch (std::exception &ex) {
          return BaseLib::PVariable(new BaseLib::Variable(setting->binaryValue));
        }
      } else return BaseLib::PVariable(new BaseLib::Variable(setting->binaryValue));
    } else return BaseLib::PVariable(new BaseLib::Variable(setting->integerValue));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::setFamilySetting(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("setFamilySetting")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 3 || parameters->at(0)->type != BaseLib::VariableType::tInteger || parameters->at(1)->type != BaseLib::VariableType::tString)
      return BaseLib::Variable::createError(-1,
                                            "Method expects family ID, parameter name and value as parameters.");

    std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
    if (!family) return BaseLib::Variable::createError(-2, "Device family not found.");

    if (parameters->at(2)->type == BaseLib::VariableType::tString) family->setFamilySetting(parameters->at(1)->stringValue, parameters->at(2)->stringValue);
    else if (parameters->at(2)->type == BaseLib::VariableType::tInteger) family->setFamilySetting(parameters->at(1)->stringValue, parameters->at(2)->integerValue);
    else if (parameters->at(2)->type == BaseLib::VariableType::tBinary) {
      std::vector<char> data(parameters->at(2)->binaryValue.data(), parameters->at(2)->binaryValue.data() + parameters->at(2)->binaryValue.size());
      family->setFamilySetting(parameters->at(1)->stringValue, data);
    } else return BaseLib::Variable::createError(-3, "Unsupported variable type.");

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::deleteFamilySetting(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("deleteFamilySetting")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2 || parameters->at(0)->type != BaseLib::VariableType::tInteger || parameters->at(1)->type != BaseLib::VariableType::tString)
      return BaseLib::Variable::createError(-1,
                                            "Method expects family ID and parameter name as parameters.");

    std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
    if (!family) return BaseLib::Variable::createError(-2, "Device family not found.");

    family->deleteFamilySettingFromDatabase(parameters->at(1)->stringValue);

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

// {{{ MQTT
BaseLib::PVariable ScriptEngineServer::mqttPublish(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("mqttPublish")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters (topic and payload).");
    if (parameters->at(0)->type != BaseLib::VariableType::tString || parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameters are not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Topic is empty.");
    if (parameters->at(1)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Payload is empty.");

    GD::mqtt->queueMessage(parameters->at(0)->stringValue, parameters->at(1)->stringValue);
    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

// {{{ User methods
BaseLib::PVariable ScriptEngineServer::auth(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("auth")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString || parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameters are not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 1 is empty.");
    if (parameters->at(1)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is empty.");

    BaseLib::HelperFunctions::toLower(parameters->at(0)->stringValue);

    return std::make_shared<BaseLib::Variable>(User::verify(parameters->at(0)->stringValue, parameters->at(1)->stringValue));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::createUser(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("createUser")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 3 && parameters->size() != 4) return BaseLib::Variable::createError(-1, "Method expects three or four parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString || parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 or 2 is not of type string.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 1 is empty.");
    if (parameters->at(2)->arrayValue->empty()) return BaseLib::Variable::createError(-1, "Parameter 3 is empty or no array.");
    if (parameters->size() == 4 && parameters->at(3)->type != BaseLib::VariableType::tStruct) return BaseLib::Variable::createError(-1, "Parameter 4 is not of type Struct.");

    BaseLib::HelperFunctions::toLower(parameters->at(0)->stringValue);

    std::vector<uint64_t> groups;
    groups.reserve(parameters->at(2)->arrayValue->size());
    for (auto &element : *parameters->at(2)->arrayValue) {
      if (element->integerValue64 != 0) groups.push_back(element->integerValue64);
    }
    if (groups.empty()) return BaseLib::Variable::createError(-1, "No groups specified.");

    return std::make_shared<BaseLib::Variable>(User::create(parameters->at(0)->stringValue, parameters->at(1)->stringValue, groups, parameters->size() == 4 ? parameters->at(4) : BaseLib::PVariable()));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::deleteUser(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("deleteUser")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    return std::make_shared<BaseLib::Variable>(User::remove(parameters->at(0)->stringValue));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getUserMetadata(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getUserMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");

    return User::getMetadata(parameters->at(0)->stringValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getUsersGroups(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getUsersGroups")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");

    auto groups = User::getGroups(parameters->at(0)->stringValue);
    if (groups.empty()) return BaseLib::Variable::createError(-1, "Unknown user.");

    auto groupArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    groupArray->arrayValue->reserve(groups.size());

    for (auto &group : groups) {
      groupArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(group));
    }

    return groupArray;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::setUserMetadata(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("setUserMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type String.");
    if (parameters->at(1)->type != BaseLib::VariableType::tStruct) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type Struct.");

    return std::make_shared<BaseLib::Variable>(User::setMetadata(parameters->at(0)->stringValue, parameters->at(1)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::updateUser(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("updateUser")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2 && parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects two or three parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type String.");
    if (parameters->at(1)->type != BaseLib::VariableType::tString && parameters->at(1)->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type string or array.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter 1 is empty.");
    if (parameters->at(1)->type == BaseLib::VariableType::tArray && parameters->at(1)->arrayValue->empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is empty.");

    BaseLib::HelperFunctions::toLower(parameters->at(0)->stringValue);

    int32_t groupsIndex = parameters->size() == 3 ? 2 : (parameters->at(1)->type == BaseLib::VariableType::tArray ? 1 : -1);
    if (groupsIndex != -1) {
      std::vector<uint64_t> groups;
      groups.reserve(parameters->at(groupsIndex)->arrayValue->size());
      for (auto &element : *parameters->at(groupsIndex)->arrayValue) {
        if (element->integerValue64 != 0) groups.push_back(element->integerValue64);
      }

      if (groupsIndex == 1) return std::make_shared<BaseLib::Variable>(User::update(parameters->at(0)->stringValue, groups));
      else return std::make_shared<BaseLib::Variable>(User::update(parameters->at(0)->stringValue, parameters->at(1)->stringValue, groups));
    } else return std::make_shared<BaseLib::Variable>(User::update(parameters->at(0)->stringValue, parameters->at(1)->stringValue));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::userExists(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("userExists")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    BaseLib::HelperFunctions::toLower(parameters->at(0)->stringValue);

    return std::make_shared<BaseLib::Variable>(User::exists(parameters->at(0)->stringValue));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::listUsers(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("listUsers")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 0) return BaseLib::Variable::createError(-1, "Method doesn't expect any parameters.");

    std::map<uint64_t, User::UserInfo> users;
    User::getAll(users);

    BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
    result->arrayValue->reserve(users.size());
    for (auto &user : users) {
      BaseLib::PVariable entry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      entry->structValue->emplace("id", std::make_shared<BaseLib::Variable>(user.first));
      entry->structValue->emplace("name", std::make_shared<BaseLib::Variable>(user.second.name));

      BaseLib::PVariable groups = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      groups->arrayValue->reserve(user.second.groups.size());
      for (auto &group : user.second.groups) {
        groups->arrayValue->push_back(std::make_shared<BaseLib::Variable>(group));
      }
      entry->structValue->emplace("groups", groups);

      entry->structValue->emplace("metadata", user.second.metadata);

      result->arrayValue->push_back(entry);
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::createOauthKeys(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("createOauthKeys")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    auto userName = BaseLib::HelperFunctions::toLower(parameters->at(0)->stringValue);
    if (!User::exists(userName)) return BaseLib::Variable::createError(-2, "User does not exist.");

    if (GD::bl->settings.oauthCertPath().empty() || GD::bl->settings.oauthKeyPath().empty()) return BaseLib::Variable::createError(-1, "No OAuth certificates specified in \"main.conf\".");
    std::string publicKey = BaseLib::Io::getFileContent(GD::bl->settings.oauthCertPath());
    std::string privateKey = BaseLib::Io::getFileContent(GD::bl->settings.oauthKeyPath());
    if (publicKey.empty() || privateKey.empty()) return BaseLib::Variable::createError(-1, "Private or public OAuth certificate couldn't be read.");

    std::string key;
    std::string refreshKey;
    if (!User::oauthCreate(userName, privateKey, publicKey, GD::bl->settings.oauthTokenLifetime(), GD::bl->settings.oauthRefreshTokenLifetime(), key, refreshKey)) {
      return BaseLib::Variable::createError(-3, "Error generating OAuth keys.");
    }

    auto oauthResponse = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    oauthResponse->structValue->emplace("token_type", std::make_shared<BaseLib::Variable>("bearer"));
    oauthResponse->structValue->emplace("access_token", std::make_shared<BaseLib::Variable>(key));
    oauthResponse->structValue->emplace("expires_in", std::make_shared<BaseLib::Variable>(std::to_string(GD::bl->settings.oauthTokenLifetime())));
    oauthResponse->structValue->emplace("refresh_token", std::make_shared<BaseLib::Variable>(refreshKey));

    return oauthResponse;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::refreshOauthKey(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("refreshOauthKey")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    auto refreshKey = parameters->at(0)->stringValue;

    if (GD::bl->settings.oauthCertPath().empty() || GD::bl->settings.oauthKeyPath().empty()) return BaseLib::Variable::createError(-1, "No OAuth certificates specified in \"main.conf\".");
    std::string publicKey = BaseLib::Io::getFileContent(GD::bl->settings.oauthCertPath());
    std::string privateKey = BaseLib::Io::getFileContent(GD::bl->settings.oauthKeyPath());
    if (publicKey.empty() || privateKey.empty()) return BaseLib::Variable::createError(-1, "Private or public OAuth certificate couldn't be read.");

    std::string key;
    std::string newRefreshKey;
    std::string userName;
    if (!User::oauthRefresh(refreshKey, privateKey, publicKey, GD::bl->settings.oauthTokenLifetime(), GD::bl->settings.oauthRefreshTokenLifetime(), key, newRefreshKey, userName)) {
      return BaseLib::Variable::createError(-3, "Could not refresh access key.");
    }

    auto oauthResponse = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    oauthResponse->structValue->emplace("token_type", std::make_shared<BaseLib::Variable>("bearer"));
    oauthResponse->structValue->emplace("access_token", std::make_shared<BaseLib::Variable>(key));
    oauthResponse->structValue->emplace("expires_in", std::make_shared<BaseLib::Variable>(std::to_string(GD::bl->settings.oauthTokenLifetime())));
    oauthResponse->structValue->emplace("refresh_token", std::make_shared<BaseLib::Variable>(newRefreshKey));
    oauthResponse->structValue->emplace("user", std::make_shared<BaseLib::Variable>(userName));

    return oauthResponse;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::verifyOauthKey(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("verifyOauthKey")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type String.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    auto key = parameters->at(0)->stringValue;

    if (GD::bl->settings.oauthCertPath().empty() || GD::bl->settings.oauthKeyPath().empty()) {
      GD::out.printError("Error: No OAuth certificates specified in \"main.conf\".");
      return BaseLib::Variable::createError(-1, "No OAuth certificates specified in \"main.conf\".");
    }
    std::string publicKey = BaseLib::Io::getFileContent(GD::bl->settings.oauthCertPath());
    std::string privateKey = BaseLib::Io::getFileContent(GD::bl->settings.oauthKeyPath());
    if (publicKey.empty() || privateKey.empty()) return BaseLib::Variable::createError(-1, "Private or public OAuth certificate couldn't be read.");

    std::string userName;
    if (!User::oauthVerify(key, privateKey, publicKey, userName)) return std::make_shared<BaseLib::Variable>(false);

    return std::make_shared<BaseLib::Variable>(userName);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

// {{{ Group methods
BaseLib::PVariable ScriptEngineServer::createGroup(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("createGroup")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects exactly two parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tStruct) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type struct.");
    if (parameters->at(1)->type != BaseLib::VariableType::tStruct || parameters->at(1)->structValue->empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type struct or is empty.");

    return _bl->db->createGroup(parameters->at(0), parameters->at(1));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::deleteGroup(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("deleteGroup")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter is not of type integer.");

    return _bl->db->deleteGroup(parameters->at(0)->integerValue64);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getGroup(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getGroup")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1 && parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects one or two parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->size() == 2 && parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type string.");

    std::string languageCode = parameters->size() == 2 ? parameters->at(1)->stringValue : "";

    return _bl->db->getGroup(parameters->at(0)->integerValue64, languageCode);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getGroups(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getGroups")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() > 1) return BaseLib::Variable::createError(-1, "Method expects one or two parameters.");
    if (parameters->size() == 1 && parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");

    return _bl->db->getGroups(parameters->empty() ? "" : parameters->at(0)->stringValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::groupExists(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("groupExists")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter is not of type integer.");

    return std::make_shared<BaseLib::Variable>(_bl->db->groupExists(parameters->at(0)->integerValue64));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::updateGroup(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("updateGroup")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 2 && parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects exactly two or three parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->at(1)->type != BaseLib::VariableType::tStruct || parameters->at(1)->structValue->empty()) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type struct or is empty.");
    if (parameters->size() == 3 && (parameters->at(2)->type != BaseLib::VariableType::tStruct || parameters->at(2)->structValue->empty())) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type struct or is empty.");

    if (parameters->size() == 2) return _bl->db->updateGroup(parameters->at(0)->integerValue64, std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct), parameters->at(1));
    else return _bl->db->updateGroup(parameters->at(0)->integerValue64, parameters->at(1), parameters->at(2));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

// {{{ Module methods
BaseLib::PVariable ScriptEngineServer::listModules(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("listModules")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 0) return BaseLib::Variable::createError(-1, "Method doesn't expect any parameters.");

    auto moduleInfo = GD::familyController->getModuleInfo();

    BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
    result->arrayValue->reserve(moduleInfo.size());
    for (auto &module : moduleInfo) {
      BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));

      element->structValue->insert(BaseLib::StructElement("FILENAME", BaseLib::PVariable(new BaseLib::Variable(module->filename))));
      element->structValue->insert(BaseLib::StructElement("FAMILY_ID", BaseLib::PVariable(new BaseLib::Variable(module->familyId))));
      element->structValue->insert(BaseLib::StructElement("FAMILY_NAME", BaseLib::PVariable(new BaseLib::Variable(module->familyName))));
      element->structValue->insert(BaseLib::StructElement("BASELIB_VERSION", BaseLib::PVariable(new BaseLib::Variable(module->baselibVersion))));
      element->structValue->insert(BaseLib::StructElement("LOADED", BaseLib::PVariable(new BaseLib::Variable(module->loaded))));

      result->arrayValue->push_back(element);
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::loadModule(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("loadModule")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->loadModule(parameters->at(0)->stringValue)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::unloadModule(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("unloadModule")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->unloadModule(parameters->at(0)->stringValue)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::reloadModule(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("reloadModule")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter is not of type string.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Parameter is empty.");

    return BaseLib::PVariable(new BaseLib::Variable(GD::familyController->reloadModule(parameters->at(0)->stringValue)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

// {{{ Licensing methods
BaseLib::PVariable ScriptEngineServer::checkLicense(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("checkLicense")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 3 && parameters->size() != 4) return BaseLib::Variable::createError(-1, "Method expects three or four parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->at(1)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type integer.");
    if (parameters->at(2)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type integer.");
    if (parameters->size() == 4 && parameters->at(3)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 4 is not of type string.");

    int32_t moduleId = parameters->at(0)->integerValue;
    int32_t familyId = parameters->at(1)->integerValue;
    int32_t deviceId = parameters->at(2)->integerValue;
    std::string licenseKey = (parameters->size() == 4) ? parameters->at(3)->stringValue : "";

    std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
    if (i == GD::licensingModules.end() || !i->second) {
      _out.printError("Error: Could not check license. License module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
      return BaseLib::PVariable(new BaseLib::Variable(-2));
    }

    return BaseLib::PVariable(new BaseLib::Variable(i->second->checkLicense(familyId, deviceId, licenseKey)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::removeLicense(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("removeLicense")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects exactly three parameters.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->at(1)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type integer.");
    if (parameters->at(2)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type integer.");

    int32_t moduleId = parameters->at(0)->integerValue;
    int32_t familyId = parameters->at(1)->integerValue;
    int32_t deviceId = parameters->at(2)->integerValue;

    std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
    if (i == GD::licensingModules.end() || !i->second) {
      _out.printError("Error: Could not check license. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
      return BaseLib::PVariable(new BaseLib::Variable(-2));
    }

    i->second->removeLicense(familyId, deviceId);

    return BaseLib::PVariable(new BaseLib::Variable(true));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getLicenseStates(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getLicenseStates")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects exactly one parameter.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");

    int32_t moduleId = parameters->at(0)->integerValue;

    std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
    if (i == GD::licensingModules.end() || !i->second) {
      _out.printError("Error: Could not check license. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
      return BaseLib::PVariable(new BaseLib::Variable(false));
    }

    BaseLib::Licensing::Licensing::DeviceStates states = i->second->getDeviceStates();
    BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tArray));
    result->arrayValue->reserve(states.size());
    for (BaseLib::Licensing::Licensing::DeviceStates::iterator i = states.begin(); i != states.end(); ++i) {
      for (std::map<int32_t, BaseLib::Licensing::Licensing::PDeviceInfo>::iterator j = i->second.begin(); j != i->second.end(); ++j) {
        BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));

        element->structValue->insert(BaseLib::StructElement("MODULE_ID", BaseLib::PVariable(new BaseLib::Variable(j->second->moduleId))));
        element->structValue->insert(BaseLib::StructElement("FAMILY_ID", BaseLib::PVariable(new BaseLib::Variable(j->second->familyId))));
        element->structValue->insert(BaseLib::StructElement("DEVICE_ID", BaseLib::PVariable(new BaseLib::Variable(j->second->deviceId))));
        element->structValue->insert(BaseLib::StructElement("ACTIVATED", BaseLib::PVariable(new BaseLib::Variable(j->second->state))));
        element->structValue->insert(BaseLib::StructElement("LICENSE_KEY", BaseLib::PVariable(new BaseLib::Variable(j->second->licenseKey))));

        result->arrayValue->push_back(element);
      }
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::getTrialStartTime(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (!scriptInfo->clientInfo || !scriptInfo->clientInfo->acls->checkMethodAccess("getTrialStartTime")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects three parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->at(1)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type integer.");
    if (parameters->at(2)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type integer.");

    int32_t moduleId = parameters->at(0)->integerValue;
    int32_t familyId = parameters->at(1)->integerValue;
    int32_t deviceId = parameters->at(2)->integerValue;

    std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
    if (i == GD::licensingModules.end() || !i->second) {
      _out.printError("Error: Could not check license. License module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
      return BaseLib::PVariable(new BaseLib::Variable(-2));
    }

    return BaseLib::PVariable(new BaseLib::Variable(i->second->getTrialStartTime(familyId, deviceId)));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

// {{{ Flows
BaseLib::PVariable ScriptEngineServer::nodeEvent(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects three parameters. " + std::to_string(parameters->size()) + " given.");

    GD::rpcClient->broadcastNodeEvent(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::nodeOutput(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3 && parameters->size() != 4) return BaseLib::Variable::createError(-1, "Method expects three parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");
    if (parameters->at(1)->type != BaseLib::VariableType::tInteger) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type integer.");
    if (parameters->size() == 4 && parameters->at(3)->type != BaseLib::VariableType::tBoolean) return BaseLib::Variable::createError(-1, "Parameter 4 is not of type boolean.");

    if (GD::nodeBlueServer) GD::nodeBlueServer->nodeOutput(parameters->at(0)->stringValue, parameters->at(1)->integerValue, parameters->at(2), parameters->size() == 4 ? parameters->at(3)->booleanValue : false);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineServer::executePhpNodeBaseMethod(PScriptEngineClientData &clientData, PClientScriptInfo scriptInfo, BaseLib::PArray &parameters) {
  try {
    if (GD::nodeBlueServer) {
      if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Method expects four parameters. " + std::to_string(parameters->size()) + " given.");
      if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");
      if (parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type string.");
      if (parameters->at(2)->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-1, "Parameter 3 is not of type array.");

      return GD::nodeBlueServer->executePhpNodeBaseMethod(parameters);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}
// }}}

}

}

#endif
