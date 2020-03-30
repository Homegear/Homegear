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

#include "UiController.h"
#include "../GD/GD.h"

namespace Homegear
{

UiController::UiController()
{
    _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
    _descriptions = std::unique_ptr<BaseLib::DeviceDescription::UiElements>(new BaseLib::DeviceDescription::UiElements(GD::bl.get()));
}

UiController::~UiController()
{

}

void UiController::load()
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);
        _uiElements.clear();
        _uiElementsByRoom.clear();
        _uiElementsByCategory.clear();
        auto rows = GD::bl->db->getUiElements();
        for(auto& row : *rows)
        {
            auto uiElement = std::make_shared<UiElement>();
            uiElement->databaseId = (uint64_t) row.second.at(0)->intValue;
            uiElement->elementId = row.second.at(1)->textValue;
            uiElement->data = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);
            uiElement->metadata = row.second.at(3)->binaryValue->empty() ? std::make_shared<BaseLib::Variable>() : _rpcDecoder->decodeResponse(*row.second.at(3)->binaryValue);

            if(uiElement->databaseId == 0 || uiElement->elementId.empty()) continue;

            addDataInfo(uiElement, uiElement->data);

            _uiElements.emplace(uiElement->databaseId, uiElement);
            _uiElementsByRoom[uiElement->roomId].emplace(uiElement);
            for(auto categoryId : uiElement->categoryIds)
            {
                _uiElementsByCategory[categoryId].emplace(uiElement);
            }
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

BaseLib::PVariable UiController::addUiElement(BaseLib::PRpcClientInfo clientInfo, const std::string& elementId, const BaseLib::PVariable& data, const BaseLib::PVariable& metadata)
{
    try
    {
        if(elementId.empty()) return BaseLib::Variable::createError(-1, "elementId is empty.");
        if(data->type != BaseLib::VariableType::tStruct) return BaseLib::Variable::createError(-1, "data is not of type Struct.");

        BaseLib::PVariable dataCopy = data;
        BaseLib::PVariable dynamicMetadata = metadata;
        if(!dynamicMetadata || dynamicMetadata->structValue->empty())
        {
            auto metadataIterator = dataCopy->structValue->find("dynamicMetadata");
            if(metadataIterator != dataCopy->structValue->end())
            {
                dynamicMetadata = metadataIterator->second;
                dataCopy->structValue->erase("dynamicMetadata");
            }
        }

        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);
        auto databaseId = GD::bl->db->addUiElement(elementId, dataCopy, dynamicMetadata);

        if(databaseId == 0) return BaseLib::Variable::createError(-1, "Error adding element to database.");

        auto uiElement = std::make_shared<UiElement>();
        uiElement->databaseId = databaseId;
        uiElement->elementId = elementId;
        uiElement->data = dataCopy;
        uiElement->metadata = dynamicMetadata;

        addDataInfo(uiElement, dataCopy);

        _uiElements.emplace(uiElement->databaseId, uiElement);
        _uiElementsByRoom[uiElement->roomId].emplace(uiElement);
        for(auto categoryId : uiElement->categoryIds)
        {
            _uiElementsByCategory[categoryId].emplace(uiElement);
        }

        return std::make_shared<BaseLib::Variable>(databaseId);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::findRoleVariables(const BaseLib::PRpcClientInfo& clientInfo, const BaseLib::PVariable& uiInfo, const BaseLib::PVariable& variable, uint64_t roomId, BaseLib::PVariable& inputPeers, BaseLib::PVariable& outputPeers)
{
    try
    {
        std::list<std::list<uint64_t>> roleIdsIn;
        std::list<std::list<uint64_t>> roleIdsOut;

        { //Get required role Ids
            auto roleIdsInIterator = uiInfo->structValue->find("roleIdsIn");
            auto roleIdsOutIterator = uiInfo->structValue->find("roleIdsOut");

            if(roleIdsInIterator != uiInfo->structValue->end())
            {
                for(auto& roleIdOuter : *roleIdsInIterator->second->arrayValue)
                {
                    std::list<uint64_t> roleIds;
                    for(auto& roleId : *roleIdOuter->arrayValue)
                    {
                        roleIds.push_back(roleId->integerValue64);
                    }
                    roleIdsIn.emplace_back(std::move(roleIds));
                }
            }

            if(roleIdsOutIterator != uiInfo->structValue->end())
            {
                for(auto& roleIdOuter : *roleIdsOutIterator->second->arrayValue)
                {
                    std::list<uint64_t> roleIds;
                    for(auto& roleId : *roleIdOuter->arrayValue)
                    {
                        roleIds.push_back(roleId->integerValue64);
                    }
                    roleIdsOut.emplace_back(std::move(roleIds));
                }
            }
        }

        { //Get nearest variables with the required roles
            inputPeers->arrayValue->reserve(roleIdsIn.size());
            outputPeers->arrayValue->reserve(roleIdsOut.size());

            for(auto& roleIdOuter : roleIdsIn)
            {
                auto outerArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                outerArray->arrayValue->reserve(roleIdOuter.size());
                for(auto roleId : roleIdOuter)
                {
                    BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    requestParameters->arrayValue->reserve(2);
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(roleId));
                    requestParameters->arrayValue->push_back(variable->arrayValue->at(0));
                    std::string methodName = "getVariablesInRole";
                    auto variables = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(variables->errorStruct) return BaseLib::Variable::createError(-1, "Error getting variables for roles required by UI element.");

                    size_t roleCount = 0;

                    auto channelIterator = variables->structValue->find(std::to_string(variable->arrayValue->at(1)->integerValue64));
                    if(channelIterator != variables->structValue->end() && !channelIterator->second->structValue->empty())
                    {
                        auto variableIterator = channelIterator->second->structValue->find(variable->arrayValue->at(2)->stringValue);
                        if(variableIterator != channelIterator->second->structValue->end())
                        {
                            bool wrongDirection = false;
                            auto directionIterator = variableIterator->second->structValue->find("direction");
                            if(directionIterator != variableIterator->second->structValue->end())
                            {
                                if(directionIterator->second->integerValue64 == 1)
                                {
                                    GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator->first + " does not have required direction (input). Simple UI element creation is not possible.");
                                    wrongDirection = true;
                                }
                            }

                            if(!wrongDirection)
                            {
                                roleCount++;

                                //Role in variable
                                auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator->first)));
                                roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator->first));
                                outerArray->arrayValue->emplace_back(std::move(roleVariable));
                            }
                        }

                        if(roleCount == 0)
                        {
                            //Variable not found, look for alternative in channel
                            for(auto& variableIterator2 : *channelIterator->second->structValue)
                            {
                                bool wrongDirection = false;
                                auto directionIterator = variableIterator2.second->structValue->find("direction");
                                if(directionIterator != variableIterator2.second->structValue->end())
                                {
                                    if(directionIterator->second->integerValue64 == 1)
                                    {
                                        GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator2.first + " does not have required direction (input). Simple UI element creation is not possible.");
                                        wrongDirection = true;
                                    }
                                }

                                if(!wrongDirection)
                                {
                                    roleCount++;

                                    //Role in channel
                                    auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                    roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                    roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator->first)));
                                    roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator2.first));
                                    outerArray->arrayValue->emplace_back(std::move(roleVariable));
                                }
                            }
                        }

                        if(roleCount > 1) return BaseLib::Variable::createError(-1, "Required role (" + std::to_string(roleId) + ") exists multiple times in channel. Simple UI element creation is not possible.");
                    }

                    if(roleCount == 0)
                    {
                        //Role not in channel
                        for(auto& channelIterator2 : *variables->structValue)
                        {
                            for(auto& variableIterator : *channelIterator2.second->structValue)
                            {
                                bool wrongDirection = false;
                                auto directionIterator = variableIterator.second->structValue->find("direction");
                                if(directionIterator != variableIterator.second->structValue->end())
                                {
                                    if(directionIterator->second->integerValue64 == 1)
                                    {
                                        GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator.first + " does not have required direction (input) in channel " + channelIterator2.first + ". Simple UI element creation is not possible.");
                                        wrongDirection = true;
                                    }
                                }

                                if(!wrongDirection)
                                {
                                    roleCount++;

                                    auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                    roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                    roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator2.first)));
                                    roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator.first));
                                    outerArray->arrayValue->emplace_back(std::move(roleVariable));
                                }
                            }
                        }

                        if(roleCount > 1) return BaseLib::Variable::createError(-1, "Required role exists multiple times in device. Simple UI element creation is not possible.");

                        if(roleCount == 0)
                        {
                            //Role not in device, try to find role in room
                            requestParameters->arrayValue->resize(1);
                            requestParameters->arrayValue->at(0) = std::make_shared<BaseLib::Variable>(roomId);
                            std::string methodName2 = "getRolesInRoom";
                            auto variables2 = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                            if(variables2->errorStruct) return BaseLib::Variable::createError(-1, "Error getting roles in room.");

                            for(auto& deviceIterator : *variables2->structValue)
                            {
                                for(auto& channelIterator2 : *deviceIterator.second->structValue)
                                {
                                    for(auto& variableIterator : *channelIterator2.second->structValue)
                                    {
                                        bool wrongDirection = false;
                                        auto directionIterator = variableIterator.second->structValue->find("direction");
                                        if(directionIterator != variableIterator.second->structValue->end())
                                        {
                                            if(directionIterator->second->integerValue64 == 1)
                                            {
                                                GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator.first + " does not have required direction (input) in channel " + channelIterator2.first + " of device " + deviceIterator.first + ". Simple UI element creation is not possible.");
                                                wrongDirection = true;
                                            }
                                        }

                                        if(!wrongDirection)
                                        {
                                            roleCount++;

                                            auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                            roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                            roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator2.first)));
                                            roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator.first));
                                            outerArray->arrayValue->emplace_back(std::move(roleVariable));
                                        }
                                    }
                                }
                            }

                            if(roleCount > 1) return BaseLib::Variable::createError(-1, "Required role exists multiple times in room. Simple UI element creation is not possible.");
                        }

                        if(roleCount == 0)
                        {
                            return BaseLib::Variable::createError(-1, "Required role \"" + std::to_string(roleId) + "\" not found in device or room. Simple UI element creation is not possible.");
                        }
                    }
                }
                inputPeers->arrayValue->emplace_back(std::move(outerArray));
            }

            for(const auto& roleIdOuter : roleIdsOut)
            {
                auto outerArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                outerArray->arrayValue->reserve(roleIdOuter.size());
                for(auto roleId : roleIdOuter)
                {
                    BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    requestParameters->arrayValue->reserve(2);
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(roleId));
                    requestParameters->arrayValue->push_back(variable->arrayValue->at(0));
                    std::string methodName = "getVariablesInRole";
                    auto variables = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(variables->errorStruct) return BaseLib::Variable::createError(-1, "Error getting variables for roles required by UI element.");

                    size_t roleCount = 0;

                    auto channelIterator = variables->structValue->find(std::to_string(variable->arrayValue->at(1)->integerValue64));
                    if(channelIterator != variables->structValue->end() && !channelIterator->second->structValue->empty())
                    {
                        auto variableIterator = channelIterator->second->structValue->find(variable->arrayValue->at(2)->stringValue);
                        if(variableIterator != channelIterator->second->structValue->end())
                        {
                            bool wrongDirection = false;
                            auto directionIterator = variableIterator->second->structValue->find("direction");
                            if(directionIterator != variableIterator->second->structValue->end())
                            {
                                if(directionIterator->second->integerValue64 == 0)
                                {
                                    GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator->first + " does not have required direction (output). Simple UI element creation is not possible.");
                                    wrongDirection = true;
                                }
                            }

                            if(!wrongDirection)
                            {
                                roleCount++;

                                //Role in channel
                                auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator->first)));
                                roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator->first));
                                outerArray->arrayValue->emplace_back(std::move(roleVariable));
                            }
                        }

                        if(roleCount == 0)
                        {
                            for(auto& variableIterator2 : *channelIterator->second->structValue)
                            {
                                bool wrongDirection = false;
                                auto directionIterator = variableIterator2.second->structValue->find("direction");
                                if(directionIterator != variableIterator2.second->structValue->end())
                                {
                                    if(directionIterator->second->integerValue64 == 0)
                                    {
                                        GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator2.first + " does not have required direction (output). Simple UI element creation is not possible.");
                                        wrongDirection = true;
                                    }
                                }

                                if(!wrongDirection)
                                {
                                    roleCount++;

                                    //Role in channel
                                    auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                    roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                    roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator->first)));
                                    roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator2.first));
                                    outerArray->arrayValue->emplace_back(std::move(roleVariable));
                                }
                            }
                        }

                        if(roleCount > 1) return BaseLib::Variable::createError(-1, "Required role (" + std::to_string(roleId) + ") exists multiple times in channel. Simple UI element creation is not possible.");
                    }

                    if(roleCount == 0)
                    {
                        //Role not in channel
                        for(auto& channelIterator2 : *variables->structValue)
                        {
                            for(auto& variableIterator : *channelIterator2.second->structValue)
                            {
                                bool wrongDirection = false;
                                auto directionIterator = variableIterator.second->structValue->find("direction");
                                if(directionIterator != variableIterator.second->structValue->end())
                                {
                                    if(directionIterator->second->integerValue64 == 0)
                                    {
                                        GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator.first + " does not have required direction (output) in channel " + channelIterator2.first + ". Simple UI element creation is not possible.");
                                        wrongDirection = true;
                                    }
                                }

                                if(!wrongDirection)
                                {
                                    roleCount++;

                                    auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                    roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                    roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator2.first)));
                                    roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator.first));
                                    outerArray->arrayValue->emplace_back(std::move(roleVariable));
                                }
                            }
                        }

                        if(roleCount > 1) return BaseLib::Variable::createError(-1, "Required role exists multiple times in device. Simple UI element creation is not possible.");

                        if(roleCount == 0)
                        {
                            //Role not in device, try to find role in room
                            requestParameters->arrayValue->resize(1);
                            requestParameters->arrayValue->at(0) = std::make_shared<BaseLib::Variable>(roomId);
                            std::string methodName2 = "getRolesInRoom";
                            auto variables2 = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                            if(variables2->errorStruct) return BaseLib::Variable::createError(-1, "Error getting roles in room.");

                            for(auto& deviceIterator : *variables2->structValue)
                            {
                                for(auto& channelIterator2 : *deviceIterator.second->structValue)
                                {
                                    for(auto& variableIterator : *channelIterator2.second->structValue)
                                    {
                                        bool wrongDirection = false;
                                        auto directionIterator = variableIterator.second->structValue->find("direction");
                                        if(directionIterator != variableIterator.second->structValue->end())
                                        {
                                            if(directionIterator->second->integerValue64 == 0)
                                            {
                                                GD::out.printDebug("Debug: Required role (" + std::to_string(roleId) + ") " + variableIterator.first + " does not have required direction (output) in channel " + channelIterator2.first + " of device " + deviceIterator.first + ". Simple UI element creation is not possible.");
                                                wrongDirection = true;
                                            }
                                        }

                                        if(!wrongDirection)
                                        {
                                            roleCount++;

                                            auto roleVariable = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                                            roleVariable->structValue->emplace("peer", variable->arrayValue->at(0));
                                            roleVariable->structValue->emplace("channel", std::make_shared<BaseLib::Variable>(BaseLib::Math::getNumber(channelIterator2.first)));
                                            roleVariable->structValue->emplace("name", std::make_shared<BaseLib::Variable>(variableIterator.first));
                                            outerArray->arrayValue->emplace_back(std::move(roleVariable));
                                        }
                                    }
                                }
                            }

                            if(roleCount > 1) return BaseLib::Variable::createError(-1, "Required role exists multiple times in room. Simple UI element creation is not possible.");
                        }

                        if(roleCount == 0)
                        {
                            return BaseLib::Variable::createError(-1, "Required role \"" + std::to_string(roleId) + "\" not found in device or room. Simple UI element creation is not possible.");
                        }
                    }
                }
                outputPeers->arrayValue->emplace_back(std::move(outerArray));
            }
        }

        return std::make_shared<BaseLib::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::addUiElementSimple(const BaseLib::PRpcClientInfo& clientInfo, const std::string& label, const BaseLib::PVariable& variable, bool dryRun)
{
    try
    {
        if(variable->arrayValue->size() < 3)
        {
            return BaseLib::Variable::createError(-1, "No variables were passed.");
        }

        std::set<uint64_t> roleIds;
        uint64_t roomId = 0;

        { //Get role ID and room ID
            BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
            requestParameters->arrayValue->push_back(variable->arrayValue->at(0));
            requestParameters->arrayValue->push_back(variable->arrayValue->at(1));
            requestParameters->arrayValue->push_back(variable->arrayValue->at(2));
            BaseLib::PVariable fields(new BaseLib::Variable(BaseLib::VariableType::tArray));
            fields->arrayValue->reserve(2);
            fields->arrayValue->push_back(std::make_shared<BaseLib::Variable>("ROOM"));
            fields->arrayValue->push_back(std::make_shared<BaseLib::Variable>("ROLES"));
            requestParameters->arrayValue->push_back(fields);
            std::string methodName = "getVariableDescription";
            auto variableDescription = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
            if(variableDescription->errorStruct) return BaseLib::Variable::createError(-1, "Error getting variable description. Are you authorized to access the variable?");
            auto rolesIterator = variableDescription->structValue->find("ROLES");
            if(rolesIterator == variableDescription->structValue->end() || rolesIterator->second->arrayValue->empty()) return BaseLib::Variable::createError(-1, "Variable has no roles.");
            for(auto& role : *rolesIterator->second->arrayValue)
            {
                auto idIterator = role->structValue->find("id");
                if(idIterator != role->structValue->end() && idIterator->second->integerValue64 != 0) roleIds.emplace(idIterator->second->integerValue64);
            }
            auto roomIterator = variableDescription->structValue->find("ROOM");
            if(roomIterator != variableDescription->structValue->end() && roomIterator->second->integerValue64 > 0)
            {
                roomId = roomIterator->second->integerValue64;
            }
            else if(variable->arrayValue->at(0)->integerValue64 != 0) //Device variable?
            {
                fields->arrayValue->resize(1);
                requestParameters->arrayValue->at(2) = fields;
                requestParameters->arrayValue->resize(3);
                std::string methodName = "getDeviceDescription";
                auto deviceDescription = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                if(deviceDescription->errorStruct) return BaseLib::Variable::createError(-1, "Error getting device description. Are you authorized to access the device?");
                roomIterator = deviceDescription->structValue->find("ROOM");
                if(roomIterator != deviceDescription->structValue->end() && roomIterator->second->integerValue64 > 0)
                {
                    roomId = roomIterator->second->integerValue64;
                }
                else
                {
                    requestParameters->arrayValue->at(1) = std::make_shared<BaseLib::Variable>(-1);
                    deviceDescription = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(deviceDescription->errorStruct) return BaseLib::Variable::createError(-1, "Error getting device description. Are you authorized to access the device?");
                    roomIterator = deviceDescription->structValue->find("ROOM");
                    if(roomIterator != deviceDescription->structValue->end())
                    {
                        roomId = roomIterator->second->integerValue64;
                    }
                }
            }
        }

        if(roomId == 0) return BaseLib::Variable::createError(-1, "Variable, channel and device have no room assigned.");

        BaseLib::PVariable roleMetadata;
        BaseLib::PVariable uiInfo;
        uint64_t uiRole = 0;
        for(auto roleId : roleIds)
        {
            roleMetadata = GD::bl->db->getRoleMetadata(roleId);
            auto uiRefIterator = roleMetadata->structValue->find("uiRef");
            if(uiRefIterator != roleMetadata->structValue->end() && uiRefIterator->second->integerValue64 != 0)
            {
                roleId = uiRefIterator->second->integerValue64;
                roleMetadata = GD::bl->db->getRoleMetadata(roleId);
            }
            auto uiIterator = roleMetadata->structValue->find("ui");
            if(uiIterator == roleMetadata->structValue->end()) continue;
            auto simpleCreationIterator = uiIterator->second->structValue->find("simpleCreationInfo");
            if(simpleCreationIterator == uiIterator->second->structValue->end()) continue;
            if(uiRole == 0) uiRole = roleId;
            if(!uiInfo) uiInfo = simpleCreationIterator->second;
            else if(uiRole != roleId) return BaseLib::Variable::createError(-1, "Variable has more than one role with UI definition. UI element creation is not possible.");
        }
        if(!uiInfo) return BaseLib::Variable::createError(-1, "Role has no UI definition.");

        auto inputPeers = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        auto outputPeers = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

        if(uiInfo->type == BaseLib::VariableType::tArray)
        {
            bool found = false;
            for(auto& definition : *uiInfo->arrayValue)
            {
                auto result = findRoleVariables(clientInfo, definition, variable, roomId, inputPeers, outputPeers);
                if(!result->errorStruct)
                {
                    found = true;
                    uiInfo = definition;
                    break;
                }
                else GD::out.printInfo("Info: No match for UI definition. Reason: " + result->structValue->at("faultString")->stringValue);
            }
            if(!found) return BaseLib::Variable::createError(-1, "Could not find a matching variable set.");

        }
        else if(uiInfo->type == BaseLib::VariableType::tStruct)
        {
            auto result = findRoleVariables(clientInfo, uiInfo, variable, roomId, inputPeers, outputPeers);
            if(result->errorStruct) return result;
        }
        else return BaseLib::Variable::createError(-1, "UI definition has unknown type.");

        std::string elementId;

        { //Get element ID
            auto elementIdIterator = uiInfo->structValue->find("element");
            if(elementIdIterator == uiInfo->structValue->end() || elementIdIterator->second->stringValue.empty())
            {
                return BaseLib::Variable::createError(-1, "Role is missing key \"element\" containing the UI element ID.");
            }
            elementId = elementIdIterator->second->stringValue;
        }

        BaseLib::PVariable metadata;
        if(!dryRun)
        { //Get metadata
            auto metadataIterator = uiInfo->structValue->find("metadata");
            if(metadataIterator != uiInfo->structValue->end() && metadataIterator->second->type == BaseLib::VariableType::tStruct)
            {
                metadata = metadataIterator->second;
            }
            else metadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

            auto addUiElementData = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            { //Create data struct
                addUiElementData->structValue->emplace("inputPeers", inputPeers);
                addUiElementData->structValue->emplace("outputPeers", outputPeers);
                addUiElementData->structValue->emplace("label", std::make_shared<BaseLib::Variable>(label));
                addUiElementData->structValue->emplace("room", std::make_shared<BaseLib::Variable>(roomId));
                addUiElementData->structValue->emplace("metadata", metadata);
            }

            return addUiElement(clientInfo, elementId, addUiElementData, std::make_shared<BaseLib::Variable>());
        }
        else return std::make_shared<BaseLib::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void UiController::addDataInfo(UiController::PUiElement& uiElement, const BaseLib::PVariable& data)
{
    try
    {
        uiElement->peerInfo->inputPeers.clear();
        uiElement->peerInfo->outputPeers.clear();
        uiElement->roomId = 0;
        uiElement->categoryIds.clear();

        auto dataIterator = data->structValue->find("label");
        if(dataIterator != data->structValue->end()) uiElement->label = dataIterator->second->stringValue;

        dataIterator = data->structValue->find("inputPeers");
        if(dataIterator != data->structValue->end())
        {
            uiElement->peerInfo->inputPeers.reserve(dataIterator->second->arrayValue->size());
            for(auto& outerArray : *dataIterator->second->arrayValue)
            {
                std::vector<BaseLib::DeviceDescription::UiElements::PUiVariableInfo> peers;
                peers.reserve(outerArray->arrayValue->size());
                for(auto& peerElement : *outerArray->arrayValue)
                {
                    auto variableInfo = std::make_shared<BaseLib::DeviceDescription::UiElements::UiVariableInfo>();

                    if(peerElement->type == BaseLib::VariableType::tStruct)
                    {
                        auto peerIdIterator = peerElement->structValue->find("peer");
                        if(peerIdIterator == peerElement->structValue->end()) continue;

                        variableInfo->peerId = (uint64_t) peerIdIterator->second->integerValue64;

                        auto channelIterator = peerElement->structValue->find("channel");
                        if(channelIterator != peerElement->structValue->end()) variableInfo->channel = channelIterator->second->integerValue;

                        auto nameIterator = peerElement->structValue->find("name");
                        if(nameIterator != peerElement->structValue->end()) variableInfo->name = nameIterator->second->stringValue;

                        auto minimumValueIterator = peerElement->structValue->find("minimum");
                        if(minimumValueIterator != peerElement->structValue->end()) variableInfo->minimumValue = minimumValueIterator->second;

                        auto maximumValueIterator = peerElement->structValue->find("maximum");
                        if(maximumValueIterator != peerElement->structValue->end()) variableInfo->maximumValue = maximumValueIterator->second;

                        auto minimumValueScaledIterator = peerElement->structValue->find("minimumScaled");
                        if(minimumValueScaledIterator != peerElement->structValue->end()) variableInfo->minimumValueScaled = minimumValueScaledIterator->second;

                        auto maximumValueScaledIterator = peerElement->structValue->find("maximumScaled");
                        if(maximumValueScaledIterator != peerElement->structValue->end()) variableInfo->maximumValueScaled = maximumValueScaledIterator->second;
                    }
                    else if(peerElement->type == BaseLib::VariableType::tInteger || peerElement->type == BaseLib::VariableType::tInteger64)
                    {
                        variableInfo->peerId = (uint64_t) peerElement->integerValue64;
                    }
                    else continue;

                    peers.push_back(variableInfo);
                }
                uiElement->peerInfo->inputPeers.emplace_back(std::move(peers));
            }
        }

        dataIterator = data->structValue->find("outputPeers");
        if(dataIterator != data->structValue->end())
        {
            uiElement->peerInfo->outputPeers.reserve(dataIterator->second->arrayValue->size());
            for(auto& outerArray : *dataIterator->second->arrayValue)
            {
                std::vector<BaseLib::DeviceDescription::UiElements::PUiVariableInfo> peers;
                peers.reserve(outerArray->arrayValue->size());
                for(auto& peerElement : *outerArray->arrayValue)
                {
                    auto variableInfo = std::make_shared<BaseLib::DeviceDescription::UiElements::UiVariableInfo>();

                    if(peerElement->type == BaseLib::VariableType::tStruct)
                    {
                        auto peerIdIterator = peerElement->structValue->find("peer");
                        if(peerIdIterator == peerElement->structValue->end()) continue;

                        variableInfo->peerId = (uint64_t) peerIdIterator->second->integerValue64;

                        auto channelIterator = peerElement->structValue->find("channel");
                        if(channelIterator != peerElement->structValue->end()) variableInfo->channel = channelIterator->second->integerValue;

                        auto nameIterator = peerElement->structValue->find("name");
                        if(nameIterator != peerElement->structValue->end()) variableInfo->name = nameIterator->second->stringValue;

                        auto valueIterator = peerElement->structValue->find("value");
                        if(valueIterator != peerElement->structValue->end()) variableInfo->value = valueIterator->second;

                        auto minimumValueIterator = peerElement->structValue->find("minimum");
                        if(minimumValueIterator != peerElement->structValue->end()) variableInfo->minimumValue = minimumValueIterator->second;

                        auto maximumValueIterator = peerElement->structValue->find("maximum");
                        if(maximumValueIterator != peerElement->structValue->end()) variableInfo->maximumValue = maximumValueIterator->second;
                    }
                    else if(peerElement->type == BaseLib::VariableType::tInteger || peerElement->type == BaseLib::VariableType::tInteger64)
                    {
                        variableInfo->peerId = (uint64_t) peerElement->integerValue64;
                    }
                    else continue;

                    peers.push_back(variableInfo);
                }
                uiElement->peerInfo->outputPeers.emplace_back(std::move(peers));
            }
        }

        dataIterator = data->structValue->find("room");
        if(dataIterator != data->structValue->end()) uiElement->roomId = (uint64_t) dataIterator->second->integerValue64;

        dataIterator = data->structValue->find("categories");
        if(dataIterator != data->structValue->end())
        {
            for(auto& categoryElement : *dataIterator->second->arrayValue)
            {
                uiElement->categoryIds.emplace((uint64_t) categoryElement->integerValue64);
            }
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void UiController::addVariableInfo(const BaseLib::PRpcClientInfo& clientInfo, const PUiElement& uiElement, BaseLib::PArray& variables, bool addValue)
{
    try
    {
        for(auto& variable : *variables)
        {
            auto peerIdIterator = variable->structValue->find("peer");
            if(peerIdIterator == variable->structValue->end()) continue;
            auto channelIterator = variable->structValue->find("channel");
            if(channelIterator == variable->structValue->end()) continue;
            auto nameIterator = variable->structValue->find("name");
            if(nameIterator == variable->structValue->end()) continue;

            std::string methodName = "getValue";
            auto parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            parameters->arrayValue->reserve(4);
            parameters->arrayValue->emplace_back(peerIdIterator->second);
            parameters->arrayValue->emplace_back(channelIterator->second);
            parameters->arrayValue->emplace_back(nameIterator->second);

            if(addValue)
            {
                auto value = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(!value->errorStruct) variable->structValue->emplace("value", value);
            }

            auto fields = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            fields->arrayValue->reserve(5);
            fields->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("TYPE"));
            fields->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("MIN"));
            fields->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("MAX"));
            fields->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("ROLES"));
            parameters->arrayValue->emplace_back(fields);

            methodName = "getVariableDescription";
            auto description = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
            if(description->errorStruct)
            {
                GD::out.printWarning("Warning: Could not get variable description for UI element " + uiElement->elementId + " with ID " + std::to_string(uiElement->databaseId) + ": " + description->structValue->at("faultString")->stringValue + ". Please set the log level to 4 to see the failed RPC call.");
                continue;
            }

            auto typeIterator = description->structValue->find("TYPE");
            if(typeIterator != description->structValue->end()) variable->structValue->emplace("type", std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::toLower(typeIterator->second->stringValue)));

            auto minimumValueIterator = variable->structValue->find("minimumValue");
            auto maximumValueIterator = variable->structValue->find("maximumValue");
            if(minimumValueIterator == variable->structValue->end() || maximumValueIterator == variable->structValue->end())
            {
                if(minimumValueIterator == variable->structValue->end())
                {
                    auto minimumValueIterator2 = description->structValue->find("MIN");
                    if(minimumValueIterator2 != description->structValue->end())
                    {
                        variable->structValue->emplace("minimumValue", minimumValueIterator2->second);
                    }
                }

                if(maximumValueIterator == variable->structValue->end())
                {
                    auto maximumValueIterator2 = description->structValue->find("MAX");
                    if(maximumValueIterator2 != description->structValue->end())
                    {
                        variable->structValue->emplace("maximumValue", maximumValueIterator2->second);
                    }
                }
            }

            auto rolesIterator = description->structValue->find("ROLES");
            if(rolesIterator != description->structValue->end())
            {
                variable->structValue->emplace("roles", rolesIterator->second);
            }
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

BaseLib::PVariable UiController::getAllUiElements(const BaseLib::PRpcClientInfo& clientInfo, const std::string& language)
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);

        bool checkAcls = clientInfo->acls->variablesRoomsCategoriesRolesDevicesReadSet();

        auto uiElements = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        uiElements->arrayValue->reserve(_uiElements.size());

        auto clickCountsData = User::getData(clientInfo->user, "ui.clickCounts", "");
        std::multimap<uint64_t, PUiElement> uiElementsByClickCount;
        std::unordered_set<uint64_t> addedElementIds;
        for(auto& uiElement : *clickCountsData->structValue)
        {
            auto uiElementId = BaseLib::Math::getUnsignedNumber64(uiElement.first);
            auto uiElementsIterator = _uiElements.find(uiElementId);
            if(uiElementsIterator == _uiElements.end())
            {
                User::deleteData(clientInfo->user, "ui.clickCounts", uiElement.first);
                continue;
            }
            uiElementsByClickCount.emplace(uiElement.second->integerValue64, uiElementsIterator->second);
            addedElementIds.emplace(uiElementId);
        }

        for(auto& uiElement : _uiElements)
        {
            if(addedElementIds.find(uiElement.first) != addedElementIds.end()) continue;

            uiElementsByClickCount.emplace(0, uiElement.second);
        }

        for(auto uiElementIterator = uiElementsByClickCount.rbegin(); uiElementIterator != uiElementsByClickCount.rend(); uiElementIterator++)
        {
            auto& uiElement = uiElementIterator->second;
            auto languageIterator = uiElement->rpcElement.find(language);
            if(languageIterator == uiElement->rpcElement.end())
            {
                auto rpcElement = _descriptions->getUiElement(language, uiElement->elementId, uiElement->peerInfo);
                if(!rpcElement) continue;
                uiElement->rpcElement.emplace(language, rpcElement);
                languageIterator = uiElement->rpcElement.find(language);
                if(languageIterator == uiElement->rpcElement.end()) continue;
            }

            if(checkAcls)
            {
                if(!clientInfo->acls->checkRoomReadAccess(uiElement->roomId)) continue;
                for(auto categoryId : uiElement->categoryIds)
                {
                    if(!clientInfo->acls->checkCategoryReadAccess(categoryId)) continue;
                }
                if(!checkElementAccess(clientInfo, uiElement, languageIterator->second)) continue;
            }

            auto elementInfo = languageIterator->second->getElementInfo();
            elementInfo->structValue->emplace("databaseId", std::make_shared<BaseLib::Variable>(uiElement->databaseId));
            elementInfo->structValue->emplace("clickCount", std::make_shared<BaseLib::Variable>(uiElementIterator->first));
            elementInfo->structValue->emplace("label", std::make_shared<BaseLib::Variable>(uiElement->label));
            elementInfo->structValue->emplace("room", std::make_shared<BaseLib::Variable>(uiElement->roomId));
            auto categories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            categories->arrayValue->reserve(uiElement->categoryIds.size());
            for(auto categoryId : uiElement->categoryIds)
            {
                categories->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(categoryId));
            }
            elementInfo->structValue->emplace("categories", categories);

            //{{{ value
            //Simple
            auto variableInputsIterator = elementInfo->structValue->find("variableInputs");
            if(variableInputsIterator != elementInfo->structValue->end()) addVariableInfo(clientInfo, uiElement, variableInputsIterator->second->arrayValue, true);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableInputsIterator = control->structValue->find("variableInputs");
                        if(variableInputsIterator != control->structValue->end()) addVariableInfo(clientInfo, uiElement, variableInputsIterator->second->arrayValue, true);
                    };
                }
            }

            //Simple
            auto variableOutputsIterator = elementInfo->structValue->find("variableOutputs");
            if(variableOutputsIterator != elementInfo->structValue->end()) addVariableInfo(clientInfo, uiElement, variableOutputsIterator->second->arrayValue, false);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableOutputsIterator = control->structValue->find("variableOutputs");
                        if(variableOutputsIterator != control->structValue->end()) addVariableInfo(clientInfo, uiElement, variableOutputsIterator->second->arrayValue, false);
                    };
                }
            }
            //}}}

            auto dataIterator = uiElement->data->structValue->find("metadata");
            if(dataIterator != uiElement->data->structValue->end())
            {
                auto metadataIterator = elementInfo->structValue->find("metadata");
                if(metadataIterator == elementInfo->structValue->end()) metadataIterator = elementInfo->structValue->emplace("metadata", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct)).first;

                for(auto& metadataElement : *dataIterator->second->structValue)
                {
                    metadataIterator->second->structValue->emplace(metadataElement);
                }
            }

            elementInfo->structValue->emplace("dynamicMetadata", uiElement->metadata);

            uiElements->arrayValue->emplace_back(elementInfo);
        }

        return uiElements;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getAvailableUiElements(const BaseLib::PRpcClientInfo& clientInfo, const std::string& language)
{
    try
    {
        return _descriptions->getUiElements(language);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getUiElementMetadata(const BaseLib::PRpcClientInfo& clientInfo, uint64_t databaseId)
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);
        auto elementIterator = _uiElements.find(databaseId);
        if(elementIterator == _uiElements.end()) return BaseLib::Variable::createError(-2, "Unknown element.");

        return elementIterator->second->metadata;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getUiElementsInRoom(const BaseLib::PRpcClientInfo& clientInfo, uint64_t roomId, const std::string& language)
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);

        if(clientInfo->acls->roomsReadSet() && !clientInfo->acls->checkRoomReadAccess(roomId)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
        bool checkAcls = clientInfo->acls->variablesRoomsCategoriesRolesDevicesReadSet();

        auto roomIterator = _uiElementsByRoom.find(roomId);
        if(roomIterator == _uiElementsByRoom.end()) return BaseLib::Variable::createError(-1, "Unknown room.");

        auto uiElements = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        uiElements->arrayValue->reserve(roomIterator->second.size());

        for(auto& uiElement : roomIterator->second)
        {
            auto languageIterator = uiElement->rpcElement.find(language);
            if(languageIterator == uiElement->rpcElement.end())
            {
                auto rpcElement = _descriptions->getUiElement(language, uiElement->elementId, uiElement->peerInfo);
                if(!rpcElement) continue;
                uiElement->rpcElement.emplace(language, rpcElement);
                languageIterator = uiElement->rpcElement.find(language);
                if(languageIterator == uiElement->rpcElement.end()) continue;
            }

            if(checkAcls && !checkElementAccess(clientInfo, uiElement, languageIterator->second)) continue;

            auto elementInfo = languageIterator->second->getElementInfo();
            elementInfo->structValue->emplace("databaseId", std::make_shared<BaseLib::Variable>(uiElement->databaseId));
            elementInfo->structValue->emplace("label", std::make_shared<BaseLib::Variable>(uiElement->label));
            elementInfo->structValue->emplace("room", std::make_shared<BaseLib::Variable>(uiElement->roomId));
            auto categories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            categories->arrayValue->reserve(uiElement->categoryIds.size());
            for(auto categoryId : uiElement->categoryIds)
            {
                categories->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(categoryId));
            }
            elementInfo->structValue->emplace("categories", categories);

            //{{{ value
            //Simple
            auto variableInputsIterator = elementInfo->structValue->find("variableInputs");
            if(variableInputsIterator != elementInfo->structValue->end()) addVariableInfo(clientInfo, uiElement, variableInputsIterator->second->arrayValue, true);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableInputsIterator = control->structValue->find("variableInputs");
                        if(variableInputsIterator != control->structValue->end()) addVariableInfo(clientInfo, uiElement, variableInputsIterator->second->arrayValue, true);
                    }
                }
            }

            //Simple
            auto variableOutputsIterator = elementInfo->structValue->find("variableOutputs");
            if(variableOutputsIterator != elementInfo->structValue->end()) addVariableInfo(clientInfo, uiElement, variableOutputsIterator->second->arrayValue, false);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableOutputsIterator = control->structValue->find("variableOutputs");
                        if(variableOutputsIterator != control->structValue->end()) addVariableInfo(clientInfo, uiElement, variableOutputsIterator->second->arrayValue, false);
                    }
                }
            }
            //}}}

            uiElements->arrayValue->emplace_back(elementInfo);
        }

        return uiElements;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getUiElementsInCategory(const BaseLib::PRpcClientInfo& clientInfo, uint64_t categoryId, const std::string& language)
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);

        if(clientInfo->acls->categoriesReadSet() && !clientInfo->acls->checkCategoryReadAccess(categoryId)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
        bool checkAcls = clientInfo->acls->variablesRoomsCategoriesRolesDevicesReadSet();

        auto categoryIterator = _uiElementsByCategory.find(categoryId);
        if(categoryIterator == _uiElementsByCategory.end()) return BaseLib::Variable::createError(-1, "Unknown category.");

        auto uiElements = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        uiElements->arrayValue->reserve(categoryIterator->second.size());

        for(auto& uiElement : categoryIterator->second)
        {
            auto languageIterator = uiElement->rpcElement.find(language);
            if(languageIterator == uiElement->rpcElement.end())
            {
                auto rpcElement = _descriptions->getUiElement(language, uiElement->elementId, uiElement->peerInfo);
                if(!rpcElement) continue;
                uiElement->rpcElement.emplace(language, rpcElement);
                languageIterator = uiElement->rpcElement.find(language);
                if(languageIterator == uiElement->rpcElement.end()) continue;
            }

            if(checkAcls && !checkElementAccess(clientInfo, uiElement, languageIterator->second)) continue;

            auto elementInfo = languageIterator->second->getElementInfo();
            elementInfo->structValue->emplace("databaseId", std::make_shared<BaseLib::Variable>(uiElement->databaseId));
            elementInfo->structValue->emplace("label", std::make_shared<BaseLib::Variable>(uiElement->label));
            elementInfo->structValue->emplace("room", std::make_shared<BaseLib::Variable>(uiElement->roomId));
            auto categories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            categories->arrayValue->reserve(uiElement->categoryIds.size());
            for(auto categoryId : uiElement->categoryIds)
            {
                categories->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(categoryId));
            }
            elementInfo->structValue->emplace("categories", categories);

            //{{{ value
            //Simple
            auto variableInputsIterator = elementInfo->structValue->find("variableInputs");
            if(variableInputsIterator != elementInfo->structValue->end()) addVariableInfo(clientInfo, uiElement, variableInputsIterator->second->arrayValue, true);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableInputsIterator = control->structValue->find("variableInputs");
                        if(variableInputsIterator != control->structValue->end()) addVariableInfo(clientInfo, uiElement, variableInputsIterator->second->arrayValue, true);
                    }
                }
            }

            //Simple
            auto variableOutputsIterator = elementInfo->structValue->find("variableOutputs");
            if(variableOutputsIterator != elementInfo->structValue->end()) addVariableInfo(clientInfo, uiElement, variableOutputsIterator->second->arrayValue, false);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableOutputsIterator = control->structValue->find("variableOutputs");
                        if(variableOutputsIterator != control->structValue->end()) addVariableInfo(clientInfo, uiElement, variableOutputsIterator->second->arrayValue, false);
                    }
                }
            }
            //}}}

            uiElements->arrayValue->emplace_back(elementInfo);
        }

        return uiElements;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool UiController::checkElementAccess(const BaseLib::PRpcClientInfo& clientInfo, const UiController::PUiElement& uiElement, const BaseLib::DeviceDescription::PHomegearUiElement& rpcElement)
{
    try
    {
        auto families = GD::familyController->getFamilies();

        //variableInputs and variableOutputs are filled by UiElements::getUiElement

        for(auto& variableInput : rpcElement->variableInputs)
        {
            if(variableInput->peerId == 0)
            {
                auto systemVariable = GD::systemVariableController->getInternal(variableInput->name);
                if(!systemVariable) return false;
                clientInfo->acls->checkSystemVariableReadAccess(systemVariable);
            }
            else
            {
                for(auto& family : families)
                {
                    auto central = family.second->getCentral();
                    if(!central) continue;

                    auto peer = central->getPeer((uint64_t) variableInput->peerId);
                    if(!peer) continue;

                    if(!clientInfo->acls->checkDeviceReadAccess(peer)) return false;
                    if(!clientInfo->acls->checkVariableReadAccess(peer, variableInput->channel, variableInput->name)) return false;
                }
            }
        }

        for(auto& variableOutput : rpcElement->variableOutputs)
        {
            if(variableOutput->peerId == 0)
            {
                auto systemVariable = GD::systemVariableController->getInternal(variableOutput->name);
                if(!systemVariable) return false;
                clientInfo->acls->checkSystemVariableReadAccess(systemVariable);
            }
            else
            {
                for(auto& family : families)
                {
                    auto central = family.second->getCentral();
                    if(!central) continue;

                    auto peer = central->getPeer((uint64_t) variableOutput->peerId);
                    if(!peer) continue;

                    if(!clientInfo->acls->checkDeviceWriteAccess(peer)) return false;
                    if(!clientInfo->acls->checkVariableWriteAccess(peer, variableOutput->channel, variableOutput->name)) return false;
                }
            }
        }

        return true;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

BaseLib::PVariable UiController::removeUiElement(const BaseLib::PRpcClientInfo& clientInfo, uint64_t databaseId)
{
    try
    {
        if(databaseId == 0) return BaseLib::Variable::createError(-1, "databaseId is invalid.");

        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);
        auto elementIterator = _uiElements.find(databaseId);
        if(elementIterator == _uiElements.end()) return BaseLib::Variable::createError(-2, "Unknown element.");

        GD::bl->db->removeUiElement(databaseId);

        auto roomIterator = _uiElementsByRoom.find(elementIterator->second->roomId);
        if(roomIterator != _uiElementsByRoom.end())
        {
            for(auto& roomUiElement : roomIterator->second)
            {
                if(roomUiElement.get() == elementIterator->second.get())
                {
                    roomIterator->second.erase(roomUiElement);
                    break;
                }
            }
            if(roomIterator->second.empty()) _uiElementsByRoom.erase(roomIterator);
        }

        for(auto categoryId : elementIterator->second->categoryIds)
        {
            auto categoryIterator = _uiElementsByCategory.find(categoryId);
            if(categoryIterator != _uiElementsByCategory.end())
            {
                for(auto& categoryUiElement : categoryIterator->second)
                {
                    if(categoryUiElement.get() == elementIterator->second.get())
                    {
                        categoryIterator->second.erase(categoryUiElement);
                        break;
                    }
                }
                if(categoryIterator->second.empty()) _uiElementsByCategory.erase(categoryIterator);
            }
        }

        _uiElements.erase(elementIterator);

        return std::make_shared<BaseLib::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::setUiElementMetadata(const BaseLib::PRpcClientInfo& clientInfo, uint64_t databaseId, const BaseLib::PVariable& metadata)
{
    try
    {
        {
            std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);
            auto elementIterator = _uiElements.find(databaseId);
            if(elementIterator == _uiElements.end()) return BaseLib::Variable::createError(-2, "Unknown element.");

            elementIterator->second->metadata = metadata;
        }

        GD::bl->db->setUiElementMetadata(databaseId, metadata);
        return std::make_shared<BaseLib::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}