/* Copyright 2013-2019 Homegear GmbH
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
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

BaseLib::PVariable UiController::addUiElement(BaseLib::PRpcClientInfo clientInfo, std::string& elementId, BaseLib::PVariable data)
{
    try
    {
        if(elementId.empty()) return BaseLib::Variable::createError(-1, "elementId is empty.");
        if(data->type != BaseLib::VariableType::tStruct) return BaseLib::Variable::createError(-1, "data is not of type Struct.");

        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);
        auto databaseId = GD::bl->db->addUiElement(elementId, data);

        if(databaseId == 0) return BaseLib::Variable::createError(-1, "Error adding element to database.");

        auto uiElement = std::make_shared<UiElement>();
        uiElement->databaseId = databaseId;
        uiElement->elementId = elementId;
        uiElement->data = data;

        addDataInfo(uiElement, data);

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
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void UiController::addDataInfo(UiController::PUiElement& uiElement, BaseLib::PVariable& data)
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
                        if(peerIdIterator == peerElement->structValue->end() || peerIdIterator->second->integerValue64 == 0) continue;

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
                    else if(peerElement->integerValue64 != 0)
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
                        if(peerIdIterator == peerElement->structValue->end() || peerIdIterator->second->integerValue64 == 0) continue;

                        variableInfo->peerId = (uint64_t) peerIdIterator->second->integerValue64;

                        auto channelIterator = peerElement->structValue->find("channel");
                        if(channelIterator != peerElement->structValue->end()) variableInfo->channel = channelIterator->second->integerValue;

                        auto nameIterator = peerElement->structValue->find("name");
                        if(nameIterator != peerElement->structValue->end()) variableInfo->name = nameIterator->second->stringValue;

                        auto minimumValueIterator = peerElement->structValue->find("minimum");
                        if(minimumValueIterator != peerElement->structValue->end()) variableInfo->minimumValue = minimumValueIterator->second;

                        auto maximumValueIterator = peerElement->structValue->find("maximum");
                        if(maximumValueIterator != peerElement->structValue->end()) variableInfo->maximumValue = maximumValueIterator->second;
                    }
                    else if(peerElement->integerValue64 != 0)
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
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void UiController::addVariableValues(const BaseLib::PRpcClientInfo& clientInfo, const PUiElement& uiElement, BaseLib::PArray& variableInputs)
{
    try
    {
        for(auto& variableInput : *variableInputs)
        {
            auto peerIdIterator = variableInput->structValue->find("peer");
            if(peerIdIterator == variableInput->structValue->end()) continue;
            auto channelIterator = variableInput->structValue->find("channel");
            if(channelIterator == variableInput->structValue->end()) continue;
            auto nameIterator = variableInput->structValue->find("name");
            if(nameIterator == variableInput->structValue->end()) continue;

            std::string methodName = "getValue";
            auto parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            parameters->arrayValue->reserve(3);
            parameters->arrayValue->emplace_back(peerIdIterator->second);
            parameters->arrayValue->emplace_back(channelIterator->second);
            parameters->arrayValue->emplace_back(nameIterator->second);

            auto result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
            if(!result->errorStruct) variableInput->structValue->emplace("value", result);

            methodName = "getVariableDescription";
            result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
            if(result->errorStruct)
            {
                GD::out.printWarning("Warning: Could not get variable description for UI element " + uiElement->elementId + " with ID " + std::to_string(uiElement->databaseId) + ": " + result->structValue->at("faultString")->stringValue);
                continue;
            }

            auto typeIterator = result->structValue->find("TYPE");
            if(typeIterator != result->structValue->end()) variableInput->structValue->emplace("type", std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::toLower(typeIterator->second->stringValue)));

            auto minimumValueIterator = variableInput->structValue->find("minimumValue");
            auto maximumValueIterator = variableInput->structValue->find("maximumValue");
            if(minimumValueIterator == variableInput->structValue->end() || maximumValueIterator == variableInput->structValue->end())
            {
                if(minimumValueIterator == variableInput->structValue->end())
                {
                    auto minimumValueIterator2 = result->structValue->find("MIN");
                    if(minimumValueIterator2 != result->structValue->end())
                    {
                        variableInput->structValue->emplace("minimumValue", minimumValueIterator2->second);
                    }
                }

                if(maximumValueIterator == variableInput->structValue->end())
                {
                    auto maximumValueIterator2 = result->structValue->find("MAX");
                    if(maximumValueIterator2 != result->structValue->end())
                    {
                        variableInput->structValue->emplace("maximumValue", maximumValueIterator2->second);
                    }
                }
            }
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

BaseLib::PVariable UiController::getAllUiElements(BaseLib::PRpcClientInfo clientInfo, std::string& language)
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);

        bool checkAcls = clientInfo->acls->variablesRoomsCategoriesRolesDevicesReadSet();

        auto uiElements = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        uiElements->arrayValue->reserve(_uiElements.size());

        for(auto& uiElement : _uiElements)
        {
            auto languageIterator = uiElement.second->rpcElement.find(language);
            if(languageIterator == uiElement.second->rpcElement.end())
            {
                auto rpcElement = _descriptions->getUiElement(language, uiElement.second->elementId, uiElement.second->peerInfo);
                if(!rpcElement) continue;
                uiElement.second->rpcElement.emplace(language, rpcElement);
                languageIterator = uiElement.second->rpcElement.find(language);
                if(languageIterator == uiElement.second->rpcElement.end()) continue;
            }

            if(checkAcls)
            {
                if(!clientInfo->acls->checkRoomReadAccess(uiElement.second->roomId)) continue;
                for(auto categoryId : uiElement.second->categoryIds)
                {
                    if(!clientInfo->acls->checkCategoryReadAccess(categoryId)) continue;
                }
                if(!checkElementAccess(clientInfo, uiElement.second, languageIterator->second)) continue;
            }

            auto elementInfo = languageIterator->second->getElementInfo();
            elementInfo->structValue->emplace("databaseId", std::make_shared<BaseLib::Variable>(uiElement.second->databaseId));
            elementInfo->structValue->emplace("label", std::make_shared<BaseLib::Variable>(uiElement.second->label));
            elementInfo->structValue->emplace("room", std::make_shared<BaseLib::Variable>(uiElement.second->roomId));
            auto categories = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            categories->arrayValue->reserve(uiElement.second->categoryIds.size());
            for(auto categoryId : uiElement.second->categoryIds)
            {
                categories->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(categoryId));
            }
            elementInfo->structValue->emplace("categories", categories);

            //{{{ value
            //Simple
            auto variableInputsIterator = elementInfo->structValue->find("variableInputs");
            if(variableInputsIterator != elementInfo->structValue->end()) addVariableValues(clientInfo, uiElement.second, variableInputsIterator->second->arrayValue);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableInputsIterator = control->structValue->find("variableInputs");
                        if(variableInputsIterator != control->structValue->end()) addVariableValues(clientInfo, uiElement.second, variableInputsIterator->second->arrayValue);
                    };
                }
            }
            //}}}

            auto dataIterator = uiElement.second->data->structValue->find("metadata");
            if(dataIterator != uiElement.second->data->structValue->end())
            {
                auto metadataIterator = elementInfo->structValue->find("metadata");
                if(metadataIterator == elementInfo->structValue->end()) metadataIterator = elementInfo->structValue->emplace("metadata", std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct)).first;

                for(auto& metadataElement : *dataIterator->second->structValue)
                {
                    metadataIterator->second->structValue->emplace(metadataElement);
                }
            }

            uiElements->arrayValue->emplace_back(elementInfo);
        }

        return uiElements;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getAvailableUiElements(BaseLib::PRpcClientInfo clientInfo, std::string& language)
{
    try
    {
        return _descriptions->getUiElements(language);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getUiElementsInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, std::string& language)
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
            if(variableInputsIterator != elementInfo->structValue->end()) addVariableValues(clientInfo, uiElement, variableInputsIterator->second->arrayValue);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableInputsIterator = control->structValue->find("variableInputs");
                        if(variableInputsIterator != control->structValue->end()) addVariableValues(clientInfo, uiElement, variableInputsIterator->second->arrayValue);
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
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable UiController::getUiElementsInCategory(BaseLib::PRpcClientInfo clientInfo, uint64_t categoryId, std::string& language)
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
            if(variableInputsIterator != elementInfo->structValue->end()) addVariableValues(clientInfo, uiElement, variableInputsIterator->second->arrayValue);
            else //Complex
            {
                auto controlsIterator = elementInfo->structValue->find("controls");
                if(controlsIterator != elementInfo->structValue->end())
                {
                    for(auto& control : *controlsIterator->second->arrayValue)
                    {
                        auto variableInputsIterator = control->structValue->find("variableInputs");
                        if(variableInputsIterator != control->structValue->end()) addVariableValues(clientInfo, uiElement, variableInputsIterator->second->arrayValue);
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
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
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

        for(auto& variableOutput : rpcElement->variableOutputs)
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

        return true;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return false;
}

BaseLib::PVariable UiController::removeUiElement(BaseLib::PRpcClientInfo clientInfo, uint64_t databaseId)
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
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}