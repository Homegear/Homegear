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

#include "BuildingPartRpcMethods.h"
#include "../../GD/GD.h"

namespace Homegear::RpcMethods {

BaseLib::PVariable RPCCreateBuildingPart::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("createBuildingPart"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    auto result = GD::bl->db->createBuildingPart(parameters->at(0), parameters->size() == 2 ? parameters->at(1) : std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));

    GD::uiController->requestUiRefresh("");

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

BaseLib::PVariable RPCDeleteBuildingPart::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    uint64_t buildingPartId = (uint64_t)parameters->at(0)->integerValue64;

    if (!clientInfo || !clientInfo->acls->checkMethodAndBuildingPartWriteAccess("deleteBuildingPart", buildingPartId))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto result = GD::bl->db->deleteBuildingPart(buildingPartId);
    GD::bl->db->removeBuildingPartFromBuildings(buildingPartId);

    GD::uiController->requestUiRefresh("");

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

BaseLib::PVariable RPCGetBuildingPartMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    uint64_t buildingPartId = (uint64_t)parameters->at(0)->integerValue64;

    if (!clientInfo || !clientInfo->acls->checkMethodAndBuildingPartReadAccess("getBuildingPartMetadata", buildingPartId))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::bl->db->getBuildingPartMetadata(buildingPartId);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetBuildingParts::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getBuildingParts"))
      return BaseLib::Variable::createError(-32603,
                                            "Unauthorized.");
    if (!parameters->empty()) {
      ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                   std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
                                                                                                               }));
      if (error != ParameterError::Enum::noError) return getError(error);
    }

    std::string languageCode = parameters->empty() ? "" : parameters->at(0)->stringValue;

    bool checkAcls = clientInfo->acls->buildingPartsReadSet();

    return GD::bl->db->getBuildingParts(languageCode, checkAcls);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSetBuildingPartMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setBuildingPartMetadata"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::bl->db->setBuildingPartMetadata((uint64_t)parameters->at(0)->integerValue64, parameters->at(1));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCUpdateBuildingPart::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("updateBuildingPart"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    BaseLib::PVariable result;

    if (parameters->size() == 3) result = GD::bl->db->updateBuildingPart((uint64_t)parameters->at(0)->integerValue64, parameters->at(1), parameters->at(2));
    else result = GD::bl->db->updateBuildingPart((uint64_t)parameters->at(0)->integerValue64, parameters->at(1), std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));

    GD::uiController->requestUiRefresh("");

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

}