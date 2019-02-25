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

#ifndef RPCMETHODS_H_
#define RPCMETHODS_H_

#include <homegear-base/BaseLib.h>

#include <vector>
#include <memory>
#include <cstdlib>

namespace Homegear
{

namespace Rpc
{

class RpcServer;

class RPCDevTest : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDevTest()
    {
        setHelp("Test method for development.");
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSystemGetCapabilities : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSystemGetCapabilities()
    {
        setHelp("Lists server's RPC capabilities.");
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSystemListMethods : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSystemListMethods()
    {
        setHelp("Lists all RPC methods.");
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSystemMethodHelp : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSystemMethodHelp()
    {
        setHelp("Returns a description of a RPC method.");
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSystemMethodSignature : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSystemMethodSignature()
    {
        setHelp("Returns the method's signature.");
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSystemMulticall : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSystemMulticall()
    {
        setHelp("Calls multiple RPC methods at once to reduce traffic.");
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAbortEventReset : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAbortEventReset()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAcknowledgeGlobalServiceMessage : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAcknowledgeGlobalServiceMessage()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCActivateLinkParamset : public BaseLib::Rpc::RpcMethod
{
public:
    RPCActivateLinkParamset()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});

    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddCategoryToChannel : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddCategoryToChannel()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddCategoryToDevice : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddCategoryToDevice()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddRoleToVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddRoleToVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddCategoryToSystemVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddCategoryToSystemVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddCategoryToVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddCategoryToVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddDevice : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddDevice()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddChannelToRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddChannelToRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddDeviceToRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddDeviceToRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddEvent : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddEvent()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddLink : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddLink()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddRoomToStory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddRoomToStory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddSystemVariableToRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddSystemVariableToRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddUiElement : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddUiElement()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCAddVariableToRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddVariableToRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCheckServiceAccess : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCheckServiceAccess()
    {
        addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCopyConfig : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCopyConfig()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCClientServerInitialized : public BaseLib::Rpc::RpcMethod
{
public:
    RPCClientServerInitialized()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCreateCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCreateCategory()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCreateDevice : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCreateDevice()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCreateRole : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCreateRole()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCreateRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCreateRoom()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCreateStory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCreateStory()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteData()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteCategory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteDevice : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteDevice()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteRole : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteRole()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteStory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteStory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteNodeData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteNodeData()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteSystemVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteSystemVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCEnableEvent : public BaseLib::Rpc::RpcMethod
{
public:
    RPCEnableEvent()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCExecuteMiscellaneousDeviceMethod : public BaseLib::Rpc::RpcMethod
{
public:
    RPCExecuteMiscellaneousDeviceMethod()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCFamilyExists : public BaseLib::Rpc::RpcMethod
{
public:
    RPCFamilyExists()
    {
        addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCForceConfigUpdate : public BaseLib::Rpc::RpcMethod
{
public:
    RPCForceConfigUpdate()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAllMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAllMetadata()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAllScripts : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAllScripts()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAllConfig : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAllConfig()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAllUiElements : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAllUiElements()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAllValues : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAllValues()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAllSystemVariables : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAllSystemVariables()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetAvailableUiElements : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetAvailableUiElements()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetCategories : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetCategories()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetCategoryMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetCategoryMetadata()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetCategoryUiElements : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetCategoryUiElements()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetChannelsInCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetChannelsInCategory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetChannelsInRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetChannelsInRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetConfigParameter : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetConfigParameter()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetData()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetDeviceDescription : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetDeviceDescription()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetDeviceInfo : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetDeviceInfo()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetDevicesInCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetDevicesInCategory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetDevicesInRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetDevicesInRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetEvent : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetEvent()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetLastEvents : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetLastEvents()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetInstallMode : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetInstallMode()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetKeyMismatchDevice : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetKeyMismatchDevice()
    {
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetLinkInfo : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetLinkInfo()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetLinkPeers : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetLinkPeers()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetLinks : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetLinks()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetMetadata()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetName : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetName()
    {
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetNodeData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetNodeData()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetFlowData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetFlowData()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetGlobalData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetGlobalData()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetNodeEvents : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetNodeEvents()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetNodesWithFixedInputs : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetNodesWithFixedInputs()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetNodeVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetNodeVariable()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetPairingInfo : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetPairingInfo()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetPairingState : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetPairingState()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetParamsetDescription : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetParamsetDescription()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetParamsetId : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetParamsetId()
    {
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetParamset : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetParamset()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetPeerId : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetPeerId()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetRoles : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetRoles()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetRoleMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetRoleMetadata()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetRoomMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetRoomMetadata()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetRooms : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetRooms()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetRoomsInStory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetRoomsInStory()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetRoomUiElements : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetRoomUiElements()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetServiceMessages : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetServiceMessages()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetSniffedDevices : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetSniffedDevices()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetStories : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetStories()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetStoryMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetStoryMetadata()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetSystemVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetSystemVariable()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetSystemVariableFlags : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetSystemVariableFlags()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetSystemVariablesInCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetSystemVariablesInCategory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetSystemVariablesInRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetSystemVariablesInRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetUpdateStatus : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetUpdateStatus()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetUserMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetUserMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetVariablesInCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetVariablesInCategory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetVariablesInRole : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetVariablesInRole()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetVariablesInRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetVariablesInRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetValue : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetValue()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetVariableDescription : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetVariableDescription()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetVersion : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetVersion()
    {
        addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCInit : public BaseLib::Rpc::RpcMethod
{
public:
    RPCInit()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    virtual ~RPCInit();

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);

private:
    bool _disposing = false;
    std::mutex _initServerThreadMutex;
    std::thread _initServerThread;
    const std::unordered_set<std::string> _reservedIds{"homegear", "scriptEngine", "ipcServer", "nodeBlue", "mqtt"};
};

class RPCInvokeFamilyMethod : public BaseLib::Rpc::RpcMethod
{
public:
    RPCInvokeFamilyMethod()
    {
        addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListBidcosInterfaces : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListBidcosInterfaces()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListClientServers : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListClientServers()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListDevices : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListDevices()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray, BaseLib::VariableType::tInteger}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListEvents : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListEvents()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}));
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}));
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListFamilies : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListFamilies()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListInterfaces : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListInterfaces()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListKnownDeviceTypes : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListKnownDeviceTypes()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}));
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCListTeams : public BaseLib::Rpc::RpcMethod
{
public:
    RPCListTeams()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCLogLevel : public BaseLib::Rpc::RpcMethod
{
public:
    RPCLogLevel()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCNodeOutput : public BaseLib::Rpc::RpcMethod
{
public:
    RPCNodeOutput()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCPeerExists : public BaseLib::Rpc::RpcMethod
{
public:
    RPCPeerExists()
    {
        addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64});
        addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger64});

    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCPing : public BaseLib::Rpc::RpcMethod
{
public:
    RPCPing()
    {
        addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});

    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCPutParamset : public BaseLib::Rpc::RpcMethod
{
public:
    RPCPutParamset()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});

    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveCategoryFromChannel : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveCategoryFromChannel()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveCategoryFromDevice : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveCategoryFromDevice()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveCategoryFromSystemVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveCategoryFromSystemVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveCategoryFromVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveCategoryFromVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveChannelFromRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveChannelFromRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveDeviceFromRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveDeviceFromRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveEvent : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveEvent()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveLink : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveLink()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveRoleFromVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveRoleFromVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveRoomFromStory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveRoomFromStory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveSystemVariableFromRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveSystemVariableFromRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveUiElement : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveUiElement()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveVariableFromRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveVariableFromRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCReportValueUsage : public BaseLib::Rpc::RpcMethod
{
public:
    RPCReportValueUsage()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger}));
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRssiInfo : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRssiInfo()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRunScript : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRunScript()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSearchDevices : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSearchDevices()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSearchInterfaces : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSearchInterfaces()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetCategoryMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetCategoryMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetGlobalServiceMessage : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetGlobalServiceMessage()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetLinkInfo : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetLinkInfo()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetData()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetId : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetId()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetInstallMode : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetInstallMode()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetInterface : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetInterface()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetLanguage : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetLanguage()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetName : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetName()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetNodeData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetNodeData()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetFlowData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetFlowData()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetGlobalData : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetGlobalData()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetNodeVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetNodeVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetRoleMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetRoleMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetRoomMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetRoomMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetStoryMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetStoryMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetSystemVariable : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetSystemVariable()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetTeam : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetTeam()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetUserMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetUserMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetValue : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetValue()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCStartSniffing : public BaseLib::Rpc::RpcMethod
{
public:
    RPCStartSniffing()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCStopSniffing : public BaseLib::Rpc::RpcMethod
{
public:
    RPCStopSniffing()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSubscribePeers : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSubscribePeers()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCTriggerEvent : public BaseLib::Rpc::RpcMethod
{
public:
    RPCTriggerEvent()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCTriggerRpcEvent : public BaseLib::Rpc::RpcMethod
{
public:
    RPCTriggerRpcEvent()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUnsubscribePeers : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUnsubscribePeers()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUpdateCategory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUpdateCategory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUpdateFirmware : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUpdateFirmware()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUpdateRole : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUpdateRole()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUpdateRoom : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUpdateRoom()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUpdateStory : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUpdateStory()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCWriteLog : public BaseLib::Rpc::RpcMethod
{
public:
    RPCWriteLog()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tVoid, BaseLib::VariableType::tString});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tVoid, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

}

}
#endif
