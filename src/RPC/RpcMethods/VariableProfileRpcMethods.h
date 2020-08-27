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

#ifndef HOMEGEAR_VARIABLEPROFILERPCMETHODS_H
#define HOMEGEAR_VARIABLEPROFILERPCMETHODS_H

#include <homegear-base/Variable.h>
#include <homegear-base/Encoding/RpcMethod.h>

namespace Homegear {
namespace RpcMethods {

class RpcActivateVariableProfile : public BaseLib::Rpc::RpcMethod {
 public:
  RpcActivateVariableProfile() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcAddVariableProfile : public BaseLib::Rpc::RpcMethod {
 public:
  RpcAddVariableProfile() {
    addSignature(BaseLib::VariableType::tInteger64,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcDeleteVariableProfile : public BaseLib::Rpc::RpcMethod {
 public:
  RpcDeleteVariableProfile() {
    addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetAllVariableProfiles : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetAllVariableProfiles() {
    addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcGetVariableProfile : public BaseLib::Rpc::RpcMethod {
 public:
  RpcGetVariableProfile() {
    addSignature(BaseLib::VariableType::tStruct,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

class RpcUpdateVariableProfile : public BaseLib::Rpc::RpcMethod {
 public:
  RpcUpdateVariableProfile() {
    addSignature(BaseLib::VariableType::tBoolean,
                 std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger64, BaseLib::VariableType::tStruct,
                                                    BaseLib::VariableType::tStruct});
  }

  BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) override;
};

}
}

#endif //HOMEGEAR_VARIABLEPROFILERPCMETHODS_H
