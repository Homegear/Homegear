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

#include "NodeBlueServer.h"
#include "../GD/GD.h"
#include "../RPC/RpcMethods/BuildingRpcMethods.h"
#include "../RPC/RpcMethods/UiRpcMethods.h"
#include "../RPC/RpcMethods/UiNotificationsRpcMethods.h"
#include "../RPC/RpcMethods/VariableProfileRpcMethods.h"
#include "../RPC/RpcMethods/NodeBlueRpcMethods.h"
#include "../RPC/RpcMethods/MaintenanceRpcMethods.h"
#include "FlowParser.h"
#include "../RPC/RpcMethods/BuildingPartRpcMethods.h"

#include <homegear-base/BaseLib.h>
#include <homegear-base/Managers/ProcessManager.h>

#include <sys/stat.h>

#include <memory>
#include <malloc.h>

namespace Homegear::NodeBlue {

NodeBlueServer::NodeBlueServer() : IQueue(GD::bl.get(), 3, 100000) {
  _out.init(GD::bl.get());
  _out.setPrefix("Node-BLUE Server: ");

  lifetick_1_.first = 0;
  lifetick_1_.second = true;
  lifetick_2_.first = 0;
  lifetick_2_.second = true;

  _nodepink = std::make_shared<Nodepink>();
  _nodepinkWebsocket = std::make_shared<NodepinkWebsocket>();
  _nodeManager = std::make_unique<NodeManager>(&_nodeEventsEnabled);
  _nodeBlueCredentials = std::make_unique<NodeBlueCredentials>(); //Constructor throws exceptions

  _rpcDecoder = std::make_unique<BaseLib::Rpc::RpcDecoder>(GD::bl.get(), false, false);
  _rpcEncoder = std::make_unique<BaseLib::Rpc::RpcEncoder>(GD::bl.get(), true, true);
  _jsonEncoder = std::make_unique<BaseLib::Rpc::JsonEncoder>(GD::bl.get());
  _jsonDecoder = std::make_unique<BaseLib::Rpc::JsonDecoder>(GD::bl.get());
  _nodeBlueClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
  _nodeBlueClientInfo->flowsServer = true;
  _nodeBlueClientInfo->initInterfaceId = "nodeBlue";
  _nodeBlueClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  std::vector<uint64_t> groups{4};
  _nodeBlueClientInfo->acls->fromGroups(groups);
  _nodeBlueClientInfo->user = "SYSTEM (4)";
  _nodePinkHttpClient =
      std::make_unique<BaseLib::HttpClient>(GD::bl.get(), "127.0.0.1", GD::bl->settings.nodeRedPort(), true, false);

  _rpcMethods.emplace("devTest", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDevTest()));
  _rpcMethods.emplace("system.getCapabilities",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemGetCapabilities()));
  _rpcMethods.emplace("system.listMethods", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemListMethods()));
  _rpcMethods.emplace("system.methodHelp", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodHelp()));
  _rpcMethods.emplace("system.methodSignature",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodSignature()));
  _rpcMethods.emplace("system.multicall", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMulticall()));
  _rpcMethods.emplace("acknowledgeGlobalServiceMessage",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAcknowledgeGlobalServiceMessage()));
  _rpcMethods.emplace("activateLinkParamset",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCActivateLinkParamset()));
  _rpcMethods.emplace("addDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDevice()));
  _rpcMethods.emplace("addLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddLink()));
  _rpcMethods.emplace("checkServiceAccess", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCheckServiceAccess()));
  _rpcMethods.emplace("copyConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCopyConfig()));
  _rpcMethods.emplace("clientServerInitialized",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCClientServerInitialized()));
  _rpcMethods.emplace("createDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateDevice()));
  _rpcMethods.emplace("deleteData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteData()));
  _rpcMethods.emplace("deleteDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteDevice()));
  _rpcMethods.emplace("deleteMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteMetadata()));
  _rpcMethods.emplace("deleteNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteNodeData()));
  _rpcMethods.emplace("deleteSystemVariable",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteSystemVariable()));
  _rpcMethods.emplace("executeMiscellaneousDeviceMethod",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCExecuteMiscellaneousDeviceMethod()));
  _rpcMethods.emplace("familyExists", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCFamilyExists()));
  _rpcMethods.emplace("forceConfigUpdate", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCForceConfigUpdate()));
  _rpcMethods.emplace("getAllConfig", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllConfig()));
  _rpcMethods.emplace("getAllMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllMetadata()));
  _rpcMethods.emplace("getAllScripts", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllScripts()));
  _rpcMethods.emplace("getAllSystemVariables",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllSystemVariables()));
  _rpcMethods.emplace("getAllValues", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetAllValues()));
  _rpcMethods.emplace("getConfigParameter", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetConfigParameter()));
  _rpcMethods.emplace("getData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetData()));
  _rpcMethods.emplace("getDeviceDescription",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDeviceDescription()));
  _rpcMethods.emplace("getDeviceInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDeviceInfo()));
  _rpcMethods.emplace("getLastEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLastEvents()));
  _rpcMethods.emplace("getInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetInstallMode()));
  _rpcMethods.emplace("getInstanceId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetInstanceId()));
  _rpcMethods.emplace("getKeyMismatchDevice",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetKeyMismatchDevice()));
  _rpcMethods.emplace("getLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkInfo()));
  _rpcMethods.emplace("getLinkPeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkPeers()));
  _rpcMethods.emplace("getLinks", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinks()));
  _rpcMethods.emplace("getMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetMetadata()));
  _rpcMethods.emplace("getName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetName()));
  _rpcMethods.emplace("getNodeBlueDataKeys",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetNodeBlueDataKeys()));
  _rpcMethods.emplace("getNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetNodeData()));
  _rpcMethods.emplace("getFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetFlowData()));
  _rpcMethods.emplace("getGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetGlobalData()));
  _rpcMethods.emplace("getNodeVariable",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetNodeVariable()));
  _rpcMethods.emplace("getPairingInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPairingInfo()));
  _rpcMethods.emplace("getPairingState", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPairingState()));
  _rpcMethods.emplace("getParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamset()));
  _rpcMethods.emplace("getParamsetDescription",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamsetDescription()));
  _rpcMethods.emplace("getParamsetId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetParamsetId()));
  _rpcMethods.emplace("getPeerId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetPeerId()));
  _rpcMethods.emplace("getServiceMessages", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetServiceMessages()));
  _rpcMethods.emplace("getSniffedDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSniffedDevices()));
  _rpcMethods.emplace("getSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariable()));
  _rpcMethods.emplace("getSystemVariableFlags",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariableFlags()));
  _rpcMethods.emplace("getUpdateStatus", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetUpdateStatus()));
  _rpcMethods.emplace("getValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetValue()));
  _rpcMethods.emplace("getVariableDescription",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariableDescription()));
  _rpcMethods.emplace("getVersion", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVersion()));
  _rpcMethods.emplace("init", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCInit()));
  _rpcMethods.emplace("invokeFamilyMethod", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCInvokeFamilyMethod()));
  _rpcMethods.emplace("lifetick", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCLifetick()));
  _rpcMethods.emplace("listBidcosInterfaces",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListBidcosInterfaces()));
  _rpcMethods.emplace("listClientServers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListClientServers()));
  _rpcMethods.emplace("listDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListDevices()));
  _rpcMethods.emplace("listFamilies", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListFamilies()));
  _rpcMethods.emplace("listInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListInterfaces()));
  _rpcMethods.emplace("listKnownDeviceTypes",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListKnownDeviceTypes()));
  _rpcMethods.emplace("listTeams", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListTeams()));
  _rpcMethods.emplace("logLevel", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCLogLevel()));
  _rpcMethods.emplace("peerExists", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPeerExists()));
  _rpcMethods.emplace("ping", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPing()));
  _rpcMethods.emplace("putParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCPutParamset()));
  _rpcMethods.emplace("removeLink", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveLink()));
  _rpcMethods.emplace("reportValueUsage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCReportValueUsage()));
  _rpcMethods.emplace("rssiInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRssiInfo()));
  _rpcMethods.emplace("runScript", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRunScript()));
  _rpcMethods.emplace("searchDevices", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSearchDevices()));
  _rpcMethods.emplace("searchInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSearchInterfaces()));
  _rpcMethods.emplace("setData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetData()));
  _rpcMethods.emplace("setGlobalServiceMessage",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetGlobalServiceMessage()));
  _rpcMethods.emplace("setId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetId()));
  _rpcMethods.emplace("setInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetInstallMode()));
  _rpcMethods.emplace("setInterface", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetInterface()));
  _rpcMethods.emplace("setLanguage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetLanguage()));
  _rpcMethods.emplace("setLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetLinkInfo()));
  _rpcMethods.emplace("setMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetMetadata()));
  _rpcMethods.emplace("setName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetName()));
  _rpcMethods.emplace("setNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetNodeData()));
  _rpcMethods.emplace("setFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetFlowData()));
  _rpcMethods.emplace("setGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetGlobalData()));
  _rpcMethods.emplace("setNodeVariable",
                      std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetNodeVariable()));
  _rpcMethods.emplace("setSerialNumber", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetSerialNumber()));
  _rpcMethods.emplace("setSystemVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetSystemVariable()));
  _rpcMethods.emplace("setTeam", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetTeam()));
  _rpcMethods.emplace("setValue", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetValue()));
  _rpcMethods.emplace("startSniffing", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCStartSniffing()));
  _rpcMethods.emplace("stopSniffing", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCStopSniffing()));
  _rpcMethods.emplace("subscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSubscribePeers()));
  _rpcMethods.emplace("triggerRpcEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCTriggerRpcEvent()));
  _rpcMethods.emplace("unsubscribePeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUnsubscribePeers()));
  _rpcMethods.emplace("updateFirmware", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateFirmware()));
  _rpcMethods.emplace("writeLog", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCWriteLog()));

  { // Node-BLUE
    _rpcMethods.emplace("addNodesToFlow",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCAddNodesToFlow()));
    _rpcMethods.emplace("flowHasTag", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCFlowHasTag()));
    _rpcMethods.emplace("removeNodesFromFlow",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRemoveNodesFromFlow()));
  }

  { // Buildings
    _rpcMethods.emplace("addBuildingPartToBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCAddBuildingPartToBuilding()));
    _rpcMethods.emplace("addStoryToBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCAddStoryToBuilding()));
    _rpcMethods.emplace("createBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCCreateBuilding()));
    _rpcMethods.emplace("deleteBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCDeleteBuilding()));
    _rpcMethods.emplace("getBuildingPartsInBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetBuildingPartsInBuilding()));
    _rpcMethods.emplace("getStoriesInBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetStoriesInBuilding()));
    _rpcMethods.emplace("getBuildingMetadata",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetBuildingMetadata()));
    _rpcMethods.emplace("getBuildings", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetBuildings()));
    _rpcMethods.emplace("removeBuildingPartFromBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRemoveBuildingPartFromBuilding()));
    _rpcMethods.emplace("removeStoryFromBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRemoveStoryFromBuilding()));
    _rpcMethods.emplace("setBuildingMetadata",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetBuildingMetadata()));
    _rpcMethods.emplace("updateBuilding",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCUpdateBuilding()));
  }

  { // Building parts
    _rpcMethods.emplace("addChannelToBuildingPart", std::make_shared<RpcMethods::RPCAddChannelToBuildingPart>());
    _rpcMethods.emplace("addDeviceToBuildingPart", std::make_shared<RpcMethods::RPCAddDeviceToBuildingPart>());
    _rpcMethods.emplace("addVariableToBuildingPart", std::make_shared<RpcMethods::RPCAddVariableToBuildingPart>());
    _rpcMethods.emplace("createBuildingPart", std::make_shared<RpcMethods::RPCCreateBuildingPart>());
    _rpcMethods.emplace("deleteBuildingPart", std::make_shared<RpcMethods::RPCDeleteBuildingPart>());
    _rpcMethods.emplace("getBuildingPartMetadata", std::make_shared<RpcMethods::RPCGetBuildingPartMetadata>());
    _rpcMethods.emplace("getBuildingParts", std::make_shared<RpcMethods::RPCGetBuildingParts>());
    _rpcMethods.emplace("getChannelsInBuildingPart", std::make_shared<RpcMethods::RPCGetChannelsInBuildingPart>());
    _rpcMethods.emplace("getDevicesInBuildingPart", std::make_shared<RpcMethods::RPCGetDevicesInBuildingPart>());
    _rpcMethods.emplace("getVariablesInBuildingPart", std::make_shared<RpcMethods::RPCGetVariablesInBuildingPart>());
    _rpcMethods.emplace("removeChannelFromBuildingPart",
                        std::make_shared<RpcMethods::RPCRemoveChannelFromBuildingPart>());
    _rpcMethods.emplace("removeDeviceFromBuildingPart",
                        std::make_shared<RpcMethods::RPCRemoveDeviceFromBuildingPart>());
    _rpcMethods.emplace("removeVariableFromBuildingPart",
                        std::make_shared<RpcMethods::RPCRemoveVariableFromBuildingPart>());
    _rpcMethods.emplace("setBuildingPartMetadata", std::make_shared<RpcMethods::RPCSetBuildingPartMetadata>());
    _rpcMethods.emplace("updateBuildingPart", std::make_shared<RpcMethods::RPCUpdateBuildingPart>());
  }

  { // Stories
    _rpcMethods.emplace("addRoomToStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddRoomToStory()));
    _rpcMethods.emplace("createStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateStory()));
    _rpcMethods.emplace("deleteStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteStory()));
    _rpcMethods.emplace("getRoomsInStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRoomsInStory()));
    _rpcMethods.emplace("getStoryMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetStoryMetadata()));
    _rpcMethods.emplace("getStories", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetStories()));
    _rpcMethods.emplace("removeRoomFromStory",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveRoomFromStory()));
    _rpcMethods.emplace("setStoryMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetStoryMetadata()));
    _rpcMethods.emplace("updateStory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateStory()));
  }

  { // Rooms
    _rpcMethods.emplace("addChannelToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddChannelToRoom()));
    _rpcMethods.emplace("addDeviceToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDeviceToRoom()));
    _rpcMethods.emplace("addSystemVariableToRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddSystemVariableToRoom()));
    _rpcMethods.emplace("addVariableToRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddVariableToRoom()));
    _rpcMethods.emplace("createRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateRoom()));
    _rpcMethods.emplace("deleteRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteRoom()));
    _rpcMethods.emplace("getChannelsInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetChannelsInRoom()));
    _rpcMethods.emplace("getDevicesInRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDevicesInRoom()));
    _rpcMethods.emplace("getRoomMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRoomMetadata()));
    _rpcMethods.emplace("getSystemVariablesInRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariablesInRoom()));
    _rpcMethods.emplace("getVariablesInRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariablesInRoom()));
    _rpcMethods.emplace("getRooms", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetRooms()));
    _rpcMethods.emplace("removeChannelFromRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveChannelFromRoom()));
    _rpcMethods.emplace("removeDeviceFromRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveDeviceFromRoom()));
    _rpcMethods.emplace("removeSystemVariableFromRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveSystemVariableFromRoom()));
    _rpcMethods.emplace("removeVariableFromRoom",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveVariableFromRoom()));
    _rpcMethods.emplace("setRoomMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetRoomMetadata()));
    _rpcMethods.emplace("updateRoom", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateRoom()));
  }

  { // Categories
    _rpcMethods.emplace("addCategoryToChannel",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToChannel()));
    _rpcMethods.emplace("addCategoryToDevice",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToDevice()));
    _rpcMethods.emplace("addCategoryToSystemVariable",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToSystemVariable()));
    _rpcMethods.emplace("addCategoryToVariable",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddCategoryToVariable()));
    _rpcMethods.emplace("createCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCCreateCategory()));
    _rpcMethods.emplace("deleteCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDeleteCategory()));
    _rpcMethods.emplace("getCategories", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetCategories()));
    _rpcMethods.emplace("getCategoryMetadata",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetCategoryMetadata()));
    _rpcMethods.emplace("getChannelsInCategory",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetChannelsInCategory()));
    _rpcMethods.emplace("getDevicesInCategory",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetDevicesInCategory()));
    _rpcMethods.emplace("getSystemVariablesInCategory",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetSystemVariablesInCategory()));
    _rpcMethods.emplace("getVariablesInCategory",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetVariablesInCategory()));
    _rpcMethods.emplace("removeCategoryFromChannel",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromChannel()));
    _rpcMethods.emplace("removeCategoryFromDevice",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromDevice()));
    _rpcMethods.emplace("removeCategoryFromSystemVariable",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromSystemVariable()));
    _rpcMethods.emplace("removeCategoryFromVariable",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCRemoveCategoryFromVariable()));
    _rpcMethods.emplace("setCategoryMetadata",
                        std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetCategoryMetadata()));
    _rpcMethods.emplace("updateCategory", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCUpdateCategory()));
  }

  { // System variables
    _rpcMethods.emplace("addRoleToSystemVariable",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCAddRoleToSystemVariable>()));
    _rpcMethods.emplace("getSystemVariablesInRole",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetSystemVariablesInRole>()));
    _rpcMethods.emplace("removeRoleFromSystemVariable",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCRemoveRoleFromSystemVariable>()));
  }

  { // Roles
    _rpcMethods.emplace("addRoleToVariable",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCAddRoleToVariable>()));
    _rpcMethods.emplace("aggregateRoles",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCAggregateRoles>()));
    _rpcMethods.emplace("createRole",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCCreateRole>()));
    _rpcMethods.emplace("deleteRole",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCDeleteRole>()));
    _rpcMethods.emplace("getRoles",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRoles>()));
    _rpcMethods.emplace("getRolesInDevice",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRolesInDevice>()));
    _rpcMethods.emplace("getRolesInRoom",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRolesInRoom>()));
    _rpcMethods.emplace("getRoleMetadata",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetRoleMetadata>()));
    _rpcMethods.emplace("getVariablesInRole",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetVariablesInRole>()));
    _rpcMethods.emplace("removeRoleFromVariable",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCRemoveRoleFromVariable>()));
    _rpcMethods.emplace("setRoleMetadata",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCSetRoleMetadata>()));
    _rpcMethods.emplace("updateRole",
                        std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCUpdateRole>()));
  }

  { // UI
    _rpcMethods.emplace("addUiElement", std::make_shared<RpcMethods::RpcAddUiElement>());
    _rpcMethods.emplace("checkUiElementSimpleCreation",
                        std::make_shared<RpcMethods::RpcCheckUiElementSimpleCreation>());
    _rpcMethods.emplace("getAllUiElements", std::make_shared<RpcMethods::RpcGetAllUiElements>());
    _rpcMethods.emplace("getAvailableUiElements", std::make_shared<RpcMethods::RpcGetAvailableUiElements>());
    _rpcMethods.emplace("getCategoryUiElements", std::make_shared<RpcMethods::RpcGetCategoryUiElements>());
    _rpcMethods.emplace("getRoomUiElements", std::make_shared<RpcMethods::RpcGetRoomUiElements>());
    _rpcMethods.emplace("getUiElement", std::make_shared<RpcMethods::RpcGetUiElement>());
    _rpcMethods.emplace("getUiElementMetadata", std::make_shared<RpcMethods::RpcGetUiElementMetadata>());
    _rpcMethods.emplace("getUiElementsWithVariable", std::make_shared<RpcMethods::RpcGetUiElementsWithVariable>());
    _rpcMethods.emplace("getUiElementTemplate", std::make_shared<RpcMethods::RpcGetUiElementTemplate>());
    _rpcMethods.emplace("requestUiRefresh", std::make_shared<RpcMethods::RpcRequestUiRefresh>());
    _rpcMethods.emplace("removeUiElement", std::make_shared<RpcMethods::RpcRemoveUiElement>());
    _rpcMethods.emplace("removeNodeUiElements", std::make_shared<RpcMethods::RpcRemoveNodeUiElements>());
    _rpcMethods.emplace("setUiElementMetadata", std::make_shared<RpcMethods::RpcSetUiElementMetadata>());
    _rpcMethods.emplace("uiElementExists", std::make_shared<RpcMethods::RpcUiElementExists>());
  }

  { // UI notification
    _rpcMethods.emplace("createUiNotification", std::make_shared<RpcMethods::RpcCreateUiNotification>());
    _rpcMethods.emplace("getUiNotification", std::make_shared<RpcMethods::RpcGetUiNotification>());
    _rpcMethods.emplace("getUiNotifications", std::make_shared<RpcMethods::RpcGetUiNotifications>());
    _rpcMethods.emplace("removeUiNotification", std::make_shared<RpcMethods::RpcRemoveUiNotification>());
    _rpcMethods.emplace("uiNotificationAction", std::make_shared<RpcMethods::RpcUiNotificationAction>());
  }

  { // Users
    _rpcMethods.emplace("getUserMetadata", std::make_shared<Rpc::RPCGetUserMetadata>());
    _rpcMethods.emplace("setUserMetadata", std::make_shared<Rpc::RPCSetUserMetadata>());
  }

  //{{{ User data
  _rpcMethods.emplace("deleteUserData",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCDeleteUserData>()));
  _rpcMethods.emplace("getUserData",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCGetUserData>()));
  _rpcMethods.emplace("setUserData",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<Rpc::RPCSetUserData>()));
  //}}}

  //{{{ Variable profiles
  _rpcMethods.emplace("activateVariableProfile",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcActivateVariableProfile>()));
  _rpcMethods.emplace("addVariableProfile",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcAddVariableProfile>()));
  _rpcMethods.emplace("deleteVariableProfile",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcDeleteVariableProfile>()));
  _rpcMethods.emplace("getAllVariableProfiles",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcGetAllVariableProfiles>()));
  _rpcMethods.emplace("getVariableProfile",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcGetVariableProfile>()));
  _rpcMethods.emplace("updateVariableProfile",
                      std::static_pointer_cast<BaseLib::Rpc::RpcMethod>(std::make_shared<RpcMethods::RpcUpdateVariableProfile>()));
  //}}}

  { // Maintenance
    _rpcMethods.emplace("enableMaintenanceMode", std::make_shared<RpcMethods::RpcEnableMaintenanceMode>());
    _rpcMethods.emplace("disableMaintenanceMode", std::make_shared<RpcMethods::RpcDisableMaintenanceMode>());
  }

#ifndef NO_SCRIPTENGINE
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("executePhpNode",
                                      std::bind(&NodeBlueServer::executePhpNode,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("executePhpNodeMethod",
                                      std::bind(&NodeBlueServer::executePhpNodeMethod,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
#endif
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("errorEvent",
                                      std::bind(&NodeBlueServer::errorEvent,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("frontendEventLog",
                                      std::bind(&NodeBlueServer::frontendEventLog,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("getCredentials",
                                      std::bind(&NodeBlueServer::getCredentials,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("invokeNodeMethod",
                                      std::bind(&NodeBlueServer::invokeNodeMethod,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("invokeIpcProcessMethod",
                                      std::bind(&NodeBlueServer::invokeIpcProcessMethod,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("nodeEvent",
                                      std::bind(&NodeBlueServer::nodeEvent,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("nodeBlueVariableEvent",
                                      std::bind(&NodeBlueServer::nodeBlueVariableEvent,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("nodeRedNodeInput",
                                      std::bind(&NodeBlueServer::nodeRedNodeInput,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("registerUiNodeRoles",
                                      std::bind(&NodeBlueServer::registerUiNodeRoles,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("setCredentials",
                                      std::bind(&NodeBlueServer::setCredentials,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
  _localRpcMethods.insert(std::pair < std::string,
                          std::function
                              < BaseLib::PVariable(PNodeBlueClientData & clientData, BaseLib::PArray & parameters)
                                  >> ("setCredentialTypes",
                                      std::bind(&NodeBlueServer::setCredentialTypes,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2)));
}

NodeBlueServer::~NodeBlueServer() {
  if (!_stopServer) { stop(); }
  GD::bl->threadManager.join(_maintenanceThread);
}

bool NodeBlueServer::lifetick() {
  try {
    {
      if (!lifetick_1_.second && BaseLib::HelperFunctions::getTime() - lifetick_1_.first > 120000) {
        _out.printCritical("Critical: RPC server's lifetick 1 was not updated for more than 120 seconds.");
        return false;
      }
    }

    {
      if (!lifetick_2_.second && BaseLib::HelperFunctions::getTime() - lifetick_2_.first > 120000) {
        _out.printCritical("Critical: RPC server's lifetick 2 was not updated for more than 120 seconds.");
        return false;
      }
    }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto result = sendRequest(client, "lifetick", std::make_shared<BaseLib::Array>(), true);
      if (result->errorStruct || !result->booleanValue) { return false; }
    }

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

BaseLib::PVariable NodeBlueServer::getLoad() {
  try {
    auto loadStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    auto serverStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    for (int32_t i = 0; i < _queueCount; i++) {
      auto queueStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

      queueStruct->structValue->emplace("Max load", std::make_shared<BaseLib::Variable>(maxThreadLoad(i)));
      queueStruct->structValue->emplace("Max load 1m", std::make_shared<BaseLib::Variable>(maxThreadLoad1m(i)));
      queueStruct->structValue->emplace("Max load 10m", std::make_shared<BaseLib::Variable>(maxThreadLoad10m(i)));
      queueStruct->structValue->emplace("Max load 1h", std::make_shared<BaseLib::Variable>(maxThreadLoad1h(i)));
      queueStruct->structValue->emplace("Max latency", std::make_shared<BaseLib::Variable>(maxWait(i)));
      queueStruct->structValue->emplace("Max latency 1m", std::make_shared<BaseLib::Variable>(maxWait1m(i)));
      queueStruct->structValue->emplace("Max latency 10m", std::make_shared<BaseLib::Variable>(maxWait10m(i)));
      queueStruct->structValue->emplace("Max latency 1h", std::make_shared<BaseLib::Variable>(maxWait1h(i)));

      serverStruct->structValue->emplace("Queue " + std::to_string(i + 1), queueStruct);
    }
    loadStruct->structValue->emplace("Node-BLUE server", serverStruct);

    std::vector<PNodeBlueClientData> clients;

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto result = sendRequest(client, "getLoad", std::make_shared<BaseLib::Array>(), true);
      if (result->errorStruct) { continue; }

      loadStruct->structValue->emplace("Node-BLUE client " + std::to_string(client->pid), result);
    }

    return loadStruct;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::getNodeProcessingTimes() {
  try {
    auto result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    std::vector<PNodeBlueClientData> clients;

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto result2 = sendRequest(client, "getNodeProcessingTimes", std::make_shared<BaseLib::Array>(), true);
      if (result2->errorStruct) { continue; }

      result->structValue->emplace("Node-BLUE client " + std::to_string(client->pid), result2);
    }

    return result;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void NodeBlueServer::collectGarbage() {
  try {
    _lastGarbageCollection = BaseLib::HelperFunctions::getTime();
    std::vector<PNodeBlueProcess> processesToShutdown;
    {
      std::lock_guard<std::mutex> processGuard(_processMutex);
      bool emptyProcess = false;
      for (auto &process : _processes) {
        if (process.second->flowCount() == 0
            && BaseLib::HelperFunctions::getTime() - process.second->lastExecution > 60000
            && process.second->getClientData()
            && !process.second->getClientData()->closed) {
          if (emptyProcess) { closeClientConnection(process.second->getClientData()); }
          else { emptyProcess = true; }
        }
      }
    }

    std::vector<PNodeBlueClientData> clientsToRemove;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      try {
        clientsToRemove.reserve(_clients.size());
        for (auto &client : _clients) {
          if (client.second->closed) { clientsToRemove.push_back(client.second); }
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
    }
    for (auto &i : clientsToRemove) {
      {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        _clients.erase(i->id);
      }
      _out.printDebug("Debug: Flows engine client " + std::to_string(i->id) + " removed.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::getMaxThreadCounts() {
  try {
    _maxThreadCounts.clear();
    auto threadCounts = _nodeManager->getMaxThreadCounts();
    for (auto &entry : threadCounts) {
      _maxThreadCounts[entry.first] = entry.second;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool NodeBlueServer::checkIntegrity(const std::string &flowsFile) {
  try {
    std::string rawFlows = BaseLib::Io::getFileContent(flowsFile);
    if (BaseLib::HelperFunctions::trim(rawFlows).empty()) { return false; }

    _jsonDecoder->decode(rawFlows);
    return true;
  }
  catch (const std::exception &ex) {
    _out.printError(std::string("Integrity check of flows file returned error: ") + ex.what());
  }
  catch (...) {
    _out.printError(std::string("Integrity check of flows file returned unknown error."));
  }
  return false;
}

void NodeBlueServer::backupFlows() {
  try {
    int32_t maxBackups = 20;
    std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
    std::string flowsFile = GD::bl->settings.nodeBlueDataPath() + "flows.json";
    std::string flowsBackupFile = GD::bl->settings.nodeBlueDataPath() + "flows.json.bak";
    if (BaseLib::Io::fileExists(flowsFile)) {
      if (!checkIntegrity(flowsFile)) {
        _out.printCritical("Critical: Integrity check on flows file failed.");
        _out.printCritical("Critical: Backing up corrupted flows file to: " + flowsFile + ".broken");
        GD::bl->io.copyFile(flowsFile, flowsFile + ".broken");
        BaseLib::Io::deleteFile(flowsFile);
        bool restored = false;
        for (int32_t i = 0; i <= 10000; i++) {
          if (BaseLib::Io::fileExists(flowsBackupFile + std::to_string(i))
              && checkIntegrity(flowsBackupFile + std::to_string(i))) {
            _out.printCritical("Critical: Restoring flows file: " + flowsBackupFile + std::to_string(i));
            if (GD::bl->io.copyFile(flowsBackupFile + std::to_string(i), flowsFile)) {
              restored = true;
              break;
            }
          }
        }
        if (!restored) {
          _out.printCritical("Critical: Could not restore flows file.");
          return;
        }
      } else {
        _out.printInfo("Info: Backing up flows file...");
        if (maxBackups > 1) {
          if (BaseLib::Io::fileExists(flowsBackupFile + std::to_string(maxBackups - 1))) {
            if (!BaseLib::Io::deleteFile(flowsBackupFile + std::to_string(maxBackups - 1))) {
              _out.printError("Error: Cannot delete file: " + flowsBackupFile + std::to_string(maxBackups - 1));
            }
          }
          for (int32_t i = maxBackups - 2; i >= 0; i--) {
            if (BaseLib::Io::fileExists(flowsBackupFile + std::to_string(i))) {
              if (!BaseLib::Io::moveFile(flowsBackupFile + std::to_string(i),
                                         flowsBackupFile + std::to_string(i + 1))) {
                _out.printError("Error: Cannot move file: " + flowsBackupFile + std::to_string(i));
              }
            }
          }
        }
        if (maxBackups > 0) {
          if (!GD::bl->io.copyFile(flowsFile, flowsBackupFile + '0')) {
            _out.printError("Error: Cannot copy flows file to: " + flowsBackupFile + '0');
          }
        }
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

bool NodeBlueServer::start() {
  try {
    _flowsRestarting = true;
    stop();

    _processCallbackHandlerId = BaseLib::ProcessManager::registerCallbackHandler(std::function < void(pid_t
    pid, int
    exitCode, int
    signal, bool
    coreDumped)>(std::bind(&NodeBlueServer::processKilled,
                           this,
                           std::placeholders::_1,
                           std::placeholders::_2,
                           std::placeholders::_3,
                           std::placeholders::_4)));
    backupFlows();
    _socketPath = GD::bl->settings.socketPath() + "homegearFE.sock";
    _shuttingDown = false;
    _stopServer = false;
    if (!getFileDescriptor(true)) {
      _flowsRestarting = false;
      return false;
    }
    _webroot = GD::bl->settings.nodeBluePath() + "www/";
    _anonymousPaths = BaseLib::HelperFunctions::splitAll(GD::bl->settings.nodeBlueUriPathsExcludedFromLogin(), ',');
    for (auto &element : _anonymousPaths) {
      BaseLib::HelperFunctions::trim(element);
      if (element.empty()) {
        _anonymousPaths.clear();
        break;
      }
    }
    getMaxThreadCounts();
    uint32_t flowsProcessingThreadCountServer = GD::bl->settings.nodeBlueProcessingThreadCountServer();
    if (flowsProcessingThreadCountServer < 5) { flowsProcessingThreadCountServer = 5; }
    startQueue(0, false, flowsProcessingThreadCountServer, 0, SCHED_OTHER);
    startQueue(1, false, flowsProcessingThreadCountServer, 0, SCHED_OTHER);
    startQueue(2, false, flowsProcessingThreadCountServer, 0, SCHED_OTHER);
    GD::bl->threadManager.start(_mainThread, true, &NodeBlueServer::mainThread, this);
    startFlows();
    _flowsRestarting = false;
    return true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  _flowsRestarting = false;
  return false;
}

void NodeBlueServer::stop() {
  try {
    _shuttingDown = true;
    _stopServer = true;
    GD::bl->threadManager.join(_maintenanceThread);
    GD::bl->threadManager.join(_mainThread); //Prevent new connections
    _out.printDebug("Debug: Waiting for flows engine server's client threads to finish.");
    closeClientConnections();
    stopQueue(0);
    stopQueue(1);
    stopQueue(2);
    unlink(_socketPath.c_str());
    BaseLib::ProcessManager::unregisterCallbackHandler(_processCallbackHandlerId);
    _nodepinkWebsocket->stop();
    _nodepink->stop();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::homegearShuttingDown() {
  try {
    _shuttingDown = true;
    stopNodes();
    sendShutdown();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::homegearReloading() {
  try {
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "reload", parameters, false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::processKilled(pid_t pid, int exitCode, int signal, bool coreDumped) {
  try {
    std::shared_ptr<NodeBlueProcess> process;

    {
      std::lock_guard<std::mutex> processGuard(_processMutex);
      auto processIterator = _processes.find(pid);
      if (processIterator != _processes.end()) {
        process = processIterator->second;
        closeClientConnection(process->getClientData());
        _processes.erase(processIterator);
      }
    }

    if (process) {
      if (signal != -1 && signal != 15) {
        _out.printCritical("Critical: Client process with pid " + std::to_string(pid) + " was killed with signal "
                               + std::to_string(signal) + '.');
        GD::rpcClient->broadcastNodeEvent("global",
                                          "flowStartError",
                                          std::make_shared<BaseLib::Variable>("crash"),
                                          false);
      } else if (exitCode != 0) {
        _out.printError(
            "Error: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode)
                + '.');
      } else {
        _out.printInfo(
            "Info: Client process with pid " + std::to_string(pid) + " exited with code " + std::to_string(exitCode)
                + '.');
      }

      if (signal != -1 && signal != 15) { exitCode = -32500; }

      process->setExited(true);

      process->requestConditionVariable.notify_all();

      process->invokeFlowFinished(exitCode);
      if (signal != -1 && signal != 15 && !_flowsRestarting && !_shuttingDown) {
        _out.printError(
            "Error: Restarting flows, because client was killed unexpectedly. Signal was: " + std::to_string(signal)
                + ", exit code was: " + std::to_string(exitCode));
        restartFlowsAsync();
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

void NodeBlueServer::nodeLog(const std::string &nodeId, uint32_t logLevel, const std::string &message) {
  try {
    PNodeBlueClientData clientData;
    int32_t clientId = 0;
    {
      std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
      auto nodeInfoIterator = _nodeInfoMap.find(nodeId);
      if (nodeInfoIterator == _nodeInfoMap.end()) { return; }
      clientId = nodeInfoIterator->second.clientId;
    }
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) { return; }
      clientData = clientIterator->second;
    }

    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(3);
    parameters->push_back(std::make_shared<BaseLib::Variable>(nodeId));
    parameters->push_back(std::make_shared<BaseLib::Variable>(logLevel));
    parameters->push_back(std::make_shared<BaseLib::Variable>(message));
    sendRequest(clientData, "nodeLog", parameters, false);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::nodeOutput(const std::string &nodeId,
                                uint32_t index,
                                const BaseLib::PVariable &message,
                                bool synchronous) {
  try {
    PNodeBlueClientData clientData;
    int32_t clientId = 0;
    {
      std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
      auto nodeInfoIterator = _nodeInfoMap.find(nodeId);
      if (nodeInfoIterator == _nodeInfoMap.end()) { return; }
      clientId = nodeInfoIterator->second.clientId;
    }
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) { return; }
      clientData = clientIterator->second;
    }

    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(4);
    parameters->push_back(std::make_shared<BaseLib::Variable>(nodeId));
    parameters->push_back(std::make_shared<BaseLib::Variable>(index));
    parameters->push_back(message);
    parameters->push_back(std::make_shared<BaseLib::Variable>(synchronous));
    sendRequest(clientData, "nodeOutput", parameters, false);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

BaseLib::PVariable NodeBlueServer::getNodesWithFixedInputs() {
  try {
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    auto nodeStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    for (auto &client : clients) {
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      auto result = sendRequest(client, "getNodesWithFixedInputs", parameters, true);
      if (result->errorStruct) { continue; }
      nodeStruct->structValue->insert(result->structValue->begin(), result->structValue->end());
    }
    return nodeStruct;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::getNodeCredentials(const std::string &nodeId) {
  return _nodeBlueCredentials->getCredentials(nodeId, true);
}

BaseLib::PVariable NodeBlueServer::setNodeCredentials(const std::string &nodeId,
                                                      const BaseLib::PVariable &credentials) {
  return _nodeBlueCredentials->setCredentials(nodeId, credentials);
}

BaseLib::PVariable NodeBlueServer::getNodeCredentialTypes(const std::string &nodeId) {
  std::string type;
  {
    std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMapMutex);
    auto nodeInfoIterator = _nodeInfoMap.find(nodeId);
    if (nodeInfoIterator == _nodeInfoMap.end()) { return std::make_shared<BaseLib::Variable>(); }
    type = nodeInfoIterator->second.type;
  }
  return _nodeManager->getNodeCredentialTypes(type);
}

void NodeBlueServer::setNodeCredentialTypes(const std::string &type,
                                            const BaseLib::PVariable &credentialTypes,
                                            bool fromIpcServer) {
  _nodeManager->registerCredentialTypes(type, credentialTypes, fromIpcServer);
}

BaseLib::PVariable NodeBlueServer::getNodeVariable(const std::string &nodeId, const std::string &topic) {
  try {
    PNodeBlueClientData clientData;
    int32_t clientId = 0;
    bool isFlow = false;
    {
      std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMapMutex);
      auto nodeInfoIterator = _nodeInfoMap.find(nodeId);
      if (nodeInfoIterator == _nodeInfoMap.end()) {
        std::lock_guard<std::mutex> flowClientIdMapGuard(_flowClientIdMapMutex);
        auto flowClientIdIterator = _flowClientIdMap.find(nodeId);
        if (flowClientIdIterator == _flowClientIdMap.end()) { return std::make_shared<BaseLib::Variable>(); }
        isFlow = true;
        clientId = flowClientIdIterator->second;
      } else {
        clientId = nodeInfoIterator->second.clientId;
      }
    }
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) { return std::make_shared<BaseLib::Variable>(); }
      clientData = clientIterator->second;
    }

    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(2);
    parameters->push_back(std::make_shared<BaseLib::Variable>(nodeId));
    parameters->push_back(std::make_shared<BaseLib::Variable>(topic));
    return sendRequest(clientData, isFlow ? "getFlowVariable" : "getNodeVariable", parameters, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void NodeBlueServer::setNodeVariable(const std::string &nodeId,
                                     const std::string &topic,
                                     const BaseLib::PVariable &value) {
  try {
    PNodeBlueClientData clientData;
    int32_t clientId = 0;
    bool isFlow = false;
    {
      std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
      auto nodeInfoIterator = _nodeInfoMap.find(nodeId);
      if (nodeInfoIterator == _nodeInfoMap.end()) {
        std::lock_guard<std::mutex> flowClientIdMapGuard(_flowClientIdMapMutex);
        auto flowClientIdIterator = _flowClientIdMap.find(nodeId);
        if (flowClientIdIterator == _flowClientIdMap.end()) { return; }
        isFlow = true;
        clientId = flowClientIdIterator->second;
      } else {
        clientId = nodeInfoIterator->second.clientId;
      }
    }
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) { return; }
      clientData = clientIterator->second;
    }

    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(3);
    parameters->push_back(std::make_shared<BaseLib::Variable>(nodeId));
    parameters->push_back(std::make_shared<BaseLib::Variable>(topic));
    parameters->push_back(value);
    sendRequest(clientData, isFlow ? "setFlowVariable" : "setNodeVariable", parameters, false);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::enableNodeEvents() {
  try {
    if (_shuttingDown) { return; }
    _out.printInfo("Info: Enabling node events...");
    _nodeEventsEnabled = true;
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "enableNodeEvents", parameters, false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::disableNodeEvents() {
  try {
    if (_shuttingDown) { return; }
    _out.printInfo("Info: Disabling node events...");
    _nodeEventsEnabled = false;
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "disableNodeEvents", parameters, false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::parseEnvironmentVariables(const BaseLib::PVariable &env, BaseLib::PVariable &output) {
  try {
    for (auto &entry : *env->arrayValue) {
      auto nameIterator = entry->structValue->find("name");
      auto valueIterator = entry->structValue->find("value");
      auto typeIterator = entry->structValue->find("type");
      if (nameIterator != entry->structValue->end() && valueIterator != entry->structValue->end()
          && typeIterator != entry->structValue->end()) {
        if (typeIterator->second->stringValue == "bool") {
          valueIterator->second->type = BaseLib::VariableType::tBoolean;
          valueIterator->second->booleanValue = (valueIterator->second->stringValue == "true");
        } else if (typeIterator->second->stringValue == "float" || typeIterator->second->stringValue == "num") {
          valueIterator->second->type = BaseLib::VariableType::tFloat;
          valueIterator->second->floatValue = BaseLib::Math::getDouble(valueIterator->second->stringValue);
        } else if (typeIterator->second->stringValue == "int") {
          valueIterator->second->type = BaseLib::VariableType::tInteger64;
          valueIterator->second->integerValue64 = BaseLib::Math::getNumber64(valueIterator->second->stringValue);
        } else if (typeIterator->second->stringValue == "array") {
          try {
            valueIterator->second = BaseLib::Rpc::JsonDecoder::decode(valueIterator->second->stringValue);
          }
          catch (const std::exception &) {
          }
          valueIterator->second->type = BaseLib::VariableType::tArray;
        } else if (typeIterator->second->stringValue == "struct") {
          try {
            valueIterator->second = BaseLib::Rpc::JsonDecoder::decode(valueIterator->second->stringValue);
          }
          catch (const std::exception &) {
          }
          valueIterator->second->type = BaseLib::VariableType::tStruct;
        }
        output->structValue->emplace(nameIterator->second->stringValue, valueIterator->second);
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

std::set<std::string> NodeBlueServer::insertSubflows(BaseLib::PVariable &subflowNode,
                                                     std::unordered_map<std::string, BaseLib::PVariable> &subflowInfos,
                                                     std::unordered_map<std::string, BaseLib::PVariable> &flowNodes,
                                                     std::unordered_map<std::string, BaseLib::PVariable> &subflowNodes,
                                                     std::set<std::string> &allNodeIds) {
  try {
    std::set<std::string> subsubflows;
    std::vector<std::pair<std::string, BaseLib::PVariable>> nodesToAdd;

    std::string thisSubflowId = subflowNode->structValue->at("id")->stringValue;
    std::string subflowIdPrefix = thisSubflowId + ':';
    std::string subflowId = subflowNode->structValue->at("type")->stringValue.substr(8);

    auto subflowInfoIterator = subflowInfos.find(subflowId);
    if (subflowInfoIterator == subflowInfos.end()) {
      _out.printError("Error: Could not find subflow info with for subflow with id " + subflowId);
      return {};
    }

    //{{{ Get environment variables
    BaseLib::PVariable environmentVariables = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    auto environmentVariablesIterator = subflowNode->structValue->find("env");
    auto defaultEnvironmentVariables = subflowInfoIterator->second->structValue->find("env");

    if (environmentVariablesIterator != subflowNode->structValue->end()) {
      parseEnvironmentVariables(environmentVariablesIterator->second, environmentVariables);
    }

    if (defaultEnvironmentVariables != subflowInfoIterator->second->structValue->end()) {
      parseEnvironmentVariables(defaultEnvironmentVariables->second, environmentVariables);
    }
    //}}}

    //Copy subflow, prefix all subflow node IDs and wires with subflow ID and identify subsubflows
    std::unordered_map<std::string, BaseLib::PVariable> subflow;
    nodesToAdd.reserve(subflowNodes.size());
    for (auto &subflowElement : subflowNodes) {
      BaseLib::PVariable subflowNode2 = std::make_shared<BaseLib::Variable>();
      *subflowNode2 = *subflowElement.second;
      subflowNode2->structValue->at("id")->stringValue =
          subflowIdPrefix + subflowNode2->structValue->at("id")->stringValue;
      subflowNode2->structValue->emplace("env", environmentVariables);
      allNodeIds.emplace(subflowNode2->structValue->at("id")->stringValue);

      auto &subflowNodeType = subflowNode2->structValue->at("type")->stringValue;
      if (subflowNodeType.compare(0, 8, "subflow:") == 0) {
        subsubflows.emplace(subflowNode2->structValue->at("id")->stringValue);
      }

      auto subflowNodeWiresIterator = subflowNode2->structValue->find("wires");
      if (subflowNodeWiresIterator == subflowNode2->structValue->end()) { continue; }
      for (auto &output : *subflowNodeWiresIterator->second->arrayValue) {
        for (auto &wire : *output->arrayValue) {
          auto idIterator = wire->structValue->find("id");
          if (idIterator == wire->structValue->end()) { continue; }
          idIterator->second->stringValue = subflowIdPrefix + idIterator->second->stringValue;
        }
      }

      subflow.emplace(subflowElement.first, subflowNode2);
      nodesToAdd.emplace_back(subflowNode2->structValue->at("id")->stringValue, subflowNode2);
    }

    //Find output node and redirect it to the subflow output
    std::multimap<uint32_t, std::pair<uint32_t, std::string>> passthroughWires;
    auto wiresOutIterator = subflowInfoIterator->second->structValue->find("out");
    if (wiresOutIterator != subflowInfoIterator->second->structValue->end()) {
      for (uint32_t outputIndex = 0; outputIndex < wiresOutIterator->second->arrayValue->size(); ++outputIndex) {
        auto &element = wiresOutIterator->second->arrayValue->at(outputIndex);
        auto innerIterator = element->structValue->find("wires");
        if (innerIterator == element->structValue->end()) { continue; }
        for (auto &wireIterator : *innerIterator->second->arrayValue) {
          auto wireIdIterator = wireIterator->structValue->find("id");
          if (wireIdIterator == wireIterator->structValue->end()) { continue; }

          auto wirePortIterator = wireIterator->structValue->find("port");
          if (wirePortIterator == wireIterator->structValue->end()) { continue; }

          if (wirePortIterator->second->type == BaseLib::VariableType::tString) {
            wirePortIterator->second->integerValue = BaseLib::Math::getNumber(wirePortIterator->second->stringValue);
          }

          auto targetNodeWiresIterator = subflowNode->structValue->find("wires");
          if (targetNodeWiresIterator == subflowNode->structValue->end()) { continue; }
          if (outputIndex >= targetNodeWiresIterator->second->arrayValue->size()) { continue; }

          auto &targetNodeWireOutput = targetNodeWiresIterator->second->arrayValue->at(outputIndex);

          if (wireIdIterator->second->stringValue == subflowId) {
            for (const auto &targetNodeWire : *targetNodeWireOutput->arrayValue) {
              auto targetNodeWireIdIterator = targetNodeWire->structValue->find("id");
              if (targetNodeWireIdIterator == targetNodeWire->structValue->end()) { continue; }

              uint32_t port = 0;
              auto targetNodeWirePortIterator = targetNodeWire->structValue->find("port");
              if (targetNodeWirePortIterator != targetNodeWire->structValue->end()) {
                if (targetNodeWirePortIterator->second->type == BaseLib::VariableType::tString) {
                  port = BaseLib::Math::getNumber(targetNodeWirePortIterator->second->stringValue);
                } else { port = targetNodeWirePortIterator->second->integerValue; }
              }

              passthroughWires.emplace(wirePortIterator->second->integerValue,
                                       std::make_pair(port, targetNodeWireIdIterator->second->stringValue));
            }
            continue;
          }

          auto sourceNodeIterator = subflow.find(wireIdIterator->second->stringValue);
          if (sourceNodeIterator == subflow.end()) { continue; }

          auto sourceNodeWiresIterator = sourceNodeIterator->second->structValue->find("wires");
          if (sourceNodeWiresIterator == sourceNodeIterator->second->structValue->end()) { continue; }
          while ((signed) sourceNodeWiresIterator->second->arrayValue->size()
              < wirePortIterator->second->integerValue + 1) {
            sourceNodeWiresIterator->second->arrayValue->push_back(std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray));
          }

          *sourceNodeWiresIterator->second->arrayValue->at(wirePortIterator->second->integerValue) =
              *targetNodeWiresIterator->second->arrayValue->at(outputIndex);
        }
      }
    }

    //Redirect all subflow inputs to subflow nodes
    auto wiresInIterator = subflowInfoIterator->second->structValue->find("in");
    if (wiresInIterator != subflowInfoIterator->second->structValue->end()) {
      std::vector<BaseLib::PArray> wiresIn;
      wiresIn.reserve(wiresInIterator->second->arrayValue->size());
      for (uint32_t i = 0; i < wiresInIterator->second->arrayValue->size(); i++) {
        wiresIn.push_back(std::make_shared<BaseLib::Array>());
        auto wiresInWiresIterator = wiresInIterator->second->arrayValue->at(i)->structValue->find("wires");
        if (wiresInWiresIterator != wiresInIterator->second->arrayValue->at(i)->structValue->end()) {
          wiresIn.back()->reserve(wiresInIterator->second->arrayValue->at(i)->structValue->size());
          for (auto &wiresInWire : *wiresInWiresIterator->second->arrayValue) {
            auto wiresInWireIdIterator = wiresInWire->structValue->find("id");
            if (wiresInWireIdIterator == wiresInWire->structValue->end()) { continue; }

            uint32_t port = 0;
            auto wiresInWirePortIterator = wiresInWire->structValue->find("port");
            if (wiresInWirePortIterator != wiresInWire->structValue->end()) {
              if (wiresInWirePortIterator->second->type == BaseLib::VariableType::tString) {
                port = BaseLib::Math::getNumber(wiresInWirePortIterator->second->stringValue);
              } else { port = wiresInWirePortIterator->second->integerValue; }
            }

            BaseLib::PVariable entry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            entry->structValue->emplace("id",
                                        std::make_shared<BaseLib::Variable>(
                                            subflowIdPrefix + wiresInWireIdIterator->second->stringValue));
            entry->structValue->emplace("port", std::make_shared<BaseLib::Variable>(port));
            wiresIn.back()->push_back(entry);
          }
        }
      }

      for (auto &node2 : flowNodes) {
        auto node2WiresIterator = node2.second->structValue->find("wires");
        if (node2WiresIterator == node2.second->structValue->end()) { continue; }
        for (auto &node2WiresOutput : *node2WiresIterator->second->arrayValue) {
          bool rebuildArray = false;
          for (auto &node2Wire : *node2WiresOutput->arrayValue) {
            auto node2WireIdIterator = node2Wire->structValue->find("id");
            if (node2WireIdIterator == node2Wire->structValue->end()) { continue; }

            if (node2WireIdIterator->second->stringValue == thisSubflowId) {
              rebuildArray = true;
              break;
            }
          }
          if (rebuildArray) {
            BaseLib::PArray wireArray = std::make_shared<BaseLib::Array>();
            for (auto &node2Wire : *node2WiresOutput->arrayValue) {
              auto node2WireIdIterator = node2Wire->structValue->find("id");
              if (node2WireIdIterator == node2Wire->structValue->end()) { continue; }

              uint32_t port = 0;
              auto node2WirePortIterator = node2Wire->structValue->find("port");
              if (node2WirePortIterator != node2Wire->structValue->end()) {
                if (node2WirePortIterator->second->type == BaseLib::VariableType::tString) {
                  port = BaseLib::Math::getNumber(node2WirePortIterator->second->stringValue);
                } else { port = node2WirePortIterator->second->integerValue; }
              }

              if (node2WireIdIterator->second->stringValue != thisSubflowId) { wireArray->push_back(node2Wire); }
              else {
                wireArray->insert(wireArray->end(), wiresIn.at(port)->begin(), wiresIn.at(port)->end());

                auto passthroughWiresIterators = passthroughWires.equal_range(port);
                for (auto passthroughWiresIterator = passthroughWiresIterators.first;
                     passthroughWiresIterator != passthroughWiresIterators.second; passthroughWiresIterator++) {
                  BaseLib::PVariable entry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                  entry->structValue->emplace("id",
                                              std::make_shared<BaseLib::Variable>(passthroughWiresIterator->second.second));
                  entry->structValue->emplace("port",
                                              std::make_shared<BaseLib::Variable>(passthroughWiresIterator->second.first));
                  wireArray->push_back(entry);
                }
              }
            }
            node2WiresOutput->arrayValue = wireArray;
          }
        }
      }
    }
    flowNodes.insert(nodesToAdd.begin(), nodesToAdd.end());
    return subsubflows;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return {};
}

void NodeBlueServer::startFlows() {
  try {
    if (_stopServer) { return; }

    BaseLib::PVariable flows;

    {
      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      std::string flowsFile = GD::bl->settings.nodeBlueDataPath() + "flows.json";
      if (!BaseLib::Io::fileExists(flowsFile)) { return; }
      auto rawFlows = BaseLib::Io::getFileContent(flowsFile);
      if (BaseLib::HelperFunctions::trim(rawFlows).empty()) { return; }
      flows = BaseLib::Rpc::JsonDecoder::decode(rawFlows);
    }

    //{{{ Filter all nodes and assign it to flows
    std::unordered_map<std::string, BaseLib::PVariable> subflowInfos;
    std::unordered_map<std::string, std::unordered_map<std::string, BaseLib::PVariable>> flowNodes;
    std::unordered_map<std::string, std::set<std::string>> subflowNodeIds;
    std::set<std::string> allNodeIds;
    std::set<std::string> disabledFlows;
    std::unordered_map<std::string, BaseLib::PVariable> flowsEnvironmentVariables;
    for (auto &element : *flows->arrayValue) {
      auto idIterator = element->structValue->find("id");
      if (idIterator == element->structValue->end()) {
        _out.printError("Error: Flow element has no id.");
        continue;
      }

      auto typeIterator = element->structValue->find("type");
      if (typeIterator == element->structValue->end()) { continue; }
      else if (typeIterator->second->stringValue == "comment") { continue; }
      else if (typeIterator->second->stringValue == "group") { continue; }
      else if (typeIterator->second->stringValue == "subflow") {
        subflowInfos.emplace(idIterator->second->stringValue, element);
        continue;
      } else if (typeIterator->second->stringValue == "tab") {
        auto disabledIterator = element->structValue->find("disabled");
        if (disabledIterator != element->structValue->end() && disabledIterator->second->booleanValue) {
          disabledFlows.emplace(idIterator->second->stringValue);
        } else {
          auto envIterator = element->structValue->find("env");
          if (envIterator != element->structValue->end()) {
            auto environmentVariables = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            parseEnvironmentVariables(envIterator->second, environmentVariables);
            flowsEnvironmentVariables.emplace(idIterator->second->stringValue, environmentVariables);
          }
        }
        continue;
      }

      auto dIterator = element->structValue->find("d");
      if (dIterator != element->structValue->end()
          && (dIterator->second->booleanValue || dIterator->second->stringValue == "true")) {
        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Debug: Ignoring disabled node: " + idIterator->second->stringValue);
        }

        if (_nodeManager->isUiNode(typeIterator->second->stringValue)) {
          _out.printInfo("Info: Deleting UI element of disabled node " + idIterator->second->stringValue);
          auto uiElementId = GD::bl->db->getNodeData(idIterator->second->stringValue, "uiElementId");
          if (!uiElementId->errorStruct && uiElementId->integerValue64 != 0) {
            GD::uiController->removeUiElement((uint64_t) uiElementId->integerValue64);
          }
        }

        continue;
      }

      std::string z;
      auto zIterator = element->structValue->find("z");
      if (zIterator != element->structValue->end()) { z = zIterator->second->stringValue; }
      if (z.empty()) {
        z = "g";
        if (zIterator != element->structValue->end()) { zIterator->second->stringValue = "g"; }
        else { element->structValue->emplace("z", std::make_shared<BaseLib::Variable>("g")); }
      }
      element->structValue->emplace("flow", std::make_shared<BaseLib::Variable>(z));

      if (allNodeIds.find(idIterator->second->stringValue) != allNodeIds.end()) {
        _out.printError("Error: Flow element is defined twice: " + idIterator->second->stringValue
                            + ". At least one of the flows is not working correctly.");
        continue;
      }

      allNodeIds.emplace(idIterator->second->stringValue);
      flowNodes[z].emplace(idIterator->second->stringValue, element);
      if (typeIterator->second->stringValue.compare(0, 8, "subflow:") == 0) {
        subflowNodeIds[z].emplace(idIterator->second->stringValue);
      }

      //Special case which can't be handled in NodeManager. The javascript node should not require Node-RED when installed but only when used. All other Node-RED nodes require Node-RED when installed, even when not used.
      if (!_nodeManager->nodeRedRequired() && typeIterator->second->stringValue == "javascript") {
        _nodeManager->setNodeRedRequired();
        _out.printInfo("Info: Enabling Node-RED because at least one javascript node is used.");
      }
    }
    //}}}

    //{{{ Insert subflows
    for (auto &subflowNodeIdsInner : subflowNodeIds) {
      if (subflowInfos.find(subflowNodeIdsInner.first) != subflowInfos.end()) {
        continue;
      } //Don't process nodes of subflows

      for (const auto &subflowNodeId : subflowNodeIdsInner.second) {
        std::set<std::string> processedSubflows;

        std::set<std::string> subsubflows;
        subsubflows.emplace(subflowNodeId);
        while (!subsubflows.empty()) {
          std::set<std::string> newSubsubflows;
          for (auto &subsubflowNodeId : subsubflows) {
            auto &innerFlowNodes = flowNodes.at(subflowNodeIdsInner.first);
            auto subflowNodeIterator = innerFlowNodes.find(subsubflowNodeId);
            if (subflowNodeIterator == innerFlowNodes.end()) {
              _out.printError("Error: Could not find subflow node ID \"" + subsubflowNodeId + "\" in flow with ID \""
                                  + subflowNodeIdsInner.first + "\".");
              continue;
            }
            std::string subflowId = subflowNodeIterator->second->structValue->at("type")->stringValue.substr(8);
            auto subflowNodesIterator = flowNodes.find(subflowId);
            if (subflowNodesIterator == flowNodes.end()) {
              _out.printError("Error: Could not find subflow with ID \"" + subflowId + "\" in flows.");
              continue;
            }
            std::set<std::string> returnedSubsubflows = insertSubflows(subflowNodeIterator->second,
                                                                       subflowInfos,
                                                                       innerFlowNodes,
                                                                       subflowNodesIterator->second,
                                                                       allNodeIds);
            processedSubflows.emplace(subsubflowNodeId);
            bool abort = false;
            for (auto &returnedSubsubflow : returnedSubsubflows) {
              if (processedSubflows.find(returnedSubsubflow) != processedSubflows.end()) {
                _out.printError("Error: Subflow recursion detected. Aborting processing subflows for flow with ID: "
                                    + subflowNodeIdsInner.first);
                abort = true;
                break;
              }
              newSubsubflows.emplace(returnedSubsubflow);
            }
            if (abort) {
              subsubflows.clear();
              break;
            }
          }
          subsubflows = newSubsubflows;
        }
      }
    }
    //}}}

    BaseLib::PVariable nodeRedFlow = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    std::string nodeRedFlowsFile = _bl->settings.nodeBlueDataPath() + "node-red/flows.json";
    bool hasNoderedFlowsFile = BaseLib::Io::fileExists(nodeRedFlowsFile);
    //Only reserve memory when there are any Node-RED nodes.
    if (!hasNoderedFlowsFile && _nodeManager->nodeRedRequired()) { nodeRedFlow->arrayValue->reserve(flowNodes.size()); }

    for (auto &element : flowNodes) {
      if (subflowInfos.find(element.first) != subflowInfos.end()) { continue; }
      BaseLib::PVariable nodeRedFlowNodes = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      if (!hasNoderedFlowsFile) { nodeRedFlowNodes->arrayValue->reserve(element.second.size()); }
      BaseLib::PVariable flow = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      flow->arrayValue->reserve(element.second.size());
      uint32_t maxThreadCount = 0;
      BaseLib::PVariable environmentVariables;

      {
        auto flowsEnvironmentVariablesIterator = flowsEnvironmentVariables.find(element.first);
        if (flowsEnvironmentVariablesIterator != flowsEnvironmentVariables.end()) {
          environmentVariables = flowsEnvironmentVariablesIterator->second;
        }
      }
      for (auto &node : element.second) {
        if (!node.second) { continue; }
        flow->arrayValue->push_back(node.second);

        //{{{ Insert environment variables
        if (environmentVariables) {
          auto envIterator = node.second->structValue->find("env");
          if (envIterator != node.second->structValue->end()) {
            envIterator->second->structValue->insert(environmentVariables->structValue->begin(),
                                                     environmentVariables->structValue->end());
          } else {
            node.second->structValue->emplace("env", environmentVariables);
          }
        }
        //}}}

        auto typeIterator = node.second->structValue->find("type");
        if (typeIterator != node.second->structValue->end()) {
          if (typeIterator->second->stringValue.compare(0, 8, "subflow:") == 0) { continue; }
          auto threadCountIterator = _maxThreadCounts.find(typeIterator->second->stringValue);
          maxThreadCount += (threadCountIterator == _maxThreadCounts.end() ? 0 : threadCountIterator->second);
          //{{{ Filter Node-RED nodes
          if (!hasNoderedFlowsFile && _nodeManager->isNodeRedNode(typeIterator->second->stringValue)) {
            auto nodeRedNode = std::make_shared<BaseLib::Variable>();
            *nodeRedNode = *node.second;
            //Remove previously added "z" with a value of "g". This is not supported by Node-RED and leads to errors.
            if (nodeRedNode->structValue->at("z")->stringValue == "g") { nodeRedNode->structValue->erase("z"); }
            auto wiresIterator = nodeRedNode->structValue->find("wires");
            if (wiresIterator != nodeRedNode->structValue->end()) { wiresIterator->second->arrayValue->clear(); }
            nodeRedFlowNodes->arrayValue->emplace_back(nodeRedNode);
          }
          //}}}
        } else { _out.printError("Error: Could not determine maximum thread count of node. No key \"type\"."); }
      }

      if (disabledFlows.find(element.first) == disabledFlows.end()) {
        PFlowInfoServer flowInfo = std::make_shared<FlowInfoServer>();
        flowInfo->nodeBlueId = element.first;
        flowInfo->maxThreadCount = maxThreadCount;
        flowInfo->flow = flow;
        startFlow(flowInfo, flowNodes[element.first]);

        //Add tab to Node-RED flow
        if (!nodeRedFlowNodes->arrayValue->empty() && element.first != "g") {
          auto tabStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
          tabStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(element.first));
          tabStruct->structValue->emplace("type", std::make_shared<BaseLib::Variable>("tab"));
          tabStruct->structValue->emplace("label", std::make_shared<BaseLib::Variable>("Flow"));
          nodeRedFlowNodes->arrayValue->emplace_back(tabStruct);
        }
        //Insert nodes into Node-RED flows
        nodeRedFlow->arrayValue->insert(nodeRedFlow->arrayValue->end(),
                                        nodeRedFlowNodes->arrayValue->begin(),
                                        nodeRedFlowNodes->arrayValue->end());
      }
    }

    //{{{ Start Node-RED
    if (!nodeRedFlow->arrayValue->empty() && !hasNoderedFlowsFile) {
      std::vector<char> flowsString;
      _jsonEncoder->encode(nodeRedFlow, flowsString);

      {
        std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
        BaseLib::Io::writeFile(nodeRedFlowsFile, flowsString, flowsString.size());
      }
    }
    if (_nodeManager->nodeRedRequired()) {
      if (_nodepink->isStarted()) {
        _nodepink->reload();
      } else {
        _nodepinkWebsocket->start();
        _nodepink->start();
        if (_nodepink->startUpError()) {
          GD::rpcClient->broadcastNodeEvent("global",
                                            "nodePinkStartError",
                                            std::make_shared<BaseLib::Variable>(),
                                            false);
        }
      }
    }
    //}}}

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    auto startTime = BaseLib::HelperFunctions::getTime();
    _out.printInfo("Info: Starting nodes.");
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "startNodes", parameters, true);
    }
    auto duration = BaseLib::HelperFunctions::getTime() - startTime;
    _out.printInfo("Info: Calling start took " + std::to_string(duration) + "ms.");

    startTime = BaseLib::HelperFunctions::getTime();
    _out.printInfo("Info: Calling \"configNodesStarted\".");
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "configNodesStarted", parameters, true);
    }
    duration = BaseLib::HelperFunctions::getTime() - startTime;
    _out.printInfo("Info: Calling configNodesStarted took " + std::to_string(duration) + "ms.");

    startTime = BaseLib::HelperFunctions::getTime();
    _out.printInfo("Info: Calling \"startUpComplete\".");
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "startUpComplete", parameters, true);
    }
    duration = BaseLib::HelperFunctions::getTime() - startTime;
    _out.printInfo("Info: Calling startUpComplete took " + std::to_string(duration) + "ms.");

    { //Remove node data of non-existent nodes
      std::set<std::string> nodeData = GD::bl->db->getAllNodeDataNodes();
      std::string dataKey;
      for (auto &nodeId : nodeData) {
        if (allNodeIds.find(nodeId) == allNodeIds.end() && flowNodes.find(nodeId) == flowNodes.end()
            && nodeId != "global") {
          GD::bl->db->deleteNodeData(nodeId, dataKey);
        }
      }
    }

    { //Remove UI elements of non-existent nodes
      auto nodeUiElements = GD::uiController->getAllNodeUiElements();
      for (auto &nodeId : nodeUiElements) {
        if (allNodeIds.find(nodeId.first) == allNodeIds.end() && flowNodes.find(nodeId.first) == flowNodes.end()) {
          GD::uiController->removeNodeUiElements(nodeId.first);
        }
      }
    }

    { //Remove cloud devices of non-existent nodes
      std::string component = "homegear-cloudconnect-devices";
      std::string key;
      auto cloud_devices = GD::bl->db->getData(component, key);
      for (auto &device : *cloud_devices->structValue) {
        auto node_id = BaseLib::HelperFunctions::splitFirst(device.first, '_').second;
        if (!node_id.empty() && allNodeIds.find(node_id) == allNodeIds.end()
            && flowNodes.find(node_id) == flowNodes.end()) {
          key = device.first;
          GD::bl->db->deleteData(component, key);
        }
      }
    }

    GD::rpcClient->broadcastNodeEvent("global", "flowsStarted", std::make_shared<BaseLib::Variable>(true), false);
    frontendNodeEventLog("Flows have been (re)started successfully.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  malloc_trim(0); //This method uses a lot of memory - clean it up (it is not necessarily cleaned up automatically)
}

void NodeBlueServer::sendShutdown() {
  try {
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }
    for (auto &client : clients) {
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      sendRequest(client, "shutdown", parameters, false);
    }

    int32_t i = 0;
    while (!_clients.empty() && i < 300) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      collectGarbage();
      i++;
    }

    std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
    _nodeInfoMap.clear();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool NodeBlueServer::sendReset() {
  try {
    _out.printInfo("Info: Resetting flows client process.");
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }
    for (auto &client : clients) {
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      auto result = sendRequest(client, "reset", parameters, true);
      if (result->errorStruct) {
        _out.printError("Error executing reset: " + result->structValue->at("faultString")->stringValue);
        return false;
      } else {
        std::lock_guard<std::mutex> processGuard(_processMutex);
        auto processIterator = _processes.find(client->pid);
        if (processIterator != _processes.end()) { processIterator->second->reset(); }
      }
    }

    std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
    _nodeInfoMap.clear();

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

void NodeBlueServer::closeClientConnections() {
  try {
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        clients.push_back(client.second);
        closeClientConnection(client.second);
        std::lock_guard<std::mutex> processGuard(_processMutex);
        auto processIterator = _processes.find(client.second->pid);
        if (processIterator != _processes.end()) {
          processIterator->second->invokeFlowFinished(-32501);
          _processes.erase(processIterator);
        }
      }
    }

    while (!_clients.empty()) {
      for (auto &client : clients) {
        std::unique_lock<std::mutex> waitLock(client->waitMutex);
        waitLock.unlock();
        client->requestConditionVariable.notify_all();
      }
      collectGarbage();
      if (!_clients.empty()) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
    }

    std::lock_guard<std::mutex> processGuard(_processMutex);
    if (!_processes.empty()) {
      _out.printWarning("Warning: There are still entries in the processes array. Clearing them.");
    }
    _processes.clear();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::stopNodes() {
  try {
    _out.printInfo("Info: Stopping nodes.");
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    int64_t startTime = BaseLib::HelperFunctions::getTime();
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "stopNodes", parameters, true);
    }
    auto duration = BaseLib::HelperFunctions::getTime() - startTime;
    _out.printInfo("Info: Calling stop took " + std::to_string(duration) + "ms.");

    startTime = BaseLib::HelperFunctions::getTime();
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      sendRequest(client, "waitForNodesStopped", parameters, true);
    }
    duration = BaseLib::HelperFunctions::getTime() - startTime;
    _out.printInfo("Info: Calling waitForStop took " + std::to_string(duration) + "ms.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void NodeBlueServer::restartFlows() {
  try {
    std::lock_guard<std::mutex> restartFlowsGuard(_restartFlowsMutex);
    if (_stopServer) { return; }
    _flowsRestarting = true;
    stopNodes();
    bool result = sendReset();
    if (!result) {
      _out.printInfo("Info: Stopping Flows...");
      sendShutdown();
      _out.printInfo("Info: Closing connections to Flows clients...");
      closeClientConnections();
    }
    _out.printInfo("Info: Reloading node information...");
    _nodeManager->fillManagerModuleInfo();
    getMaxThreadCounts();
    _out.printInfo("Info: Starting Flows...");
    startFlows();

    std::string source = "nodeblue";
    std::shared_ptr<std::vector<std::string>> variables(new std::vector<std::string>{"FLOWS_RESTARTED"});
    BaseLib::PArray values(new BaseLib::Array{std::make_shared<BaseLib::Variable>(true)});
    GD::familyController->onEvent(source, 0, -1, variables, values);
    GD::familyController->onRPCEvent(source, 0, -1, "CENTRAL", variables, values);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  _flowsRestarting = false;
}

void NodeBlueServer::stopFlows() {
  try {
    std::lock_guard<std::mutex> restartFlowsGuard(_restartFlowsMutex);
    stopNodes();
    bool result = sendReset();
    if (!result) {
      _out.printInfo("Info: Stopping Flows...");
      sendShutdown();
      _out.printInfo("Info: Closing connections to Flows clients...");
      closeClientConnections();
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  _flowsRestarting = false;
}

bool NodeBlueServer::restartFlowsAsync() {
  try {
    if (_flowsRestarting) { return false; }
    _flowsRestarting = true; //There is a race condition here - but it doesn't matter.
    _out.printInfo("Info: Restarting flows...");
    GD::bl->threadManager.start(_maintenanceThread, true, &NodeBlueServer::restartFlows, this);
    return true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

std::string NodeBlueServer::handleGet(std::string &path,
                                      BaseLib::Http &http,
                                      std::string &responseEncoding,
                                      std::string &responseHeader) {
#ifdef NO_SCRIPTENGINE
  return "unauthorized";
#else
  try {
    bool loginValid = false;
    for (auto &element : _anonymousPaths) {
      if (path.size() >= 9 + element.size() && path.compare(9, element.size(), element) == 0) {
        loginValid = true;
        break;
      }
    }
    if (!loginValid) {
      auto sessionId = http.getHeader().cookies.find("PHPSESSID");
      if (sessionId != http.getHeader().cookies.end()) {
        loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDUI");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDADMIN");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
    }

    std::string contentString;
    if (path.compare(0, 18, "node-blue/context/") == 0 && path.size() > 18) {
      if (!loginValid) { return "unauthorized"; }
      auto subpath = path.substr(18);
      auto subpathParts = BaseLib::HelperFunctions::splitAll(subpath, '/');
      std::string dataId;

      if (!subpathParts.empty() && subpathParts.at(0) == "global") { dataId = "global"; }
      else if (subpathParts.size() > 1) { dataId = subpathParts.at(1); }
      std::string key;
      if (dataId == "global" && subpathParts.size() > 1) { key = subpathParts.at(1); }
      else { key = subpathParts.size() > 2 ? BaseLib::HelperFunctions::splitFirst(subpathParts.at(2), '?').first : ""; }

      if (!dataId.empty()) {
        auto nodeData = _bl->db->getNodeData(dataId, key, false);
        auto contextData = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        auto innerContextData = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

        if (key.empty()) {
          for (auto &innerNodeData : *nodeData->structValue) {
            auto innerInnerContextData = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            innerInnerContextData->structValue->emplace("msg", innerNodeData.second);
            innerInnerContextData->structValue->emplace("format",
                                                        std::make_shared<BaseLib::Variable>(
                                                            getNodeBlueFormatFromVariableType(innerNodeData.second)));
            innerContextData->structValue->emplace(innerNodeData.first, innerInnerContextData);
          }
        } else {
          innerContextData->structValue->emplace("msg", nodeData);
          innerContextData->structValue->emplace("format",
                                                 std::make_shared<BaseLib::Variable>(getNodeBlueFormatFromVariableType(
                                                     nodeData)));
        }

        if (key.empty()) {
          contextData->structValue->emplace("memory", innerContextData);
          _jsonEncoder->encode(contextData, contentString);
        } else { _jsonEncoder->encode(innerContextData, contentString); }
      } else { contentString = "{\"memory\":{}}"; }
      responseEncoding = "application/json";
    } else if (path == "node-blue/locales/nodes") {
      if (!loginValid) { return "unauthorized"; }
      std::string language = "en";
      std::vector<std::string> args = BaseLib::HelperFunctions::splitAll(http.getHeader().args, '&');
      for (auto &arg : args) {
        auto argPair = BaseLib::HelperFunctions::splitFirst(arg, '=');
        if (argPair.first == "lng") {
          language = argPair.second;
          break;
        }
      }
      contentString = _nodeManager->getNodeLocales(language);
      responseEncoding = "application/json";
    } else if (path.compare(0, 16, "node-blue/nodes/") == 0 && path.size() > 16) {
      if (!loginValid) { return "unauthorized"; }
      auto id = path.substr(16);
      auto idPair = BaseLib::HelperFunctions::splitLast(id, '/');
      contentString = _nodeManager->getFrontendCode(idPair.first, idPair.second);
      responseEncoding = "text/html";
    } else if (path.compare(0, 18, "node-blue/locales/") == 0) {
      if (!loginValid) { return "unauthorized"; }
      std::string localePath = _webroot + "static/locales/";
      std::string language = "en";
      std::vector<std::string> args = BaseLib::HelperFunctions::splitAll(http.getHeader().args, '&');
      for (auto &arg : args) {
        auto argPair = BaseLib::HelperFunctions::splitFirst(arg, '=');
        if (argPair.first == "lng") {
          if (BaseLib::Io::directoryExists(localePath + argPair.second)) { language = argPair.second; }
          break;
        }
      }
      auto file = path.substr(17);
      path = localePath + language + file;
      if (BaseLib::Io::fileExists(path)) { contentString = BaseLib::Io::getFileContent(path); }
      if (BaseLib::Io::fileExists(path + "-extra")) {
        auto extraContent = BaseLib::Io::getFileContent(path + "-extra");
        auto decodedContent = BaseLib::Rpc::JsonDecoder::decode(contentString);
        contentString.clear();
        auto decodedExtraContent = BaseLib::Rpc::JsonDecoder::decode(extraContent);

        for (auto &element : *decodedExtraContent->structValue) {
          auto subelementIterator = decodedContent->structValue->find(element.first);
          if (subelementIterator == decodedContent->structValue->end()) {
            decodedContent->structValue->emplace(element.first, element.second);
            continue;
          }

          if (element.second->type == BaseLib::VariableType::tStruct) {
            for (auto &subelement : *element.second->structValue) {
              auto subsubelementIterator = subelementIterator->second->structValue->find(subelement.first);
              if (subsubelementIterator == subelementIterator->second->structValue->end()) {
                subelementIterator->second->structValue->emplace(subelement.first, subelement.second);
                continue;
              }

              if (subelement.second->type == BaseLib::VariableType::tStruct) {
                for (auto &subsubelement : *subelement.second->structValue) {
                  auto subsubsubelementIterator = subsubelementIterator->second->structValue->find(subsubelement.first);
                  if (subsubsubelementIterator == subsubelementIterator->second->structValue->end()) {
                    subsubelementIterator->second->structValue->emplace(subsubelement.first, subsubelement.second);
                    continue;
                  }

                  if (subsubelement.second->type == BaseLib::VariableType::tStruct) {
                    for (auto &subsubsubelement : *subsubelement.second->structValue) {
                      auto subsubsubsubelementIterator =
                          subsubsubelementIterator->second->structValue->find(subsubsubelement.first);
                      if (subsubsubsubelementIterator == subsubsubelementIterator->second->structValue->end()) {
                        subsubsubelementIterator->second->structValue->emplace(subsubsubelement.first,
                                                                               subsubsubelement.second);
                        continue;
                      }

                      if (subsubsubelement.second->type == BaseLib::VariableType::tStruct) {
                        for (auto &subsubsubsubelement : *subsubsubelement.second->structValue) {
                          auto subsubsubsubsubelementIterator =
                              subsubsubsubelementIterator->second->structValue->find(subsubsubsubelement.first);
                          if (subsubsubsubsubelementIterator
                              == subsubsubsubelementIterator->second->structValue->end()) {
                            subsubsubsubelementIterator->second->structValue->emplace(subsubsubsubelement.first,
                                                                                      subsubsubsubelement.second);
                            continue;
                          }

                          if (subsubsubsubelement.second->type == BaseLib::VariableType::tStruct) {
                            _out.printWarning(
                                "Warning: File " + path + "-extra has too many levels. Only five levels are allowed.");
                          } else {
                            (*(*(*(*(*decodedContent->structValue)[element.first]->structValue)[subelement.first]->structValue)[subsubelement.first]->structValue)[subsubsubelement.first]->structValue)[subsubsubsubelement.first] =
                                subsubsubsubelement.second;
                          }
                        }
                      } else {
                        (*(*(*(*decodedContent->structValue)[element.first]->structValue)[subelement.first]->structValue)[subsubelement.first]->structValue)[subsubsubelement.first] =
                            subsubsubelement.second;
                      }
                    }
                  } else {
                    (*(*(*decodedContent->structValue)[element.first]->structValue)[subelement.first]->structValue)[subsubelement.first] =
                        subsubelement.second;
                  }
                }
              } else {
                (*(*decodedContent->structValue)[element.first]->structValue)[subelement.first] = subelement.second;
              }
            }
          } else {
            (*decodedContent->structValue)[element.first] = element.second;
          }
        }

        _jsonEncoder->encode(decodedContent, contentString);
      }
      responseEncoding = "application/json";
    } else if (path.compare(0, 22, "node-blue/credentials/") == 0 && path.size() > 22) {
      if (!loginValid) { return "unauthorized"; }
      auto id = path.substr(2);
      auto idPair = BaseLib::HelperFunctions::splitLast(id, '/');
      auto credentials = _nodeBlueCredentials->getCredentials(idPair.second, false);
      _jsonEncoder->encode(credentials, contentString);
      responseEncoding = "application/json";
    } else if (path == "node-blue/flows") {
      if (!loginValid) { return "unauthorized"; }
      std::string flowsFile = _bl->settings.nodeBlueDataPath() + "flows.json";
      std::vector<char> fileContent;

      {
        std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
        if (BaseLib::Io::fileExists(flowsFile)) { fileContent = BaseLib::Io::getBinaryFileContent(flowsFile); }
        else {
          std::string
              tempString = R"([{"id":")" + FlowParser::generateRandomId() + R"(","label":"Flow 1","type":"tab"}])";
          fileContent.insert(fileContent.end(), tempString.begin(), tempString.end());
        }
      }

      std::vector<char> md5;
      BaseLib::Security::Hash::md5(fileContent, md5);
      std::string md5String = BaseLib::HelperFunctions::getHexString(md5);
      BaseLib::HelperFunctions::toLower(md5String);

      BaseLib::PVariable flowsJson = _jsonDecoder->decode(fileContent);
      BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      responseJson->structValue->emplace("flows", flowsJson);
      responseJson->structValue->emplace("rev", std::make_shared<BaseLib::Variable>(md5String));
      _jsonEncoder->encode(responseJson, contentString);
      responseEncoding = "application/json";
    } else if (path == "node-blue/settings" || path == "node-blue/theme" || path == "node-blue/library/flows"
        || path == "node-blue/debug/view/debug-utils.js") {
      if (!loginValid) { return "unauthorized"; }
      path = _webroot + "static/" + path.substr(10);
      if (BaseLib::Io::fileExists(path)) { contentString = BaseLib::Io::getFileContent(path); }
      responseEncoding = "application/json";
    } else if (path.compare(0, 23, "node-blue/settings/user") == 0) {
      if (!loginValid) { return "unauthorized"; }

      auto settingsPath = _bl->settings.nodeBlueDataPath() + "userSettings.json";
      if (BaseLib::Io::fileExists(settingsPath)) { contentString = BaseLib::Io::getFileContent(settingsPath); }
      else {
        path = _webroot + "static/" + path.substr(19);
        if (BaseLib::Io::fileExists(path)) { contentString = BaseLib::Io::getFileContent(path); }
      }
      responseEncoding = "application/json";
    } else if (path == "node-blue/plugins") {
      if (!loginValid) { return "unauthorized"; }
      responseEncoding = "text/html";
      contentString = "\n";
    } else if (path.compare(0, 26, "node-blue/plugins/messages") == 0) {
      if (!loginValid) { return "unauthorized"; }
      responseEncoding = "application/json";
      contentString = "{}";
    } else if (path == "node-blue/nodes") {
      if (!loginValid) { return "unauthorized"; }
      path = _webroot + "static/" + path.substr(10);

      if (http.getHeader().fields["accept"] == "text/html") {
        responseEncoding = "text/html";
        contentString = _nodeManager->getFrontendCode();
        if (contentString.empty()) {
          _out.printCritical("Could not find any installed nodes.");
        }
      } else {
        responseEncoding = "application/json";
        _jsonEncoder->encode(_nodeManager->getModuleInfo(), contentString);
      }
    } else if (path == "node-blue/icons") {
      if (!loginValid) { return "unauthorized"; }
      responseEncoding = "application/json";
      _jsonEncoder->encode(_nodeManager->getIcons(), contentString);
    } else if (path.compare(0, 16, "node-blue/icons/") == 0) {
      if (!loginValid) { return "unauthorized"; }
      auto pathPair = BaseLib::HelperFunctions::splitLast(path.substr(16), '/');
      auto iconsDirectory = _nodeManager->getModuleIconsDirectory(pathPair.first);
      std::string newPath = iconsDirectory + pathPair.second;
      if (BaseLib::Io::fileExists(newPath)) { contentString = BaseLib::Io::getFileContent(newPath); }
      else {
        newPath = _webroot + "icons/" + pathPair.second;
        if (BaseLib::Io::fileExists(newPath)) { contentString = BaseLib::Io::getFileContent(newPath); }
      }
      auto filePair = BaseLib::HelperFunctions::splitLast(pathPair.second, '.');
      if (!filePair.second.empty()) { responseEncoding = http.getMimeType(filePair.second); }
    } else if (path.compare(0, 30, "node-blue/library/local/flows/") == 0
        || path.compare(0, 35, "node-blue/library/_examples_/flows/") == 0) {
      if (!loginValid) { return "unauthorized"; }

      responseEncoding = "application/json";

      if (path.compare(0, 30, "node-blue/library/local/flows/") == 0) {
        auto libraryPath = _bl->settings.nodeBlueDataPath() + "library/";
        if (!BaseLib::Io::directoryExists(libraryPath)) {
          if (!BaseLib::Io::createDirectory(libraryPath, S_IRWXU | S_IRWXG)) { return ""; }
        }
        if (path.size() > 30) { libraryPath += path.substr(30); }

        bool isDirectory = false;
        if (BaseLib::Io::isDirectory(libraryPath, isDirectory) == -1) { return ""; }

        if (!isDirectory) {
          contentString = BaseLib::Io::getFileContent(libraryPath);
        } else {
          auto directories = BaseLib::Io::getDirectories(libraryPath);
          auto files = BaseLib::Io::getFiles(libraryPath, false);

          auto responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          responseJson->arrayValue->reserve(directories.size() + files.size());

          for (auto &directory : directories) {
            responseJson->arrayValue->push_back(std::make_shared<BaseLib::Variable>(directory.substr(0,
                                                                                                     directory.size()
                                                                                                         - 1)));
          }

          for (auto &file : files) {
            auto fileEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            fileEntry->structValue->emplace("fn", std::make_shared<BaseLib::Variable>(file));
            responseJson->arrayValue->push_back(fileEntry);
          }

          _jsonEncoder->encode(responseJson, contentString);
        }
      } else if (path.compare(0, 35, "node-blue/library/_examples_/flows/") == 0) {
        std::string libraryPath;
        if (path.size() > 35) { libraryPath = path.substr(35); }

        auto responseJson = _nodeManager->getExamples(libraryPath);

        if (responseJson->type == BaseLib::VariableType::tString) {
          contentString = std::move(responseJson->stringValue);
        }
        else { _jsonEncoder->encode(responseJson, contentString); }
      }
    } else if (path.compare(0, 10, "node-blue/") == 0 && path != "node-blue/index.php"
        && path != "node-blue/signin.php") {
      if (!loginValid && path != "node-blue/red/images/node-blue.svg" && path != "node-blue/starting.html") {
        return "unauthorized";
      }
      auto webrootPath = _webroot + path.substr(10);
      if (BaseLib::Io::fileExists(webrootPath)) {
        contentString = BaseLib::Io::getFileContent(webrootPath);

        std::string ending;
        int32_t pos = webrootPath.find_last_of('.');
        if (pos != (signed) std::string::npos && (unsigned) pos < webrootPath.size() - 1) {
          ending = webrootPath.substr(pos + 1);
        }
        BaseLib::HelperFunctions::toLower(ending);
        responseEncoding = http.getMimeType(ending);
        if (responseEncoding.empty()) { responseEncoding = "application/octet-stream"; }
      } else if (_nodeManager->nodeRedRequired()) {
        if (!loginValid) { return "unauthorized"; }
        BaseLib::Http nodeRedHttp;
        auto packet = BaseLib::Http::stripHeader(std::string(http.getRawHeader().begin(), http.getRawHeader().end()),
                                                 std::unordered_set < std::string
                                                     > {"host", "accept-encoding", "referer", "origin"},
                                                 "Host: 127.0.0.1:" + std::to_string(GD::bl->settings.nodeRedPort())
                                                     + "\r\n");
        BaseLib::HelperFunctions::stringReplace(packet, "GET /node-blue/", "GET /");
        packet.insert(packet.end(), http.getContent().data(), http.getContent().data() + http.getContentSize());
        try {
          uint32_t retries = 300;
          while (_flowsRestarting && !_shuttingDown && retries) {
            _out.printInfo("Info: Node-PINK is not ready yet. Waiting...");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            retries--;
          }
          _nodePinkHttpClient->sendRequest(packet, nodeRedHttp);
        } catch (const BaseLib::HttpClientException &ex) {
          _out.printInfo(std::string("Info: ") + ex.what());
        }
        if (!(http.getHeader().connection
            & BaseLib::Http::Connection::keepAlive)) { _nodePinkHttpClient->disconnect(); }
        responseHeader = BaseLib::Http::stripHeader(std::string(nodeRedHttp.getRawHeader().begin(),
                                                                nodeRedHttp.getRawHeader().end()),
                                                    std::unordered_set < std::string
                                                        > {"transfer-encoding", "content-length"},
                                                    "Content-Length: " + std::to_string(nodeRedHttp.getContentSize())
                                                        + "\r\n");
        contentString = std::string(nodeRedHttp.getContent().data(), nodeRedHttp.getContentSize());
      }
    }
    return contentString;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return "";
#endif
}

std::string NodeBlueServer::handlePost(std::string &path,
                                       BaseLib::Http &http,
                                       std::string &responseEncoding,
                                       std::string &responseHeader) {
#ifdef NO_SCRIPTENGINE
  return "unauthorized";
#else
  try {
    bool loginValid = false;
    for (auto &element : _anonymousPaths) {
      if (path.size() >= 9 + element.size() && path.compare(9, element.size(), element) == 0) {
        loginValid = true;
        break;
      }
    }
    if (!loginValid) {
      auto sessionId = http.getHeader().cookies.find("PHPSESSID");
      if (sessionId != http.getHeader().cookies.end()) {
        loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDUI");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDADMIN");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
    }

    if (path == "node-blue/flows" && http.getHeader().contentType == "application/json" && !http.getContent().empty()) {
      if (!loginValid) { return "unauthorized"; }
      responseEncoding = "application/json";
      return deploy(http);
    } else if (path == "node-blue/flows/restart") {
      if (!loginValid) { return "unauthorized"; }
      restartFlowsAsync();
      return R"({"result":"success"})";
    } else if (path == "node-blue/nodes" && http.getHeader().contentType == "application/json"
        && !http.getContent().empty()) {
      if (!loginValid) { return "unauthorized"; }
      responseEncoding = "application/json";
      return installNode(http);
    } else if (path == "node-blue/nodes" && http.getHeader().contentType == "multipart/form-data"
        && !http.getContent().empty()) {
      if (!loginValid) { return "unauthorized"; }
      return processNodeUpload(http);
    } else if (path == "node-blue/settings/user") {
      if (!loginValid) { return "unauthorized"; }

      auto settingsPath = _bl->settings.nodeBlueDataPath() + "userSettings.json";
      BaseLib::Io::writeFile(settingsPath, http.getContent(), http.getContentSize());

      responseEncoding = "application/json";
      return R"({"result":"success"})";
    } else if (path.compare(0, 30, "node-blue/library/local/flows/") == 0 && !http.getContent().empty()) {
      if (!loginValid) { return "unauthorized"; }
      std::string libraryDirectory = _bl->settings.nodeBlueDataPath() + "library/";
      if (!BaseLib::Io::directoryExists(libraryDirectory)) {
        if (!BaseLib::Io::createDirectory(libraryDirectory, S_IRWXU | S_IRWXG)) { return ""; }
      }

      auto subdirectories = path.substr(30);
      auto subdirectoryParts = BaseLib::HelperFunctions::splitAll(subdirectories, '/');
      if (subdirectoryParts.empty()) { return ""; }

      for (int32_t i = 0; i < (signed) subdirectoryParts.size() - 1; i++) {
        libraryDirectory += subdirectoryParts.at(i) + '/';
        if (!BaseLib::Io::directoryExists(libraryDirectory)) {
          if (!BaseLib::Io::createDirectory(libraryDirectory, S_IRWXU | S_IRWXG)) { return ""; }
        }
      }

      auto filepath = libraryDirectory + subdirectoryParts.back();

      BaseLib::Io::writeFile(filepath, http.getContent(), http.getContentSize());

      responseEncoding = "application/json";
      return R"({"result":"success"})";
    } else if (path.compare(0, 10, "node-blue/") == 0 && path != "node-blue/index.php" && path != "node-blue/signin.php"
        && _nodeManager->nodeRedRequired()) {
      if (!loginValid) { return "unauthorized"; }
      BaseLib::Http nodeRedHttp;
      auto packet = BaseLib::Http::stripHeader(std::string(http.getRawHeader().begin(), http.getRawHeader().end()),
                                               std::unordered_set < std::string
                                                   > {"host", "accept-encoding", "referer", "origin"},
                                               "Host: 127.0.0.1:" + std::to_string(GD::bl->settings.nodeRedPort())
                                                   + "\r\n");
      BaseLib::HelperFunctions::stringReplace(packet, "POST /node-blue/", "POST /");
      packet.insert(packet.end(), http.getContent().data(), http.getContent().data() + http.getContentSize());
      try {
        uint32_t retries = 300;
        while (_flowsRestarting && !_shuttingDown && retries) {
          _out.printInfo("Info: Node-PINK is not ready yet. Waiting...");
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          retries--;
        }
        _nodePinkHttpClient->sendRequest(packet, nodeRedHttp);
      } catch (const BaseLib::HttpClientException &ex) {
        _out.printInfo(std::string("Info: ") + ex.what());
      }
      if (!(http.getHeader().connection & BaseLib::Http::Connection::keepAlive)) { _nodePinkHttpClient->disconnect(); }
      responseHeader =
          BaseLib::Http::stripHeader(std::string(nodeRedHttp.getRawHeader().begin(), nodeRedHttp.getRawHeader().end()),
                                     std::unordered_set < std::string > {"transfer-encoding", "content-length"},
                                     "Content-Length: " + std::to_string(nodeRedHttp.getContentSize()) + "\r\n");
      return std::string(nodeRedHttp.getContent().data(), nodeRedHttp.getContentSize());
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return "";
#endif
}

std::string NodeBlueServer::deploy(BaseLib::Http &http) {
  try {
    _out.printInfo("Info: Deploying (1)...");
    std::lock_guard<std::mutex> flowsPostGuard(_flowsPostMutex);
    _out.printInfo("Info: Deploying (2)...");
    BaseLib::PVariable json = _jsonDecoder->decode(http.getContent());
    auto flowsIterator = json->structValue->find("flows");
    if (flowsIterator == json->structValue->end()) { return ""; }
    auto flowsJson = flowsIterator->second;

    _nodeBlueCredentials->processFlowCredentials(flowsJson);

    std::vector<char> flows;
    _jsonEncoder->encode(flowsJson, flows);

    backupFlows();

    {
      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      BaseLib::Io::deleteFile(_bl->settings.nodeBlueDataPath() + "node-red/flows.json");
      std::string filename = _bl->settings.nodeBlueDataPath() + "flows.json";
      BaseLib::Io::writeFile(filename, flows, flows.size());
    }

    std::vector<char> md5;
    BaseLib::Security::Hash::md5(flows, md5);
    std::string md5String = BaseLib::HelperFunctions::getHexString(md5);
    BaseLib::HelperFunctions::toLower(md5String);

    _out.printInfo("Info: Deploying (3)...");
    restartFlowsAsync();

    auto eventData = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    eventData->structValue->emplace("revision", std::make_shared<BaseLib::Variable>(md5String));
    GD::rpcClient->broadcastNodeEvent("", "notification/runtime-deploy", eventData, false);

    return R"({"rev": ")" + md5String + "\"}";
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "";
}

std::string NodeBlueServer::installNode(BaseLib::Http &http) {
  try {
    _out.printInfo("Info: Installing node (1)...");
    std::lock_guard<std::mutex> nodesInstallGuard(_nodesInstallMutex);
    _out.printInfo("Info: Installing node (2)...");
    BaseLib::PVariable json = _jsonDecoder->decode(http.getContent());
    auto moduleIterator = json->structValue->find("module");
    if (moduleIterator == json->structValue->end() || moduleIterator->second->stringValue.empty()) {
      return R"({"result":"error","error":"module is missing."})";
    }
    std::string url;
    {
      auto urlIterator = json->structValue->find("url");
      if (urlIterator != json->structValue->end()) { url = urlIterator->second->stringValue; }
    }

    //{{{ Check if this is an update
    bool isUpdate = false;
    auto updateInfo = _nodeManager->getNodesUpdatedInfo(moduleIterator->second->stringValue);
    isUpdate = !updateInfo->structValue->empty();
    //}}}

    std::string method = "managementInstallNode";
    auto parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(2);
    parameters->push_back(moduleIterator->second);
    parameters->push_back(std::make_shared<BaseLib::Variable>(url));
    BaseLib::PVariable result = GD::ipcServer->callRpcMethod(_nodeBlueClientInfo, method, parameters);
    if (result->errorStruct) {
      _out.printError("Error: Could not install node: " + result->structValue->at("faultString")->stringValue);
      return R"({"result":"error","error":")"
          + _jsonEncoder->encodeString(result->structValue->at("faultString")->stringValue) + "\"}";
    }

    _nodeManager->fillManagerModuleInfo();
    //Start Node-RED if not started yet
    if (_nodeManager->nodeRedRequired() && !_nodepink->isStarted()) {
      _nodepinkWebsocket->start();
      _nodepink->start();
      if (_nodepink->startUpError()) {
        GD::rpcClient->broadcastNodeEvent("global",
                                          "nodePinkStartError",
                                          std::make_shared<BaseLib::Variable>(),
                                          false);
      }
    }

    BaseLib::PVariable moduleInfo;
    if (isUpdate) {
      //Get new version
      updateInfo = _nodeManager->getNodesUpdatedInfo(moduleIterator->second->stringValue);
    } else {
      moduleInfo = _nodeManager->getNodesAddedInfo(moduleIterator->second->stringValue);
      if (moduleInfo->arrayValue->empty()) {
        _out.printError("Error: Could not install node: Node could not be loaded after installation.");
        return R"({"result":"error","error":"Node could not be loaded after installation. See error log for more details."})";
      }
    }

    if (_nodeEventsEnabled) {
      if (isUpdate) {
        GD::rpcClient->broadcastNodeEvent("", "notification/node/upgraded", updateInfo, false);

        auto restartRequiredNotification = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        restartRequiredNotification->structValue->emplace("type", std::make_shared<BaseLib::Variable>("warning"));
        restartRequiredNotification->structValue->emplace("header",
                                                          std::make_shared<BaseLib::Variable>("notification.warning"));
        restartRequiredNotification->structValue->emplace("text",
                                                          std::make_shared<BaseLib::Variable>(
                                                              "notification.warnings.restartRequired"));
        restartRequiredNotification->structValue->emplace("fixed", std::make_shared<BaseLib::Variable>(true));
        GD::rpcClient->broadcastNodeEvent("global", "showNotification", restartRequiredNotification, false);
      } else { GD::rpcClient->broadcastNodeEvent("", "notification/node/added", moduleInfo, false); }
    }

    return R"({"result":"success"})";
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "";
}

std::string NodeBlueServer::processNodeUpload(BaseLib::Http &http) {
  std::string boundary;

  {
    auto contentTypeParts = BaseLib::HelperFunctions::splitAll(http.getHeader().contentTypeFull, ';');
    for (auto &part : contentTypeParts) {
      BaseLib::HelperFunctions::trim(part);
      auto pair = BaseLib::HelperFunctions::splitFirst(part, '=');
      BaseLib::HelperFunctions::toLower(pair.first);
      if (pair.first == "boundary") {
        boundary = pair.second;
        break;
      }
    }
  }

  if (boundary.empty()) { return R"({"result":"error","error":"Could not interprete data."})"; }

  auto &content = http.getContent();
  auto end = (char *) memmem(content.data(), content.size(), (boundary + "--").c_str(), boundary.size() + 2);
  if (!end) { return R"({"result":"error","error":"Could not interprete data."})"; }
  auto start = (char *) memmem(content.data(), content.size(), (boundary).c_str(), boundary.size());
  if (!start) { return R"({"result":"error","error":"Could not interprete data."})"; }

  std::vector<char> data(start + boundary.size() + 2, end - 4);
  char *headerEnd = (char *) memmem(data.data(), data.size(), "\r\n\r\n", 4);
  if (!headerEnd) { return R"({"result":"error","error":"Could not interprete data."})"; }
  std::string header(data.data(), headerEnd - data.data());
  auto headerLines = BaseLib::HelperFunctions::splitAll(header, '\n');
  std::string contentType;
  for (auto &line : headerLines) {
    auto pair = BaseLib::HelperFunctions::splitFirst(line, ':');
    BaseLib::HelperFunctions::trim(pair.first);
    BaseLib::HelperFunctions::toLower(pair.first);
    if (pair.first == "content-type") {
      contentType = BaseLib::HelperFunctions::toLower(BaseLib::HelperFunctions::trim(pair.second));
    }
  }

  if (contentType != "application/gzip") {
    _out.printWarning("Warning: File upload has unknown content type.");
    return R"({"result":"error","error":"Unsupported content type."})";
  }

  std::vector<char> packedData(headerEnd + 4, data.data() + data.size());
  auto tempPath = "/tmp/" + BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomBytes(32)) + "/";
  BaseLib::Io::createDirectory(tempPath, S_IRWXU | S_IRWXG);
  BaseLib::Io::writeFile(tempPath + "archive.tar.gz", packedData, packedData.size());

  _out.printInfo("Info: Installing node (1)...");
  std::lock_guard<std::mutex> nodesInstallGuard(_nodesInstallMutex);
  _out.printInfo("Info: Installing node (2)...");

  std::string output;
  if (BaseLib::ProcessManager::exec("tar -C '" + tempPath + "' -zxf '" + tempPath + "archive.tar.gz'",
                                    GD::bl->fileDescriptorManager.getMax(),
                                    output) != 0) {
    BaseLib::ProcessManager::exec("rm -Rf \"" + tempPath + "\"", GD::bl->fileDescriptorManager.getMax(), output);
    return R"({"result":"error"})";
  }

  auto directories = BaseLib::Io::getDirectories(tempPath);
  if (directories.empty()) { return R"({"result":"error"})"; }

  auto directory = tempPath + directories.front();
  if (!BaseLib::Io::fileExists(directory + "package.json")) {
    return R"({"result":"error","error":"No package.json found."})";
  }

  auto packageJsonContent = BaseLib::Io::getFileContent(directory + "package.json");
  auto packageJson = BaseLib::Rpc::JsonDecoder::decode(packageJsonContent);

  auto nameIterator = packageJson->structValue->find("name");
  if (nameIterator == packageJson->structValue->end()) {
    return R"({"result":"error","error":"No name in package.json."})";
  }

  auto module = nameIterator->second->stringValue;

  if (BaseLib::ProcessManager::exec("mv '" + directory + "' '" + tempPath + module + "'",
                                    GD::bl->fileDescriptorManager.getMax(),
                                    output) != 0) {
    return R"({"result":"error"})";
  }

  //{{{ Check if this is an update
  bool isUpdate = false;
  auto updateInfo = _nodeManager->getNodesUpdatedInfo(module);
  isUpdate = !updateInfo->structValue->empty();
  //}}}

  std::string method = "managementInstallNode";
  auto parameters = std::make_shared<BaseLib::Array>();
  parameters->reserve(2);
  parameters->push_back(std::make_shared<BaseLib::Variable>(module));
  parameters->push_back(std::make_shared<BaseLib::Variable>(tempPath));
  BaseLib::PVariable result = GD::ipcServer->callRpcMethod(_nodeBlueClientInfo, method, parameters);
  if (result->errorStruct) {
    _out.printError("Error: Could not install node: " + result->structValue->at("faultString")->stringValue);
    return R"({"result":"error","error":")"
        + _jsonEncoder->encodeString(result->structValue->at("faultString")->stringValue) + "\"}";
  }

  _nodeManager->fillManagerModuleInfo();

  BaseLib::PVariable moduleInfo;
  if (isUpdate) {
    //Get new version
    updateInfo = _nodeManager->getNodesUpdatedInfo(module);
  } else {
    moduleInfo = _nodeManager->getNodesAddedInfo(module);
    if (moduleInfo->arrayValue->empty()) {
      _out.printError("Error: Could not install node: Node could not be loaded after installation.");
      return R"({"result":"error","error":"Node could not be loaded after installation. See error log for more details."})";
    }
  }

  if (_nodeEventsEnabled) {
    if (isUpdate) {
      GD::rpcClient->broadcastNodeEvent("", "notification/node/upgraded", updateInfo, false);

      auto restartRequiredNotification = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      restartRequiredNotification->structValue->emplace("type", std::make_shared<BaseLib::Variable>("warning"));
      restartRequiredNotification->structValue->emplace("header",
                                                        std::make_shared<BaseLib::Variable>("notification.warning"));
      restartRequiredNotification->structValue->emplace("text",
                                                        std::make_shared<BaseLib::Variable>(
                                                            "notification.warnings.restartRequired"));
      restartRequiredNotification->structValue->emplace("fixed", std::make_shared<BaseLib::Variable>(true));
      GD::rpcClient->broadcastNodeEvent("global", "showNotification", restartRequiredNotification, false);
    } else { GD::rpcClient->broadcastNodeEvent("", "notification/node/added", moduleInfo, false); }
  }

  return R"({"result":"success"})";
}

std::string NodeBlueServer::handleDelete(std::string &path,
                                         BaseLib::Http &http,
                                         std::string &responseEncoding,
                                         std::string &responseHeader) {
#ifdef NO_SCRIPTENGINE
  return "unauthorized";
#else
  try {
    bool loginValid = false;
    for (auto &element : _anonymousPaths) {
      if (path.size() >= 9 + element.size() && path.compare(9, element.size(), element) == 0) {
        loginValid = true;
        break;
      }
    }
    if (!loginValid) {
      auto sessionId = http.getHeader().cookies.find("PHPSESSID");
      if (sessionId != http.getHeader().cookies.end()) {
        loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDUI");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDADMIN");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
    }

    if (!loginValid) { return "unauthorized"; }

    std::string contentString;
    if (path.compare(0, 18, "node-blue/context/") == 0 && path.size() > 18) {
      auto subpath = path.substr(18);
      auto subpathParts = BaseLib::HelperFunctions::splitAll(subpath, '/');
      std::string dataId;

      if (!subpathParts.empty() && subpathParts.at(0) == "global") { dataId = "global"; }
      else if (subpathParts.size() > 1) { dataId = subpathParts.at(1); }
      std::string key;
      if (dataId == "global" && subpathParts.size() > 1) { key = subpathParts.at(1); }
      else if (subpathParts.size() > 2) { key = BaseLib::HelperFunctions::splitFirst(subpathParts.at(2), '?').first; }

      if (!dataId.empty() && !key.empty()) { _bl->db->deleteNodeData(dataId, key); }
    } else if (path.compare(0, 16, "node-blue/nodes/") == 0 && path.size() > 16) {
      responseEncoding = "application/json";
      auto module = path.substr(16);
      _out.printInfo("Info: Uninstalling node (1)...");
      std::lock_guard<std::mutex> nodesInstallGuard(_nodesInstallMutex);
      _out.printInfo("Info: Uninstalling node (2)...");

      std::string method = "managementUninstallNode";
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->push_back(std::make_shared<BaseLib::Variable>(module));
      BaseLib::PVariable result = GD::ipcServer->callRpcMethod(_nodeBlueClientInfo, method, parameters);
      if (result->errorStruct) {
        _out.printError("Error: Could not uninstall node: " + result->structValue->at("faultString")->stringValue);
        return R"({"result":"error","error":")"
            + _jsonEncoder->encodeString(result->structValue->at("faultString")->stringValue) + "\"}";
      }

      auto moduleInfo = _nodeManager->getNodesRemovedInfo(module);

      _nodeManager->fillManagerModuleInfo();

      if (_nodeEventsEnabled && !moduleInfo->arrayValue->empty()) {
        GD::rpcClient->broadcastNodeEvent("",
                                          "notification/node/removed",
                                          moduleInfo,
                                          false);
      }

      return R"({"result":"success"})";
    } else if (path.compare(0, 10, "node-blue/") == 0 && _nodeManager->nodeRedRequired()) {
      BaseLib::Http nodeRedHttp;
      auto packet = BaseLib::Http::stripHeader(std::string(http.getRawHeader().begin(), http.getRawHeader().end()),
                                               std::unordered_set < std::string
                                                   > {"host", "accept-encoding", "referer", "origin"},
                                               "Host: 127.0.0.1:" + std::to_string(GD::bl->settings.nodeRedPort())
                                                   + "\r\n");
      BaseLib::HelperFunctions::stringReplace(packet, "DELETE /node-blue/", "DELETE /");
      packet.insert(packet.end(), http.getContent().data(), http.getContent().data() + http.getContentSize());
      try {
        uint32_t retries = 300;
        while (_flowsRestarting && !_shuttingDown && retries) {
          _out.printInfo("Info: Node-PINK is not ready yet. Waiting...");
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          retries--;
        }
        _nodePinkHttpClient->sendRequest(packet, nodeRedHttp);
      } catch (const BaseLib::HttpClientException &ex) {
        _out.printInfo(std::string("Info: ") + ex.what());
      }
      if (!(http.getHeader().connection & BaseLib::Http::Connection::keepAlive)) { _nodePinkHttpClient->disconnect(); }
      responseHeader =
          BaseLib::Http::stripHeader(std::string(nodeRedHttp.getRawHeader().begin(), nodeRedHttp.getRawHeader().end()),
                                     std::unordered_set < std::string > {"transfer-encoding", "content-length"},
                                     "Content-Length: " + std::to_string(nodeRedHttp.getContentSize()) + "\r\n");
      contentString = std::string(nodeRedHttp.getContent().data(), nodeRedHttp.getContentSize());
    }

    return contentString;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return "";
#endif
}

std::string NodeBlueServer::handlePut(std::string &path,
                                      BaseLib::Http &http,
                                      std::string &responseEncoding,
                                      std::string &responseHeader) {
#ifdef NO_SCRIPTENGINE
  return "unauthorized";
#else
  try {
    bool loginValid = false;
    for (auto &element : _anonymousPaths) {
      if (path.size() >= 9 + element.size() && path.compare(9, element.size(), element) == 0) {
        loginValid = true;
        break;
      }
    }
    if (!loginValid) {
      auto sessionId = http.getHeader().cookies.find("PHPSESSID");
      if (sessionId != http.getHeader().cookies.end()) {
        loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDUI");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
      if (!loginValid) {
        sessionId = http.getHeader().cookies.find("PHPSESSIDADMIN");
        if (sessionId != http.getHeader().cookies.end()) {
          loginValid = !GD::scriptEngineServer->checkSessionId(sessionId->second).empty();
        }
      }
    }

    if (path.compare(0, 10, "node-blue/") == 0 && _nodeManager->nodeRedRequired()) {
      if (!loginValid) { return "unauthorized"; }
      BaseLib::Http nodeRedHttp;
      auto packet = BaseLib::Http::stripHeader(std::string(http.getRawHeader().begin(), http.getRawHeader().end()),
                                               std::unordered_set < std::string
                                                   > {"host", "accept-encoding", "referer", "origin"},
                                               "Host: 127.0.0.1:" + std::to_string(GD::bl->settings.nodeRedPort())
                                                   + "\r\n");
      BaseLib::HelperFunctions::stringReplace(packet, "PUT /node-blue/", "PUT /");
      packet.insert(packet.end(), http.getContent().data(), http.getContent().data() + http.getContentSize());
      try {
        uint32_t retries = 300;
        while (_flowsRestarting && !_shuttingDown && retries) {
          _out.printInfo("Info: Node-PINK is not ready yet. Waiting...");
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
          retries--;
        }
        _nodePinkHttpClient->sendRequest(packet, nodeRedHttp);
      } catch (const BaseLib::HttpClientException &ex) {
        _out.printInfo(std::string("Info: ") + ex.what());
      }
      if (!(http.getHeader().connection & BaseLib::Http::Connection::keepAlive)) { _nodePinkHttpClient->disconnect(); }
      responseHeader =
          BaseLib::Http::stripHeader(std::string(nodeRedHttp.getRawHeader().begin(), nodeRedHttp.getRawHeader().end()),
                                     std::unordered_set < std::string > {"transfer-encoding", "content-length"},
                                     "Content-Length: " + std::to_string(nodeRedHttp.getContentSize()) + "\r\n");
      return std::string(nodeRedHttp.getContent().data(), nodeRedHttp.getContentSize());
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return "";
#endif
}

BaseLib::PVariable NodeBlueServer::getVariablesInRole(BaseLib::PRpcClientInfo clientInfo,
                                                      uint64_t roleId,
                                                      uint64_t peerId) {
  try {
    auto inputChannels = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    auto outputChannels = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMapMutex);
    for (auto &nodeInfo : _nodeInfoMap) {
      if (peerId == 0 || peerId == 0x50000001) {
        for (auto &output : nodeInfo.second.outputRoles) {
          if (output.second == roleId) {
            if (!clientInfo->acls->checkNodeBlueVariableWriteAccess(nodeInfo.first, output.first)) { continue; }

            auto channelIterator = outputChannels->structValue->find(std::to_string(output.first));
            if (channelIterator == outputChannels->structValue->end()) {
              auto result = outputChannels->structValue->emplace(std::to_string(output.first),
                                                                 std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
              if (!result.second) { continue; }
              channelIterator = result.first;
            }

            auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

            roleStruct->structValue->emplace("direction",
                                             std::make_shared<BaseLib::Variable>((int32_t) BaseLib::RoleDirection::output));
            roleStruct->structValue->emplace("level",
                                             std::make_shared<BaseLib::Variable>(
                                                 (roleId / 10000) * 10000 == roleId ? 0 : ((roleId / 100) * 100
                                                                                               == roleId ? 1 : 2)));

            channelIterator->second->structValue->emplace(nodeInfo.first, roleStruct);
          }
        }
      }
    }

    if (peerId != 0) {
      return outputChannels;
    } else {
      auto devices = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      if (!outputChannels->structValue->empty()) {
        devices->structValue->emplace(std::to_string(0x50000001),
                                      outputChannels);
      }
      return devices;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

uint32_t NodeBlueServer::flowCount() {
  try {
    if (_shuttingDown) { return 0; }
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    uint32_t count = 0;
    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array());
      BaseLib::PVariable response = sendRequest(client, "flowCount", parameters, true);
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

bool NodeBlueServer::isReady() {
  return !((_nodeManager->nodeRedRequired() && (!_nodepink->isStarted() || _nodepink->startUpError()))
      || _flowsRestarting);
}

void NodeBlueServer::broadcastEvent(std::string &source,
                                    uint64_t id,
                                    int32_t channel,
                                    std::shared_ptr<std::vector<std::string>> &variables,
                                    BaseLib::PArray &values) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("event")) { return; }

    bool checkAcls = _nodeBlueClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesDevicesReadSet();
    std::shared_ptr<BaseLib::Systems::Peer> peer;

    if (id != 0 && id != 0x50000001) {
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central) { peer = central->getPeer(id); }
        if (peer) { break; }
      }
      if (!peer) { return; }
    }

    if (checkAcls) {
      std::shared_ptr<std::vector<std::string>> newVariables;
      BaseLib::PArray newValues = std::make_shared<BaseLib::Array>();
      newVariables->reserve(variables->size());
      newValues->reserve(values->size());
      for (int32_t i = 0; i < (int32_t) variables->size(); i++) {
        if (id == 0) {
          if (_nodeBlueClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesReadSet()) {
            auto systemVariable = GD::systemVariableController->getInternal(variables->at(i));
            if (systemVariable && _nodeBlueClientInfo->acls->checkSystemVariableReadAccess(systemVariable)) {
              newVariables->push_back(variables->at(i));
              newValues->push_back(values->at(i));
            }
          }
        } else if (id == 0x50000001) {
          if (_nodeBlueClientInfo->acls->variablesWriteSet()) { //Yes, this is a write as it is a output from the node, even though it's in broadcast
            if (_nodeBlueClientInfo->acls->checkNodeBlueVariableWriteAccess(variables->at(i), channel)) {
              newVariables->push_back(variables->at(i));
              newValues->push_back(values->at(i));
            }
          }
        } else if (peer && _nodeBlueClientInfo->acls->checkVariableReadAccess(peer, channel, variables->at(i))) {
          newVariables->push_back(variables->at(i));
          newValues->push_back(values->at(i));
        }
      }
      variables = newVariables;
      values = newValues;
    }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      if (peer) {
        metadata->structValue->emplace("name", std::make_shared<BaseLib::Variable>(peer->getName()));
        metadata->structValue->emplace("room", std::make_shared<BaseLib::Variable>(peer->getRoom(-1)));

        {
          auto categories = peer->getCategories(-1);
          auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          categoryArray->arrayValue->reserve(categories.size());
          for (auto category : categories) {
            categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
          }
          metadata->structValue->emplace("categories", categoryArray);
        }

        if (channel != -1) {
          metadata->structValue->emplace("channelName", std::make_shared<BaseLib::Variable>(peer->getName(channel)));
          metadata->structValue->emplace("channelRoom", std::make_shared<BaseLib::Variable>(peer->getRoom(channel)));

          {
            auto categories = peer->getCategories(channel);
            auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            categoryArray->arrayValue->reserve(categories.size());
            for (auto category : categories) {
              categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
            }
            metadata->structValue->emplace("channelCategories", categoryArray);
          }

          auto variableMetadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
          metadata->structValue->emplace("variableMetadata", variableMetadata);

          for (auto &variable : *variables) {
            auto room = peer->getVariableRoom(channel, variable);
            variableMetadata->structValue->emplace("room", std::make_shared<BaseLib::Variable>(room));

            {
              auto categories = peer->getVariableCategories(channel, variable);
              auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
              categoryArray->arrayValue->reserve(categories.size());
              for (auto category : categories) {
                categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
              }
              variableMetadata->structValue->emplace("categories", categoryArray);
            }

            {
              auto roles = peer->getVariableRoles(channel, variable);
              auto rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
              for (auto &role : roles) {
                auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
                roleStruct->structValue->emplace("direction",
                                                 std::make_shared<BaseLib::Variable>((int32_t) role.second.direction));
                if (role.second.invert) {
                  roleStruct->structValue->emplace("invert",
                                                   std::make_shared<BaseLib::Variable>(role.second.invert));
                }
                rolesArray->arrayValue->emplace_back(std::move(roleStruct));
              }
              variableMetadata->structValue->emplace("roles", rolesArray);
            }
          }
        }
      } else if (id == 0) {
        //System variables
        auto variableMetadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        metadata->structValue->emplace("variableMetadata", variableMetadata);

        for (auto &variable : *variables) {
          auto systemVariable = GD::systemVariableController->getInternal(variable);
          if (!systemVariable) { continue; }

          variableMetadata->structValue->emplace("room", std::make_shared<BaseLib::Variable>(systemVariable->room));

          auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          categoryArray->arrayValue->reserve(systemVariable->categories.size());
          for (auto category : systemVariable->categories) {
            categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
          }
          variableMetadata->structValue->emplace("categories", categoryArray);

          auto rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          for (auto &role : systemVariable->roles) {
            auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
            roleStruct->structValue->emplace("direction",
                                             std::make_shared<BaseLib::Variable>((int32_t) role.second.direction));
            if (role.second.invert) {
              roleStruct->structValue->emplace("invert",
                                               std::make_shared<BaseLib::Variable>(role.second.invert));
            }
            rolesArray->arrayValue->emplace_back(std::move(roleStruct));
          }
          variableMetadata->structValue->emplace("roles", rolesArray);
        }
      }

      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(6);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(source));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(id));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(channel));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(*variables));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(values));
      parameters->emplace_back(metadata);
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastEvent", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastEvent\". Queue is full.");
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

void NodeBlueServer::broadcastFlowVariableEvent(std::string &flowId, std::string &variable, BaseLib::PVariable &value) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("event")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(3);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(flowId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(variable));
      parameters->emplace_back(value);
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastFlowVariableEvent", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastFlowVariableEvent\". Queue is full.");
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

void NodeBlueServer::broadcastGlobalVariableEvent(std::string &variable, BaseLib::PVariable &value) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("event")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(2);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(variable));
      parameters->emplace_back(value);
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastGlobalVariableEvent", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastGlobalVariableEvent\". Queue is full.");
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

void NodeBlueServer::broadcastServiceMessage(const BaseLib::PServiceMessage &serviceMessage) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("serviceMessage")) { return; }

    bool checkAcls = _nodeBlueClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesDevicesReadSet();
    std::shared_ptr<BaseLib::Systems::Peer> peer;

    if (serviceMessage->peerId > 0) {
      auto families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central) { peer = central->getPeer(serviceMessage->peerId); }
        if (peer) { break; }
      }
      if (!peer) { return; }
    }

    if (checkAcls && peer && !serviceMessage->variable.empty() && !_nodeBlueClientInfo->acls->checkVariableReadAccess(
        peer,
        serviceMessage->channel,
        serviceMessage->variable)) {
      return;
    }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      if (peer) {
        metadata->structValue->emplace("name", std::make_shared<BaseLib::Variable>(peer->getName()));
        metadata->structValue->emplace("room", std::make_shared<BaseLib::Variable>(peer->getRoom(-1)));

        {
          auto categories = peer->getCategories(-1);
          auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          categoryArray->arrayValue->reserve(categories.size());
          for (auto category : categories) {
            categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
          }
          metadata->structValue->emplace("categories", categoryArray);
        }

        if (serviceMessage->channel != -1) {
          metadata->structValue->emplace("channelName",
                                         std::make_shared<BaseLib::Variable>(peer->getName(serviceMessage->channel)));
          metadata->structValue->emplace("channelRoom",
                                         std::make_shared<BaseLib::Variable>(peer->getRoom(serviceMessage->channel)));

          {
            auto categories = peer->getCategories(serviceMessage->channel);
            auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            categoryArray->arrayValue->reserve(categories.size());
            for (auto category : categories) {
              categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
            }
            metadata->structValue->emplace("channelCategories", categoryArray);
          }

          if (!serviceMessage->variable.empty()) {
            auto variableMetadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            metadata->structValue->emplace("variableMetadata", variableMetadata);

            auto room = peer->getVariableRoom(serviceMessage->channel, serviceMessage->variable);
            variableMetadata->structValue->emplace("room", std::make_shared<BaseLib::Variable>(room));

            {
              auto categories = peer->getVariableCategories(serviceMessage->channel, serviceMessage->variable);
              auto categoryArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
              categoryArray->arrayValue->reserve(categories.size());
              for (auto category : categories) {
                categoryArray->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(category));
              }
              variableMetadata->structValue->emplace("categories", categoryArray);
            }

            {
              auto roles = peer->getVariableRoles(serviceMessage->channel, serviceMessage->variable);
              auto rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
              for (auto &role : roles) {
                auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
                roleStruct->structValue->emplace("direction",
                                                 std::make_shared<BaseLib::Variable>((int32_t) role.second.direction));
                if (role.second.invert) {
                  roleStruct->structValue->emplace("invert",
                                                   std::make_shared<BaseLib::Variable>(role.second.invert));
                }
                rolesArray->arrayValue->emplace_back(std::move(roleStruct));
              }
              variableMetadata->structValue->emplace("roles", rolesArray);
            }
          }
        }
      }

      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(2);
      parameters->emplace_back(serviceMessage->serialize());
      parameters->emplace_back(metadata);
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastServiceMessage", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastServiceMessage\". Queue is full.");
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

void NodeBlueServer::broadcastNewDevices(const std::vector<uint64_t> &ids,
                                         const BaseLib::PVariable &deviceDescriptions) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("newDevices")) { return; }
    if (_nodeBlueClientInfo->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet()) {
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central && central->peerExists((uint64_t) ids.front())) //All ids are from the same family
        {
          for (auto id : ids) {
            auto peer = central->getPeer((uint64_t) id);
            if (!peer || !_nodeBlueClientInfo->acls->checkDeviceReadAccess(peer)) { return; }
          }
        }
      }
    }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array{deviceDescriptions});
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastNewDevices", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastNewDevices\". Queue is full.");
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

void NodeBlueServer::broadcastDeleteDevices(const BaseLib::PVariable &deviceInfo) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      BaseLib::PArray parameters(new BaseLib::Array{deviceInfo});
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastDeleteDevices", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastDeleteDevices\". Queue is full.");
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

void NodeBlueServer::broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("updateDevice")) { return; }
    if (_nodeBlueClientInfo->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet()) {
      std::shared_ptr<BaseLib::Systems::Peer> peer;
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central) { peer = central->getPeer(id); }
        if (!peer || !_nodeBlueClientInfo->acls->checkDeviceReadAccess(peer)) { return; }
      }
    }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      BaseLib::PArray parameters
          (new BaseLib::Array{std::make_shared<BaseLib::Variable>(id), std::make_shared<BaseLib::Variable>(channel),
                              std::make_shared<BaseLib::Variable>(hint)});
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastUpdateDevice", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastUpdateDevice\". Queue is full.");
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastStatus(const std::string &nodeId, const BaseLib::PVariable &status) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("event")) { return; } //"event" provides access to this method. This is correct.

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) continue;
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(2);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(nodeId));
      parameters->emplace_back(status);
      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastStatus", parameters);
      if (!enqueue(2, queueEntry))
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastStatus\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastError(const std::string &nodeId, int32_t level, const BaseLib::PVariable &error) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }
    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("event")) { return; } //"event" provides access to this method. This is correct.

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) continue;
        clients.push_back(client.second);
      }
    }

    bool handled = false;
    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(3);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(nodeId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(level));
      parameters->emplace_back(error);

      BaseLib::PVariable response = sendRequest(client, "broadcastError", parameters, true);
      if (response->booleanValue) handled = true;
    }

    if (!handled && _nodeEventsEnabled) {
      auto sourceStruct = error->structValue->at("error")->structValue->at("source");

      auto value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      value->structValue->emplace("id", std::make_shared<BaseLib::Variable>(nodeId));
      value->structValue->emplace("type", std::make_shared<BaseLib::Variable>("function"));
      value->structValue->emplace("z", sourceStruct->structValue->at("z"));
      value->structValue->emplace("_source", sourceStruct);
      value->structValue->emplace("name", sourceStruct->structValue->at("name"));
      value->structValue->emplace("level", std::make_shared<BaseLib::Variable>(level));
      value->structValue->emplace("msg", error->structValue->at("error")->structValue->at("message"));
      GD::rpcClient->broadcastNodeEvent(nodeId, "debug", value, true);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastVariableProfileStateChanged(uint64_t profileId, bool state) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("variableProfileStateChanged")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(2);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(profileId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(state));

      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastVariableProfileStateChanged", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastVariableProfileStateChanged\". Queue is full.");
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastUiNotificationCreated(uint64_t uiNotificationId) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("uiNotificationCreated")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));

      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastUiNotificationCreated", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastUiNotificationCreated\". Queue is full.");
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastUiNotificationRemoved(uint64_t uiNotificationId) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("uiNotificationRemoved")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));

      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastUiNotificationRemoved", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastUiNotificationRemoved\". Queue is full.");
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastUiNotificationAction(uint64_t uiNotificationId,
                                                   const std::string &uiNotificationType,
                                                   uint64_t buttonId) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("uiNotificationAction")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(3);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationType));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(buttonId));

      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastUiNotificationAction", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastUiNotificationAction\". Queue is full.");
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::broadcastRawPacketEvent(int32_t familyId,
                                             const std::string &interfaceId,
                                             const BaseLib::PVariable &packet) {
  try {
    if (_shuttingDown || _flowsRestarting) { return; }

    if (!_nodeBlueClientInfo->acls->checkEventServerMethodAccess("rawPacketEvent")) { return; }

    std::vector<PNodeBlueClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) { continue; }
        clients.push_back(client.second);
      }
    }

    for (auto &client : clients) {
      auto parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(3);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(familyId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(interfaceId));
      parameters->emplace_back(packet);

      std::shared_ptr<BaseLib::IQueueEntry>
          queueEntry = std::make_shared<QueueEntry>(client, "broadcastRawPacketEvent", parameters);
      if (!enqueue(2, queueEntry)) {
        printQueueFullError(_out,
                            "Error: Could not queue RPC method call \"broadcastRawPacketEvent\". Queue is full.");
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodeBlueServer::closeClientConnection(const PNodeBlueClientData &client) {
  try {
    if (!client) { return; }
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

void NodeBlueServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  try {
    std::shared_ptr<QueueEntry> queueEntry;
    queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
    if (!queueEntry || queueEntry->clientData->closed) { return; }

    if (BaseLib::HelperFunctions::getTime() - queueEntry->time > 2000) {
      _lastQueueSlowErrorCounter++;
      if (BaseLib::HelperFunctions::getTime() - _lastQueueSlowError > 10000) {
        _lastQueueSlowError = BaseLib::HelperFunctions::getTime();
        _lastQueueSlowErrorCounter = 0;
        _out.printWarning(
            "Warning: Queue entry was queued for "
                + std::to_string(BaseLib::HelperFunctions::getTime() - queueEntry->time)
                + "ms. Either something is hanging or you need to increase your number of processing threads. Messages since last log entry: "
                + std::to_string(_lastQueueSlowErrorCounter));
      }
    }

    if (index == 0) //Request
    {
      if (queueEntry->parameters->size() != 4) {
        _out.printError("Error: Wrong parameter count while calling method " + queueEntry->methodName);
        return;
      }

      auto localMethodIterator = _localRpcMethods.find(queueEntry->methodName);
      if (localMethodIterator != _localRpcMethods.end()) {
        if (GD::bl->debugLevel >= 5) {
          _out.printInfo(
              "Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: "
                  + queueEntry->methodName);
          for (auto i = queueEntry->parameters->at(3)->arrayValue->begin();
               i != queueEntry->parameters->at(3)->arrayValue->end(); ++i) {
            (*i)->print(true, false);
          }
        }
        BaseLib::PVariable
            result = localMethodIterator->second(queueEntry->clientData, queueEntry->parameters->at(3)->arrayValue);
        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Response: ");
          result->print(true, false);
        }

        if (queueEntry->parameters->at(2)->booleanValue) {
          sendResponse(queueEntry->clientData,
                       queueEntry->parameters->at(0),
                       queueEntry->parameters->at(1),
                       result);
        }
        return;
      }

      auto methodIterator = _rpcMethods.find(queueEntry->methodName);
      if (methodIterator == _rpcMethods.end()) {
        if (GD::bl->hgdc && queueEntry->methodName.compare(0, 4, "hgdc") == 0) {
          auto hgdcMethodName = queueEntry->methodName.substr(4);
          hgdcMethodName.at(0) = std::tolower(hgdcMethodName.at(0));
          auto result = GD::bl->hgdc->invoke(hgdcMethodName, queueEntry->parameters->at(3)->arrayValue);
          if (queueEntry->parameters->at(2)->booleanValue) {
            sendResponse(queueEntry->clientData,
                         queueEntry->parameters->at(0),
                         queueEntry->parameters->at(1),
                         result);
          }
          return;
        } else {
          BaseLib::PVariable result = GD::ipcServer->callRpcMethod(_nodeBlueClientInfo,
                                                                   queueEntry->methodName,
                                                                   queueEntry->parameters->at(3)->arrayValue);
          if (queueEntry->parameters->at(2)->booleanValue) {
            sendResponse(queueEntry->clientData,
                         queueEntry->parameters->at(0),
                         queueEntry->parameters->at(1),
                         result);
          }
          return;
        }
      }

      if (GD::bl->debugLevel >= 5) {
        _out.printInfo("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: "
                           + queueEntry->methodName);
        for (auto i = queueEntry->parameters->at(3)->arrayValue->begin();
             i != queueEntry->parameters->at(3)->arrayValue->end(); ++i) {
          (*i)->print(true, false);
        }
      }

      BaseLib::PVariable result = _rpcMethods.at(queueEntry->methodName)->invoke(_nodeBlueClientInfo,
                                                                                 queueEntry->parameters->at(3)->arrayValue);
      if (GD::bl->debugLevel >= 5) {
        _out.printDebug("Response: ");
        result->print(true, false);
      }

      if (queueEntry->parameters->at(2)->booleanValue) {
        sendResponse(queueEntry->clientData,
                     queueEntry->parameters->at(0),
                     queueEntry->parameters->at(1),
                     result);
      }
    } else if (index == 1) //Response
    {
      BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
      int32_t packetId = response->arrayValue->at(0)->integerValue;

      {
        std::lock_guard<std::mutex> responseGuard(queueEntry->clientData->rpcResponsesMutex);
        auto responseIterator = queueEntry->clientData->rpcResponses.find(packetId);
        if (responseIterator != queueEntry->clientData->rpcResponses.end()) {
          PNodeBlueResponseServer element = responseIterator->second;
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
      if (!_shuttingDown && !_flowsRestarting) {
        sendRequest(queueEntry->clientData,
                    queueEntry->methodName,
                    queueEntry->parameters,
                    false);
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

BaseLib::PVariable NodeBlueServer::send(PNodeBlueClientData &clientData, std::vector<char> &data) {
  try {
    int32_t totallySentBytes = 0;
    std::lock_guard<std::mutex> sendGuard(clientData->sendMutex);
    while (totallySentBytes < (signed) data.size()) {
      int32_t sentBytes = ::send(clientData->fileDescriptor->descriptor,
                                 data.data() + totallySentBytes,
                                 data.size() - totallySentBytes,
                                 MSG_NOSIGNAL);
      if (sentBytes == -1) {
        if (errno == EAGAIN) { continue; }
        if (clientData->fileDescriptor->descriptor != -1) {
          _out.printError("Could not send data to client: " + std::to_string(clientData->fileDescriptor->descriptor));
        }
        return BaseLib::Variable::createError(-32500, "Unknown application error.");
      } else if (sentBytes == 0) { return BaseLib::Variable::createError(-32500, "Unknown application error."); }
      totallySentBytes += sentBytes;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::make_shared<BaseLib::Variable>();
}

BaseLib::PVariable NodeBlueServer::sendRequest(PNodeBlueClientData &clientData,
                                               const std::string &methodName,
                                               const BaseLib::PArray &parameters,
                                               bool wait) {
  try {
    lifetick_1_.first = BaseLib::HelperFunctions::getTime();
    lifetick_1_.second = false;

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

    PNodeBlueResponseServer response;
    if (wait) {
      {
        std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
        auto result = clientData->rpcResponses.emplace(packetId, std::make_shared<NodeBlueResponseServer>());
        if (result.second) { response = result.first->second; }
      }
      if (!response) {
        _out.printError("Critical: Could not insert response struct into map.");
        lifetick_1_.second = true;
        return BaseLib::Variable::createError(-32500, "Unknown application error.");
      }
    }

    if (GD::ipcLogger->enabled()) {
      GD::ipcLogger->log(IpcModule::nodeBlue,
                         packetId,
                         clientData->pid,
                         IpcLoggerPacketDirection::toClient,
                         data);
    }

    std::unique_lock<std::mutex> waitLock(clientData->waitMutex);
    BaseLib::PVariable result = send(clientData, data);
    if (result->errorStruct || !wait) {
      std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
      clientData->rpcResponses.erase(packetId);

      lifetick_1_.second = true;

      if (!wait) {
        return std::make_shared<BaseLib::Variable>();
      } else {
        return result;
      }
    }

    int32_t i = 0;
    while (!clientData->requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
      return response->finished || clientData->closed || _stopServer;
    })) {
      i++;
      if (i == 60) {
        _out.printError(
            "Error: Flows client with process ID " + std::to_string(clientData->pid) + " is not responding...");
        break;
      }
    }

    if (!response->finished || (response->response && response->response->arrayValue->size() != 2)
        || response->packetId != packetId) {
      _out.printError(
          "Error: No or invalid response received to RPC request. Method: " + methodName + "."
              + (response->response ? " Response was: " + response->response->print(false, false, true) : ""));
      result = BaseLib::Variable::createError(-1, "No response received.");
    } else { result = response->response->arrayValue->at(1); }

    {
      std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
      clientData->rpcResponses.erase(packetId);
    }

    lifetick_1_.second = true;
    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  lifetick_1_.second = true;
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void NodeBlueServer::sendResponse(PNodeBlueClientData &clientData,
                                  BaseLib::PVariable &scriptId,
                                  BaseLib::PVariable &packetId,
                                  BaseLib::PVariable &variable) {
  try {
    lifetick_2_.first = BaseLib::HelperFunctions::getTime();
    lifetick_2_.second = false;

    BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{scriptId, packetId, variable})));
    std::vector<char> data;
    _rpcEncoder->encodeResponse(array, data);
    if (GD::ipcLogger->enabled()) {
      GD::ipcLogger->log(IpcModule::nodeBlue,
                         packetId->integerValue,
                         clientData->pid,
                         IpcLoggerPacketDirection::toClient,
                         data);
    }
    send(clientData, data);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  lifetick_2_.second = true;
}

void NodeBlueServer::mainThread() {
  try {
    int32_t result = 0;
    while (!_stopServer) {
      if (!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        getFileDescriptor();
        continue;
      }

      timeval timeout{};
      timeout.tv_sec = 0;
      timeout.tv_usec = 100000;
      fd_set readFileDescriptor;
      int32_t maxfd = 0;
      FD_ZERO(&readFileDescriptor);
      {
        std::vector<PNodeBlueClientData> clients;
        {
          std::lock_guard<std::mutex> stateGuard(_stateMutex);
          clients.reserve(_clients.size());
          for (auto &client : _clients) {
            if (client.second->closed) { continue; }
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
          if (descriptor == -1) {
            continue;
          } //Never pass -1 to FD_SET => undefined behaviour. This is just a safety precaution, because the file descriptor manager is locked and descriptors shouldn't be modified.
          FD_SET(descriptor, &readFileDescriptor);
          if (descriptor > maxfd) maxfd = descriptor;
        }
      }

      result = select(maxfd + 1, &readFileDescriptor, nullptr, nullptr, &timeout);
      if (result == 0) {
        if (BaseLib::HelperFunctions::getTime() - _lastGarbageCollection > 60000
            || _clients.size() > GD::bl->settings.nodeBlueServerMaxConnections() * 100 / 112) {
          collectGarbage();
        }
        continue;
      } else if (result == -1) {
        if (errno == EINTR) { continue; }
        _out.printError("Error: select returned -1: " + std::string(strerror(errno)));
        continue;
      }

      if (FD_ISSET(_serverFileDescriptor->descriptor, &readFileDescriptor) && !_shuttingDown) {
        sockaddr_un clientAddress{};
        socklen_t addressSize = sizeof(addressSize);
        std::shared_ptr<BaseLib::FileDescriptor>
            clientFileDescriptor = GD::bl->fileDescriptorManager.add(accept4(_serverFileDescriptor->descriptor,
                                                                             (struct sockaddr *) &clientAddress,
                                                                             &addressSize,
                                                                             SOCK_CLOEXEC));
        if (!clientFileDescriptor || clientFileDescriptor->descriptor == -1) { continue; }
        _out.printInfo("Info: Connection accepted. Client number: " + std::to_string(clientFileDescriptor->id));

        if (_clients.size() > GD::bl->settings.nodeBlueServerMaxConnections()) {
          collectGarbage();
          if (_clients.size() > GD::bl->settings.nodeBlueServerMaxConnections()) {
            _out.printError(
                "Error: There are too many clients connected to me. Closing connection. You can increase the number of allowed connections in main.conf.");
            GD::bl->fileDescriptorManager.close(clientFileDescriptor);
            continue;
          }
        }

        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        if (_shuttingDown) {
          GD::bl->fileDescriptorManager.close(clientFileDescriptor);
          continue;
        }
        PNodeBlueClientData clientData = std::make_shared<NodeBlueClientData>(clientFileDescriptor);
        clientData->id = _currentClientId++;
        _clients[clientData->id] = clientData;
        continue;
      }

      PNodeBlueClientData clientData;
      {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        for (auto i = _clients.begin(); i != _clients.end(); ++i) {
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

      if (clientData) { readClient(clientData); }
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

PNodeBlueProcess NodeBlueServer::getFreeProcess(uint32_t maxThreadCount) {
  try {
    if (GD::bl->settings.maxNodeThreadsPerProcess() != -1
        && maxThreadCount > (unsigned) GD::bl->settings.maxNodeThreadsPerProcess()) {
      _out.printError(
          "Error: Could not get flow process, because maximum number of threads in flow is greater than the number of threads allowed per flow process.");
      return PNodeBlueProcess();
    }

    std::lock_guard<std::mutex> processGuard(_newProcessMutex);
    {
      std::lock_guard<std::mutex> processGuard2(_processMutex);
      for (auto &process : _processes) {
        if (process.second->getClientData()->closed) { continue; }
        if (GD::bl->settings.maxNodeThreadsPerProcess() == -1 || process.second->nodeThreadCount() + maxThreadCount
            <= (unsigned) GD::bl->settings.maxNodeThreadsPerProcess()) {
          process.second->lastExecution = BaseLib::HelperFunctions::getTime();
          return process.second;
        }
      }
    }
    _out.printInfo("Info: Spawning new flows process.");
    PNodeBlueProcess process(new NodeBlueProcess());
    std::vector<std::string> arguments{"-c", GD::configPath, "-rl"};
    if (GD::bl->settings.nodeBlueManualClientStart()) {
      pid_t currentProcessId = _manualClientCurrentProcessId++;
      process->setPid(currentProcessId);
      {
        std::lock_guard<std::mutex> unconnectedProcessesGuard(_unconnectedProcessesMutex);
        _unconnectedProcesses.push(currentProcessId);
      }
    } else {
      process->setPid(BaseLib::ProcessManager::system(GD::executablePath + "/" + GD::executableFile,
                                                      arguments,
                                                      _bl->fileDescriptorManager.getMax()));
    }
    if (process->getPid() != -1) {
      {
        std::lock_guard<std::mutex> processGuard2(_processMutex);
        _processes[process->getPid()] = process;
      }

      std::unique_lock<std::mutex> requestLock(_processRequestMutex);
      process->requestConditionVariable.wait_for(requestLock,
                                                 std::chrono::milliseconds(120000),
                                                 [&] {
                                                   return (bool) (process->getClientData()) || process->getExited();
                                                 });

      if (!process->getClientData()) {
        std::lock_guard<std::mutex> processGuard2(_processMutex);
        _processes.erase(process->getPid());
        _out.printError("Error: Could not start new flows process.");
        return PNodeBlueProcess();
      }
      _out.printInfo("Info: Flows process successfully spawned. Process id is " + std::to_string(process->getPid())
                         + ". Client id is: " + std::to_string(process->getClientData()->id) + ".");
      if (_nodeEventsEnabled) {
        BaseLib::PArray parameters(new BaseLib::Array());
        sendRequest(process->getClientData(), "enableNodeEvents", parameters, false);
      }
      return process;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return PNodeBlueProcess();
}

void NodeBlueServer::readClient(PNodeBlueClientData &clientData) {
  try {
    int32_t processedBytes = 0;
    int32_t bytesRead = 0;
    bytesRead = read(clientData->fileDescriptor->descriptor, &(clientData->buffer[0]), clientData->buffer.size());
    if (bytesRead <= 0) //read returns 0, when connection is disrupted.
    {
      _out.printInfo(
          "Info: Connection to flows server's client number " + std::to_string(clientData->fileDescriptor->id)
              + " closed.");
      closeClientConnection(clientData);
      return;
    }

    if (bytesRead > (signed) clientData->buffer.size()) { bytesRead = clientData->buffer.size(); }

    try {
      processedBytes = 0;
      while (processedBytes < bytesRead) {
        processedBytes +=
            clientData->binaryRpc->process(&(clientData->buffer[processedBytes]), bytesRead - processedBytes);
        if (clientData->binaryRpc->isFinished()) {
          if (GD::ipcLogger->enabled()) {
            if (clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request) {
              std::string methodName;
              BaseLib::PArray request = _rpcDecoder->decodeRequest(clientData->binaryRpc->getData(), methodName);
              GD::ipcLogger->log(IpcModule::nodeBlue,
                                 request->at(1)->integerValue,
                                 clientData->pid,
                                 IpcLoggerPacketDirection::toServer,
                                 clientData->binaryRpc->getData());
            } else {
              BaseLib::PVariable response = _rpcDecoder->decodeResponse(clientData->binaryRpc->getData());
              GD::ipcLogger->log(IpcModule::nodeBlue,
                                 response->arrayValue->at(0)->integerValue,
                                 clientData->pid,
                                 IpcLoggerPacketDirection::toServer,
                                 clientData->binaryRpc->getData());
            }
          }

          if (clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request) {
            std::string methodName;
            BaseLib::PArray parameters = _rpcDecoder->decodeRequest(clientData->binaryRpc->getData(), methodName);

            if (methodName == "registerFlowsClient" && parameters->size() == 4) {
              BaseLib::PVariable result = registerFlowsClient(clientData, parameters->at(3)->arrayValue);
              sendResponse(clientData, parameters->at(0), parameters->at(1), result);
            } else {
              std::shared_ptr<BaseLib::IQueueEntry>
                  queueEntry = std::make_shared<QueueEntry>(clientData, methodName, parameters);
              if (!enqueue(0, queueEntry)) {
                printQueueFullError(_out,
                                    "Error: Could not queue incoming RPC method call \"" + methodName
                                        + "\". Queue is full.");
              }
            }
          } else {
            std::shared_ptr<BaseLib::IQueueEntry>
                queueEntry = std::make_shared<QueueEntry>(clientData, clientData->binaryRpc->getData());
            if (!enqueue(1, queueEntry)) {
              printQueueFullError(_out,
                                  "Error: Could not queue RPC response. Queue is full.");
            }
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

bool NodeBlueServer::getFileDescriptor(bool deleteOldSocket) {
  try {
    if (!BaseLib::Io::directoryExists(GD::bl->settings.socketPath())) {
      if (errno == ENOENT) {
        _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath()
                               + " does not exist. Please create it before starting Homegear otherwise flows won't work.");
      } else {
        _out.printCritical("Critical: Error reading information of directory " + GD::bl->settings.socketPath()
                               + ". The script engine won't work: " + strerror(errno));
      }
      _stopServer = true;
      return false;
    }
    bool isDirectory = false;
    int32_t result = BaseLib::Io::isDirectory(GD::bl->settings.socketPath(), isDirectory);
    if (result != 0 || !isDirectory) {
      _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath()
                             + " does not exist. Please create it before starting Homegear otherwise flows won't work.");
      _stopServer = true;
      return false;
    }
    if (deleteOldSocket) {
      if (unlink(_socketPath.c_str()) == -1 && errno != ENOENT) {
        _out.printCritical("Critical: Couldn't delete existing socket: " + _socketPath
                               + ". Please delete it manually. Flows won't work. Error: " + strerror(errno));
        return false;
      }
    } else if (BaseLib::Io::fileExists(_socketPath)) { return false; }

    _serverFileDescriptor =
        GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
    if (_serverFileDescriptor->descriptor == -1) {
      _out.printCritical(
          "Critical: Couldn't create socket: " + _socketPath + ". Flows won't work. Error: " + strerror(errno));
      return false;
    }
    int32_t reuseAddress = 1;
    if (setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void *) &reuseAddress, sizeof(int32_t))
        == -1) {
      GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
      _out.printCritical(
          "Critical: Couldn't set socket options: " + _socketPath + ". Flows won't work correctly. Error: "
              + strerror(errno));
      return false;
    }
    sockaddr_un serverAddress{};
    serverAddress.sun_family = AF_LOCAL;
    //104 is the size on BSD systems - slightly smaller than in Linux
    if (_socketPath.length() > 104) {
      //Check for buffer overflow
      _out.printCritical("Critical: Socket path is too long.");
      return false;
    }
    strncpy(serverAddress.sun_path, _socketPath.c_str(), 104);
    serverAddress.sun_path[103] = 0; //Just to make sure the string is null terminated.
    bool bound = (bind(_serverFileDescriptor->descriptor.load(),
                       (sockaddr * ) & serverAddress,
                       strlen(serverAddress.sun_path) + 1 + sizeof(serverAddress.sun_family)) != -1);
    if (_serverFileDescriptor->descriptor == -1 || !bound
        || listen(_serverFileDescriptor->descriptor, _backlog) == -1) {
      GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
      _out.printCritical("Critical: Flows server could not start listening. Error: " + std::string(strerror(errno)));
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

void NodeBlueServer::startFlow(PFlowInfoServer &flowInfo,
                               const std::unordered_map<std::string, BaseLib::PVariable> &flowNodes) {
  try {
    if (_shuttingDown) { return; }
    PNodeBlueProcess process = getFreeProcess(flowInfo->maxThreadCount);
    if (!process) {
      GD::rpcClient->broadcastNodeEvent("global",
                                        "flowStartError",
                                        std::make_shared<BaseLib::Variable>(flowInfo->nodeBlueId),
                                        false);

      _out.printError("Error: Could not get free process. Not executing flow.");
      flowInfo->exitCode = -1;
      return;
    }

    {
      std::lock_guard<std::mutex> processGuard(_currentFlowIdMutex);
      while (flowInfo->id == 0) { flowInfo->id = _currentFlowId++; }
    }

    _out.printInfo("Info: Starting flow with id " + std::to_string(flowInfo->id) + ".");
    process->registerFlow(flowInfo->id, flowInfo);

    PNodeBlueClientData clientData = process->getClientData();

    {
      std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
      for (auto &node : flowNodes) {
        auto type = node.second->structValue->at("type")->stringValue;
        _nodeInfoMap.emplace(node.first, NodeServerInfo{clientData->id, type, _nodeManager->getNodeCodeType(type)});
      }
    }

    {
      std::lock_guard<std::mutex> flowClientIdMapGuard(_flowClientIdMapMutex);
      _flowClientIdMap.emplace(flowInfo->nodeBlueId, clientData->id);
    }

    BaseLib::PArray parameters(new BaseLib::Array{
        std::make_shared<BaseLib::Variable>(flowInfo->id),
        flowInfo->flow
    });

    BaseLib::PVariable result = sendRequest(clientData, "startFlow", parameters, true);
    if (result->errorStruct) {
      _out.printError("Error: Could not execute flow: " + result->structValue->at("faultString")->stringValue);
      process->invokeFlowFinished(flowInfo->id, result->structValue->at("faultCode")->integerValue);
      process->unregisterFlow(flowInfo->id);

      {
        std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
        for (auto &node : flowNodes) {
          _nodeInfoMap.erase(node.first);
        }
      }

      GD::rpcClient->broadcastNodeEvent("global",
                                        "flowStartError",
                                        std::make_shared<BaseLib::Variable>(flowInfo->nodeBlueId),
                                        false);

      return;
    }
    flowInfo->started = true;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

BaseLib::PVariable NodeBlueServer::addNodesToFlow(const std::string &tab,
                                                  const std::string &tag,
                                                  const BaseLib::PVariable &nodes) {
  try {
    std::string flowsFile = _bl->settings.nodeBlueDataPath() + "flows.json";
    std::vector<char> fileContent;

    {
      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      if (BaseLib::Io::fileExists(flowsFile)) { fileContent = BaseLib::Io::getBinaryFileContent(flowsFile); }
      else {
        std::string tempString = R"([])";
        fileContent.insert(fileContent.end(), tempString.begin(), tempString.end());
      }
    }

    BaseLib::PVariable flowsJson = _jsonDecoder->decode(fileContent);

    std::unordered_map<std::string, std::string> nodeIdReplacementMap;
    auto newFlow = FlowParser::addNodesToFlow(flowsJson, tab, tag, nodes, nodeIdReplacementMap);
    if (newFlow) {
      backupFlows();

      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      std::string filename = _bl->settings.nodeBlueDataPath() + "flows.json";
      fileContent.clear();
      _jsonEncoder->encode(newFlow, fileContent);
      BaseLib::Io::writeFile(filename, fileContent, fileContent.size());

      { //Notify frontend of changed flow
        std::vector<char> md5;
        BaseLib::Security::Hash::md5(fileContent, md5);
        std::string md5String = BaseLib::HelperFunctions::getHexString(md5);
        BaseLib::HelperFunctions::toLower(md5String);
        auto eventData = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        eventData->structValue->emplace("revision", std::make_shared<BaseLib::Variable>(md5String));
        GD::rpcClient->broadcastNodeEvent("", "notification/runtime-deploy", eventData, false);
      }

      auto nodeIdReplacementStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      for (auto &entry : nodeIdReplacementMap) {
        nodeIdReplacementStruct->structValue->emplace(entry.first, std::make_shared<BaseLib::Variable>(entry.second));
      }

      return nodeIdReplacementStruct;
    } else {
      return std::make_shared<BaseLib::Variable>(false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::removeNodesFromFlow(const std::string &tab, const std::string &tag) {
  try {
    std::string flowsFile = _bl->settings.nodeBlueDataPath() + "flows.json";
    std::vector<char> fileContent;

    {
      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      if (BaseLib::Io::fileExists(flowsFile)) { fileContent = BaseLib::Io::getBinaryFileContent(flowsFile); }
      else {
        std::string
            tempString = R"([{"id":")" + FlowParser::generateRandomId() + R"(","label":"Flow 1","type":"tab"}])";
        fileContent.insert(fileContent.end(), tempString.begin(), tempString.end());
      }
    }

    BaseLib::PVariable flowsJson = _jsonDecoder->decode(fileContent);

    auto newFlow = FlowParser::removeNodesFromFlow(flowsJson, tab, tag);
    if (newFlow) {
      backupFlows();

      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      std::string filename = _bl->settings.nodeBlueDataPath() + "flows.json";
      fileContent.clear();
      _jsonEncoder->encode(newFlow, fileContent);
      BaseLib::Io::writeFile(filename, fileContent, fileContent.size());
      return std::make_shared<BaseLib::Variable>(true);
    } else {
      return std::make_shared<BaseLib::Variable>(false);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::flowHasTag(const std::string &tab, const std::string &tag) {
  try {
    std::string flowsFile = _bl->settings.nodeBlueDataPath() + "flows.json";
    std::vector<char> fileContent;

    {
      std::lock_guard<std::mutex> flowsFileGuard(_flowsFileMutex);
      if (BaseLib::Io::fileExists(flowsFile)) { fileContent = BaseLib::Io::getBinaryFileContent(flowsFile); }
      else {
        std::string
            tempString = R"([{"id":")" + FlowParser::generateRandomId() + R"(","label":"Flow 1","type":"tab"}])";
        fileContent.insert(fileContent.end(), tempString.begin(), tempString.end());
      }
    }

    BaseLib::PVariable flowsJson = _jsonDecoder->decode(fileContent);

    return std::make_shared<BaseLib::Variable>(FlowParser::flowHasTag(flowsJson, tab, tag));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::executePhpNodeBaseMethod(BaseLib::PArray &parameters) {
  try {
    PNodeBlueClientData clientData;
    int32_t clientId = 0;
    {
      std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
      auto nodeInfoIterator = _nodeInfoMap.find(parameters->at(0)->stringValue);
      if (nodeInfoIterator == _nodeInfoMap.end()) {
        return BaseLib::Variable::createError(-1,
                                              "Node is unknown in Node-BLUE server (2).");
      }
      clientId = nodeInfoIterator->second.clientId;
    }

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(clientId);
      if (clientIterator == _clients.end()) {
        return BaseLib::Variable::createError(-1,
                                              "Node is unknown in Node-BLUE server (3).");
      }
      clientData = clientIterator->second;
    }

    return sendRequest(clientData, "executePhpNodeBaseMethod", parameters, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

std::string NodeBlueServer::getNodeBlueFormatFromVariableType(const BaseLib::PVariable &variable) {
  std::string format;
  switch (variable->type) {
    case BaseLib::VariableType::tArray:format = "array[" + std::to_string(variable->arrayValue->size()) + "]";
      break;
    case BaseLib::VariableType::tBoolean:format = "boolean";
      break;
    case BaseLib::VariableType::tFloat:format = "number";
      break;
    case BaseLib::VariableType::tInteger:format = "number";
      break;
    case BaseLib::VariableType::tInteger64:format = "number";
      break;
    case BaseLib::VariableType::tString:format = "string[" + std::to_string(variable->stringValue.size()) + "]";
      if (variable->stringValue.size() > 1000) {
        variable->stringValue = variable->stringValue.substr(0, 1000) + "...";
      }
      variable->stringValue = BaseLib::HelperFunctions::stripNonPrintable(variable->stringValue);
      break;
    case BaseLib::VariableType::tStruct:format = "Object";
      break;
    case BaseLib::VariableType::tBase64:format = "string[" + std::to_string(variable->stringValue.size()) + "]";
      if (variable->stringValue.size() > 1000) {
        variable->stringValue = variable->stringValue.substr(0, 1000) + "...";
      }
      variable->stringValue = BaseLib::HelperFunctions::stripNonPrintable(variable->stringValue);
      break;
    case BaseLib::VariableType::tVariant:break;
    case BaseLib::VariableType::tBinary:break;
    case BaseLib::VariableType::tVoid:format = "null";
      break;
  }
  return format;
}

void NodeBlueServer::frontendNodeEventLog(const std::string &message) {
  try {
    if (!_nodeEventsEnabled) { return; }
    auto value = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    value->structValue->emplace("ts", std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::getTime()));
    value->structValue->emplace("data", std::make_shared<BaseLib::Variable>(message));
    GD::rpcClient->broadcastNodeEvent("", "event-log/flowsStarted", value, false);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

// {{{ RPC methods
BaseLib::PVariable NodeBlueServer::registerFlowsClient(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    pid_t pid = parameters->at(0)->integerValue;
    if (GD::bl->settings.nodeBlueManualClientStart()) {
      std::lock_guard<std::mutex> unconnectedProcessesGuard(_unconnectedProcessesMutex);
      if (!_unconnectedProcesses.empty()) {
        pid = _unconnectedProcesses.front();
        _unconnectedProcesses.pop();
      }
    }
    PNodeBlueProcess process;
    {
      std::lock_guard<std::mutex> processGuard(_processMutex);
      auto processIterator = _processes.find(pid);
      if (processIterator == _processes.end()) {
        _out.printError("Error: Cannot register client. No process with pid " + std::to_string(pid) + " found.");
        return BaseLib::Variable::createError(-1, "No matching process found.");
      }
      process = processIterator->second;
    }
    if (process->getClientData()) {
      _out.printError("Error: Cannot register client, because it already is registered.");
      return std::make_shared<BaseLib::Variable>();
    }
    clientData->pid = pid;
    process->setClientData(clientData);
    std::unique_lock<std::mutex> requestLock(_processRequestMutex);
    requestLock.unlock();
    process->requestConditionVariable.notify_all();
    _out.printInfo("Info: Client with pid " + std::to_string(pid) + " successfully registered.");
    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::errorEvent(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly three parameters.");
    }

    broadcastError(parameters->at(0)->stringValue, parameters->at(1)->integerValue, parameters->at(2));

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

#ifndef NO_SCRIPTENGINE
BaseLib::PVariable NodeBlueServer::executePhpNode(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3 && parameters->size() != 4) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly three or four parameters.");
    }

    std::string filename;
    BaseLib::ScriptEngine::PScriptInfo scriptInfo;
    bool wait = false;
    if (parameters->size() == 4) {
      filename = parameters->at(1)->stringValue.substr(parameters->at(1)->stringValue.find_last_of('/') + 1);
      scriptInfo =
          std::make_shared<BaseLib::ScriptEngine::ScriptInfo>(BaseLib::ScriptEngine::ScriptInfo::ScriptType::simpleNode,
                                                              parameters->at(0),
                                                              parameters->at(1)->stringValue,
                                                              filename,
                                                              parameters->at(2)->integerValue,
                                                              parameters->at(3));
      auto internalMessagesIterator = parameters->at(3)->structValue->find("_internal");
      if (internalMessagesIterator != parameters->at(3)->structValue->end()) {
        auto synchronousOutputIterator = internalMessagesIterator->second->structValue->find("synchronousOutput");
        if (synchronousOutputIterator != internalMessagesIterator->second->structValue->end()) {
          wait = synchronousOutputIterator->second->booleanValue;
        }
      }
    } else {
      filename = parameters->at(2)->stringValue.substr(parameters->at(2)->stringValue.find_last_of('/') + 1);
      uint32_t threadCount = 0;
      auto threadCountIterator = _maxThreadCounts.find(parameters->at(0)->stringValue);
      if (threadCountIterator != _maxThreadCounts.end()) { threadCount = threadCountIterator->second; }
      scriptInfo =
          std::make_shared<BaseLib::ScriptEngine::ScriptInfo>(BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode,
                                                              parameters->at(1),
                                                              parameters->at(2)->stringValue,
                                                              filename,
                                                              threadCount);
    }
    GD::scriptEngineServer->executeScript(scriptInfo, wait);
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

BaseLib::PVariable NodeBlueServer::executePhpNodeMethod(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly three or four parameters.");
    }

    return GD::scriptEngineServer->executePhpNodeMethod(parameters);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

#endif

BaseLib::PVariable NodeBlueServer::frontendEventLog(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) { return BaseLib::Variable::createError(-1, "Method expects exactly two parameter."); }

    if (parameters->at(0)->stringValue.empty()) { frontendNodeEventLog(parameters->at(1)->stringValue); }
    else { frontendNodeEventLog("Node " + parameters->at(0)->stringValue + ": " + parameters->at(1)->stringValue); }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::getCredentials(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) { return BaseLib::Variable::createError(-1, "Method expects exactly one parameter."); }

    return _nodeBlueCredentials->getCredentials(parameters->at(0)->stringValue, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::invokeNodeMethod(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 4) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly four parameters.");
    }

    PNodeBlueClientData nodeClientData;
    NodeServerInfo nodeInfo;
    {
      std::lock_guard<std::mutex> nodeInfoMapGuard(_nodeInfoMapMutex);
      auto nodeInfoIterator = _nodeInfoMap.find(parameters->at(0)->stringValue);
      if (nodeInfoIterator == _nodeInfoMap.end()) {
        return BaseLib::Variable::createError(-1,
                                              "Node is unknown in Node-BLUE server (1).");
      }
      nodeInfo = nodeInfoIterator->second;
    }

    if (nodeInfo.nodeCodeType == NodeManager::NodeCodeType::javascript
        || nodeInfo.nodeCodeType == NodeManager::NodeCodeType::javascriptEncrypted) {
      return _nodepink->invoke("invokeNodeMethod", parameters);
    }

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      auto clientIterator = _clients.find(nodeInfo.clientId);
      if (clientIterator == _clients.end()) {
        return BaseLib::Variable::createError(-32501,
                                              "Node process not found.");
      }
      nodeClientData = clientIterator->second;
    }

    return sendRequest(nodeClientData, "invokeNodeMethod", parameters, parameters->at(3)->booleanValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::invokeIpcProcessMethod(PNodeBlueClientData &clientData,
                                                          BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly three parameters.");
    }
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger
        && parameters->at(0)->type != BaseLib::VariableType::tInteger64) {
      return BaseLib::Variable::createError(-1,
                                            "First parameter is not of type Integer.");
    }
    if (parameters->at(1)->type != BaseLib::VariableType::tString) {
      return BaseLib::Variable::createError(-1,
                                            "Second parameter is not of type String.");
    }
    if (parameters->at(2)->type != BaseLib::VariableType::tArray) {
      return BaseLib::Variable::createError(-1,
                                            "Third parameter is not of type Array.");
    }

    return GD::ipcServer->callProcessRpcMethod(parameters->at(0)->integerValue64,
                                               _nodeBlueClientInfo,
                                               parameters->at(1)->stringValue,
                                               parameters->at(2)->arrayValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::nodeEvent(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 4) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly three parameters.");
    }

    if (BaseLib::HelperFunctions::getTime() - _lastNodeEvent >= 10000) {
      _lastNodeEvent = BaseLib::HelperFunctions::getTime();
      _nodeEventCounter = 0;
    }

    if ((parameters->at(1)->stringValue.compare(0, 14, "highlightNode/") == 0
        || parameters->at(1)->stringValue.compare(0, 14, "highlightLink/") == 0)
        && _nodeEventCounter > _bl->settings.nodeBlueEventLimit1()) {
      return std::make_shared<BaseLib::Variable>();
    } else if (parameters->at(1)->stringValue != "debug" && _nodeEventCounter > _bl->settings.nodeBlueEventLimit2()) {
      return std::make_shared<BaseLib::Variable>();
    } else if (_nodeEventCounter
        > _bl->settings.nodeBlueEventLimit3()) { return std::make_shared<BaseLib::Variable>(); }
    _nodeEventCounter++;

    GD::rpcClient->broadcastNodeEvent(parameters->at(0)->stringValue,
                                      parameters->at(1)->stringValue,
                                      parameters->at(2),
                                      parameters->at(3)->booleanValue);
    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::nodeBlueVariableEvent(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 5) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly five parameters.");
    }

    auto variables = std::make_shared < std::vector < std::string >> ();
    variables->emplace_back(parameters->at(3)->stringValue);
    auto values = std::make_shared < std::vector < BaseLib::PVariable >> ();
    values->emplace_back(parameters->at(4));
    GD::variableProfileManager->variableEvent(parameters->at(0)->stringValue,
                                              parameters->at(1)->integerValue64,
                                              parameters->at(2)->integerValue,
                                              variables,
                                              values);
    GD::rpcClient->broadcastEvent(parameters->at(0)->stringValue,
                                  parameters->at(1)->integerValue64,
                                  parameters->at(2)->integerValue,
                                  "",
                                  variables,
                                  values);
    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::nodeRedNodeInput(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 5) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly four parameters.");
    }

    _nodepink->nodeInput(parameters->at(0)->stringValue,
                         parameters->at(1),
                         parameters->at(2)->integerValue,
                         parameters->at(3),
                         parameters->at(4)->booleanValue);
    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::registerUiNodeRoles(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly two parameters.");
    }
    if (parameters->at(0)->type != BaseLib::VariableType::tString) {
      return BaseLib::Variable::createError(-1,
                                            "First parameter is not of type String.");
    }
    if (parameters->at(1)->type != BaseLib::VariableType::tStruct) {
      return BaseLib::Variable::createError(-1,
                                            "Second parameter is not of type Struct.");
    }

    auto nodeId = parameters->at(0)->stringValue;
    auto roles = parameters->at(1)->structValue;

    std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMapMutex);
    auto nodeInfoIterator = _nodeInfoMap.find(nodeId);
    if (nodeInfoIterator == _nodeInfoMap.end()) { return std::make_shared<BaseLib::Variable>(); }

    for (auto &role : *roles) {
      if (role.first.at(0) == 'o') {
        nodeInfoIterator->second.outputRoles.emplace(BaseLib::Math::getUnsignedNumber(role.first.substr(1)),
                                                     role.second->integerValue);
      }
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::setCredentials(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly two parameters.");
    }

    return _nodeBlueCredentials->setCredentials(parameters->at(0)->stringValue, parameters->at(1));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable NodeBlueServer::setCredentialTypes(PNodeBlueClientData &clientData, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) {
      return BaseLib::Variable::createError(-1,
                                            "Method expects exactly two parameters.");
    }

    _nodeManager->registerCredentialTypes(parameters->at(0)->stringValue, parameters->at(1), false);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}
