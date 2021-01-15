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

#include "BuildingRpcMethods.h"
#include "../../GD/GD.h"

namespace Homegear {
namespace RpcMethods {

BaseLib::PVariable RPCAddStoryToBuilding::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger,
                                                                                                                      BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("addStoryToBuilding",
                                                                        (uint64_t)parameters->at(1)->integerValue64))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto result = GD::bl->db->addStoryToBuilding((uint64_t)parameters->at(0)->integerValue64,
                                                 (uint64_t)parameters->at(1)->integerValue64);

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

BaseLib::PVariable RPCCreateBuilding::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("createBuilding"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tStruct}),
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tStruct,
                                                                                                                      BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    auto result = GD::bl->db->createBuilding(parameters->at(0),
                                             parameters->size()
                                                 == 2 ? parameters->at(1) : std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));

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

BaseLib::PVariable RPCDeleteBuilding::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("deleteBuilding"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto result = GD::bl->db->deleteBuilding((uint64_t)parameters->at(0)->integerValue64);

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

BaseLib::PVariable RPCGetStoriesInBuilding::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getStoriesInBuilding"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");
    bool checkAcls = clientInfo->acls->roomsReadSet();

    return GD::bl->db->getStoriesInBuilding(clientInfo, (uint64_t)parameters->at(0)->integerValue64, checkAcls);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetBuildings::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getBuildings"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");
    if (!parameters->empty()) {
      ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                   std::vector<
                                                                                                                       BaseLib::VariableType>(
                                                                                                                       {BaseLib::VariableType::tString})
                                                                                                               }));
      if (error != ParameterError::Enum::noError) return getError(error);
    }

    std::string languageCode = parameters->empty() ? "" : parameters->at(0)->stringValue;

    return GD::bl->db->getBuildings(languageCode);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetBuildingMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getBuildingMetadata"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::bl->db->getBuildingMetadata((uint64_t)parameters->at(0)->integerValue64);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCRemoveStoryFromBuilding::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger,
                                                                                                                      BaseLib::VariableType::tInteger})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("removeStoryFromBuilding",
                                                                        (uint64_t)parameters->at(1)->integerValue64))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto result = GD::bl->db->removeStoryFromBuilding((uint64_t)parameters->at(0)->integerValue64,
                                                      (uint64_t)parameters->at(1)->integerValue64);

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

BaseLib::PVariable RPCSetBuildingMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger,
                                                                                                                      BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setBuildingMetadata"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::bl->db->setBuildingMetadata((uint64_t)parameters->at(0)->integerValue64, parameters->at(1));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCUpdateBuilding::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger,
                                                                                                                      BaseLib::VariableType::tStruct}),
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tInteger,
                                                                                                                      BaseLib::VariableType::tStruct,
                                                                                                                      BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("updateBuilding"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    BaseLib::PVariable result;

    if (parameters->size() == 3)
      result = GD::bl->db->updateBuilding((uint64_t)parameters->at(0)->integerValue64,
                                          parameters->at(1),
                                          parameters->at(2));
    else
      result = GD::bl->db->updateBuilding((uint64_t)parameters->at(0)->integerValue64,
                                          parameters->at(1),
                                          std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));

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
}