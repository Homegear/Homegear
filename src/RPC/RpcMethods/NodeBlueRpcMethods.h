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

#ifndef HOMEGEAR_NODEBLUERPCMETHODS_H
#define HOMEGEAR_NODEBLUERPCMETHODS_H

#include <homegear-base/Variable.h>
#include <homegear-base/Encoding/RpcMethod.h>

namespace Homegear::RpcMethods {

class RPCAddNodesToFlow : public BaseLib::Rpc::RpcMethod {
 public:
  RPCAddNodesToFlow() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCFlowHasTag : public BaseLib::Rpc::RpcMethod {
 public:
  RPCFlowHasTag() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetNodeBlueDataKeys : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetNodeBlueDataKeys() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetNodeData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetNodeData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetFlowData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetFlowData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetGlobalData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetGlobalData() {
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetNodeEvents : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetNodeEvents() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetNodesWithFixedInputs : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetNodesWithFixedInputs() {
    addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCGetNodeVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCGetNodeVariable() {
    addSignature(BaseLib::VariableType::tVariant,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCRemoveNodesFromFlow : public BaseLib::Rpc::RpcMethod {
 public:
  RPCRemoveNodesFromFlow() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetNodeData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetNodeData() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tArray});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetFlowData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetFlowData() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tArray});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetGlobalData : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetGlobalData() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray, BaseLib::VariableType::tArray});
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RPCSetNodeVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RPCSetNodeVariable() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tVariant});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

}
#endif //HOMEGEAR_NODEBLUERPCMETHODS_H
