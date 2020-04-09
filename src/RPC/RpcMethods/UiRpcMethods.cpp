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

#include "UiRpcMethods.h"
#include "../../GD/GD.h"

namespace Homegear
{
namespace RpcMethods
{

BaseLib::PVariable RpcAddUiElement::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                         //UI element name; UI element data
                                                                                                                         std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tStruct}),
                                                                                                                         //UI element name; UI element data; UI element metadata
                                                                                                                         std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct}),
                                                                                                                         //Peer ID; channel; variable name; label
                                                                                                                         std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString, BaseLib::VariableType::tString})
                                                                                                                 }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("addUiElement")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        if(parameters->size() == 4)
        {
            BaseLib::PVariable variableArray;
            std::string label;
            variableArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            variableArray->arrayValue->reserve(3);
            variableArray->arrayValue->push_back(parameters->at(0));
            variableArray->arrayValue->push_back(parameters->at(1));
            variableArray->arrayValue->push_back(parameters->at(2));
            label = parameters->at(3)->stringValue;
            return GD::uiController->addUiElementSimple(clientInfo, label, variableArray, false);
        }
        else
        {
            auto metadata = (parameters->size() == 3 ? parameters->at(2) : std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
            return GD::uiController->addUiElement(clientInfo, parameters->at(0)->stringValue, parameters->at(1), metadata);
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcCheckUiElementSimpleCreation::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                //Peer ID; channel; variable name
                std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString})
        }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("checkUiElementSimpleCreation")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        BaseLib::PVariable variableArray;
        variableArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        variableArray->arrayValue->reserve(3);
        variableArray->arrayValue->push_back(parameters->at(0));
        variableArray->arrayValue->push_back(parameters->at(1));
        variableArray->arrayValue->push_back(parameters->at(2));
        auto result = GD::uiController->addUiElementSimple(clientInfo, "", variableArray, true);
        return std::make_shared<BaseLib::Variable>(!result->errorStruct);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetAllUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
        }));
        if(error != ParameterError::Enum::noError) return getError(error);

        return GD::uiController->getAllUiElements(clientInfo, parameters->at(0)->stringValue);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetAvailableUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAvailableUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                         std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
                                                                                                                 }));
        if(error != ParameterError::Enum::noError) return getError(error);

        return GD::uiController->getAvailableUiElements(clientInfo, parameters->at(0)->stringValue);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetCategoryUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString})
        }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("getCategoryUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        return GD::uiController->getUiElementsInCategory(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->stringValue);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetRoomUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                                                                                                                         std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
                                                                                                                 }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("getRoomUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        return GD::uiController->getUiElementsInRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->stringValue);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcGetUiElementMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64})
        }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("getUiElementMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        return GD::uiController->getUiElementMetadata(clientInfo, (uint64_t) parameters->at(0)->integerValue64);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcRemoveUiElement::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64})
        }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("removeUiElement")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        return GD::uiController->removeUiElement(clientInfo, (uint64_t) parameters->at(0)->integerValue64);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RpcSetUiElementMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
    try
    {
        ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
                std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tStruct})
        }));
        if(error != ParameterError::Enum::noError) return getError(error);

        if(!clientInfo || !clientInfo->acls->checkMethodAccess("setUiElementMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        return GD::uiController->setUiElementMetadata(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1));
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}
}