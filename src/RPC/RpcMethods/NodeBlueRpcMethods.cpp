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

#include "NodeBlueRpcMethods.h"
#include "../../GD/GD.h"

namespace Homegear {
namespace RpcMethods {

BaseLib::PVariable RPCAddNodesToFlow::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tArray})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("addNodesToFlow"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::nodeBlueServer->addNodesToFlow(parameters->at(0)->stringValue,
                                              parameters->at(1)->stringValue,
                                              parameters->at(2));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCFlowHasTag::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tString})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("flowHasTag"))
      return BaseLib::Variable::createError(-32603,
                                            "Unauthorized.");

    return GD::nodeBlueServer->flowHasTag(parameters->at(0)->stringValue, parameters->at(1)->stringValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetNodeBlueDataKeys::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeBlueDataKeys"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);


    auto result = GD::bl->db->getNodeData(parameters->at(0)->stringValue, "", clientInfo->flowsServer || clientInfo->scriptEngineServer);
    if (result->errorStruct) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    auto keys = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    keys->arrayValue->reserve(result->structValue->size());
    for (auto &element : *result->structValue) {
      keys->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(element.first));
    }
    return keys;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetNodeData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeData"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (parameters->size() == 2 && parameters->at(1)->type == BaseLib::VariableType::tArray) {
      auto data = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      data->arrayValue->reserve(parameters->at(1)->arrayValue->size());
      for (auto &key : *parameters->at(1)->arrayValue) {
        auto result = GD::bl->db->getNodeData(parameters->at(0)->stringValue, key->stringValue, clientInfo->flowsServer || clientInfo->scriptEngineServer);
        if (result->errorStruct) return result;
        data->arrayValue->emplace_back(result);
      }
      return data;
    } else {
      std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
      return GD::bl->db->getNodeData(parameters->at(0)->stringValue, key, clientInfo->flowsServer || clientInfo->scriptEngineServer);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetFlowData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getFlowData"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (parameters->size() == 2 && parameters->at(1)->type == BaseLib::VariableType::tArray) {
      auto data = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      data->arrayValue->reserve(parameters->at(1)->arrayValue->size());
      for (auto &key : *parameters->at(1)->arrayValue) {
        auto result = GD::bl->db->getNodeData(parameters->at(0)->stringValue, key->stringValue, clientInfo->flowsServer || clientInfo->scriptEngineServer);
        if (result->errorStruct) return result;
        data->arrayValue->emplace_back(result);
      }
      return data;
    } else {
      std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
      return GD::bl->db->getNodeData(parameters->at(0)->stringValue, key, clientInfo->flowsServer || clientInfo->scriptEngineServer);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetGlobalData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getGlobalData"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>(),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!parameters->empty() && parameters->at(0)->type == BaseLib::VariableType::tArray) {
      auto data = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      data->arrayValue->reserve(parameters->at(0)->arrayValue->size());
      for (auto &key : *parameters->at(0)->arrayValue) {
        auto result = GD::bl->db->getNodeData("global", key->stringValue, clientInfo->flowsServer || clientInfo->scriptEngineServer);
        if (result->errorStruct) return result;
        data->arrayValue->emplace_back(result);
      }
      return data;
    } else {
      std::string key = parameters->empty() ? "" : parameters->at(0)->stringValue;
      return GD::bl->db->getNodeData("global", key, clientInfo->flowsServer || clientInfo->scriptEngineServer);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetNodeEvents::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeEvents"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::rpcClient->getNodeEvents();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetNodesWithFixedInputs::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getNodesWithFixedInputs"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    if (GD::nodeBlueServer) return GD::nodeBlueServer->getNodesWithFixedInputs();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetNodeVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeVariable"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tString})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (GD::nodeBlueServer)
      return GD::nodeBlueServer->getNodeVariable(parameters->at(0)->stringValue,
                                                 parameters->at(1)->stringValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCRemoveNodesFromFlow::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tString})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (!clientInfo || !clientInfo->acls->checkMethodAccess("removeNodesFromFlow"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    return GD::nodeBlueServer->removeNodesFromFlow(parameters->at(0)->stringValue, parameters->at(1)->stringValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSetNodeData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setNodeData"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    //The signatures are the same as for Node-RED
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tArray}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (parameters->at(1)->type == BaseLib::VariableType::tArray) {
      for (uint32_t i = 0; i < parameters->at(1)->arrayValue->size(); i++) {
        BaseLib::PVariable value;
        if (parameters->at(2)->type == BaseLib::VariableType::tArray) {
          value = (i >= parameters->at(2)->arrayValue->size() ? std::make_shared<BaseLib::Variable>() : parameters->at(2)->arrayValue->at(i));
        } else value = parameters->at(2);
        auto result = GD::bl->db->setNodeData(parameters->at(0)->stringValue, parameters->at(1)->arrayValue->at(i)->stringValue, value);
        if (result->errorStruct) return result;
      }
      return std::make_shared<BaseLib::Variable>();
    } else {
      return GD::bl->db->setNodeData(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSetFlowData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setFlowData"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    //The signatures are the same as for Node-RED
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tArray}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (parameters->at(1)->type == BaseLib::VariableType::tArray) {
      for (uint32_t i = 0; i < parameters->at(1)->arrayValue->size(); i++) {
        BaseLib::PVariable value;
        if (parameters->at(2)->type == BaseLib::VariableType::tArray) {
          value = (i >= parameters->at(2)->arrayValue->size() ? std::make_shared<BaseLib::Variable>() : parameters->at(2)->arrayValue->at(i));
        } else value = parameters->at(2);
        GD::nodeBlueServer->broadcastFlowVariableEvent(parameters->at(0)->stringValue, parameters->at(1)->arrayValue->at(i)->stringValue, value);
        auto result = GD::bl->db->setNodeData(parameters->at(0)->stringValue, parameters->at(1)->arrayValue->at(i)->stringValue, value);
        if (result->errorStruct) return result;
      }
      return std::make_shared<BaseLib::Variable>();
    } else {
      GD::nodeBlueServer->broadcastFlowVariableEvent(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
      return GD::bl->db->setNodeData(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSetGlobalData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setGlobalData"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    //The signatures are the same as for Node-RED
    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tVariant}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray, BaseLib::VariableType::tArray}),
                                                                                                                 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray, BaseLib::VariableType::tVariant})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (parameters->at(0)->type == BaseLib::VariableType::tArray) {
      for (uint32_t i = 0; i < parameters->at(0)->arrayValue->size(); i++) {
        BaseLib::PVariable value;
        if (parameters->at(1)->type == BaseLib::VariableType::tArray) {
          value = (i >= parameters->at(1)->arrayValue->size() ? std::make_shared<BaseLib::Variable>() : parameters->at(1)->arrayValue->at(i));
        } else value = parameters->at(1);
        GD::nodeBlueServer->broadcastGlobalVariableEvent(parameters->at(0)->arrayValue->at(i)->stringValue, value);
        auto result = GD::bl->db->setNodeData("global", parameters->at(0)->arrayValue->at(i)->stringValue, value);
        if (result->errorStruct) return result;
      }
      return std::make_shared<BaseLib::Variable>();
    } else {
      GD::nodeBlueServer->broadcastGlobalVariableEvent(parameters->at(0)->stringValue, parameters->at(1));
      return GD::bl->db->setNodeData("global", parameters->at(0)->stringValue, parameters->at(1));
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSetNodeVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters) {
  try {
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setNodeVariable"))
      return BaseLib::Variable::createError(-32603, "Unauthorized.");

    ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                 std::vector<
                                                                                                                     BaseLib::VariableType>(
                                                                                                                     {BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tString,
                                                                                                                      BaseLib::VariableType::tVariant})
                                                                                                             }));
    if (error != ParameterError::Enum::noError) return getError(error);

    if (GD::nodeBlueServer)
      GD::nodeBlueServer->setNodeVariable(parameters->at(0)->stringValue,
                                          parameters->at(1)->stringValue,
                                          parameters->at(2));
    return std::make_shared<BaseLib::Variable>();
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