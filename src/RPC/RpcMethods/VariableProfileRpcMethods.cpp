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

#include "VariableProfileRpcMethods.h"
#include "../../GD/GD.h"

namespace Homegear {
namespace RpcMethods {

BaseLib::PVariable RpcActivateVariableProfile::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger64}),
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("executeVariableProfile"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::variableProfileManager->activateVariableProfile(clientInfo, (uint64_t)parameters->at(0)->integerValue64);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcAddVariableProfile::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 //Name translations; profile
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tStruct,
                                                                                                                      BaseLib::VariableType::tStruct}),
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("addVariableProfile"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::variableProfileManager->addVariableProfile(clientInfo, parameters->at(0), parameters->at(1));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcDeleteVariableProfile::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 //Name translations; profile
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger64}),
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("deleteVariableProfile"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::variableProfileManager->deleteVariableProfile((uint64_t)parameters->at(0)->integerValue64);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetAllVariableProfiles::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!parameters->empty()) {
      ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                   std::vector<
                                                                                                                       BaseLib::VariableType>(
                                                                                                                       {BaseLib::VariableType::tString})
                                                                                                               }));
      if (error != ParameterError::Enum::noError) return getError(error);
    }

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getAllVariableProfiles"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    std::string languageCode = parameters->empty() ? "" : parameters->at(0)->stringValue;
    return GD::variableProfileManager->getAllVariableProfiles(languageCode);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetVariableProfile::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger64}),
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger64,
                                                                                                                      BaseLib::VariableType::tString}),
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getVariableProfile"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    std::string languageCode = parameters->size() < 2 ? "" : parameters->at(1)->stringValue;
    return GD::variableProfileManager->getVariableProfile((uint64_t)parameters->at(0)->integerValue64, languageCode);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcUpdateVariableProfile::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger64,
                                                                                                                      BaseLib::VariableType::tStruct,
                                                                                                                      BaseLib::VariableType::tStruct}),
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("updateVariableProfile"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::variableProfileManager->updateVariableProfile((uint64_t)parameters->at(0)->integerValue64,
                                                             parameters->at(1),
                                                             parameters->at(2));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}
}