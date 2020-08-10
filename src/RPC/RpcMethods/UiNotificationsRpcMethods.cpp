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

#include "UiNotificationsRpcMethods.h"
#include "../../GD/GD.h"

namespace Homegear {
namespace RpcMethods {

BaseLib::PVariable RpcCreateUiNotification::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({ //UI notification description
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("createUiNotification")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto notificationId = GD::bl->db->createUiNotification(parameters->at(0));

    if (GD::nodeBlueServer) GD::nodeBlueServer->broadcastUiNotificationCreated(notificationId);
    #ifndef NO_SCRIPTENGINE
    if (GD::scriptEngineServer) GD::scriptEngineServer->broadcastUiNotificationCreated(notificationId);
    #endif
    GD::rpcClient->broadcastUiNotificationCreated(notificationId);
    if (GD::ipcServer) GD::ipcServer->broadcastUiNotificationCreated(notificationId);

    return std::make_shared<BaseLib::Variable>(notificationId);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetUiNotification::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({ //Database ID, language code
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getUiNotification")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::bl->db->getUiNotification(parameters->at(0)->integerValue64, parameters->at(1)->stringValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetUiNotifications::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({ //Language code
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getUiNotifications")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::bl->db->getUiNotifications(parameters->at(0)->stringValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcRemoveUiNotification::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({ //Language code
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("removeUiNotification")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto notificationId = (uint64_t)parameters->at(0)->integerValue64;

    GD::bl->db->removeUiNotification(notificationId);

    if (GD::nodeBlueServer) GD::nodeBlueServer->broadcastUiNotificationRemoved(notificationId);
    #ifndef NO_SCRIPTENGINE
    if (GD::scriptEngineServer) GD::scriptEngineServer->broadcastUiNotificationRemoved(notificationId);
    #endif
    if (GD::ipcServer) GD::ipcServer->broadcastUiNotificationRemoved(notificationId);
    GD::rpcClient->broadcastUiNotificationRemoved(notificationId);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcUiNotificationAction::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({ //Database ID, button ID
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tInteger64})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("uiNotificationAction")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

    auto notificationId = (uint64_t)parameters->at(0)->integerValue64;
    auto buttonId = (uint64_t)parameters->at(1)->integerValue64;

    auto notification = GD::bl->db->getUiNotification(notificationId, "en-US");
    if (notification->errorStruct) return notification;

    std::string type;
    auto typeIterator = notification->structValue->find("type");
    if (typeIterator != notification->structValue->end()) type = typeIterator->second->stringValue;

    if (GD::nodeBlueServer) GD::nodeBlueServer->broadcastUiNotificationAction(notificationId, type, buttonId);
    #ifndef NO_SCRIPTENGINE
    if (GD::scriptEngineServer) GD::scriptEngineServer->broadcastUiNotificationAction(notificationId, type, buttonId);
    #endif
    if (GD::ipcServer) GD::ipcServer->broadcastUiNotificationAction(notificationId, type, buttonId);
    GD::rpcClient->broadcastUiNotificationAction(notificationId, type, buttonId);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}
}