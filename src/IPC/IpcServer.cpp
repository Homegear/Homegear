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

#include "IpcServer.h"

#include <memory>
#include "../GD/GD.h"
#include "../CLI/CliServer.h"
#include "../RPC/RpcMethods/BuildingRpcMethods.h"
#include "../RPC/RpcMethods/UiRpcMethods.h"
#include "../RPC/RpcMethods/UiNotificationsRpcMethods.h"
#include "../RPC/RpcMethods/VariableProfileRpcMethods.h"
#include "../RPC/RpcMethods/NodeBlueRpcMethods.h"
#include "../RPC/RpcMethods/MaintenanceRpcMethods.h"
#include "../RPC/RpcMethods/BuildingPartRpcMethods.h"

namespace Homegear {

IpcServer::IpcServer() : IQueue(GD::bl.get(), 3, 100000) {
  _out.init(GD::bl.get());
  _out.setPrefix("IPC Server: ");

  lifetick_1_.first = 0;
  lifetick_1_.second = true;
  lifetick_2_.first = 0;
  lifetick_2_.second = true;

  _rpcDecoder = std::make_unique<BaseLib::Rpc::RpcDecoder>(GD::bl.get(), false, false);
  _rpcEncoder = std::make_unique<BaseLib::Rpc::RpcEncoder>(GD::bl.get(), true, true);
  _dummyClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
  _dummyClientInfo->ipcServer = true;
  _dummyClientInfo->initInterfaceId = "ipcServer";
  _dummyClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  std::vector<uint64_t> groups{3};
  _dummyClientInfo->acls->fromGroups(groups);
  _dummyClientInfo->user = "SYSTEM (3)";

  _rpcMethods.emplace("devTest", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCDevTest()));
  _rpcMethods.emplace("system.getCapabilities", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemGetCapabilities()));
  _rpcMethods.emplace("system.listMethods", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemListMethods()));
  _rpcMethods.emplace("system.methodHelp", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodHelp()));
  _rpcMethods.emplace("system.methodSignature", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMethodSignature()));
  _rpcMethods.emplace("system.multicall", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSystemMulticall()));
  _rpcMethods.emplace("acknowledgeGlobalServiceMessage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAcknowledgeGlobalServiceMessage()));
  _rpcMethods.emplace("activateLinkParamset", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCActivateLinkParamset()));
  _rpcMethods.emplace("addDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCAddDevice()));
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
  _rpcMethods.emplace("getLastEvents", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLastEvents()));
  _rpcMethods.emplace("getInstallMode", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetInstallMode()));
  _rpcMethods.emplace("getInstanceId", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetInstanceId()));
  _rpcMethods.emplace("getKeyMismatchDevice", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetKeyMismatchDevice()));
  _rpcMethods.emplace("getLinkInfo", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkInfo()));
  _rpcMethods.emplace("getLinkPeers", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinkPeers()));
  _rpcMethods.emplace("getLinks", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetLinks()));
  _rpcMethods.emplace("getMetadata", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetMetadata()));
  _rpcMethods.emplace("getName", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCGetName()));
  _rpcMethods.emplace("getNodeBlueDataKeys", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetNodeBlueDataKeys()));
  _rpcMethods.emplace("getNodeData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetNodeData()));
  _rpcMethods.emplace("getFlowData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetFlowData()));
  _rpcMethods.emplace("getGlobalData", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetGlobalData()));
  _rpcMethods.emplace("getNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCGetNodeVariable()));
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
  _rpcMethods.emplace("listFamilies", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListFamilies()));
  _rpcMethods.emplace("listInterfaces", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListInterfaces()));
  _rpcMethods.emplace("listKnownDeviceTypes", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCListKnownDeviceTypes()));
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
  _rpcMethods.emplace("setGlobalServiceMessage", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new Rpc::RPCSetGlobalServiceMessage()));
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
  _rpcMethods.emplace("setNodeVariable", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCSetNodeVariable()));
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
    _rpcMethods.emplace("addNodesToFlow", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCAddNodesToFlow()));
    _rpcMethods.emplace("flowHasTag", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCFlowHasTag()));
    _rpcMethods.emplace("nodeBlueIsReady", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCNodeBlueIsReady()));
    _rpcMethods.emplace("nodeEvent", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCNodeEvent()));
    _rpcMethods.emplace("nodeLog", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCNodeLog()));
    _rpcMethods.emplace("nodeOutput", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCNodeOutput()));
    _rpcMethods.emplace("removeNodesFromFlow", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRemoveNodesFromFlow()));
    _rpcMethods.emplace("restartFlows", std::shared_ptr<BaseLib::Rpc::RpcMethod>(new RpcMethods::RPCRestartFlows()));
  }

  { // Buildings
    _rpcMethods.emplace("addBuildingPartToBuilding", std::make_shared<RpcMethods::RPCAddBuildingPartToBuilding>());
    _rpcMethods.emplace("addStoryToBuilding", std::make_shared<RpcMethods::RPCAddStoryToBuilding>());
    _rpcMethods.emplace("createBuilding", std::make_shared<RpcMethods::RPCCreateBuilding>());
    _rpcMethods.emplace("deleteBuilding", std::make_shared<RpcMethods::RPCDeleteBuilding>());
    _rpcMethods.emplace("getBuildingPartsInBuilding", std::make_shared<RpcMethods::RPCGetBuildingPartsInBuilding>());
    _rpcMethods.emplace("getStoriesInBuilding", std::make_shared<RpcMethods::RPCGetStoriesInBuilding>());
    _rpcMethods.emplace("getBuildingMetadata", std::make_shared<RpcMethods::RPCGetBuildingMetadata>());
    _rpcMethods.emplace("getBuildings", std::make_shared<RpcMethods::RPCGetBuildings>());
    _rpcMethods.emplace("removeBuildingPartFromBuilding", std::make_shared<RpcMethods::RPCRemoveBuildingPartFromBuilding>());
    _rpcMethods.emplace("removeStoryFromBuilding", std::make_shared<RpcMethods::RPCRemoveStoryFromBuilding>());
    _rpcMethods.emplace("setBuildingMetadata", std::make_shared<RpcMethods::RPCSetBuildingMetadata>());
    _rpcMethods.emplace("updateBuilding", std::make_shared<RpcMethods::RPCUpdateBuilding>());
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
    _rpcMethods.emplace("removeChannelFromBuildingPart", std::make_shared<RpcMethods::RPCRemoveChannelFromBuildingPart>());
    _rpcMethods.emplace("removeDeviceFromBuildingPart", std::make_shared<RpcMethods::RPCRemoveDeviceFromBuildingPart>());
    _rpcMethods.emplace("removeVariableFromBuildingPart", std::make_shared<RpcMethods::RPCRemoveVariableFromBuildingPart>());
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
    _rpcMethods.emplace("getUiElementTemplate", std::make_shared<RpcMethods::RpcGetUiElementTemplate>());
    _rpcMethods.emplace("requestUiRefresh", std::make_shared<RpcMethods::RpcRequestUiRefresh>());
    _rpcMethods.emplace("removeUiElement", std::make_shared<RpcMethods::RpcRemoveUiElement>());
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

  { // Maintenance
    _rpcMethods.emplace("enableMaintenanceMode", std::make_shared<RpcMethods::RpcEnableMaintenanceMode>());
    _rpcMethods.emplace("disableMaintenanceMode", std::make_shared<RpcMethods::RpcDisableMaintenanceMode>());
  }

  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("getHomegearPid",
                                                                                                                                                               std::bind(&IpcServer::getHomegearPid,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("setPid",
                                                                                                                                                               std::bind(&IpcServer::setPid,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("getClientId",
                                                                                                                                                               std::bind(&IpcServer::getClientId,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("registerRpcMethod",
                                                                                                                                                               std::bind(&IpcServer::registerRpcMethod,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("cliGeneralCommand",
                                                                                                                                                               std::bind(&IpcServer::cliGeneralCommand,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("cliFamilyCommand",
                                                                                                                                                               std::bind(&IpcServer::cliFamilyCommand,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("cliPeerCommand",
                                                                                                                                                               std::bind(&IpcServer::cliPeerCommand,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("getNodeCredentials",
                                                                                                                                                               std::bind(&IpcServer::getNodeCredentials,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("setNodeCredentials",
                                                                                                                                                               std::bind(&IpcServer::setNodeCredentials,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("setNodeCredentialTypes",
                                                                                                                                                               std::bind(&IpcServer::setNodeCredentialTypes,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("noderedEvent",
                                                                                                                                                               std::bind(&IpcServer::noderedEvent,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
  _localRpcMethods.insert(std::pair<std::string, std::function<BaseLib::PVariable(PIpcClientData &clientData, int32_t scriptId, BaseLib::PArray &parameters)>>("ptyOutput",
                                                                                                                                                               std::bind(&IpcServer::ptyOutput,
                                                                                                                                                                         this,
                                                                                                                                                                         std::placeholders::_1,
                                                                                                                                                                         std::placeholders::_2,
                                                                                                                                                                         std::placeholders::_3)));
}

IpcServer::~IpcServer() {
  if (!_stopServer) stop();
}

std::vector<PIpcClientData> IpcServer::GetClientsForRpcMethod(const std::string &rpc_method) {
  try {
    std::vector<PIpcClientData> clientData;

    {
      std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
      auto clientIterator = _clientsByRpcMethods.find(rpc_method);
      if (clientIterator == _clientsByRpcMethods.end()) {
        return {};
      }
      clientData.reserve(clientIterator->second.second.size());
      for (auto &client : clientIterator->second.second) {
        clientData.push_back(client.second);
      }
    }

    return clientData;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

bool IpcServer::lifetick() {
  try {
    {
      if (!lifetick_1_.second && BaseLib::HelperFunctions::getTime() - lifetick_1_.first > 120000) {
        GD::out.printCritical("Critical: RPC server's lifetick 1 was not updated for more than 120 seconds.");
        return false;
      }
    }

    {
      if (!lifetick_2_.second && BaseLib::HelperFunctions::getTime() - lifetick_2_.first > 120000) {
        GD::out.printCritical("Critical: RPC server's lifetick 2 was not updated for more than 120 seconds.");
        return false;
      }
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

void IpcServer::collectGarbage() {
  try {
    _lastGargabeCollection = GD::bl->hf.getTime();
    std::vector<PIpcClientData> clientsToRemove;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      try {
        for (std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
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
    for (std::vector<PIpcClientData>::iterator i = clientsToRemove.begin(); i != clientsToRemove.end(); ++i) {
      {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        _clients.erase((*i)->id);
      }
      _out.printInfo("Info: IPC client " + std::to_string((*i)->id) + " removed.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool IpcServer::start() {
  try {
    stop();
    _socketPath = GD::bl->settings.socketPath() + "homegearIPC.sock";
    _shuttingDown = false;
    _stopServer = false;
    if (!getFileDescriptor(true)) return false;
    uint32_t ipcThreadCount = GD::bl->settings.ipcThreadCount();
    if (ipcThreadCount < 5) ipcThreadCount = 5;
    startQueue(0, false, ipcThreadCount, 0, SCHED_OTHER);
    startQueue(1, false, ipcThreadCount, 0, SCHED_OTHER);
    startQueue(2, false, ipcThreadCount, 0, SCHED_OTHER);
    GD::bl->threadManager.start(_mainThread, true, &IpcServer::mainThread, this);
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

void IpcServer::stop() {
  try {
    _shuttingDown = true;
    _out.printDebug("Debug: Waiting for IPC server's client threads to finish.");
    std::vector<PIpcClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        clients.push_back(client.second);
        closeClientConnection(client.second);
      }
    }

    _stopServer = true;
    GD::bl->threadManager.join(_mainThread);
    while (!_clients.empty()) {
      for (auto &client : clients) {
        std::unique_lock<std::mutex> waitLock(client->waitMutex);
        waitLock.unlock();
        client->requestConditionVariable.notify_all();
      }
      collectGarbage();
      if (!_clients.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    stopQueue(0);
    stopQueue(1);
    stopQueue(2);
    unlink(_socketPath.c_str());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void IpcServer::homegearShuttingDown() {
  try {
    _shuttingDown = true;

    //Don't close client connections here as they are still needed until "stop()" is called.
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

std::string IpcServer::generateWebSshToken() {
  try {
    auto clientData = GetClientsForRpcMethod("websshInput");
    if (clientData.empty()) return "";

    auto parameters = std::make_shared<BaseLib::Array>();
    BaseLib::PVariable result = sendRequest(clientData.front(), "websshGenerateToken", parameters);

    if (result->errorStruct) _out.printError("Error generating web SSH token: " + result->structValue->at("faultString")->stringValue);
    return result->stringValue;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "";
}

void IpcServer::broadcastEvent(std::string &source, uint64_t id, int32_t channel, std::shared_ptr<std::vector<std::string>> &variables, BaseLib::PArray &values) {
  try {
    if (_shuttingDown) return;
    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("event")) return;

    if (_dummyClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesDevicesReadSet()) {
      std::shared_ptr<BaseLib::Systems::Peer> peer;
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central) peer = central->getPeer(id);
        if (peer) break;
      }

      if (!peer) return;

      std::shared_ptr<std::vector<std::string>> newVariables;
      BaseLib::PArray newValues = std::make_shared<BaseLib::Array>();
      newVariables->reserve(variables->size());
      newValues->reserve(values->size());
      for (int32_t i = 0; i < (int32_t)variables->size(); i++) {
        if (id == 0) {
          if (_dummyClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesReadSet()) {
            auto systemVariable = GD::systemVariableController->getInternal(variables->at(i));
            if (systemVariable && _dummyClientInfo->acls->checkSystemVariableReadAccess(systemVariable)) {
              newVariables->push_back(variables->at(i));
              newValues->push_back(values->at(i));
            }
          }
        } else if (peer && _dummyClientInfo->acls->checkVariableReadAccess(peer, channel, variables->at(i))) {
          newVariables->push_back(variables->at(i));
          newValues->push_back(values->at(i));
        }
      }
      variables = newVariables;
      values = newValues;
    }

    std::vector<PIpcClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) continue;
        clients.push_back(client.second);
      }
    }

    auto parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(5);
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(source));
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(id));
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(channel));
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(*variables));
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(values));

    for (auto &client : clients) {
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(client, "broadcastEvent", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastEvent\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::broadcastServiceMessage(const BaseLib::PServiceMessage &serviceMessage) {
  try {
    if (_shuttingDown) return;
    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("serviceMessage")) return;

    if (serviceMessage->peerId > 0 && _dummyClientInfo->acls->variablesBuildingPartsRoomsCategoriesRolesDevicesReadSet()) {
      std::shared_ptr<BaseLib::Systems::Peer> peer;
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central) peer = central->getPeer(serviceMessage->peerId);
        if (peer) break;
      }

      if (!peer) return;

      if (!_dummyClientInfo->acls->checkVariableReadAccess(peer, serviceMessage->channel, serviceMessage->variable)) {
        return;
      }
    }

    std::vector<PIpcClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (auto &client : _clients) {
        if (client.second->closed) continue;
        clients.push_back(client.second);
      }
    }

    auto parameters = std::make_shared<BaseLib::Array>();
    parameters->emplace_back(serviceMessage->serialize());

    for (auto &client : clients) {
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(client, "broadcastServiceMessage", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastServiceMessage\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::broadcastNewDevices(std::vector<uint64_t> &ids, BaseLib::PVariable deviceDescriptions) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("newDevices")) return;
    if (_dummyClientInfo->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet()) {
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
        if (central && central->peerExists((uint64_t)ids.front())) //All ids are from the same family
        {
          for (auto id : ids) {
            auto peer = central->getPeer((uint64_t)id);
            if (!peer || !_dummyClientInfo->acls->checkDeviceReadAccess(peer)) return;
          }
        }
      }
    }

    std::vector<PIpcClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
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

void IpcServer::broadcastDeleteDevices(BaseLib::PVariable deviceInfo) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("deleteDevices")) return;

    std::vector<PIpcClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
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

void IpcServer::broadcastUpdateDevice(uint64_t id, int32_t channel, int32_t hint) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("updateDevice")) return;
    if (_dummyClientInfo->acls->buildingPartsRoomsCategoriesRolesDevicesReadSet()) {
      std::shared_ptr<BaseLib::Systems::Peer> peer;
      std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
      for (std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
        if (central) peer = central->getPeer(id);
        if (!peer || !_dummyClientInfo->acls->checkDeviceReadAccess(peer)) return;
      }
    }

    std::vector<PIpcClientData> clients;
    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(_clients.size());
      for (std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
        if (i->second->closed) continue;
        clients.push_back(i->second);
      }
    }

    for (std::vector<PIpcClientData>::iterator i = clients.begin(); i != clients.end(); ++i) {
      BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id)), BaseLib::PVariable(new BaseLib::Variable(channel)), BaseLib::PVariable(new BaseLib::Variable(hint))});
      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(*i, "broadcastUpdateDevice", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastUpdateDevice\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::broadcastVariableProfileStateChanged(uint64_t profileId, bool state) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("variableProfileStateChanged")) return;

    std::vector<PIpcClientData> clients;

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
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(profileId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(state));

      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(client, "broadcastVariableProfileStateChanged", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastVariableProfileStateChanged\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::broadcastUiNotificationCreated(uint64_t uiNotificationId) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("uiNotificationCreated")) return;

    std::vector<PIpcClientData> clients;
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
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));

      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(client, "broadcastUiNotificationCreated", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastUiNotificationCreated\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::broadcastUiNotificationRemoved(uint64_t uiNotificationId) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("uiNotificationRemoved")) return;

    std::vector<PIpcClientData> clients;
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
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));

      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(client, "broadcastUiNotificationRemoved", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastUiNotificationRemoved\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::broadcastUiNotificationAction(uint64_t uiNotificationId, const std::string &uiNotificationType, uint64_t buttonId) {
  try {
    if (_shuttingDown) return;

    if (!_dummyClientInfo->acls->checkEventServerMethodAccess("uiNotificationAction")) return;

    std::vector<PIpcClientData> clients;
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
      parameters->reserve(3);
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationId));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(uiNotificationType));
      parameters->emplace_back(std::make_shared<BaseLib::Variable>(buttonId));

      std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(client, "broadcastUiNotificationAction", parameters);
      if (!enqueue(2, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC method call \"broadcastUiNotificationAction\". Queue is full.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void IpcServer::closeClientConnection(const PIpcClientData &client) {
  try {
    if (!client) return;
    GD::bl->fileDescriptorManager.shutdown(client->fileDescriptor);
    client->closed = true;
    std::vector<std::string> rpcMethodsToRemove;
    std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
    for (auto &element : _clientsByRpcMethods) {
      element.second.second.erase(client->id);
      if (element.second.second.empty()) rpcMethodsToRemove.push_back(element.first);
    }
    for (auto &rpcMethod : rpcMethodsToRemove) {
      _clientsByRpcMethods.erase(rpcMethod);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

bool IpcServer::methodExists(const BaseLib::PRpcClientInfo &clientInfo, std::string &methodName) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return false;

    std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
    auto clientIterator = _clientsByRpcMethods.find(methodName);
    return clientIterator != _clientsByRpcMethods.end();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return false;
}

BaseLib::PVariable IpcServer::callProcessRpcMethod(pid_t processId, const BaseLib::PRpcClientInfo &clientInfo, const std::string &methodName, const BaseLib::PArray &parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    std::vector<PIpcClientData> clients;

    {
      std::lock_guard<std::mutex> stateGuard(_stateMutex);
      clients.reserve(2);
      for (auto &client : _clients) {
        if (client.second->closed || client.second->pid != processId) continue;
        clients.push_back(client.second);
      }
    }

    if (clients.empty()) return BaseLib::Variable::createError(-1, "Process not found");

    BaseLib::PVariable responses;
    BaseLib::PVariable responseStruct;
    if (clients.size() > 1) {
      responses = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      responses->structValue->emplace("multipleClients", std::make_shared<BaseLib::Variable>(true));
      responseStruct = responses->structValue->emplace("returnValues", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct)).first->second;
    }
    for (auto &client : clients) {
      BaseLib::PVariable response = sendRequest(client, methodName, parameters);
      if (response->errorStruct) {
        _out.printError("Error calling \"" + methodName + "\" on client " + std::to_string(client->id) + ": " + response->structValue->at("faultString")->stringValue);
      }
      if (clients.size() == 1) return response;
      else responseStruct->structValue->emplace(std::to_string(client->id), response);
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

BaseLib::PVariable IpcServer::callRpcMethod(const BaseLib::PRpcClientInfo &clientInfo, const std::string &methodName, const BaseLib::PArray &parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto clientData = GetClientsForRpcMethod(methodName);
    if (clientData.empty()) return BaseLib::Variable::createError(-32601, "Requested method not found.");

    BaseLib::PVariable responses;
    BaseLib::PVariable responseStruct;
    if (clientData.size() > 1) {
      responses = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      responses->structValue->emplace("multipleClients", std::make_shared<BaseLib::Variable>(true));
      responseStruct = responses->structValue->emplace("returnValues", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct)).first->second;
    }
    for (auto &client : clientData) {
      BaseLib::PVariable response = sendRequest(client, methodName, parameters);
      if (response->errorStruct) {
        _out.printError("Error calling \"" + methodName + "\" on client " + std::to_string(client->id) + ": " + response->structValue->at("faultString")->stringValue);
      }
      if (clientData.size() == 1) return response;
      else responseStruct->structValue->emplace(std::to_string(client->id), response);
    }
    return responses;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void IpcServer::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  try {
    std::shared_ptr<QueueEntry> queueEntry;
    queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
    if (!queueEntry || queueEntry->clientData->closed) return;

    if (queueEntry->type == QueueEntry::QueueEntryType::defaultType) {
      if (index == 0) {
        std::string methodName;
        BaseLib::PArray parameters = _rpcDecoder->decodeRequest(queueEntry->packet, methodName);

        if (parameters->size() != 3) {
          _out.printError("Error: Wrong parameter count while calling method " + methodName);
          return;
        }

        auto localMethodIterator = _localRpcMethods.find(methodName);
        if (localMethodIterator != _localRpcMethods.end()) {
          if (GD::bl->debugLevel >= 4) {
            _out.printDebug("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + methodName);
            if (GD::bl->debugLevel >= 5) {
              for (BaseLib::Array::iterator i = parameters->at(2)->arrayValue->begin(); i != parameters->at(2)->arrayValue->end(); ++i) {
                (*i)->print(true, false);
              }
            }
          }
          BaseLib::PVariable result = localMethodIterator->second(queueEntry->clientData, parameters->at(0)->integerValue, parameters->at(2)->arrayValue);
          if (GD::bl->debugLevel >= 5) {
            _out.printDebug("Response: ");
            result->print(true, false);
          }

          sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
          return;
        }

        auto methodIterator = _rpcMethods.find(methodName);
        if (methodIterator == _rpcMethods.end()) {
          if (GD::bl->hgdc && methodName.compare(0, 4, "hgdc") == 0) {
            auto hgdcMethodName = methodName.substr(4);
            hgdcMethodName.at(0) = (char)(uint8_t)std::tolower(hgdcMethodName.at(0));
            auto result = GD::bl->hgdc->invoke(hgdcMethodName, parameters->at(2)->arrayValue);
            sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
            return;
          } else {
            auto result = callRpcMethod(_dummyClientInfo, methodName, parameters->at(2)->arrayValue);
            sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);
            return;
          }
          return;
        }

        if (GD::bl->debugLevel >= 4) {
          _out.printInfo("Info: Client number " + std::to_string(queueEntry->clientData->id) + " is calling RPC method: " + methodName + " Parameters:");
          for (std::vector<BaseLib::PVariable>::iterator i = parameters->at(2)->arrayValue->begin(); i != parameters->at(2)->arrayValue->end(); ++i) {
            (*i)->print(true, false);
          }
        }
        BaseLib::PVariable result = _rpcMethods.at(methodName)->invoke(_dummyClientInfo, parameters->at(2)->arrayValue);
        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Response: ");
          result->print(true, false);
        }

        sendResponse(queueEntry->clientData, parameters->at(0), parameters->at(1), result);

        if (queueEntry->clientData->closed) closeClientConnection(queueEntry->clientData); //unregisterIpcClient was called
      } else if (index == 1) {
        BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
        int32_t packetId = response->arrayValue->at(0)->integerValue;

        {
          std::lock_guard<std::mutex> responseGuard(queueEntry->clientData->rpcResponsesMutex);
          auto responseIterator = queueEntry->clientData->rpcResponses.find(packetId);
          if (responseIterator != queueEntry->clientData->rpcResponses.end()) {
            PIpcResponse element = responseIterator->second;
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
      }
    } else if (index == 2 && queueEntry->type == QueueEntry::QueueEntryType::broadcast) //Second queue for sending packets. Response is processed by first queue
    {
      BaseLib::PVariable response = sendRequest(queueEntry->clientData, queueEntry->methodName, queueEntry->parameters);
      if (response->errorStruct) {
        _out.printError("Error calling \"" + queueEntry->methodName + "\" on client " + std::to_string(queueEntry->clientData->id) + ": " + response->structValue->at("faultString")->stringValue);
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

BaseLib::PVariable IpcServer::send(const PIpcClientData &clientData, const std::vector<char> &data) {
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

BaseLib::PVariable IpcServer::sendRequest(const PIpcClientData &clientData, const std::string &methodName, const BaseLib::PArray &parameters) {
  try {
    if (methodName.empty() || !clientData) {
      _out.printError("Error: Invalid input to sendRequest().");
      return BaseLib::Variable::createError(-32500, "Unknown application error.");
    }

    lifetick_1_.first = BaseLib::HelperFunctions::getTime();
    lifetick_1_.second = false;

    int32_t packetId;
    {
      std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
      packetId = _currentPacketId++;
    }
    BaseLib::PArray array(new BaseLib::Array{std::make_shared<BaseLib::Variable>(packetId), std::make_shared<BaseLib::Variable>(parameters)});
    std::vector<char> data;
    _rpcEncoder->encodeRequest(methodName, array, data);

    PIpcResponse response;
    {
      std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
      auto result = clientData->rpcResponses.emplace(packetId, std::make_shared<IpcResponse>());
      if (result.second) response = result.first->second;
    }
    if (!response) {
      _out.printError("Critical: Could not insert response struct into map.");
      return BaseLib::Variable::createError(-32500, "Unknown application error.");
    }

    if (GD::ipcLogger->enabled()) GD::ipcLogger->log(IpcModule::ipc, packetId, clientData->pid, IpcLoggerPacketDirection::toClient, data);

    BaseLib::PVariable result = send(clientData, data);
    if (result->errorStruct) {
      std::lock_guard<std::mutex> responseGuard(clientData->rpcResponsesMutex);
      clientData->rpcResponses.erase(packetId);

      lifetick_1_.second = true;
      return result;
    }

    int32_t i = 0;
    std::unique_lock<std::mutex> waitLock(clientData->waitMutex);
    while (!clientData->requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
      return response->finished || clientData->closed || _stopServer;
    })) {
      i++;
      if (i > 60 && (i % 60 == 0)) {
        _out.printWarning("Warning: IPC client with ID " + std::to_string(clientData->id) + " (Process ID: " + std::to_string(clientData->pid) + ") is still executing RPC method \"." + methodName + "\" after " + std::to_string(i) + " seconds.");
      } else if (i == 3600) {
        _out.printError("Error: IPC client with ID " + std::to_string(clientData->id) + " is not responding... Closing connection.");
        closeClientConnection(clientData);
        break;
      }
    }

    if (!response->finished || response->response->arrayValue->size() != 2 || response->packetId != packetId) {
      _out.printError("Error: No or invalid response received to RPC request. Method: " + methodName);
      result = BaseLib::Variable::createError(-1, "No response received.");
    } else result = response->response->arrayValue->at(1);

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

void IpcServer::sendResponse(PIpcClientData &clientData, BaseLib::PVariable &threadId, BaseLib::PVariable &packetId, BaseLib::PVariable &variable) {
  try {
    lifetick_2_.first = BaseLib::HelperFunctions::getTime();
    lifetick_2_.second = false;

    BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{threadId, packetId, variable})));
    std::vector<char> data;
    _rpcEncoder->encodeResponse(array, data);
    if (GD::ipcLogger->enabled()) GD::ipcLogger->log(IpcModule::ipc, packetId->integerValue, clientData->pid, IpcLoggerPacketDirection::toClient, data);
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

void IpcServer::mainThread() {
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
        std::vector<PIpcClientData> clients;
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
        if (GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.ipcServerMaxConnections() * 100 / 112) collectGarbage();
        continue;
      } else if (result == -1) {
        if (errno == EINTR) continue;
        _out.printError("Error: select returned -1: " + std::string(strerror(errno)));
        continue;
      }

      if (FD_ISSET(_serverFileDescriptor->descriptor, &readFileDescriptor) && !_shuttingDown) {
        sockaddr_un clientAddress;
        socklen_t addressSize = sizeof(addressSize);
        std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = GD::bl->fileDescriptorManager.add(accept4(_serverFileDescriptor->descriptor, (struct sockaddr *)&clientAddress, &addressSize, SOCK_CLOEXEC));
        if (!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;

        int32_t clientId = -1;

        {
          std::lock_guard<std::mutex> stateGuard(_stateMutex);
          clientId = _currentClientId++;
        }

        _out.printInfo("Info: Connection accepted. Client number: " + std::to_string(clientId) + ", file descriptor ID: " + std::to_string(clientFileDescriptor->id));

        if (_clients.size() > GD::bl->settings.ipcServerMaxConnections()) {
          collectGarbage();
          if (_clients.size() > GD::bl->settings.ipcServerMaxConnections()) {
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
        PIpcClientData clientData = std::make_shared<IpcClientData>(clientFileDescriptor);
        clientData->id = clientId;
        _clients.emplace(clientData->id, clientData);
        continue;
      }

      PIpcClientData clientData;
      {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        for (std::map<int32_t, PIpcClientData>::iterator i = _clients.begin(); i != _clients.end(); ++i) {
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

void IpcServer::readClient(PIpcClientData &clientData) {
  try {
    int32_t processedBytes = 0;
    int32_t bytesRead = 0;
    bytesRead = read(clientData->fileDescriptor->descriptor, clientData->buffer.data(), clientData->buffer.size());
    if (bytesRead <= 0) //read returns 0, when connection is disrupted.
    {
      _out.printInfo("Info: Connection to IPC server's client number " + std::to_string(clientData->id) + " closed.");
      closeClientConnection(clientData);
      return;
    }

    if (bytesRead > (signed)clientData->buffer.size()) bytesRead = clientData->buffer.size();

    try {
      processedBytes = 0;
      while (processedBytes < bytesRead) {
        processedBytes += clientData->binaryRpc->process(clientData->buffer.data() + processedBytes, bytesRead - processedBytes);
        if (clientData->binaryRpc->isFinished()) {
          if (GD::ipcLogger->enabled()) {
            if (clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request) {
              std::string methodName;
              BaseLib::PArray request = _rpcDecoder->decodeRequest(clientData->binaryRpc->getData(), methodName);
              GD::ipcLogger->log(IpcModule::ipc, request->at(1)->integerValue, clientData->id, IpcLoggerPacketDirection::toServer, clientData->binaryRpc->getData());
            } else {
              BaseLib::PVariable response = _rpcDecoder->decodeResponse(clientData->binaryRpc->getData());
              GD::ipcLogger->log(IpcModule::ipc, response->arrayValue->at(0)->integerValue, clientData->id, IpcLoggerPacketDirection::toServer, clientData->binaryRpc->getData());
            }
          }

          std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(clientData, clientData->binaryRpc->getData());
          if (!enqueue(clientData->binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request ? 0 : 1, queueEntry)) printQueueFullError(_out, "Error: Could not queue incoming RPC packet. Queue is full.");
          clientData->binaryRpc->reset();
        }
      }
    }
    catch (BaseLib::Rpc::BinaryRpcException &ex) {
      _out.printError(std::string("Error processing packet: ") + ex.what());
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

bool IpcServer::getFileDescriptor(bool deleteOldSocket) {
  try {
    if (!BaseLib::Io::directoryExists(GD::bl->settings.socketPath().c_str())) {
      if (errno == ENOENT) _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise IPC won't work.");
      else _out.printCritical("Critical: Error reading information of directory " + GD::bl->settings.socketPath() + ". The script engine won't work: " + strerror(errno));
      _stopServer = true;
      return false;
    }
    bool isDirectory = false;
    int32_t result = BaseLib::Io::isDirectory(GD::bl->settings.socketPath(), isDirectory);
    if (result != 0 || !isDirectory) {
      _out.printCritical("Critical: Directory " + GD::bl->settings.socketPath() + " does not exist. Please create it before starting Homegear otherwise IPC won't work.");
      _stopServer = true;
      return false;
    }
    if (deleteOldSocket) {
      if (unlink(_socketPath.c_str()) == -1 && errno != ENOENT) {
        _out.printCritical("Critical: Couldn't delete existing socket: " + _socketPath + ". Please delete it manually. Flows won't work. Error: " + strerror(errno));
        return false;
      }
    } else if (BaseLib::Io::fileExists(_socketPath)) return false;

    _serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
    if (_serverFileDescriptor->descriptor == -1) {
      _out.printCritical("Critical: Couldn't create socket: " + _socketPath + ". Flows won't work. Error: " + strerror(errno));
      return false;
    }
    int32_t reuseAddress = 1;
    if (setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, (void *)&reuseAddress, sizeof(int32_t)) == -1) {
      GD::bl->fileDescriptorManager.close(_serverFileDescriptor);
      _out.printCritical("Critical: Couldn't set socket options: " + _socketPath + ". Flows won't work correctly. Error: " + strerror(errno));
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
    bool bound = (bind(_serverFileDescriptor->descriptor.load(), (sockaddr *)&serverAddress, strlen(serverAddress.sun_path) + 1 + sizeof(serverAddress.sun_family)) != -1);
    if (_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1) {
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

std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> IpcServer::getRpcMethods() {
  try {
    std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>> rpcMethods;
    std::lock_guard<std::mutex> rpcMethodsGuard(_clientsByRpcMethodsMutex);
    for (auto &element : _clientsByRpcMethods) {
      rpcMethods.emplace(element.first, element.second.first);
    }
    return rpcMethods;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return std::unordered_map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>>();
}

// {{{ RPC methods
BaseLib::PVariable IpcServer::getHomegearPid(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (!parameters->empty()) return BaseLib::Variable::createError(-1, "Method expects no parameters.");

    return std::make_shared<BaseLib::Variable>(getpid());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::setPid(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 1) return BaseLib::Variable::createError(-1, "Method expects one parameter. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");

    clientData->pid = parameters->at(0)->integerValue64;

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

BaseLib::PVariable IpcServer::getClientId(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    return std::make_shared<BaseLib::Variable>(clientData->id);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::registerRpcMethod(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 1) return BaseLib::Variable::createError(-1, "Method expects at least one parameter. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");

    std::string methodName = parameters->at(0)->stringValue;
    BaseLib::Rpc::PRpcMethod rpcMethod = std::make_shared<BaseLib::Rpc::RpcMethod>();
    if (parameters->size() == 2) {
      for (auto &signature : *(parameters->at(1)->arrayValue)) {
        BaseLib::VariableType returnType = BaseLib::VariableType::tVoid;
        std::vector<BaseLib::VariableType> parameterTypes;
        if (!signature->arrayValue->empty()) {
          returnType = signature->arrayValue->at(0)->type;
          for (uint32_t i = 1; i < signature->arrayValue->size(); i++) {
            parameterTypes.push_back(signature->arrayValue->at(i)->type);
          }
        }
        rpcMethod->addSignature(returnType, parameterTypes);
      }
    }

    std::lock_guard<std::mutex> clientsGuard(_clientsByRpcMethodsMutex);
    _clientsByRpcMethods[methodName].second.emplace(clientData->id, clientData);
    _out.printInfo("Info: Client " + std::to_string(clientData->id) + " successfully registered RPC method \"" + methodName + "\" (this method is registered by " + std::to_string(_clientsByRpcMethods[methodName].second.size()) + " client(s)).");

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

BaseLib::PVariable IpcServer::cliGeneralCommand(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 1) return BaseLib::Variable::createError(-1, "Method expects at least one parameter. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");

    std::string &command = parameters->at(0)->stringValue;
    if (command.empty()) return std::make_shared<BaseLib::Variable>();
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: CLI client " + std::to_string(clientData->id) + " is executing command: " + command);

    CliServer cliServer(clientData->id);
    return cliServer.generalCommand(command);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::cliFamilyCommand(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 2) return BaseLib::Variable::createError(-1, "Method expects at least two parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type string.");

    std::string &command = parameters->at(1)->stringValue;
    if (command.empty()) return std::make_shared<BaseLib::Variable>();
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: CLI client " + std::to_string(clientData->id) + " is executing family command: " + command);

    CliServer cliServer(clientData->id);
    return std::make_shared<BaseLib::Variable>(cliServer.familyCommand(parameters->at(0)->integerValue, command));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::cliPeerCommand(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 2) return BaseLib::Variable::createError(-1, "Method expects at least two parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tInteger && parameters->at(0)->type != BaseLib::VariableType::tInteger64) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type integer.");
    if (parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type string.");

    std::string &command = parameters->at(1)->stringValue;
    if (command.empty()) return std::make_shared<BaseLib::Variable>();
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: CLI client " + std::to_string(clientData->id) + " is executing peer command: " + command);

    CliServer cliServer(clientData->id);
    return std::make_shared<BaseLib::Variable>(cliServer.peerCommand((uint64_t)parameters->at(0)->integerValue64, command));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::getNodeCredentials(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Method expects one parameter. " + std::to_string(parameters->size()) + " given.");

    return GD::nodeBlueServer->getNodeCredentials(parameters->at(0)->stringValue);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::setNodeCredentials(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects two parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");

    return GD::nodeBlueServer->setNodeCredentials(parameters->at(0)->stringValue, parameters->at(1));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable IpcServer::setNodeCredentialTypes(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects two parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");

    GD::nodeBlueServer->setNodeCredentialTypes(parameters->at(0)->stringValue, parameters->at(1), true);

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

BaseLib::PVariable IpcServer::noderedEvent(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    GD::nodeBlueServer->getNodered()->event(parameters);

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

BaseLib::PVariable IpcServer::ptyOutput(PIpcClientData &clientData, int32_t threadId, BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Method expects two parameters. " + std::to_string(parameters->size()) + " given.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");
    if (parameters->at(1)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 2 is not of type string.");

    GD::rpcClient->broadcastPtyOutput(parameters->at(0)->stringValue, parameters->at(1)->stringValue);

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

}
