/* Copyright 2013-2017 Sathya Laufer
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

UiController::UiController()
{
    _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
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
            uiElement->databaseId = (uint64_t)row.second.at(0)->intValue;
            uiElement->elementId = row.second.at(1)->textValue;
            uiElement->data = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);

            if(uiElement->databaseId == 0 || uiElement->elementId.empty()) continue;

            addDataInfo(uiElement, uiElement->data);

            _uiElements.emplace(uiElement->databaseId, uiElement);
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

        auto dataIterator = data->structValue->find("inputPeers");
        if(dataIterator != data->structValue->end())
        {
            uiElement->peerInfo->inputPeers.reserve(dataIterator->second->arrayValue->size());
            for(auto& outerArray : *dataIterator->second->arrayValue)
            {
                std::vector<uint64_t> peers;
                peers.reserve(outerArray->arrayValue->size());
                for(auto& peerIdElement : *outerArray->arrayValue)
                {
                    peers.push_back((uint64_t)peerIdElement->integerValue64);
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
                std::vector<uint64_t> peers;
                peers.reserve(outerArray->arrayValue->size());
                for(auto& peerIdElement : *outerArray->arrayValue)
                {
                    peers.push_back((uint64_t)peerIdElement->integerValue64);
                }
                uiElement->peerInfo->outputPeers.emplace_back(std::move(peers));
            }
        }

        dataIterator = data->structValue->find("room");
        if(dataIterator != data->structValue->end()) uiElement->roomId = (uint64_t)dataIterator->second->integerValue64;

        dataIterator = data->structValue->find("categories");
        if(dataIterator != data->structValue->end())
        {
            for(auto& categoryElement : *dataIterator->second->arrayValue)
            {
                uiElement->categoryIds.emplace((uint64_t)categoryElement->integerValue64);
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

BaseLib::PVariable UiController::getUiElementsInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, std::string& language)
{
    try
    {
        std::lock_guard<std::mutex> uiElementsGuard(_uiElementsMutex);

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