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

#include "RpcServer.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>
#include <gnutls/gnutls.h>

#if GNUTLS_VERSION_NUMBER < 0x030400
int gnutls_system_recv_timeout(gnutls_transport_ptr_t ptr, unsigned int ms)
{
    unsigned int indefiniteTimeout = (unsigned int)-2;

    int ret;
    int fd = (int)(int64_t)ptr;

    fd_set rfds;
    struct timeval _tv, *tv = NULL;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    if (ms != indefiniteTimeout)
    {
        _tv.tv_sec = ms / 1000;
        _tv.tv_usec = (ms % 1000) * 1000;
        tv = &_tv;
    }

    ret = select(fd + 1, &rfds, NULL, NULL, tv);

    if (ret <= 0) return ret;

    return ret;
}
#endif

namespace Homegear
{

namespace Rpc
{

int32_t RpcServer::_currentClientID = 0;

RpcServer::Client::Client()
{
    socket = std::shared_ptr<BaseLib::TcpSocket>(new BaseLib::TcpSocket(GD::bl.get()));
    socketDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor());
    waitForResponse = false;
}

RpcServer::Client::~Client()
{
    GD::bl->fileDescriptorManager.shutdown(socketDescriptor);
    GD::bl->threadManager.join(readThread);
}

RpcServer::RpcServer()
{
    _out.init(GD::bl.get());

    _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get()));
    _rpcDecoderAnsi = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), true));
    _rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get()));
    _xmlRpcDecoder = std::unique_ptr<BaseLib::Rpc::XmlrpcDecoder>(new BaseLib::Rpc::XmlrpcDecoder(GD::bl.get()));
    _xmlRpcEncoder = std::unique_ptr<BaseLib::Rpc::XmlrpcEncoder>(new BaseLib::Rpc::XmlrpcEncoder(GD::bl.get()));
    _jsonDecoder = std::unique_ptr<BaseLib::Rpc::JsonDecoder>(new BaseLib::Rpc::JsonDecoder(GD::bl.get()));
    _jsonEncoder = std::unique_ptr<BaseLib::Rpc::JsonEncoder>(new BaseLib::Rpc::JsonEncoder(GD::bl.get()));

    _info.reset(new BaseLib::Rpc::ServerInfo::Info());
    _serverFileDescriptor.reset(new BaseLib::FileDescriptor);
    _threadPriority = GD::bl->settings.rpcServerThreadPriority();
    _threadPolicy = GD::bl->settings.rpcServerThreadPolicy();

    _stopServer = false;
    _stopped = true;

    _lifetick1.first = 0;
    _lifetick1.second = true;
    _lifetick2.first = 0;
    _lifetick2.second = true;

    _rpcMethods = std::make_shared<std::map<std::string, std::shared_ptr<BaseLib::Rpc::RpcMethod>>>();
    _rpcMethods->emplace("devTest", std::make_shared<RPCDevTest>());
    _rpcMethods->emplace("system.getCapabilities", std::make_shared<RPCSystemGetCapabilities>());
    _rpcMethods->emplace("system.listMethods", std::make_shared<RPCSystemListMethods>());
    _rpcMethods->emplace("system.methodHelp", std::make_shared<RPCSystemMethodHelp>());
    _rpcMethods->emplace("system.methodSignature", std::make_shared<RPCSystemMethodSignature>());
    _rpcMethods->emplace("system.multicall", std::make_shared<RPCSystemMulticall>());
    _rpcMethods->emplace("acknowledgeGlobalServiceMessage", std::make_shared<RPCAcknowledgeGlobalServiceMessage>());
    _rpcMethods->emplace("activateLinkParamset", std::make_shared<RPCActivateLinkParamset>());
    _rpcMethods->emplace("abortEventReset", std::make_shared<RPCTriggerEvent>());
    _rpcMethods->emplace("addCategoryToChannel", std::make_shared<RPCAddCategoryToChannel>());
    _rpcMethods->emplace("addCategoryToDevice", std::make_shared<RPCAddCategoryToDevice>());
    _rpcMethods->emplace("addCategoryToSystemVariable", std::make_shared<RPCAddCategoryToSystemVariable>());
    _rpcMethods->emplace("addCategoryToVariable", std::make_shared<RPCAddCategoryToVariable>());
    _rpcMethods->emplace("addChannelToRoom", std::make_shared<RPCAddChannelToRoom>());
    _rpcMethods->emplace("addDevice", std::make_shared<RPCAddDevice>());
    _rpcMethods->emplace("addDeviceToRoom", std::make_shared<RPCAddDeviceToRoom>());
    _rpcMethods->emplace("addEvent", std::make_shared<RPCAddEvent>());
    _rpcMethods->emplace("addLink", std::make_shared<RPCAddLink>());
    _rpcMethods->emplace("addRoomToStory", std::make_shared<RPCAddRoomToStory>());
    _rpcMethods->emplace("addSystemVariableToRoom", std::make_shared<RPCAddSystemVariableToRoom>());
    _rpcMethods->emplace("addVariableToRoom", std::make_shared<RPCAddVariableToRoom>());
    _rpcMethods->emplace("checkServiceAccess", std::make_shared<RPCCheckServiceAccess>());
    _rpcMethods->emplace("copyConfig", std::make_shared<RPCCopyConfig>());
    _rpcMethods->emplace("clientServerInitialized", std::make_shared<RPCClientServerInitialized>());
    _rpcMethods->emplace("createCategory", std::make_shared<RPCCreateCategory>());
    _rpcMethods->emplace("createDevice", std::make_shared<RPCCreateDevice>());
    _rpcMethods->emplace("createRoom", std::make_shared<RPCCreateRoom>());
    _rpcMethods->emplace("createStory", std::make_shared<RPCCreateStory>());
    _rpcMethods->emplace("deleteCategory", std::make_shared<RPCDeleteCategory>());
    _rpcMethods->emplace("deleteData", std::make_shared<RPCDeleteData>());
    _rpcMethods->emplace("deleteDevice", std::make_shared<RPCDeleteDevice>());
    _rpcMethods->emplace("deleteMetadata", std::make_shared<RPCDeleteMetadata>());
    _rpcMethods->emplace("deleteNodeData", std::make_shared<RPCDeleteNodeData>());
    _rpcMethods->emplace("deleteRoom", std::make_shared<RPCDeleteRoom>());
    _rpcMethods->emplace("deleteStory", std::make_shared<RPCDeleteStory>());
    _rpcMethods->emplace("deleteSystemVariable", std::make_shared<RPCDeleteSystemVariable>());
    _rpcMethods->emplace("enableEvent", std::make_shared<RPCEnableEvent>());
    _rpcMethods->emplace("executeMiscellaneousDeviceMethod", std::make_shared<RPCExecuteMiscellaneousDeviceMethod>());
    _rpcMethods->emplace("familyExists", std::make_shared<RPCFamilyExists>());
    _rpcMethods->emplace("forceConfigUpdate", std::make_shared<RPCForceConfigUpdate>());
    _rpcMethods->emplace("getAllConfig", std::make_shared<RPCGetAllConfig>());
    _rpcMethods->emplace("getAllMetadata", std::make_shared<RPCGetAllMetadata>());
    _rpcMethods->emplace("getAllScripts", std::make_shared<RPCGetAllScripts>());
    _rpcMethods->emplace("getAllSystemVariables", std::make_shared<RPCGetAllSystemVariables>());
    _rpcMethods->emplace("getAllValues", std::make_shared<RPCGetAllValues>());
    _rpcMethods->emplace("getCategories", std::make_shared<RPCGetCategories>());
    _rpcMethods->emplace("getCategoryMetadata", std::make_shared<RPCGetCategoryMetadata>());
    _rpcMethods->emplace("getChannelsInCategory", std::make_shared<RPCGetChannelsInCategory>());
    _rpcMethods->emplace("getChannelsInRoom", std::make_shared<RPCGetChannelsInRoom>());
    _rpcMethods->emplace("getConfigParameter", std::make_shared<RPCGetConfigParameter>());
    _rpcMethods->emplace("getData", std::make_shared<RPCGetData>());
    _rpcMethods->emplace("getDeviceDescription", std::make_shared<RPCGetDeviceDescription>());
    _rpcMethods->emplace("getDeviceInfo", std::make_shared<RPCGetDeviceInfo>());
    _rpcMethods->emplace("getDevicesInCategory", std::make_shared<RPCGetDevicesInCategory>());
    _rpcMethods->emplace("getDevicesInRoom", std::make_shared<RPCGetDevicesInRoom>());
    _rpcMethods->emplace("getEvent", std::make_shared<RPCGetEvent>());
    _rpcMethods->emplace("getLastEvents", std::make_shared<RPCGetLastEvents>());
    _rpcMethods->emplace("getInstallMode", std::make_shared<RPCGetInstallMode>());
    _rpcMethods->emplace("getKeyMismatchDevice", std::make_shared<RPCGetKeyMismatchDevice>());
    _rpcMethods->emplace("getLinkInfo", std::make_shared<RPCGetLinkInfo>());
    _rpcMethods->emplace("getLinkPeers", std::make_shared<RPCGetLinkPeers>());
    _rpcMethods->emplace("getLinks", std::make_shared<RPCGetLinks>());
    _rpcMethods->emplace("getMetadata", std::make_shared<RPCGetMetadata>());
    _rpcMethods->emplace("getName", std::make_shared<RPCGetName>());
    _rpcMethods->emplace("getNodeData", std::make_shared<RPCGetNodeData>());
    _rpcMethods->emplace("getFlowData", std::make_shared<RPCGetFlowData>());
    _rpcMethods->emplace("getGlobalData", std::make_shared<RPCGetGlobalData>());
    _rpcMethods->emplace("getNodeEvents", std::make_shared<RPCGetNodeEvents>());
    _rpcMethods->emplace("getNodesWithFixedInputs", std::make_shared<RPCGetNodesWithFixedInputs>());
    _rpcMethods->emplace("getNodeVariable", std::make_shared<RPCGetNodeVariable>());
    _rpcMethods->emplace("getPairingInfo", std::make_shared<RPCGetPairingInfo>());
    _rpcMethods->emplace("getParamset", std::make_shared<RPCGetParamset>());
    _rpcMethods->emplace("getParamsetDescription", std::make_shared<RPCGetParamsetDescription>());
    _rpcMethods->emplace("getParamsetId", std::make_shared<RPCGetParamsetId>());
    _rpcMethods->emplace("getPeerId", std::make_shared<RPCGetPeerId>());
    _rpcMethods->emplace("getRoomMetadata", std::make_shared<RPCGetRoomMetadata>());
    _rpcMethods->emplace("getRooms", std::make_shared<RPCGetRooms>());
    _rpcMethods->emplace("getRoomsInStory", std::make_shared<RPCGetRoomsInStory>());
    _rpcMethods->emplace("getServiceMessages", std::make_shared<RPCGetServiceMessages>());
    _rpcMethods->emplace("getSniffedDevices", std::make_shared<RPCGetSniffedDevices>());
    _rpcMethods->emplace("getStories", std::make_shared<RPCGetStories>());
    _rpcMethods->emplace("getStoryMetadata", std::make_shared<RPCGetStoryMetadata>());
    _rpcMethods->emplace("getSystemVariable", std::make_shared<RPCGetSystemVariable>());
    _rpcMethods->emplace("getSystemVariableFlags", std::make_shared<RPCGetSystemVariableFlags>());
    _rpcMethods->emplace("getSystemVariablesInCategory", std::make_shared<RPCGetSystemVariablesInCategory>());
    _rpcMethods->emplace("getSystemVariablesInRoom", std::make_shared<RPCGetSystemVariablesInRoom>());
    _rpcMethods->emplace("getUpdateStatus", std::make_shared<RPCGetUpdateStatus>());
    _rpcMethods->emplace("getValue", std::make_shared<RPCGetValue>());
    _rpcMethods->emplace("getVariableDescription", std::make_shared<RPCGetVariableDescription>());
    _rpcMethods->emplace("getVariablesInCategory", std::make_shared<RPCGetVariablesInCategory>());
    _rpcMethods->emplace("getVariablesInRoom", std::make_shared<RPCGetVariablesInRoom>());
    _rpcMethods->emplace("getVersion", std::make_shared<RPCGetVersion>());
    _rpcMethods->emplace("init", std::make_shared<RPCInit>());
    _rpcMethods->emplace("invokeFamilyMethod", std::make_shared<RPCInvokeFamilyMethod>());
    _rpcMethods->emplace("listBidcosInterfaces", std::make_shared<RPCListBidcosInterfaces>());
    _rpcMethods->emplace("listClientServers", std::make_shared<RPCListClientServers>());
    _rpcMethods->emplace("listDevices", std::make_shared<RPCListDevices>());
    _rpcMethods->emplace("listEvents", std::make_shared<RPCListEvents>());
    _rpcMethods->emplace("listFamilies", std::make_shared<RPCListFamilies>());
    _rpcMethods->emplace("listInterfaces", std::make_shared<RPCListInterfaces>());
    _rpcMethods->emplace("listKnownDeviceTypes", std::make_shared<RPCListKnownDeviceTypes>());
    _rpcMethods->emplace("listTeams", std::make_shared<RPCListTeams>());
    _rpcMethods->emplace("logLevel", std::make_shared<RPCLogLevel>());
    _rpcMethods->emplace("nodeOutput", std::make_shared<RPCNodeOutput>());
    _rpcMethods->emplace("peerExists", std::make_shared<RPCPeerExists>());
    _rpcMethods->emplace("ping", std::make_shared<RPCPing>());
    _rpcMethods->emplace("putParamset", std::make_shared<RPCPutParamset>());
    _rpcMethods->emplace("removeCategoryFromChannel", std::make_shared<RPCRemoveCategoryFromChannel>());
    _rpcMethods->emplace("removeCategoryFromDevice", std::make_shared<RPCRemoveCategoryFromDevice>());
    _rpcMethods->emplace("removeCategoryFromSystemVariable", std::make_shared<RPCRemoveCategoryFromSystemVariable>());
    _rpcMethods->emplace("removeCategoryFromVariable", std::make_shared<RPCRemoveCategoryFromVariable>());
    _rpcMethods->emplace("removeChannelFromRoom", std::make_shared<RPCRemoveChannelFromRoom>());
    _rpcMethods->emplace("removeDeviceFromRoom", std::make_shared<RPCRemoveDeviceFromRoom>());
    _rpcMethods->emplace("removeRoomFromStory", std::make_shared<RPCRemoveRoomFromStory>());
    _rpcMethods->emplace("removeSystemVariableFromRoom", std::make_shared<RPCRemoveSystemVariableFromRoom>());
    _rpcMethods->emplace("removeVariableFromRoom", std::make_shared<RPCRemoveVariableFromRoom>());
    _rpcMethods->emplace("removeEvent", std::make_shared<RPCRemoveEvent>());
    _rpcMethods->emplace("removeLink", std::make_shared<RPCRemoveLink>());
    _rpcMethods->emplace("reportValueUsage", std::make_shared<RPCReportValueUsage>());
    _rpcMethods->emplace("rssiInfo", std::make_shared<RPCRssiInfo>());
    _rpcMethods->emplace("runScript", std::make_shared<RPCRunScript>());
    _rpcMethods->emplace("searchDevices", std::make_shared<RPCSearchDevices>());
    _rpcMethods->emplace("searchInterfaces", std::make_shared<RPCSearchInterfaces>());
    _rpcMethods->emplace("setCategoryMetadata", std::make_shared<RPCSetCategoryMetadata>());
    _rpcMethods->emplace("setData", std::make_shared<RPCSetData>());
    _rpcMethods->emplace("setGlobalServiceMessage", std::make_shared<RPCSetGlobalServiceMessage>());
    _rpcMethods->emplace("setId", std::make_shared<RPCSetId>());
    _rpcMethods->emplace("setInstallMode", std::make_shared<RPCSetInstallMode>());
    _rpcMethods->emplace("setInterface", std::make_shared<RPCSetInterface>());
    _rpcMethods->emplace("setLanguage", std::make_shared<RPCSetLanguage>());
    _rpcMethods->emplace("setLinkInfo", std::make_shared<RPCSetLinkInfo>());
    _rpcMethods->emplace("setMetadata", std::make_shared<RPCSetMetadata>());
    _rpcMethods->emplace("setName", std::make_shared<RPCSetName>());
    _rpcMethods->emplace("setNodeData", std::make_shared<RPCSetNodeData>());
    _rpcMethods->emplace("setFlowData", std::make_shared<RPCSetFlowData>());
    _rpcMethods->emplace("setGlobalData", std::make_shared<RPCSetGlobalData>());
    _rpcMethods->emplace("setNodeVariable", std::make_shared<RPCSetNodeVariable>());
    _rpcMethods->emplace("setRoomMetadata", std::make_shared<RPCSetRoomMetadata>());
    _rpcMethods->emplace("setStoryMetadata", std::make_shared<RPCSetStoryMetadata>());
    _rpcMethods->emplace("setSystemVariable", std::make_shared<RPCSetSystemVariable>());
    _rpcMethods->emplace("setTeam", std::make_shared<RPCSetTeam>());
    _rpcMethods->emplace("setValue", std::make_shared<RPCSetValue>());
    _rpcMethods->emplace("startSniffing", std::make_shared<RPCStartSniffing>());
    _rpcMethods->emplace("stopSniffing", std::make_shared<RPCStopSniffing>());
    _rpcMethods->emplace("subscribePeers", std::make_shared<RPCSubscribePeers>());
    _rpcMethods->emplace("triggerEvent", std::make_shared<RPCTriggerEvent>());
    _rpcMethods->emplace("triggerRpcEvent", std::make_shared<RPCTriggerRpcEvent>());
    _rpcMethods->emplace("unsubscribePeers", std::make_shared<RPCUnsubscribePeers>());
    _rpcMethods->emplace("updateCategory", std::make_shared<RPCUpdateCategory>());
    _rpcMethods->emplace("updateFirmware", std::make_shared<RPCUpdateFirmware>());
    _rpcMethods->emplace("updateRoom", std::make_shared<RPCUpdateRoom>());
    _rpcMethods->emplace("updateStory", std::make_shared<RPCUpdateStory>());
    _rpcMethods->emplace("writeLog", std::make_shared<RPCWriteLog>());

    { // Roles
        _rpcMethods->emplace("addRoleToVariable", std::make_shared<RPCAddRoleToVariable>());
        _rpcMethods->emplace("aggregateRoles", std::make_shared<RPCAggregateRoles>());
        _rpcMethods->emplace("createRole", std::make_shared<RPCCreateRole>());
        _rpcMethods->emplace("deleteRole", std::make_shared<RPCDeleteRole>());
        _rpcMethods->emplace("getRoles", std::make_shared<RPCGetRoles>());
        _rpcMethods->emplace("getRoleMetadata", std::make_shared<RPCGetRoleMetadata>());
        _rpcMethods->emplace("getVariablesInRole", std::make_shared<RPCGetVariablesInRole>());
        _rpcMethods->emplace("removeRoleFromVariable", std::make_shared<RPCRemoveRoleFromVariable>());
        _rpcMethods->emplace("setRoleMetadata", std::make_shared<RPCSetRoleMetadata>());
        _rpcMethods->emplace("updateRole", std::make_shared<RPCUpdateRole>());
    }
    
    //{{{ UI
        _rpcMethods->emplace("addUiElement", std::make_shared<RPCAddUiElement>());
        _rpcMethods->emplace("getAllUiElements", std::make_shared<RPCGetAllUiElements>());
        _rpcMethods->emplace("getAvailableUiElements", std::make_shared<RPCGetAvailableUiElements>());
        _rpcMethods->emplace("getCategoryUiElements", std::make_shared<RPCGetCategoryUiElements>());
        _rpcMethods->emplace("getRoomUiElements", std::make_shared<RPCGetRoomUiElements>());
        _rpcMethods->emplace("removeUiElement", std::make_shared<RPCRemoveUiElement>());
    //}}}

    //{{{ Users
        _rpcMethods->emplace("getUserMetadata", std::make_shared<RPCGetUserMetadata>());
        _rpcMethods->emplace("setUserMetadata", std::make_shared<RPCSetUserMetadata>());
    //}}}
}

RpcServer::~RpcServer()
{
    dispose();
}

void RpcServer::dispose()
{
    stop();
    if(_rpcMethods) _rpcMethods->clear();
    _webServer.reset();
    _restServer.reset();
}

bool RpcServer::lifetick()
{
    try
    {
        _lifetick1Mutex.lock();
        if(!_lifetick1.second && BaseLib::HelperFunctions::getTime() - _lifetick1.first > 120000)
        {
            GD::out.printCritical("Critical: RPC server's lifetick 1 was not updated for more than 120 seconds.");
            _lifetick1Mutex.unlock();
            return false;
        }
        _lifetick1Mutex.unlock();

        _lifetick2Mutex.lock();
        if(!_lifetick2.second && BaseLib::HelperFunctions::getTime() - _lifetick2.first > 120000)
        {
            GD::out.printCritical("Critical: RPC server's lifetick 2 was not updated for more than 120 seconds.");
            _lifetick2Mutex.unlock();
            return false;
        }
        _lifetick2Mutex.unlock();
        return true;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

/*int verifyClientCert(gnutls_session_t tlsSession)
{
    //Called during handshake just after the certificate message has been received.

    uint32_t status = (uint32_t)-1;
    if(gnutls_certificate_verify_peers3(tlsSession, nullptr, &status) != GNUTLS_E_SUCCESS) return -1; //Terminate handshake
    if(status > 0) return -1;

    return 0;
}*/

void RpcServer::start(BaseLib::Rpc::PServerInfo& info)
{
    try
    {
        stop();
        _stopServer = false;
        _info = info;
        if(!_info)
        {
            _out.printError("Error: Settings is nullptr.");
            return;
        }
        if(!_info->webServer && !_info->xmlrpcServer && !_info->jsonrpcServer && !_info->restServer) return;
        _out.setPrefix("RPC Server (Port " + std::to_string(info->port) + "): ");

        if(info->familyServer)
        {
            //Disable WebSocket, REST and Webserver
            info->webSocket = false;
            info->restServer = false;
            info->webServer = false;
            if(info->interface != "::1" && info->interface != "127.0.0.1")
            {
                if(!_info->ssl || _info->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::none)
                {
                    _out.printError("Error: For a family RPC server to listen on other addresses than \"::1\" or \"127.0.0.1\", SSL and authentication have to be enabled. Not starting server.");
                    return;
                }
            }
        }

        if(!_info->ssl && _info->interface != "::1" && _info->interface != "127.0.0.1")
        {
            _out.printWarning("Warning: SSL is not enabled for this RPC server. It is strongly recommended to disable all unencrypted RPC servers when the connected clients support it.");
        }

        if((_info->xmlrpcServer || _info->jsonrpcServer || _info->restServer) && _info->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::none && _info->interface != "::1" && _info->interface != "127.0.0.1")
        {
            _out.printWarning("Warning: RPC server has no authorization enabled. Everybody on your local network can login into this installation. It is strongly recommended to enable authorization on all RPC servers when the connected clients support it.");
        }

        if(_info->webSocket && _info->websocketAuthType == BaseLib::Rpc::ServerInfo::Info::AuthType::none && _info->interface != "::1" && _info->interface != "127.0.0.1")
        {
            _out.printWarning("Warning: RPC server has no WebSocket authorization enabled. This is very dangerous as every webpage opened in a browser (also remote one's!!!) can control this installation.");
        }

        if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic) _out.printInfo("Info: Enabling basic authentication.");
        if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::cert) _out.printInfo("Info: Enabling client certificate authentication.");
        if(_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic) _out.printInfo("Info: Enabling basic authentication for WebSockets.");
        if(_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::session) _out.printInfo("Info: Enabling session authentication for WebSockets.");

        if(_info->ssl)
        {
            int32_t result = 0;
            if((result = gnutls_certificate_allocate_credentials(&_x509Cred)) != GNUTLS_E_SUCCESS)
            {
                _out.printError("Error: Could not allocate TLS certificate structure: " + std::string(gnutls_strerror(result)));
                return;
            }
            if((result = gnutls_certificate_set_x509_key_file(_x509Cred, _info->certPath.c_str(), _info->keyPath.c_str(), GNUTLS_X509_FMT_PEM)) < 0)
            {
                _out.printError("Error: Could not load certificate or key file: " + std::string(gnutls_strerror(result)));
                gnutls_certificate_free_credentials(_x509Cred);
                _x509Cred = nullptr;
                return;
            }
            if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::cert)
            {
                if(!_info->caPath.empty())
                {
                    if((result = gnutls_certificate_set_x509_trust_file(_x509Cred, _info->caPath.c_str(), GNUTLS_X509_FMT_PEM)) < 0)
                    {
                        gnutls_certificate_free_credentials(_x509Cred);
                        _x509Cred = nullptr;
                        return;
                    }
                }
                else
                {
                    _out.printError("Client certificate authentication is enabled, but \"caFile\" is not specified in main.conf.");
                    gnutls_certificate_free_credentials(_x509Cred);
                    _x509Cred = nullptr;
                    return;
                }

                if(result == 0)
                {
                    _out.printError("Client certificate authentication is enabled, but no CA certificates specified.");
                    gnutls_certificate_free_credentials(_x509Cred);
                    _x509Cred = nullptr;
                    return;
                }

                //gnutls_certificate_set_verify_function(_x509Cred, &verifyClientCert);
            }
            if(!_dhParams)
            {
                if((result = gnutls_dh_params_init(&_dhParams)) != GNUTLS_E_SUCCESS)
                {
                    _out.printError("Error: Could not initialize DH parameters: " + std::string(gnutls_strerror(result)));
                    gnutls_certificate_free_credentials(_x509Cred);
                    _x509Cred = nullptr;
                    _dhParams = nullptr;
                    return;
                }
                std::vector<uint8_t> binaryData;
                try
                {
                    binaryData = GD::bl->io.getUBinaryFileContent(_info->dhParamPath);
                    binaryData.push_back(0); //gnutls_datum_t.data needs to be null terminated
                }
                catch(BaseLib::Exception& ex)
                {
                    _out.printError("Error: Could not load DH parameter file \"" + _info->dhParamPath + "\": " + std::string(ex.what()));
                    gnutls_certificate_free_credentials(_x509Cred);
                    gnutls_dh_params_deinit(_dhParams);
                    _x509Cred = nullptr;
                    _dhParams = nullptr;
                    return;
                }
                catch(...)
                {
                    _out.printError("Error: Could not load DH parameter file \"" + _info->dhParamPath + "\".");
                    gnutls_certificate_free_credentials(_x509Cred);
                    gnutls_dh_params_deinit(_dhParams);
                    _x509Cred = nullptr;
                    _dhParams = nullptr;
                    return;
                }
                gnutls_datum_t data;
                data.data = &binaryData.at(0);
                data.size = binaryData.size();
                if((result = gnutls_dh_params_import_pkcs3(_dhParams, &data, GNUTLS_X509_FMT_PEM)) != GNUTLS_E_SUCCESS)
                {
                    _out.printError("Error: Could not import DH parameters: " + std::string(gnutls_strerror(result)));
                    gnutls_certificate_free_credentials(_x509Cred);
                    gnutls_dh_params_deinit(_dhParams);
                    _x509Cred = nullptr;
                    _dhParams = nullptr;
                    return;
                }
            }
            if((result = gnutls_priority_init(&_tlsPriorityCache, "NORMAL", NULL)) != GNUTLS_E_SUCCESS)
            {
                _out.printError("Error: Could not initialize cipher priorities: " + std::string(gnutls_strerror(result)));
                gnutls_certificate_free_credentials(_x509Cred);
                if(_dhParams)
                {
                    gnutls_dh_params_deinit(_dhParams);
                    _dhParams = nullptr;
                }
                return;
            }
            gnutls_certificate_set_dh_params(_x509Cred, _dhParams);
        }
        _webServer.reset(new WebServer::WebServer(_info));
        _restServer.reset(new RestServer(_info));
        GD::bl->threadManager.start(_mainThread, true, _threadPriority, _threadPolicy, &RpcServer::mainThread, this);
        _stopped = false;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::stop()
{
    try
    {
        if(_stopped) return;
        _stopped = true;
        _stopServer = true;
        GD::bl->threadManager.join(_mainThread);
        _out.printInfo("Info: Waiting for threads to finish.");

        {
            std::lock_guard<std::mutex> stateGuard(_stateMutex);
            for(std::map<int32_t, std::shared_ptr<Client>>::iterator i = _clients.begin(); i != _clients.end(); ++i)
            {
                closeClientConnection(i->second);
            }
        }

        while(_clients.size() > 0)
        {
            collectGarbage();
            if(_clients.size() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if(_x509Cred)
        {
            gnutls_certificate_free_credentials(_x509Cred);
            _x509Cred = nullptr;
        }
        if(_tlsPriorityCache)
        {
            gnutls_priority_deinit(_tlsPriorityCache);
            _tlsPriorityCache = nullptr;
        }
        if(_dhParams)
        {
            gnutls_dh_params_deinit(_dhParams);
            _dhParams = nullptr;
        }
        _webServer.reset();
        _restServer.reset();
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

uint32_t RpcServer::connectionCount()
{
    try
    {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        uint32_t connectionCount = _clients.size();
        return connectionCount;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
}

void RpcServer::closeClientConnection(std::shared_ptr<Client> client)
{
    try
    {
        if(!client) return;
        GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
        client->closed = true;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::mainThread()
{
    try
    {
        getSocketDescriptor();
        std::string address;
        int32_t port = -1;
        while(!_stopServer)
        {
            try
            {
                if(!_serverFileDescriptor || _serverFileDescriptor->descriptor == -1)
                {
                    if(_stopServer) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                    getSocketDescriptor();
                    continue;
                }
                std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor = getClientSocketDescriptor(address, port);
                if(!clientFileDescriptor || clientFileDescriptor->descriptor == -1) continue;
                std::shared_ptr<Client> client = std::make_shared<Client>();

                {
                    std::lock_guard<std::mutex> stateGuard(_stateMutex);
                    client->id = _currentClientID++;
                    while(client->id == -1) client->id = _currentClientID++; //-1 is not allowed
                    client->socketDescriptor = clientFileDescriptor;
                    while(_clients.find(client->id) != _clients.end())
                    {
                        std::cerr
                                << "Error: Client id was used twice. This shouldn't happen. Please report this error to the developer."
                                << std::endl;
                        _currentClientID++;
                        client->id++;
                    }
                    client->auth = std::make_shared<Auth>(_info->validGroups);
                    client->initInterfaceId = "rpc-client-" + address + ":" + std::to_string(port);
                    _clients[client->id] = client;
                }

                _out.printInfo("Info: RPC server client id for client number " + std::to_string(client->socketDescriptor->id) + " is: " + std::to_string(client->id));

                client->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), client->id);
                if(_info->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::none)
                {
                    std::vector<uint64_t> groups{8}; //No user
                    client->acls->fromGroups(groups);
                }
                else
                {
                    std::vector<uint64_t> groups{9}; //Unauthorized
                    client->acls->fromGroups(groups);
                }

                try
                {
                    if(_info->ssl)
                    {
                        getSSLSocketDescriptor(client);
                        if(!client->socketDescriptor->tlsSession)
                        {
                            //Remove client from _clients again. Socket is already closed.
                            closeClientConnection(client);
                            continue;
                        }
                    }
                    client->socket = std::shared_ptr<BaseLib::TcpSocket>(new BaseLib::TcpSocket(GD::bl.get(), client->socketDescriptor));
                    client->socket->setReadTimeout(100000);
                    client->socket->setWriteTimeout(15000000);
                    client->address = address;
                    client->port = port;

#ifdef CCU2
                    if(client->address == "127.0.0.1")
                    {
                        client->clientType = BaseLib::RpcClientType::ccu2;
                        _out.printInfo("Info: Client type set to \"CCU\".");
                    }
#endif

                    GD::bl->threadManager.start(client->readThread, false, _threadPriority, _threadPolicy, &RpcServer::readClient, this, client);
                }
                catch(const std::exception& ex)
                {
                    closeClientConnection(client);
                    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
                }
                catch(BaseLib::Exception& ex)
                {
                    closeClientConnection(client);
                    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
                }
                catch(...)
                {
                    closeClientConnection(client);
                    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
                }
            }
            catch(const std::exception& ex)
            {
                _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
            }
            catch(BaseLib::Exception& ex)
            {
                _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
            }
            catch(...)
            {
                _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
            }
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    if(!_info->socketDescriptor) GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
}

bool RpcServer::clientValid(std::shared_ptr<Client>& client)
{
    try
    {
        if(client->socketDescriptor->descriptor == -1) return false;
        return true;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

void RpcServer::sendRPCResponseToClient(std::shared_ptr<Client> client, std::vector<char>& data, bool keepAlive)
{
    try
    {
        if(_stopped) return;
        if(!clientValid(client)) return;
        if(data.empty()) return;
        bool error = false;
        try
        {
            //Sleep a tiny little bit. Some clients like the linux version of IP-Symcon don't accept responses too fast.
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            if(!keepAlive || client->rpcType != BaseLib::RpcType::binary) std::this_thread::sleep_for(std::chrono::milliseconds(20)); //Add additional time for XMLRPC requests. Otherwise clients might not receive response.
            client->socket->proofwrite(data);
        }
        catch(BaseLib::SocketDataLimitException& ex)
        {
            _out.printWarning("Warning: " + ex.what());
        }
        catch(const BaseLib::SocketOperationException& ex)
        {
            _out.printError("Error: " + ex.what());
            error = true;
        }
        if(!keepAlive || error) closeClientConnection(client);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::analyzeRPC(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
    try
    {
        if(_stopped) return;

        PacketType::Enum responseType = PacketType::Enum::xmlResponse;
        if(packetType == PacketType::Enum::binaryRequest) responseType = PacketType::Enum::binaryResponse;
        else if(packetType == PacketType::Enum::xmlRequest) responseType = PacketType::Enum::xmlResponse;
        else if(packetType == PacketType::Enum::jsonRequest) responseType = PacketType::Enum::jsonResponse;
        else if(packetType == PacketType::Enum::webSocketRequest) responseType = PacketType::Enum::webSocketResponse;

        std::string methodName;
        int32_t messageId = 0;
        std::shared_ptr<std::vector<BaseLib::PVariable>> parameters;
        if(packetType == PacketType::Enum::binaryRequest)
        {
            if(client->rpcType == BaseLib::RpcType::unknown) client->rpcType = BaseLib::RpcType::binary;
            if(client->clientType == BaseLib::RpcClientType::ccu2) parameters = _rpcDecoderAnsi->decodeRequest(packet, methodName);
            else parameters = _rpcDecoder->decodeRequest(packet, methodName);
        }
        else if(packetType == PacketType::Enum::xmlRequest)
        {
            if(client->rpcType == BaseLib::RpcType::unknown) client->rpcType = BaseLib::RpcType::xml;
            parameters = _xmlRpcDecoder->decodeRequest(packet, methodName);
        }
        else if(packetType == PacketType::Enum::jsonRequest || packetType == PacketType::Enum::webSocketRequest)
        {
            if(client->rpcType == BaseLib::RpcType::unknown) client->rpcType = BaseLib::RpcType::json;
            BaseLib::PVariable result = _jsonDecoder->decode(packet);
            if(result->type == BaseLib::VariableType::tStruct)
            {
                if(result->structValue->find("user") != result->structValue->end())
                {
                    _out.printWarning("Warning: WebSocket auth packet received but auth is disabled for WebSockets. Closing connection.");
                    closeClientConnection(client);
                    return;
                }
                auto idIterator = result->structValue->find("id");
                if(idIterator != result->structValue->end()) messageId = idIterator->second->integerValue;
                auto methodIterator = result->structValue->find("method");
                if(methodIterator == result->structValue->end())
                {
                    if(result->structValue->find("result") == result->structValue->end() && !result->structValue->empty())
                    {
                        _out.printWarning("Warning: Could not decode JSON RPC packet:\n" + result->print(false, false));
                        sendRPCResponseToClient(client, BaseLib::Variable::createError(-32500, "Could not decode RPC packet. \"method\" and \"result\" not found in JSON."), messageId, responseType, keepAlive);
                        return;
                    }
                    analyzeRPCResponse(client, packet, PacketType::Enum::webSocketResponse, keepAlive);
                    return;
                }
                methodName = methodIterator->second->stringValue;
                if(result->structValue->find("params") != result->structValue->end()) parameters = result->structValue->at("params")->arrayValue;
                else parameters.reset(new std::vector<BaseLib::PVariable>());
            }
        }
        if(!parameters)
        {
            _out.printWarning("Warning: Could not decode RPC packet. Parameters are empty.");
            sendRPCResponseToClient(client, BaseLib::Variable::createError(-32500, "Could not decode RPC packet. Parameters are empty."), messageId, responseType, keepAlive);
            return;
        }

        if(!parameters->empty() && parameters->at(0)->errorStruct)
        {
            sendRPCResponseToClient(client, parameters->at(0), messageId, responseType, keepAlive);
            return;
        }
        callMethod(client, methodName, parameters, messageId, responseType, keepAlive);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::sendRPCResponseToClient(std::shared_ptr<Client> client, BaseLib::PVariable variable, int32_t messageId, PacketType::Enum responseType, bool keepAlive)
{
    try
    {
        if(_stopped) return;
        std::vector<char> data;
        if(responseType == PacketType::Enum::xmlResponse)
        {
            _xmlRpcEncoder->encodeResponse(variable, data);
            data.push_back('\r');
            data.push_back('\n');
            std::string header = getHttpResponseHeader("text/xml", data.size() + 21, !keepAlive);
            header.append("<?xml version=\"1.0\"?>");
            data.insert(data.begin(), header.begin(), header.end());
            if(GD::bl->debugLevel >= 5)
            {
                _out.printDebug("Response packet: " + std::string(&data.at(0), data.size()));
            }
        }
        else if(responseType == PacketType::Enum::binaryResponse)
        {
            _rpcEncoder->encodeResponse(variable, data);
            if(GD::bl->debugLevel >= 5)
            {
                _out.printDebug("Response binary:");
                _out.printBinary(data);
            }
        }
        else if(responseType == PacketType::Enum::jsonResponse)
        {
            _jsonEncoder->encodeResponse(variable, messageId, data);
            data.push_back('\r');
            data.push_back('\n');
            std::string header = getHttpResponseHeader("application/json", data.size(), !keepAlive);
            data.insert(data.begin(), header.begin(), header.end());
            if(GD::bl->debugLevel >= 5)
            {
                _out.printDebug("Response packet: " + std::string(&data.at(0), data.size()));
            }
        }
        else if(responseType == PacketType::Enum::webSocketResponse)
        {
            std::vector<char> json;
            _jsonEncoder->encodeResponse(variable, messageId, json);
            BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, data);
            if(GD::bl->debugLevel >= 5)
            {
                _out.printDebug("Response WebSocket packet: ");
                _out.printBinary(data);
            }
        }
        sendRPCResponseToClient(client, data, keepAlive);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

bool RpcServer::methodExists(BaseLib::PRpcClientInfo clientInfo, std::string& methodName)
{
    try
    {
        if(!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return false;

        return _rpcMethods->find(methodName) != _rpcMethods->end() || GD::ipcServer->methodExists(clientInfo, methodName) || (_info->familyServer && GD::familyServer->methodExists(clientInfo, methodName));
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

BaseLib::PVariable RpcServer::callMethod(BaseLib::PRpcClientInfo clientInfo, std::string& methodName, BaseLib::PVariable& parameters)
{
    try
    {
        if(!parameters) parameters = BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tArray));
        if(_stopped || GD::bl->shuttingDown) return BaseLib::Variable::createError(100000, "Server is stopped.");
        if(_rpcMethods->find(methodName) == _rpcMethods->end())
        {
            if(_info->familyServer)
            {
                auto result = GD::familyServer->callRpcMethod(clientInfo, methodName, parameters->arrayValue);
                if(!result->errorStruct || result->structValue->at("faultCode")->integerValue != 32601) return result;
            }
            BaseLib::PVariable result = GD::ipcServer->callRpcMethod(clientInfo, methodName, parameters->arrayValue);
            return result;
        }
        _lifetick1Mutex.lock();
        _lifetick1.second = false;
        _lifetick1.first = BaseLib::HelperFunctions::getTime();
        _lifetick1Mutex.unlock();
        if(GD::bl->debugLevel >= 4)
        {
            _out.printInfo("Info: RPC Method called: " + methodName + " Parameters:");
            for(std::vector<BaseLib::PVariable>::iterator i = parameters->arrayValue->begin(); i != parameters->arrayValue->end(); ++i)
            {
                (*i)->print(true, false);
            }
        }
        BaseLib::PVariable ret = _rpcMethods->at(methodName)->invoke(clientInfo, parameters->arrayValue);
        if(GD::bl->debugLevel >= 5)
        {
            _out.printDebug("Response: ");
            ret->print(true, false);
        }
        _lifetick1Mutex.lock();
        _lifetick1.second = true;
        _lifetick1Mutex.unlock();
        return ret;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, ": Unknown application error.");
}

void RpcServer::callMethod(std::shared_ptr<Client> client, std::string methodName, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters, int32_t messageId, PacketType::Enum responseType, bool keepAlive)
{
    try
    {
        if(_stopped || GD::bl->shuttingDown) return;

        if(methodName == "setClientType" && parameters->size() > 0)
        {
            if(parameters->at(0)->integerValue == 1)
            {
                _out.printInfo("Info: Type of client " + std::to_string(client->id) + " set to addon.");
                client->addon = true;
                BaseLib::PVariable ret(new BaseLib::Variable());
                sendRPCResponseToClient(client, ret, messageId, responseType, keepAlive);
            }
            return;
        }

        if(_rpcMethods->find(methodName) == _rpcMethods->end())
        {
            if(_info->familyServer)
            {
                auto result = GD::familyServer->callRpcMethod(client, methodName, parameters);
                if(!result->errorStruct || result->structValue->at("faultCode")->integerValue != 32601)
                {
                    sendRPCResponseToClient(client, result, messageId, responseType, keepAlive);
                    return;
                }
            }
            BaseLib::PVariable result = GD::ipcServer->callRpcMethod(client, methodName, parameters);
            sendRPCResponseToClient(client, result, messageId, responseType, keepAlive);
            return;
        }
        _lifetick2Mutex.lock();
        _lifetick2.first = BaseLib::HelperFunctions::getTime();
        _lifetick2.second = false;
        _lifetick2Mutex.unlock();
        if(GD::bl->debugLevel >= 4 && methodName != "getNodeVariable")
        {
            _out.printInfo("Info: Client number " + std::to_string(client->socketDescriptor->id) + (client->clientType == BaseLib::RpcClientType::ccu2 ? " (CCU)" : "") + (client->clientType == BaseLib::RpcClientType::ipsymcon ? " (IP-Symcon)" : "") + " is calling RPC method: " + methodName + " (" + std::to_string((int32_t) (client->rpcType)) + ") Parameters:");
            for(std::vector<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i)
            {
                (*i)->print(true, false);
            }
        }
        BaseLib::PVariable ret = _rpcMethods->at(methodName)->invoke(client, parameters);
        if(GD::bl->debugLevel >= 5)
        {
            _out.printDebug("Response: ");
            ret->print(true, false);
        }
        sendRPCResponseToClient(client, ret, messageId, responseType, keepAlive);
        _lifetick2Mutex.lock();
        _lifetick2.second = true;
        _lifetick2Mutex.unlock();
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::string RpcServer::getHttpResponseHeader(std::string contentType, uint32_t contentLength, bool closeConnection)
{
    std::string header;
    header.append("HTTP/1.1 200 OK\r\n");
    header.append("Connection: ");
    header.append(closeConnection ? "close\r\n" : "Keep-Alive\r\n");
    header.append("Content-Type: " + contentType + "\r\n");
    header.append("Content-Length: ").append(std::to_string(contentLength)).append("\r\n\r\n");
    return header;
}

void RpcServer::analyzeRPCResponse(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
    try
    {
        if(_stopped) return;
        std::unique_lock<std::mutex> requestLock(client->requestMutex);
        if(packetType == PacketType::Enum::binaryResponse)
        {
            if(client->clientType == BaseLib::RpcClientType::ccu2) client->rpcResponse = _rpcDecoderAnsi->decodeResponse(packet);
            else client->rpcResponse = _rpcDecoder->decodeResponse(packet);
        }
        else if(packetType == PacketType::Enum::xmlResponse)
        {
            client->rpcResponse = _xmlRpcDecoder->decodeResponse(packet);
        }
        else if(packetType == PacketType::Enum::jsonResponse || packetType == PacketType::Enum::webSocketResponse)
        {
            auto jsonStruct = _jsonDecoder->decode(packet);
            auto resultIterator = jsonStruct->structValue->find("result");
            if(resultIterator == jsonStruct->structValue->end()) client->rpcResponse = std::make_shared<BaseLib::Variable>();
            else client->rpcResponse = resultIterator->second;
        }
        requestLock.unlock();
        client->requestConditionVariable.notify_all();
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::packetReceived(std::shared_ptr<Client> client, std::vector<char>& packet, PacketType::Enum packetType, bool keepAlive)
{
    try
    {
        if(packetType == PacketType::Enum::binaryRequest || packetType == PacketType::Enum::xmlRequest || packetType == PacketType::Enum::jsonRequest || packetType == PacketType::Enum::webSocketRequest) analyzeRPC(client, packet, packetType, keepAlive);
        else if(packetType == PacketType::Enum::binaryResponse || packetType == PacketType::Enum::xmlResponse || packetType == PacketType::Enum::jsonResponse) analyzeRPCResponse(client, packet, packetType, keepAlive);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

const std::vector<BaseLib::PRpcClientInfo> RpcServer::getClientInfo()
{
    std::vector<BaseLib::PRpcClientInfo> clients;
    try
    {
        std::lock_guard<std::mutex> stateGuard(_stateMutex);
        for(std::map<int32_t, std::shared_ptr<Client>>::const_iterator i = _clients.begin(); i != _clients.end(); i++)
        {
            if(i->second->closed) continue;
            clients.push_back(i->second);
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return clients;
}

BaseLib::PEventHandler RpcServer::addWebserverEventHandler(BaseLib::Rpc::IWebserverEventSink* eventHandler)
{
    if(_webServer) return _webServer->addEventHandler(eventHandler);
    return BaseLib::PEventHandler();
}

void RpcServer::removeWebserverEventHandler(BaseLib::PEventHandler eventHandler)
{
    if(_webServer) _webServer->removeEventHandler(eventHandler);
}

void RpcServer::collectGarbage()
{
    try
    {
        std::lock_guard<std::mutex> garbageCollectionGuard(_garbageCollectionMutex);
        _lastGargabeCollection = GD::bl->hf.getTime();
        std::vector<std::shared_ptr<Client>> clientsToRemove;

        {
            std::lock_guard<std::mutex> stateGuard(_stateMutex);
            for(auto& client : _clients)
            {
                if(client.second->closed) clientsToRemove.push_back(client.second);
                else if(client.second->rpcType == BaseLib::RpcType::webserver && client.second->lastReceivedPacket.load() != 0 && BaseLib::HelperFunctions::getTime() - client.second->lastReceivedPacket.load() > 10000)
                {
                    closeClientConnection(client.second);
                    clientsToRemove.push_back(client.second);
                }
            }
        }

        for(auto& client : clientsToRemove)
        {
            _out.printDebug("Debug: Joining read thread of client " + std::to_string(client->id));
            GD::bl->threadManager.join(client->readThread);

            {
                std::lock_guard<std::mutex> stateGuard(_stateMutex);
                _clients.erase(client->id);
            }

            _out.printDebug("Debug: Client " + std::to_string(client->id) + " removed.");
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::handleConnectionUpgrade(std::shared_ptr<Client> client, BaseLib::Http& http)
{
    try
    {
        if(http.getHeader().fields.find("upgrade") != http.getHeader().fields.end() && BaseLib::HelperFunctions::toLower(http.getHeader().fields["upgrade"]) == "websocket")
        {
            if(http.getHeader().fields.find("sec-websocket-protocol") == http.getHeader().fields.end() && (http.getHeader().path.empty() || http.getHeader().path == "/"))
            {
                closeClientConnection(client);
                _out.printError("Error: No websocket protocol specified.");
                return;
            }
            if(http.getHeader().fields.find("sec-websocket-key") == http.getHeader().fields.end())
            {
                closeClientConnection(client);
                _out.printError("Error: No websocket key specified.");
                return;
            }
            std::string protocol = http.getHeader().fields["sec-websocket-protocol"];
            BaseLib::HelperFunctions::toLower(protocol);
            std::string websocketKey = http.getHeader().fields["sec-websocket-key"] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
            std::vector<char> data(websocketKey.begin(), websocketKey.end());
            std::vector<char> sha1;
            BaseLib::Security::Hash::sha1(data, sha1);
            std::string websocketAccept;
            BaseLib::Base64::encode(sha1, websocketAccept);
            std::string pathProtocol;
            int32_t pos = http.getHeader().path.find('/', 1);
            if(http.getHeader().path.size() == 7 || pos == 7) pathProtocol = http.getHeader().path.substr(1, 6);
            else if(http.getHeader().path.size() == 8 || pos == 8) pathProtocol = http.getHeader().path.substr(1, 7);
            else if(http.getHeader().path.size() == 11 || pos == 11) pathProtocol = http.getHeader().path.substr(1, 10);
            if(pathProtocol == "client" || pathProtocol == "server")
            {
                //path starts with "/client/" or "/server/". Both are not part of the client id.
                if(http.getHeader().path.size() > 8) client->webSocketClientId = http.getHeader().path.substr(8);
            }
            else if(pathProtocol == "server2")
            {
                //path starts with "/client/" or "/server/". Both are not part of the client id.
                if(http.getHeader().path.size() > 9) client->webSocketClientId = http.getHeader().path.substr(9);
            }
            else if(pathProtocol == "nodeclient" || pathProtocol == "nodeserver")
            {
                //path starts with "/nodeclient/" or "/nodeserver/". Both are not part of the client id.
                if(http.getHeader().path.size() > 12) client->webSocketClientId = http.getHeader().path.substr(12);
            }
            else if(http.getHeader().path.size() > 1)
            {
                pathProtocol.clear();
                //Full path is client id.
                client->webSocketClientId = http.getHeader().path.substr(1);
            }
            BaseLib::HelperFunctions::toLower(client->webSocketClientId);

            if(protocol == "server" || pathProtocol == "server" || protocol == "server2" || pathProtocol == "server2" || protocol == "nodeserver" || pathProtocol == "nodeserver")
            {
                client->rpcType = BaseLib::RpcType::websocket;
                client->initJsonMode = true;
                client->initKeepAlive = true;
                client->initNewFormat = true;
                client->initSubscribePeers = true;
                if(protocol == "nodeserver" || pathProtocol == "nodeserver") client->nodeClient = true;
                std::string header;
                header.reserve(133 + websocketAccept.size());
                header.append("HTTP/1.1 101 Switching Protocols\r\n");
                header.append("Connection: Upgrade\r\n");
                header.append("Upgrade: websocket\r\n");
                header.append("Sec-WebSocket-Accept: ").append(websocketAccept).append("\r\n");
                if(!protocol.empty()) header.append("Sec-WebSocket-Protocol: " + protocol + "\r\n");
                header.append("\r\n");
                std::vector<char> data(header.begin(), header.end());
                sendRPCResponseToClient(client, data, true);
                if(protocol == "server2" || pathProtocol == "server2" || protocol == "nodeserver" || pathProtocol == "nodeserver")
                {
                    client->sendEventsToRpcServer = true;
                    _out.printInfo("Info: Transferring client number " + std::to_string(client->id) + " to RPC client.");
                    GD::rpcClient->addWebSocketServer(client->socket, client->webSocketClientId, client, client->address, client->nodeClient);
                }
            }
            else if(protocol == "client" || pathProtocol == "client" || protocol == "nodeclient" || pathProtocol == "nodeclient")
            {
                client->rpcType = BaseLib::RpcType::websocket;
                client->webSocketClient = true;
                std::string header;
                header.reserve(133 + websocketAccept.size());
                header.append("HTTP/1.1 101 Switching Protocols\r\n");
                header.append("Connection: Upgrade\r\n");
                header.append("Upgrade: websocket\r\n");
                header.append("Sec-WebSocket-Accept: ").append(websocketAccept).append("\r\n");
                if(!protocol.empty()) header.append("Sec-WebSocket-Protocol: " + protocol + "\r\n");
                header.append("\r\n");
                std::vector<char> data(header.begin(), header.end());
                sendRPCResponseToClient(client, data, true);
                if(_info->websocketAuthType == BaseLib::Rpc::ServerInfo::Info::AuthType::none)
                {
                    _out.printInfo("Info: Transferring client number " + std::to_string(client->id) + " to RPC client.");
                    auto server = GD::rpcClient->addWebSocketServer(client->socket, client->webSocketClientId, client, client->address, client->nodeClient);
                    client->socketDescriptor.reset(new BaseLib::FileDescriptor());
                    client->socket.reset(new BaseLib::TcpSocket(GD::bl.get()));
                    client->closed = true;
                }
            }
            else
            {
                closeClientConnection(client);
                _out.printError("Error: Unknown websocket protocol. Known protocols are \"server\" and \"client\".");
            }
        }
        else
        {
            closeClientConnection(client);
            _out.printError("Error: Connection upgrade type not supported: " + http.getHeader().fields["upgrade"]);
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RpcServer::readClient(std::shared_ptr<Client> client)
{
    try
    {
        if(!client) return;
        std::array<char, 1025> buffer;
        //Make sure the buffer is null terminated.
        buffer.at(buffer.size() - 1) = '\0';
        int32_t processedBytes = 0;
        int32_t bytesRead = 0;
        PacketType::Enum packetType = PacketType::binaryRequest;
        BaseLib::Rpc::BinaryRpc binaryRpc(GD::bl.get());
        BaseLib::Http http;
        BaseLib::WebSocket webSocket;

        _out.printDebug("Listening for incoming packets from client number " + std::to_string(client->socketDescriptor->id) + ".");
        while(!_stopServer)
        {
            try
            {
                bytesRead = client->socket->proofread(buffer.data(), buffer.size() - 1);
                //Some clients send only one byte in the first packet
                if(bytesRead == 1 && !binaryRpc.processingStarted() && !http.headerProcessingStarted() && !webSocket.dataProcessingStarted()) bytesRead += client->socket->proofread(buffer.data() + 1, buffer.size() - 2);
                buffer.at(buffer.size() - 1) = '\0'; //Even though it shouldn't matter, make sure there is a null termination.
            }
            catch(const BaseLib::SocketTimeOutException& ex)
            {
                continue;
            }
            catch(const BaseLib::SocketClosedException& ex)
            {
                if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: " + ex.what());
                break;
            }
            catch(const BaseLib::SocketOperationException& ex)
            {
                _out.printError(ex.what());
                break;
            }

            if(!clientValid(client)) break;

            if(GD::bl->debugLevel >= 5)
            {
                std::vector<uint8_t> rawPacket(buffer.begin(), buffer.begin() + bytesRead);
                _out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
            }

            if(binaryRpc.processingStarted() || (!binaryRpc.processingStarted() && !http.headerProcessingStarted() && !webSocket.dataProcessingStarted() && !strncmp(buffer.data(), "Bin", 3)))
            {
                if(!_info->xmlrpcServer) continue;

                try
                {
                    processedBytes = 0;
                    bool doBreak = false;
                    while(processedBytes < bytesRead)
                    {
                        processedBytes += binaryRpc.process(buffer.data() + processedBytes, bytesRead - processedBytes);
                        if(binaryRpc.isFinished())
                        {
                            std::shared_ptr<BaseLib::Rpc::RpcHeader> header = _rpcDecoder->decodeHeader(binaryRpc.getData());
                            if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic)
                            {
                                try
                                {
                                    if(!client->auth->basicServer(client->socket, header, client->user, client->acls))
                                    {
                                        _out.printError("Error: Authorization failed. Closing connection.");
                                        doBreak = true;
                                        break;
                                    }
                                    else _out.printDebug("Client successfully authorized as user [" + client->user + "] using basic authentication.");
                                }
                                catch(AuthException& ex)
                                {
                                    _out.printError("Error: Authorization failed. Closing connection. Error was: " + ex.what());
                                    doBreak = true;
                                    break;
                                }
                            }

                            packetType = (binaryRpc.getType() == BaseLib::Rpc::BinaryRpc::Type::request) ? PacketType::Enum::binaryRequest : PacketType::Enum::binaryResponse;

                            packetReceived(client, binaryRpc.getData(), packetType, true);
                            binaryRpc.reset();
                        }
                    }
                    if(doBreak) break;
                }
                catch(BaseLib::Rpc::BinaryRpcException& ex)
                {
                    _out.printError("Error processing binary RPC packet. Closing connection. Error was: " + ex.what());
                    binaryRpc.reset();
                    break;
                }
                continue;
            }
            else if(client->rpcType == BaseLib::RpcType::websocket)
            {
                packetType = PacketType::Enum::webSocketRequest;
                processedBytes = 0;
                bool doBreak = false;
                while(processedBytes < bytesRead)
                {
                    processedBytes += webSocket.process(buffer.data() + processedBytes, bytesRead - processedBytes);
                    if(webSocket.isFinished())
                    {
                        if(webSocket.getHeader().close)
                        {
                            std::vector<char> response;
                            webSocket.encode(webSocket.getContent(), BaseLib::WebSocket::Header::Opcode::close, response);
                            sendRPCResponseToClient(client, response, false);
                            closeClientConnection(client);
                        }
                        else if(((_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic) || (_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::session)) && !client->webSocketAuthorized)
                        {
                            try
                            {
                                if((_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic) && !client->auth->basicServer(client->socket, webSocket, client->user, client->acls))
                                {
                                    _out.printError("Error: Basic authentication failed for host " + client->address + ". Closing connection.");
                                    std::vector<char> output;
                                    BaseLib::WebSocket::encodeClose(output);
                                    sendRPCResponseToClient(client, output, false);
                                    doBreak = true;
                                    break;
                                }
                                else if((_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::session) && !client->auth->sessionServer(client->socket, webSocket, client->user, client->acls))
                                {
                                    _out.printError("Error: Session authentication failed for host " + client->address + ". Closing connection.");
                                    std::vector<char> output;
                                    BaseLib::WebSocket::encodeClose(output);
                                    sendRPCResponseToClient(client, output, false);
                                    doBreak = true;
                                    break;
                                }
                                else
                                {
                                    client->webSocketAuthorized = true;
                                    if(_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic) _out.printInfo(std::string("Client ") + (client->webSocketClient ? "(direction browser => Homegear)" : "(direction Homegear => browser)") + " successfully authorized as user [" + client->user + "] using basic authentication.");
                                    else if(_info->websocketAuthType & BaseLib::Rpc::ServerInfo::Info::AuthType::session) _out.printInfo(std::string("Client ") + (client->webSocketClient ? "(direction browser => Homegear)" : "(direction Homegear => browser)") + " successfully authorized as user [" + client->user + "] using session authentication.");
                                    if(client->webSocketClient || client->sendEventsToRpcServer)
                                    {
                                        _out.printInfo("Info: Transferring client number " + std::to_string(client->id) + " to rpc client.");
                                        GD::rpcClient->addWebSocketServer(client->socket, client->webSocketClientId, client, client->address, client->nodeClient);
                                        if(client->webSocketClient)
                                        {
                                            client->socketDescriptor.reset(new BaseLib::FileDescriptor());
                                            client->socket.reset(new BaseLib::TcpSocket(GD::bl.get()));
                                            client->closed = true;
                                        }
                                    }
                                }
                            }
                            catch(AuthException& ex)
                            {
                                _out.printError("Error: Authorization failed for host " + http.getHeader().host + ". Closing connection. Error was: " + ex.what());
                                doBreak = true;
                                break;
                            }
                        }
                        else if(webSocket.getHeader().opcode == BaseLib::WebSocket::Header::Opcode::ping)
                        {
                            std::vector<char> response;
                            webSocket.encode(webSocket.getContent(), BaseLib::WebSocket::Header::Opcode::pong, response);
                            sendRPCResponseToClient(client, response, false);
                        }
                        else
                        {
                            packetReceived(client, webSocket.getContent(), packetType, true);
                        }
                        webSocket.reset();
                    }
                }
                if(doBreak) break;
                continue;
            }
            else //HTTP
            {
                bool processHttp = false;

                if(http.headerProcessingStarted()) processHttp = true;
                else
                {
                    if(!strncmp(buffer.data(), "GET ", 4) || !strncmp(buffer.data(), "HEAD ", 5))
                    {
                        buffer.at(bytesRead) = '\0';
                        packetType = PacketType::Enum::xmlRequest;

                        if(!_info->redirectTo.empty())
                        {
                            std::vector<char> data;
                            std::vector<std::string> additionalHeaders({std::string("Location: ") + _info->redirectTo});
                            _webServer->getError(301, "Moved Permanently", "The document has moved <a href=\"" + _info->redirectTo + "\">here</a>.", data, additionalHeaders);
                            sendRPCResponseToClient(client, data, false);
                            continue;
                        }
                        if(!_info->webServer && !_info->restServer)
                        {
                            std::vector<char> data;
                            _webServer->getError(400, "Bad Request", "Your client sent a request that this server could not understand.", data);
                            sendRPCResponseToClient(client, data, false);
                            continue;
                        }

                        processHttp = true;
                        http.reset();
                    }
                    else if(!strncmp(buffer.data(), "POST", 4) || !strncmp(buffer.data(), "PUT", 3) || !strncmp(buffer.data(), "HTTP/1.", 7))
                    {
                        if(bytesRead < 8) continue;
                        buffer.at(bytesRead) = '\0';
                        packetType = (!strncmp(buffer.data(), "POST", 4)) || (!strncmp(buffer.data(), "PUT", 3)) ? PacketType::Enum::xmlRequest : PacketType::Enum::xmlResponse;

                        processHttp = true;
                        http.reset();
                    }
                    else
                    {
                        _out.printError("Error: Uninterpretable packet received. Closing connection. Packet was: " + std::string(buffer.begin(), buffer.begin() + bytesRead));
                        break;
                    }
                }

                if(processHttp)
                {
                    try
                    {
                        processedBytes = 0;
                        while(processedBytes < bytesRead)
                        {
                            processedBytes += http.process(buffer.data() + processedBytes, bytesRead - processedBytes);

                            if(http.getContentSize() > 104857600)
                            {
                                http.reset();
                                std::vector<char> data;
                                _webServer->getError(400, "Bad Request", "Your client sent a request larger than 100 MiB.", data);
                                sendRPCResponseToClient(client, data, false);
                                break;
                            }

                            if(http.isFinished())
                            {
                                if(_info->webSocket && (http.getHeader().connection & BaseLib::Http::Connection::upgrade))
                                {
                                    //Do this before basic auth, because currently basic auth is not supported by WebSockets. Authorization takes place after the upgrade.
                                    handleConnectionUpgrade(client, http);
                                    http.reset();
                                    break;
                                }

                                if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::basic)
                                {
                                    try
                                    {
                                        if(!client->auth->basicServer(client->socket, http, client->user, client->acls))
                                        {
                                            _out.printError("Error: Authorization failed for host " + http.getHeader().host + ". Closing connection.");
                                            http.reset();
                                            break;
                                        }
                                        else _out.printInfo("Info: Client successfully authorized as user [" + client->user + "] using basic authentication.");
                                    }
                                    catch(AuthException& ex)
                                    {
                                        _out.printError("Error: Authorization failed for host " + http.getHeader().host + ". Closing connection. Error was: " + ex.what());
                                        http.reset();
                                        break;
                                    }
                                }
                                if(_info->restServer && http.getHeader().path.compare(0, 5, "/api/") == 0)
                                {
                                    _restServer->process(client, http, client->socket);
                                }
                                else if(_info->webServer && (
                                    !_info->xmlrpcServer ||
                                    http.getHeader().method != "POST" ||
                                    (!http.getHeader().contentType.empty() && http.getHeader().contentType != "text/xml") ||
                                    http.getHeader().path.compare(0, 4, "/ui/") == 0 ||
                                    http.getHeader().path.compare(0, 7, "/admin/") == 0
                                ) && (
                                    !_info->jsonrpcServer ||
                                    http.getHeader().method != "POST" ||
                                    (!http.getHeader().contentType.empty() && http.getHeader().contentType != "application/json") ||
                                    http.getHeader().path == "/node-blue/flows" ||
                                    http.getHeader().path.compare(0, 4, "/ui/") == 0 ||
                                    http.getHeader().path.compare(0, 7, "/admin/") == 0
                                ))
                                {
                                    client->rpcType = BaseLib::RpcType::webserver;
                                    http.getHeader().remoteAddress = client->address;
                                    http.getHeader().remotePort = client->port;
                                    if(http.getHeader().method == "POST" || http.getHeader().method == "PUT") _webServer->post(http, client->socket);
                                    else if(http.getHeader().method == "GET" || http.getHeader().method == "HEAD") _webServer->get(http, client->socket, _info->cacheAssets);
                                    if(http.getHeader().connection & BaseLib::Http::Connection::Enum::close) closeClientConnection(client);
                                    client->lastReceivedPacket = BaseLib::HelperFunctions::getTime();
                                }
                                else if(http.getContentSize() > 0 && (_info->xmlrpcServer || _info->jsonrpcServer))
                                {
                                    if(http.getHeader().contentType == "application/json" || http.getContent().at(0) == '{') packetType = packetType == PacketType::xmlRequest ? PacketType::jsonRequest : PacketType::jsonResponse;
                                    packetReceived(client, http.getContent(), packetType, http.getHeader().connection & BaseLib::Http::Connection::Enum::keepAlive);
                                }
                                http.reset();
                                if(client->socketDescriptor->descriptor == -1)
                                {
                                    if(GD::bl->debugLevel >= 5) _out.printDebug("Debug: Connection to client number " + std::to_string(client->socketDescriptor->id) + " closed.");
                                    break;
                                }
                            }
                        }
                        continue;
                    }
                    catch(BaseLib::HttpException& ex)
                    {
                        _out.printError("XML RPC Server: Could not process HTTP packet: " + ex.what() + " Buffer: " + std::string(buffer.begin(), buffer.begin() + bytesRead));
                        std::vector<char> data;
                        _webServer->getError(400, "Bad Request", "Your client sent a request that this server could not understand.", data);
                        sendRPCResponseToClient(client, data, false);
                    }
                }
            }
        }
        if(client->rpcType == BaseLib::RpcType::websocket) //Send close packet
        {
            std::vector<char> payload;
            std::vector<char> response;
            BaseLib::WebSocket::encode(payload, BaseLib::WebSocket::Header::Opcode::close, response);
            sendRPCResponseToClient(client, response, false);
        }
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    //This point is only reached, when stopServer is true, the socket is closed or an error occured
    closeClientConnection(client);
}

std::shared_ptr<BaseLib::FileDescriptor> RpcServer::getClientSocketDescriptor(std::string& address, int32_t& port)
{
    std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
    try
    {
        {
            //Don't lock _stateMutex => no synchronisation needed
            if(_clients.size() > GD::bl->settings.rpcServerMaxConnections())
            {
                collectGarbage();
                if(_clients.size() > GD::bl->settings.rpcServerMaxConnections())
                {
                    _out.printError("Error: There are too many clients connected to me. Closing oldest connection. You can increase the number of allowed connections in main.conf.");

                    std::shared_ptr<Client> clientToClose;
                    {
                        std::lock_guard<std::mutex> stateGuard(_stateMutex);
                        for(auto& client : _clients)
                        {
                            if(!client.second->closed) clientToClose = client.second;
                        }
                    }

                    closeClientConnection(clientToClose);
                    collectGarbage();
                }
            }
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        fd_set readFileDescriptor;
        int32_t nfds = 0;
        FD_ZERO(&readFileDescriptor);
        {
            auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
            fileDescriptorGuard.lock();
            nfds = _serverFileDescriptor->descriptor + 1;
            if(nfds <= 0)
            {
                fileDescriptorGuard.unlock();
                GD::out.printError("Error: Server file descriptor is invalid.");
                return fileDescriptor;
            }
            FD_SET(_serverFileDescriptor->descriptor, &readFileDescriptor);
        }
        if(!select(nfds, &readFileDescriptor, NULL, NULL, &timeout))
        {
            if(GD::bl->hf.getTime() - _lastGargabeCollection > 60000 || _clients.size() > GD::bl->settings.rpcServerMaxConnections() * 100 / 112) collectGarbage();
            return fileDescriptor;
        }

        struct sockaddr_storage clientInfo;
        socklen_t addressSize = sizeof(addressSize);
        fileDescriptor = GD::bl->fileDescriptorManager.add(accept(_serverFileDescriptor->descriptor, (struct sockaddr*) &clientInfo, &addressSize));
        if(!fileDescriptor) return fileDescriptor;

        getpeername(fileDescriptor->descriptor, (struct sockaddr*) &clientInfo, &addressSize);

        char ipString[INET6_ADDRSTRLEN];
        if(clientInfo.ss_family == AF_INET)
        {
            struct sockaddr_in* s = (struct sockaddr_in*) &clientInfo;
            port = ntohs(s->sin_port);
            inet_ntop(AF_INET, &s->sin_addr, ipString, sizeof(ipString));
        }
        else
        { // AF_INET6
            struct sockaddr_in6* s = (struct sockaddr_in6*) &clientInfo;
            port = ntohs(s->sin6_port);
            inet_ntop(AF_INET6, &s->sin6_addr, ipString, sizeof(ipString));
        }
        address = std::string(&ipString[0]);

        _out.printInfo("Info: Connection from " + address + ":" + std::to_string(port) + " accepted. Client number: " + std::to_string(fileDescriptor->id));
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return fileDescriptor;
}

void RpcServer::getSSLSocketDescriptor(std::shared_ptr<Client> client)
{
    try
    {
        if(!_tlsPriorityCache)
        {
            _out.printError("Error: Could not initiate TLS connection. _tlsPriorityCache is nullptr.");
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        if(!_x509Cred)
        {
            _out.printError("Error: Could not initiate TLS connection. _x509Cred is nullptr.");
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        int32_t result = 0;
        if((result = gnutls_init(&client->socketDescriptor->tlsSession, GNUTLS_SERVER)) != GNUTLS_E_SUCCESS)
        {
            _out.printError("Error: Could not initialize TLS session: " + std::string(gnutls_strerror(result)));
            client->socketDescriptor->tlsSession = nullptr;
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        if(!client->socketDescriptor->tlsSession)
        {
            _out.printError("Error: Client TLS session is nullptr.");
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        gnutls_transport_set_pull_timeout_function(client->socketDescriptor->tlsSession, gnutls_system_recv_timeout);
        if((result = gnutls_priority_set(client->socketDescriptor->tlsSession, _tlsPriorityCache)) != GNUTLS_E_SUCCESS)
        {
            _out.printError("Error: Could not set cipher priority on TLS session: " + std::string(gnutls_strerror(result)));
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        if((result = gnutls_credentials_set(client->socketDescriptor->tlsSession, GNUTLS_CRD_CERTIFICATE, _x509Cred)) != GNUTLS_E_SUCCESS)
        {
            _out.printError("Error: Could not set x509 credentials on TLS session: " + std::string(gnutls_strerror(result)));
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        if(!client->socketDescriptor || client->socketDescriptor->descriptor == -1)
        {
            _out.printError("Error setting TLS socket descriptor: Provided socket descriptor is invalid.");
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }
        gnutls_transport_set_ptr(client->socketDescriptor->tlsSession, (gnutls_transport_ptr_t) (uintptr_t) client->socketDescriptor->descriptor);

        if(_info->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::cert) gnutls_certificate_server_set_request(client->socketDescriptor->tlsSession, GNUTLS_CERT_REQUIRE);
        else if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::cert) gnutls_certificate_server_set_request(client->socketDescriptor->tlsSession, GNUTLS_CERT_REQUEST);
        gnutls_handshake_set_timeout(client->socketDescriptor->tlsSession, 5000);
        do
        {
            result = gnutls_handshake(client->socketDescriptor->tlsSession);
        } while(result < 0 && gnutls_error_is_fatal(result) == 0);
        if(result < 0)
        {
            _out.printWarning("Warning: TLS handshake has failed: " + std::string(gnutls_strerror(result)));
            GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
            return;
        }

        if(_info->authType & BaseLib::Rpc::ServerInfo::Info::AuthType::cert)
        {
            std::string error;
            if(!client->auth->certificateServer(client->socketDescriptor, client->user, client->acls, error))
            {
                if(_info->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::cert)
                {
                    _out.printError("Error: Authentication failed: " + error);
                    GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
                    return;
                }
                else _out.printInfo("Info: Certificate authentication failed. Falling back to next authentication type. Error was: " + error);
            }
            else _out.printInfo("Info: User [" + client->user + "] was successfully authenticated using certificate authentication.");
        }

        return;
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::bl->fileDescriptorManager.shutdown(client->socketDescriptor);
}

void RpcServer::getSocketDescriptor()
{
    try
    {
        if(_info->socketDescriptor)
        {
            _serverFileDescriptor = _info->socketDescriptor;
            return;
        }

        addrinfo hostInfo;
        addrinfo* serverInfo = nullptr;

        int32_t yes = 1;

        memset(&hostInfo, 0, sizeof(hostInfo));

        hostInfo.ai_family = AF_UNSPEC;
        hostInfo.ai_socktype = SOCK_STREAM;
        hostInfo.ai_flags = AI_PASSIVE;
        std::array<char, 100> buffer;
        std::string port = std::to_string(_info->port);
        int32_t result;
        if((result = getaddrinfo(_info->interface.c_str(), port.c_str(), &hostInfo, &serverInfo)) != 0)
        {
            _out.printCritical("Error: Could not get address information: " + std::string(gai_strerror(result)));
            return;
        }

        bool bound = false;
        int32_t error = 0;
        for(struct addrinfo* info = serverInfo; info != 0; info = info->ai_next)
        {
            _serverFileDescriptor = GD::bl->fileDescriptorManager.add(socket(info->ai_family, info->ai_socktype, info->ai_protocol));
            if(_serverFileDescriptor->descriptor == -1) continue;
            if(!(fcntl(_serverFileDescriptor->descriptor, F_GETFL) & O_NONBLOCK))
            {
                if(fcntl(_serverFileDescriptor->descriptor, F_SETFL, fcntl(_serverFileDescriptor->descriptor, F_GETFL) | O_NONBLOCK) < 0) throw BaseLib::Exception("Error: Could not set socket options.");
            }
            if(setsockopt(_serverFileDescriptor->descriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1) throw BaseLib::Exception("Error: Could not set socket options.");
            if(bind(_serverFileDescriptor->descriptor, info->ai_addr, info->ai_addrlen) == -1)
            {
                error = errno;
                continue;
            }
            switch(info->ai_family)
            {
                case AF_INET:
                    inet_ntop(info->ai_family, &((struct sockaddr_in*) info->ai_addr)->sin_addr, buffer.data(), buffer.size());
                    _info->address = std::string(buffer.data());
                    break;
                case AF_INET6:
                    inet_ntop(info->ai_family, &((struct sockaddr_in6*) info->ai_addr)->sin6_addr, buffer.data(), buffer.size());
                    _info->address = std::string(buffer.data());
                    break;
            }
            _out.printInfo("Info: RPC Server started listening on address " + _info->address + " and port " + port);
            bound = true;
            break;
        }
        freeaddrinfo(serverInfo);
        if(!bound)
        {
            GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
            _out.printCritical("Error: Server could not start listening on port " + port + ": " + std::string(strerror(error)));
            return;
        }
        if(_serverFileDescriptor->descriptor == -1 || !bound || listen(_serverFileDescriptor->descriptor, _backlog) == -1)
        {
            GD::bl->fileDescriptorManager.shutdown(_serverFileDescriptor);
            _out.printCritical("Error: Server could not start listening on port " + port + ": " + std::string(strerror(errno)));
            return;
        }
        if(_info->address == "0.0.0.0" || _info->address == "::") _info->address = BaseLib::Net::getMyIpAddress();
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}

}
