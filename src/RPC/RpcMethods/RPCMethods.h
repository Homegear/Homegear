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

#ifndef RPCMETHODS_H_
#define RPCMETHODS_H_

#include <homegear-base/BaseLib.h>

#include <vector>
#include <memory>
#include <cstdlib>

namespace Homegear::Rpc {

class RpcServer;

class RPCDevTest : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDevTest() {
    setHelp("Test method for development.");
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSystemGetCapabilities : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSystemGetCapabilities() {
    setHelp("Lists server's RPC capabilities.");
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSystemListMethods : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSystemListMethods() {
    setHelp("Lists all RPC methods.");
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSystemMethodHelp : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSystemMethodHelp() {
    setHelp("Returns a description of a RPC method.");
    addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSystemMethodSignature : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSystemMethodSignature() {
    setHelp("Returns the method's signature.");
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSystemMulticall : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSystemMulticall() {
    setHelp("Calls multiple RPC methods at once to reduce traffic.");
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAcknowledgeGlobalServiceMessage : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAcknowledgeGlobalServiceMessage() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCActivateLinkParamset : public BaseLib::Rpc::RpcMethod {
 public:
  RPCActivateLinkParamset() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tBoolean});

  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddCategoryToChannel : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddCategoryToChannel() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddCategoryToDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddCategoryToDevice() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddRoleToVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddRoleToVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean,
                                                    BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddCategoryToSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddCategoryToSystemVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddCategoryToVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddCategoryToVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddDevice() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddChannelToRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddChannelToRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddDeviceToRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddDeviceToRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddLink : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddLink() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddRoleToSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddRoleToSystemVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddRoomToStory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddRoomToStory() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddSystemVariableToRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddSystemVariableToRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAddVariableToRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddVariableToRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCAggregateRoles : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAggregateRoles() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray,
                                                    BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tStruct, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray,
                                                    BaseLib::VariableType::tStruct, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCheckServiceAccess : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCheckServiceAccess() {
    addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCopyConfig : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCopyConfig() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCClientServerInitialized : public BaseLib::Rpc::RpcMethod {
 public:
  RPCClientServerInitialized() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCreateCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCreateCategory() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCreateDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCreateDevice() {
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCreateRole : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCreateRole() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCreateRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCreateRoom() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCCreateStory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCCreateStory() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteCategory() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteDevice() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteRole : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteRole() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteRoom() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteStory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteStory() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteMetadata() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteNodeData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteNodeData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteSystemVariable() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCDeleteUserData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCDeleteUserData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCExecuteMiscellaneousDeviceMethod : public BaseLib::Rpc::RpcMethod {
 public:
  RPCExecuteMiscellaneousDeviceMethod() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCFamilyExists : public BaseLib::Rpc::RpcMethod {
 public:
  RPCFamilyExists() {
    addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCForceConfigUpdate : public BaseLib::Rpc::RpcMethod {
 public:
  RPCForceConfigUpdate() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetAllMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetAllMetadata() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetAllScripts : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetAllScripts() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetAllConfig : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetAllConfig() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetAllValues : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetAllValues() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetAllSystemVariables : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetAllSystemVariables() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetCategories : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetCategories() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetCategoryMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetCategoryMetadata() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetChannelsInCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetChannelsInCategory() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetChannelsInRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetChannelsInRoom() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetConfigParameter : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetConfigParameter() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetDeviceDescription : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetDeviceDescription() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetDeviceInfo : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetDeviceInfo() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetDevicesInCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetDevicesInCategory() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetDevicesInRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetDevicesInRoom() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetLastEvents : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetLastEvents() {
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetInstallMode : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetInstallMode() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetInstanceId : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetInstanceId() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetKeyMismatchDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetKeyMismatchDevice() {
    addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetLinkInfo : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetLinkInfo() {
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetLinkPeers : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetLinkPeers() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetLinks : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetLinks() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetMetadata() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetName : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetName() {
    addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tString,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetPairingInfo : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetPairingInfo() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetPairingState : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetPairingState() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetParamsetDescription : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetParamsetDescription() {
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetParamsetId : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetParamsetId() {
    addSignature(BaseLib::VariableType::tString,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tString,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tString,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetParamset : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetParamset() {
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetPeerId : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetPeerId() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRoles : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRoles() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRolesInDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRolesInDevice() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRolesInRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRolesInRoom() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRoleMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRoleMetadata() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRoomMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRoomMetadata() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRooms : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRooms() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetRoomsInStory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetRoomsInStory() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetServiceMessages : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetServiceMessages() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetSniffedDevices : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetSniffedDevices() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetStories : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetStories() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetStoryMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetStoryMetadata() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetSystemVariable() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetSystemVariableFlags : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetSystemVariableFlags() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetSystemVariablesInCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetSystemVariablesInCategory() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetSystemVariablesInRole : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetSystemVariablesInRole() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetSystemVariablesInRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetSystemVariablesInRoom() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetUpdateStatus : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetUpdateStatus() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetUserData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetUserData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetUserMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetUserMetadata() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetVariablesInCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetVariablesInCategory() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetVariablesInRole : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetVariablesInRole() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetVariablesInRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetVariablesInRoom() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetValue : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetValue() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean,
                                                    BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetVariableDescription : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetVariableDescription() {
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetVersion : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetVersion() {
    addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCInit : public BaseLib::Rpc::RpcMethod {
 public:
  RPCInit() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  virtual ~RPCInit();

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;

 private:
  bool _disposing = false;
  std::mutex _initServerThreadMutex;
  std::thread _initServerThread;
  const std::unordered_set<std::string> _reservedIds{"homegear", "scriptEngine", "ipcServer", "nodeBlue", "mqtt"};
};

class RPCInvokeFamilyMethod : public BaseLib::Rpc::RpcMethod {
 public:
  RPCInvokeFamilyMethod() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCLifetick : public BaseLib::Rpc::RpcMethod {
 public:
  RPCLifetick() {
    addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListBidcosInterfaces : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListBidcosInterfaces() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListClientServers : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListClientServers() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListDevices : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListDevices() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray,
                                                     BaseLib::VariableType::tInteger}));
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListFamilies : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListFamilies() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}));
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListInterfaces : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListInterfaces() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListKnownDeviceTypes : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListKnownDeviceTypes() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}));
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean,
                                                     BaseLib::VariableType::tArray}));
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCListTeams : public BaseLib::Rpc::RpcMethod {
 public:
  RPCListTeams() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCLogLevel : public BaseLib::Rpc::RpcMethod {
 public:
  RPCLogLevel() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCPeerExists : public BaseLib::Rpc::RpcMethod {
 public:
  RPCPeerExists() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64});
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger64});

  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCPing : public BaseLib::Rpc::RpcMethod {
 public:
  RPCPing() {
    addSignature(BaseLib::VariableType::tBoolean, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});

  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCPutParamset : public BaseLib::Rpc::RpcMethod {
 public:
  RPCPutParamset() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tStruct});

  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveCategoryFromChannel : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveCategoryFromChannel() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveCategoryFromDevice : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveCategoryFromDevice() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveCategoryFromSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveCategoryFromSystemVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveCategoryFromVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveCategoryFromVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveChannelFromRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveChannelFromRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveDeviceFromRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveDeviceFromRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveLink : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveLink() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveRoleFromSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveRoleFromSystemVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveRoleFromVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveRoleFromVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveRoomFromStory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveRoomFromStory() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveSystemVariableFromRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveSystemVariableFromRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveVariableFromRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveVariableFromRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCReportValueUsage : public BaseLib::Rpc::RpcMethod {
 public:
  RPCReportValueUsage() {
    addSignature(BaseLib::VariableType::tArray,
                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                     BaseLib::VariableType::tInteger}));
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRssiInfo : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRssiInfo() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRunScript : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRunScript() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tBoolean});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSearchDevices : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSearchDevices() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSearchInterfaces : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSearchInterfaces() {
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tInteger,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetCategoryMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetCategoryMetadata() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetGlobalServiceMessage : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetGlobalServiceMessage() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tArray,
                                                    BaseLib::VariableType::tVariant, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetLinkInfo : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetLinkInfo() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetMetadata() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetData() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetId : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetId() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetInstallMode : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetInstallMode() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean,
                                                    BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean,
                                                    BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetInterface : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetInterface() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetLanguage : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetLanguage() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetName : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetName() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetRoleMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetRoleMetadata() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetRoomMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetRoomMetadata() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetStoryMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetStoryMetadata() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetSystemVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetSystemVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetTeam : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetTeam() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetUserData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetUserData() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetUserMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetUserMetadata() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetValue : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetValue() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tVariant,
                                                    BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger,
                                                    BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCStartSniffing : public BaseLib::Rpc::RpcMethod {
 public:
  RPCStartSniffing() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCStopSniffing : public BaseLib::Rpc::RpcMethod {
 public:
  RPCStopSniffing() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSubscribePeers : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSubscribePeers() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCTriggerRpcEvent : public BaseLib::Rpc::RpcMethod {
 public:
  RPCTriggerRpcEvent() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCUnsubscribePeers : public BaseLib::Rpc::RpcMethod {
 public:
  RPCUnsubscribePeers() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCUpdateCategory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCUpdateCategory() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct,
                                                    BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCUpdateFirmware : public BaseLib::Rpc::RpcMethod {
 public:
  RPCUpdateFirmware() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCUpdateRole : public BaseLib::Rpc::RpcMethod {
 public:
  RPCUpdateRole() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct,
                                                    BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCUpdateRoom : public BaseLib::Rpc::RpcMethod {
 public:
  RPCUpdateRoom() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct,
                                                    BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCUpdateStory : public BaseLib::Rpc::RpcMethod {
 public:
  RPCUpdateStory() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct,
                                                    BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCWriteLog : public BaseLib::Rpc::RpcMethod {
 public:
  RPCWriteLog() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tVoid, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tVoid, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

}
#endif
