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

#ifndef HOMEGEAR_UIRPCMETHODS_H
#define HOMEGEAR_UIRPCMETHODS_H

#include <homegear-base/Variable.h>
#include <homegear-base/Encoding/RpcMethod.h>

namespace Homegear::RpcMethods {

class RpcAddUiElement : public BaseLib::Rpc::RpcMethod {
 public:
  RpcAddUiElement() {
    addSignature(BaseLib::VariableType::tInteger64,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tInteger64,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tStruct,
                                                    BaseLib::VariableType::tStruct});
    addSignature(BaseLib::VariableType::tInteger64,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64,
                                                    BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString,
                                                    BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcCheckUiElementSimpleCreation : public BaseLib::Rpc::RpcMethod {
 public:
  RpcCheckUiElementSimpleCreation() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64,
                                                    BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetAllUiElements : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetAllUiElements() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};


class RpcGetAvailableUiElements : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetAvailableUiElements() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetCategoryUiElements : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetCategoryUiElements() {
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetRoomUiElements : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetRoomUiElements() {
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetUiElement : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetUiElement() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetUiElementMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetUiElementMetadata() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetUiElementsWithVariable : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetUiElementsWithVariable() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetUiElementTemplate : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetUiElementTemplate() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcRequestUiRefresh : public BaseLib::Rpc::RpcMethod {
 public:
  RpcRequestUiRefresh() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcRemoveUiElement : public BaseLib::Rpc::RpcMethod {
 public:
  RpcRemoveUiElement() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcRemoveNodeUiElements : public BaseLib::Rpc::RpcMethod {
 public:
  RpcRemoveNodeUiElements() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcSetUiElementMetadata : public BaseLib::Rpc::RpcMethod {
 public:
  RpcSetUiElementMetadata() {
    addSignature(BaseLib::VariableType::tVoid,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

}

#endif //HOMEGEAR_UIRPCMETHODS_H
