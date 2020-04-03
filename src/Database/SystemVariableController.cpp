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

#include "SystemVariableController.h"
#include "../GD/GD.h"

namespace Homegear
{

SystemVariableController::SystemVariableController()
{
    try
    {
        _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
        _rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), false, true));

        {
            auto systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
            systemVariable->name = "homegearStartTime";
            systemVariable->value = std::make_shared<BaseLib::Variable>((double)(BaseLib::HelperFunctions::getTime() / 1000.0));
            systemVariable->flags = 0b00000101; //System, -, readonly

            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            _systemVariables.emplace("homegearStartTime", systemVariable);
            _specialSystemVariables.emplace("homegearStartTime");
        }

        {
            auto systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
            systemVariable->name = "socketPath";
            systemVariable->value = std::make_shared<BaseLib::Variable>(GD::bl->settings.socketPath());
            systemVariable->flags = 0b00000111; //System, hidden, readonly

            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            _systemVariables.emplace("socketPath", systemVariable);
            _specialSystemVariables.emplace("socketPath");
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

BaseLib::PVariable SystemVariableController::erase(std::string& variableId)
{
    try
    {
        if(variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator != _systemVariables.end())
            {
                if(systemVariableIterator->second->flags != -1 && (systemVariableIterator->second->flags & 4)) return BaseLib::Variable::createError(-1, "Variable is not deletable.");
                _systemVariables.erase(variableId);
            }
        }

        GD::bl->db->deleteSystemVariable(variableId);

        std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{variableId});
        std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>());
        BaseLib::PVariable value(new BaseLib::Variable(BaseLib::VariableType::tStruct));
        value->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(0))));
        value->structValue->insert(BaseLib::StructElement("CODE", BaseLib::PVariable(new BaseLib::Variable(1))));
        values->push_back(value);
#ifdef EVENTHANDLER
        GD::eventHandler->trigger(variableId, value);
#endif
        std::string source;
        if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(source, 0, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
        GD::scriptEngineServer->broadcastEvent(source, 0, -1, valueKeys, values);
#endif
        if(GD::ipcServer) GD::ipcServer->broadcastEvent(source, 0, -1, valueKeys, values);
        std::string deviceAddress;
        GD::rpcClient->broadcastEvent(source, 0, -1, deviceAddress, valueKeys, values);

        return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::getAll(BaseLib::PRpcClientInfo clientInfo, bool returnRoomsCategoriesRolesFlags, bool checkAcls)
{
    try
    {
        auto rows = GD::bl->db->getAllSystemVariables();
        if(!rows) return BaseLib::Variable::createError(-1, "Could not read from database.");

        BaseLib::PVariable systemVariableStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

        { //Insert special variables
            for(auto& specialSystemVariable : _specialSystemVariables)
            {
                BaseLib::Database::PSystemVariable systemVariable;

                {
                    std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                    auto systemVariableIterator = _systemVariables.find(specialSystemVariable);
                    if(systemVariableIterator != _systemVariables.end()) systemVariable = systemVariableIterator->second;
                }

                if(systemVariable && (!checkAcls || (checkAcls && clientInfo->acls->checkSystemVariableReadAccess(systemVariable))))
                {
                    if(returnRoomsCategoriesRolesFlags)
                    {
                        BaseLib::PVariable element = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

                        if(systemVariable->room != 0) element->structValue->emplace("ROOM", std::make_shared<BaseLib::Variable>(systemVariable->room));

                        BaseLib::PVariable categoriesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                        categoriesArray->arrayValue->reserve(systemVariable->categories.size());
                        for(auto category : systemVariable->categories)
                        {
                            if(category != 0) categoriesArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(category));
                        }
                        if(!categoriesArray->arrayValue->empty()) element->structValue->emplace("CATEGORIES", categoriesArray);

                        BaseLib::PVariable rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                        rolesArray->arrayValue->reserve(systemVariable->roles.size());
                        for(auto role : systemVariable->roles)
                        {
                            auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                            roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
                            roleStruct->structValue->emplace("direction", std::make_shared<BaseLib::Variable>((int32_t)role.second.direction));
                            if(role.second.invert) roleStruct->structValue->emplace("invert", std::make_shared<BaseLib::Variable>(role.second.invert));
                            rolesArray->arrayValue->emplace_back(std::move(roleStruct));
                        }
                        if(!rolesArray->arrayValue->empty()) element->structValue->emplace("ROLES", rolesArray);

                        if(systemVariable->flags > 0) element->structValue->emplace("FLAGS", std::make_shared<BaseLib::Variable>(systemVariable->flags));

                        element->structValue->emplace("VALUE", systemVariable->value);

                        systemVariableStruct->structValue->insert(BaseLib::StructElement(systemVariable->name, element));
                    }
                    else systemVariableStruct->structValue->insert(BaseLib::StructElement(systemVariable->name, systemVariable->value));
                }
            }
        }

        for(auto i = rows->begin(); i != rows->end(); ++i)
        {
            if(i->second.size() < 4) continue;

            BaseLib::Database::PSystemVariable systemVariable;

            {
                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                auto systemVariableIterator = _systemVariables.find(i->second.at(0)->textValue);
                if(systemVariableIterator != _systemVariables.end()) systemVariable = systemVariableIterator->second;
            }

            if(!systemVariable)
            {
                systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
                systemVariable->name = i->second.at(0)->textValue;
                systemVariable->value = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
                systemVariable->room = (uint64_t)i->second.at(2)->intValue;

                std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(i->second.at(3)->textValue, ',');
                for(auto& categoryString : categoryStrings)
                {
                    uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
                    if(category != 0) systemVariable->categories.emplace(category);
                }

                std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(i->second.at(4)->textValue, ',');
                for(auto& roleString : roleStrings)
                {
                    if(roleString.empty()) continue;
                    auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
                    uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
                    BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
                    bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
                    if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
                }

                systemVariable->flags = (int32_t)i->second.at(5)->intValue;

                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                _systemVariables.emplace(systemVariable->name, systemVariable);
            }

            if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) continue;

            if(systemVariable->flags != -1 && (systemVariable->flags & 2))
            {
                auto& source = clientInfo->initInterfaceId;
                if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
                {
                    continue;
                }
            }

            if(returnRoomsCategoriesRolesFlags)
            {
                BaseLib::PVariable element = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

                if(systemVariable->room != 0) element->structValue->emplace("ROOM", std::make_shared<BaseLib::Variable>(systemVariable->room));

                BaseLib::PVariable categoriesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                categoriesArray->arrayValue->reserve(systemVariable->categories.size());
                for(auto category : systemVariable->categories)
                {
                    if(category != 0) categoriesArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(category));
                }
                if(!categoriesArray->arrayValue->empty()) element->structValue->emplace("CATEGORIES", categoriesArray);

                BaseLib::PVariable rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                rolesArray->arrayValue->reserve(systemVariable->roles.size());
                for(auto role : systemVariable->roles)
                {
                    auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
                    roleStruct->structValue->emplace("direction", std::make_shared<BaseLib::Variable>((int32_t)role.second.direction));
                    if(role.second.invert) roleStruct->structValue->emplace("invert", std::make_shared<BaseLib::Variable>(role.second.invert));
                    rolesArray->arrayValue->emplace_back(std::move(roleStruct));
                }
                if(!rolesArray->arrayValue->empty()) element->structValue->emplace("ROLES", rolesArray);

                if(systemVariable->flags > 0) element->structValue->emplace("FLAGS", std::make_shared<BaseLib::Variable>(systemVariable->flags));

                element->structValue->emplace("VALUE", systemVariable->value);

                systemVariableStruct->structValue->insert(BaseLib::StructElement(systemVariable->name, element));
            }
            else systemVariableStruct->structValue->insert(BaseLib::StructElement(systemVariable->name, systemVariable->value));
        }

        return systemVariableStruct;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void SystemVariableController::removeCategory(uint64_t categoryId)
{
    try
    {
        if(categoryId == 0) return;

        GD::bl->db->removeCategoryFromSystemVariables(categoryId);

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            _systemVariables.clear();
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void SystemVariableController::removeRole(uint64_t roleId)
{
    try
    {
        if(roleId == 0) return;

        GD::bl->db->removeRoleFromSystemVariables(roleId);

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            _systemVariables.clear();
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void SystemVariableController::removeRoom(uint64_t roomId)
{
    try
    {
        if(roomId == 0) return;

        GD::bl->db->removeRoomFromSystemVariables(roomId);

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            _systemVariables.clear();
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

BaseLib::PVariable SystemVariableController::getValue(BaseLib::PRpcClientInfo clientInfo, std::string& variableId, bool checkAcls)
{
    try
    {
        if(variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

        auto systemVariable = getInternal(variableId);
        if(!systemVariable) return std::make_shared<BaseLib::Variable>();

        if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        if(systemVariable->flags != -1 && (systemVariable->flags & 2))
        {
            auto& source = clientInfo->initInterfaceId;
            if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
            {
                return BaseLib::Variable::createError(-32603, "Unauthorized.");
            }
        }

        return systemVariable->value;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::Database::PSystemVariable SystemVariableController::getInternal(const std::string& variableId)
{
    try
    {
        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator != _systemVariables.end())
            {
                return systemVariableIterator->second;
            }
        }

        auto rows = GD::bl->db->getSystemVariable(variableId);
        if(!rows || rows->empty() || rows->at(0).empty()) return BaseLib::Database::PSystemVariable();

        auto value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);

        auto systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
        systemVariable->name = variableId;
        systemVariable->value = value;
        systemVariable->room = (uint64_t)rows->at(0).at(1)->intValue;

        std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(2)->textValue, ',');
        for(auto& categoryString : categoryStrings)
        {
            uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
            if(category != 0) systemVariable->categories.emplace(category);
        }

        std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(3)->textValue, ',');
        for(auto& roleString : roleStrings)
        {
            if(roleString.empty()) continue;
            auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
            uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
            BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
            bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
            if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
        }

        systemVariable->flags = (int32_t)rows->at(0).at(4)->intValue;

        std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
        _systemVariables.emplace(variableId, systemVariable);
        return systemVariable;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Database::PSystemVariable();
}

BaseLib::PVariable SystemVariableController::getCategories(std::string& variableId)
{
    try
    {
        auto categories = getCategoriesInternal(variableId);
        BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        result->arrayValue->reserve(categories.size());
        for(auto category : categories)
        {
            result->arrayValue->push_back(std::make_shared<BaseLib::Variable>(category));
        }
        return result;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

std::set<uint64_t> SystemVariableController::getCategoriesInternal(std::string& variableId)
{
    try
    {
        if(variableId.size() > 250) return std::set<uint64_t>();

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator != _systemVariables.end())
            {
                return systemVariableIterator->second->categories;
            }
        }

        auto rows = GD::bl->db->getSystemVariable(variableId);
        if(!rows || rows->empty() || rows->at(0).empty()) return std::set<uint64_t>();

        auto value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);

        auto systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
        systemVariable->name = variableId;
        systemVariable->value = value;
        systemVariable->room = (uint64_t)rows->at(0).at(1)->intValue;

        std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(2)->textValue, ',');
        for(auto& categoryString : categoryStrings)
        {
            uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
            if(category != 0) systemVariable->categories.emplace(category);
        }

        std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(3)->textValue, ',');
        for(auto& roleString : roleStrings)
        {
            if(roleString.empty()) continue;
            auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
            uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
            BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
            bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
            if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
        }

        systemVariable->flags = (int32_t)rows->at(0).at(4)->intValue;

        std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
        _systemVariables.emplace(variableId, systemVariable);
        return systemVariable->categories;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return std::set<uint64_t>();
}

BaseLib::PVariable SystemVariableController::getRoles(std::string& variableId)
{
    try
    {
        auto roles = getRolesInternal(variableId);
        BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        result->arrayValue->reserve(roles.size());
        for(auto role : roles)
        {
            auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
            roleStruct->structValue->emplace("direction", std::make_shared<BaseLib::Variable>((int32_t)role.second.direction));
            if(role.second.invert) roleStruct->structValue->emplace("invert", std::make_shared<BaseLib::Variable>(role.second.invert));
            result->arrayValue->emplace_back(std::move(roleStruct));
        }
        return result;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::getRolesInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, bool checkAcls)
{
    try
    {
        auto rows = GD::bl->db->getSystemVariablesInRoom(roomId);
        if(!rows) return BaseLib::Variable::createError(-1, "Could not read from database.");

        BaseLib::PVariable systemVariableStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        if(rows->empty()) return systemVariableStruct;
        for(auto i = rows->begin(); i != rows->end(); ++i)
        {
            if(i->second.size() < 3) continue;

            BaseLib::Database::PSystemVariable systemVariable;

            {
                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                auto systemVariableIterator = _systemVariables.find(i->second.at(0)->textValue);
                if(systemVariableIterator != _systemVariables.end()) systemVariable = systemVariableIterator->second;
            }

            if(!systemVariable)
            {
                systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
                systemVariable->name = i->second.at(0)->textValue;
                systemVariable->value = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
                systemVariable->room = roomId;

                std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(i->second.at(2)->textValue, ',');
                for(auto& categoryString : categoryStrings)
                {
                    uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
                    if(category != 0) systemVariable->categories.emplace(category);
                }

                std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(i->second.at(3)->textValue, ',');
                for(auto& roleString : roleStrings)
                {
                    if(roleString.empty()) continue;
                    auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
                    uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
                    BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
                    bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
                    if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
                }

                systemVariable->flags = (int32_t)i->second.at(4)->intValue;

                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                _systemVariables.emplace(systemVariable->name, systemVariable);
            }

            if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) continue;

            if(systemVariable->flags != -1 && (systemVariable->flags & 2))
            {
                auto& source = clientInfo->initInterfaceId;
                if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
                {
                    continue;
                }
            }

            if(systemVariable->room != roomId || systemVariable->roles.empty()) continue;

            BaseLib::PVariable rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
            rolesArray->arrayValue->reserve(systemVariable->roles.size());
            for(auto role : systemVariable->roles)
            {
                auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
                roleStruct->structValue->emplace("direction", std::make_shared<BaseLib::Variable>((int32_t)role.second.direction));
                if(role.second.invert) roleStruct->structValue->emplace("invert", std::make_shared<BaseLib::Variable>(role.second.invert));
                rolesArray->arrayValue->emplace_back(std::move(roleStruct));
            }
            systemVariableStruct->structValue->emplace(systemVariable->name, rolesArray);
        }

        return systemVariableStruct;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

std::unordered_map<uint64_t, BaseLib::Role> SystemVariableController::getRolesInternal(std::string& variableId)
{
    try
    {
        if(variableId.size() > 250) return std::unordered_map<uint64_t, BaseLib::Role>();

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator != _systemVariables.end())
            {
                return systemVariableIterator->second->roles;
            }
        }

        auto rows = GD::bl->db->getSystemVariable(variableId);
        if(!rows || rows->empty() || rows->at(0).empty()) return std::unordered_map<uint64_t, BaseLib::Role>();

        auto value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);

        auto systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
        systemVariable->name = variableId;
        systemVariable->value = value;
        systemVariable->room = (uint64_t)rows->at(0).at(1)->intValue;

        std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(2)->textValue, ',');
        for(auto& categoryString : categoryStrings)
        {
            uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
            if(category != 0) systemVariable->categories.emplace(category);
        }

        std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(3)->textValue, ',');
        for(auto& roleString : roleStrings)
        {
            if(roleString.empty()) continue;
            auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
            uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
            BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
            bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
            if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
        }

        systemVariable->flags = (int32_t)rows->at(0).at(4)->intValue;

        std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
        _systemVariables.emplace(variableId, systemVariable);
        return systemVariable->roles;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return std::unordered_map<uint64_t, BaseLib::Role>();
}

BaseLib::PVariable SystemVariableController::getRoom(std::string& variableId)
{
    return std::make_shared<BaseLib::Variable>(getRoomInternal(variableId));
}

uint64_t SystemVariableController::getRoomInternal(std::string& variableId)
{
    try
    {
        if(variableId.size() > 250) return 0;

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator != _systemVariables.end())
            {
                return systemVariableIterator->second->room;
            }
        }

        auto rows = GD::bl->db->getSystemVariable(variableId);
        if(!rows || rows->empty() || rows->at(0).empty()) return 0;

        auto value = _rpcDecoder->decodeResponse(*rows->at(0).at(0)->binaryValue);

        auto systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
        systemVariable->name = variableId;
        systemVariable->value = value;
        systemVariable->room = (uint64_t)rows->at(0).at(1)->intValue;

        std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(2)->textValue, ',');
        for(auto& categoryString : categoryStrings)
        {
            uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
            if(category != 0) systemVariable->categories.emplace(category);
        }

        std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(rows->at(0).at(3)->textValue, ',');
        for(auto& roleString : roleStrings)
        {
            if(roleString.empty()) continue;
            auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
            uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
            BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
            bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
            if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
        }

        systemVariable->flags = (int32_t)rows->at(0).at(4)->intValue;

        std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
        _systemVariables.emplace(variableId, systemVariable);
        return systemVariable->room;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return 0;
}

BaseLib::PVariable SystemVariableController::getVariablesInCategory(BaseLib::PRpcClientInfo clientInfo, uint64_t categoryId, bool checkAcls)
{
    try
    {
        auto rows = GD::bl->db->getAllSystemVariables();
        if(!rows) return BaseLib::Variable::createError(-1, "Could not read from database.");

        BaseLib::PVariable systemVariableArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        systemVariableArray->arrayValue->reserve(rows->size());
        if(rows->empty()) return systemVariableArray;
        for(auto i = rows->begin(); i != rows->end(); ++i)
        {
            if(i->second.size() < 4) continue;

            BaseLib::Database::PSystemVariable systemVariable;

            {
                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                auto systemVariableIterator = _systemVariables.find(i->second.at(0)->textValue);
                if(systemVariableIterator != _systemVariables.end()) systemVariable = systemVariableIterator->second;
            }

            if(!systemVariable)
            {
                systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
                systemVariable->name = i->second.at(0)->textValue;
                systemVariable->value = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
                systemVariable->room = (uint64_t) i->second.at(2)->intValue;

                std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(i->second.at(3)->textValue, ',');
                for(auto& categoryString : categoryStrings)
                {
                    uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
                    if(category != 0) systemVariable->categories.emplace(category);
                }

                std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(i->second.at(4)->textValue, ',');
                for(auto& roleString : roleStrings)
                {
                    if(roleString.empty()) continue;
                    auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
                    uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
                    BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
                    bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
                    if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
                }

                systemVariable->flags = (int32_t)i->second.at(5)->intValue;

                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                _systemVariables.emplace(systemVariable->name, systemVariable);
            }

            if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) continue;

            if(systemVariable->flags != -1 && (systemVariable->flags & 2))
            {
                auto& source = clientInfo->initInterfaceId;
                if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
                {
                    continue;
                }
            }

            if((systemVariable->categories.empty() && categoryId == 0) || systemVariable->categories.find(categoryId) != systemVariable->categories.end())
            {
                systemVariableArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(systemVariable->name));
            }
        }

        return systemVariableArray;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::getVariablesInRole(BaseLib::PRpcClientInfo clientInfo, uint64_t roleId, bool checkAcls)
{
    try
    {
        auto rows = GD::bl->db->getAllSystemVariables();
        if(!rows) return BaseLib::Variable::createError(-1, "Could not read from database.");

        BaseLib::PVariable systemVariableStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        if(rows->empty()) return systemVariableStruct;
        for(auto i = rows->begin(); i != rows->end(); ++i)
        {
            if(i->second.size() < 4) continue;

            BaseLib::Database::PSystemVariable systemVariable;

            {
                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                auto systemVariableIterator = _systemVariables.find(i->second.at(0)->textValue);
                if(systemVariableIterator != _systemVariables.end()) systemVariable = systemVariableIterator->second;
            }

            if(!systemVariable)
            {
                systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
                systemVariable->name = i->second.at(0)->textValue;
                systemVariable->value = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
                systemVariable->room = (uint64_t) i->second.at(2)->intValue;

                std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(i->second.at(3)->textValue, ',');
                for(auto& categoryString : categoryStrings)
                {
                    uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
                    if(category != 0) systemVariable->categories.emplace(category);
                }

                std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(i->second.at(4)->textValue, ',');
                for(auto& roleString : roleStrings)
                {
                    if(roleString.empty()) continue;
                    auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
                    uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
                    BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
                    bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
                    if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
                }

                systemVariable->flags = (int32_t)i->second.at(5)->intValue;

                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                _systemVariables.emplace(systemVariable->name, systemVariable);
            }

            if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) continue;

            if(systemVariable->flags != -1 && (systemVariable->flags & 2))
            {
                auto& source = clientInfo->initInterfaceId;
                if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
                {
                    continue;
                }
            }

            auto rolesIterator = systemVariable->roles.find(roleId);
            if((systemVariable->roles.empty() && roleId == 0) || rolesIterator != systemVariable->roles.end())
            {
                auto entry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                if(rolesIterator != systemVariable->roles.end())
                {
                    entry->structValue->emplace("direction", std::make_shared<BaseLib::Variable>((int32_t)rolesIterator->second.direction));
                    if(rolesIterator->second.invert) entry->structValue->emplace("invert", std::make_shared<BaseLib::Variable>(rolesIterator->second.invert));
                }
                systemVariableStruct->structValue->emplace(systemVariable->name, entry);
            }
        }

        return systemVariableStruct;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::getVariablesInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, bool checkAcls)
{
    try
    {
        auto rows = GD::bl->db->getSystemVariablesInRoom(roomId);
        if(!rows) return BaseLib::Variable::createError(-1, "Could not read from database.");

        BaseLib::PVariable systemVariableArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        systemVariableArray->arrayValue->reserve(rows->size());
        if(rows->empty()) return systemVariableArray;
        for(auto i = rows->begin(); i != rows->end(); ++i)
        {
            if(i->second.size() < 3) continue;

            BaseLib::Database::PSystemVariable systemVariable;

            {
                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                auto systemVariableIterator = _systemVariables.find(i->second.at(0)->textValue);
                if(systemVariableIterator != _systemVariables.end()) systemVariable = systemVariableIterator->second;
            }

            if(!systemVariable)
            {
                systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
                systemVariable->name = i->second.at(0)->textValue;
                systemVariable->value = _rpcDecoder->decodeResponse(*i->second.at(1)->binaryValue);
                systemVariable->room = roomId;

                std::vector<std::string> categoryStrings = BaseLib::HelperFunctions::splitAll(i->second.at(2)->textValue, ',');
                for(auto& categoryString : categoryStrings)
                {
                    uint64_t category = BaseLib::Math::getUnsignedNumber64(categoryString);
                    if(category != 0) systemVariable->categories.emplace(category);
                }

                std::vector<std::string> roleStrings = BaseLib::HelperFunctions::splitAll(i->second.at(3)->textValue, ',');
                for(auto& roleString : roleStrings)
                {
                    if(roleString.empty()) continue;
                    auto parts = BaseLib::HelperFunctions::splitAll(roleString, '-');
                    uint64_t roleId = BaseLib::Math::getUnsignedNumber64(parts.at(0));
                    BaseLib::RoleDirection direction = parts.size() > 1 ? (BaseLib::RoleDirection)BaseLib::Math::getNumber(parts.at(1)) : BaseLib::RoleDirection::both;
                    bool invert = parts.size() > 2 ? (bool)BaseLib::Math::getNumber(parts.at(2)) : false;
                    if(roleId != 0) systemVariable->roles.emplace(roleId, std::move(BaseLib::Role(roleId, direction, invert)));
                }

                systemVariable->flags = (int32_t)i->second.at(4)->intValue;

                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                _systemVariables.emplace(systemVariable->name, systemVariable);
            }

            if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) continue;

            if(systemVariable->flags != -1 && (systemVariable->flags & 2))
            {
                auto& source = clientInfo->initInterfaceId;
                if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
                {
                    continue;
                }
            }

            systemVariableArray->arrayValue->push_back(std::make_shared<BaseLib::Variable>(systemVariable->name));
        }

        return systemVariableArray;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::setValue(BaseLib::PRpcClientInfo clientInfo, std::string& variableId, BaseLib::PVariable& value, int32_t flags, bool checkAcls)
{
    try
    {
        if(!value) return BaseLib::Variable::createError(-32602, "Could not parse data.");
        if(variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
        if(variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");
        //Don't check for type here, so base64, string and future data types that use stringValue are handled
        if(value->type != BaseLib::VariableType::tBase64 && value->type != BaseLib::VariableType::tString && value->type != BaseLib::VariableType::tInteger && value->type != BaseLib::VariableType::tInteger64 && value->type != BaseLib::VariableType::tFloat && value->type != BaseLib::VariableType::tBoolean && value->type != BaseLib::VariableType::tStruct && value->type != BaseLib::VariableType::tArray) return BaseLib::Variable::createError(-32602, "Type " + BaseLib::Variable::getTypeString(value->type) + " is currently not supported.");

        BaseLib::Database::PSystemVariable systemVariable = getInternal(variableId);

        {
            if(!systemVariable)
            {
                systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
                systemVariable->name = variableId;
                systemVariable->value = value;
                systemVariable->flags = flags;

                if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

                std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
                _systemVariables.emplace(variableId, systemVariable);
            }
            else
            {
                if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

                { //Check flags
                    if(systemVariable->flags != -1 && (systemVariable->flags & 3)) //Readonly or invisible
                    {
                        auto& source = clientInfo->initInterfaceId;
                        if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
                        {
                            return BaseLib::Variable::createError(-32603, "Unauthorized.");
                        }
                    }
                }

                //Set type to old value type, value already is converted by constructor of class Variable or RpcDecoder
                if(systemVariable->value->type != BaseLib::VariableType::tVoid)
                {
                    if(value->type != systemVariable->value->type)
                    {
                        value->type = systemVariable->value->type;
                    }

                    if(value->type == BaseLib::VariableType::tInteger64 && value->integerValue64 == (int64_t)value->integerValue)
                    {
                        value->type = BaseLib::VariableType::tInteger;
                    }
                }

                systemVariable->value = value;
                if(systemVariable->flags != -1 && (systemVariable->flags & 4)) flags |= 4;
                if(flags != -1) systemVariable->flags = flags;
            }
        }

        std::ostringstream categories;
        for(auto category : systemVariable->categories)
        {
            categories << std::to_string(category) << ",";
        }
        std::string categoryString = categories.str();
        std::ostringstream roles;
        for(auto role : systemVariable->roles)
        {
            roles << std::to_string(role.first) << "-" << std::to_string((int32_t)role.second.direction) << "-" << std::to_string((int32_t)role.second.invert) << ",";
        }
        std::string roleString = roles.str();

        auto result = GD::bl->db->setSystemVariable(variableId, value, systemVariable->room, categoryString, roleString, flags);
        if(result->errorStruct)
        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            _systemVariables.erase(variableId);
            return result;
        }

#ifdef EVENTHANDLER
        GD::eventHandler->trigger(variableId, value);
#endif
        std::string& source = clientInfo->initInterfaceId;
        std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{variableId});
        std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{value});
        if(GD::nodeBlueServer) GD::nodeBlueServer->broadcastEvent(source, 0, -1, valueKeys, values);
#ifndef NO_SCRIPTENGINE
        GD::scriptEngineServer->broadcastEvent(source, 0, -1, valueKeys, values);
#endif
        if(GD::ipcServer) GD::ipcServer->broadcastEvent(source, 0, -1, valueKeys, values);
        std::string deviceAddress;
        GD::rpcClient->broadcastEvent(source, 0, -1, deviceAddress, valueKeys, values);

        return std::make_shared<BaseLib::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::setCategories(std::string& variableId, std::set<uint64_t>& categoryIds)
{
    try
    {
        if(variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
        if(variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator == _systemVariables.end())
            {
                auto value = getInternal(variableId);
                systemVariableIterator = _systemVariables.find(variableId);
                if(systemVariableIterator == _systemVariables.end()) return BaseLib::Variable::createError(-5, "Unknown variable.");
            }

            systemVariableIterator->second->categories = categoryIds;
        }

        std::ostringstream categories;
        for(auto category : categoryIds)
        {
            categories << std::to_string(category) << ",";
        }
        std::string categoryString = categories.str();

        return GD::bl->db->setSystemVariableCategories(variableId, categoryString);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::setRoles(std::string& variableId, std::unordered_map<uint64_t, BaseLib::Role>& roles)
{
    try
    {
        if(variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
        if(variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator == _systemVariables.end())
            {
                auto value = getInternal(variableId);
                systemVariableIterator = _systemVariables.find(variableId);
                if(systemVariableIterator == _systemVariables.end()) return BaseLib::Variable::createError(-5, "Unknown variable.");
            }

            systemVariableIterator->second->roles = roles;
        }

        std::ostringstream rolesStream;
        for(auto& role : roles)
        {
            rolesStream << std::to_string(role.first) << "-" << std::to_string((int32_t)role.second.direction) << "-" << std::to_string((int32_t)role.second.invert) << ",";
        }
        std::string roleString = rolesStream.str();

        return GD::bl->db->setSystemVariableRoles(variableId, roleString);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SystemVariableController::setRoom(std::string& variableId, uint64_t roomId)
{
    try
    {
        if(variableId.empty()) return BaseLib::Variable::createError(-32602, "variableId is an empty string.");
        if(variableId.size() > 250) return BaseLib::Variable::createError(-32602, "variableId has more than 250 characters.");

        {
            std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
            auto systemVariableIterator = _systemVariables.find(variableId);
            if(systemVariableIterator == _systemVariables.end())
            {
                auto value = getInternal(variableId);
                systemVariableIterator = _systemVariables.find(variableId);
                if(systemVariableIterator == _systemVariables.end()) return BaseLib::Variable::createError(-5, "Unknown variable.");
            }

            if(systemVariableIterator->second->room != roomId) systemVariableIterator->second->room = roomId;
            else return std::make_shared<BaseLib::Variable>();
        }

        return GD::bl->db->setSystemVariableRoom(variableId, roomId);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool SystemVariableController::hasCategory(std::string& variableId, uint64_t categoryId)
{
    //No try/catch to throw exceptions in calling method => avoid valid return on errors. Important for ACLs.
    {
        std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
        auto systemVariableIterator = _systemVariables.find(variableId);
        if(systemVariableIterator != _systemVariables.end())
        {
            return systemVariableIterator->second->categories.find(categoryId) != systemVariableIterator->second->categories.end();
        }
    }

    //Not in cache
    auto categories = getCategoriesInternal(variableId);
    return categories.find(categoryId) != categories.end();
}

bool SystemVariableController::hasRole(std::string& variableId, uint64_t roleId)
{
    //No try/catch to throw exceptions in calling method => avoid valid return on errors. Important for ACLs.
    {
        std::lock_guard<std::mutex> systemVariableGuard(_systemVariableMutex);
        auto systemVariableIterator = _systemVariables.find(variableId);
        if(systemVariableIterator != _systemVariables.end())
        {
            return systemVariableIterator->second->roles.find(roleId) != systemVariableIterator->second->roles.end();
        }
    }

    //Not in cache
    auto roles = getRolesInternal(variableId);
    return roles.find(roleId) != roles.end();
}

BaseLib::PVariable SystemVariableController::getVariableDescription(BaseLib::PRpcClientInfo clientInfo, std::string name, const std::unordered_set<std::string>& fields, bool checkAcls)
{
    try
    {
        BaseLib::Database::PSystemVariable systemVariable = getInternal(name);
        if(!systemVariable) return BaseLib::Variable::createError(-5, "Unknown parameter.");
        if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        if(systemVariable->flags != -1 && (systemVariable->flags & 2))
        {
            auto& source = clientInfo->initInterfaceId;
            if(source != "homegear" && source != "scriptEngine" && source != "ipcServer" && source != "nodeBlue")
            {
                return BaseLib::Variable::createError(-32603, "Unauthorized.");
            }
        }

#ifdef CCU2
        if(systemVariable->value->type == BaseLib::VariableType::tInteger64) systemVariable->value->type = BaseLib::VariableType::tInteger;
#endif
        BaseLib::PVariable description = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

        int32_t operations = 0;
        operations += 4; //Readable
        if(systemVariable->flags == -1 || !(systemVariable->flags & 1))  operations += 2; //Writeable
        int32_t uiFlags = 0;
        if(systemVariable->flags == -1 || !(systemVariable->flags & 2)) uiFlags += 1; //Visible

        std::string type;
        if(systemVariable->value->type == BaseLib::VariableType::tBoolean) type = "BOOL";
        else if(systemVariable->value->type == BaseLib::VariableType::tString) type = "STRING";
        else if(systemVariable->value->type == BaseLib::VariableType::tInteger) type = "INTEGER";
        else if(systemVariable->value->type == BaseLib::VariableType::tInteger64) type = "INTEGER64";
        else if(systemVariable->value->type == BaseLib::VariableType::tFloat) type = "FLOAT";
        else if(systemVariable->value->type == BaseLib::VariableType::tArray) type = "ARRAY";
        else if(systemVariable->value->type == BaseLib::VariableType::tStruct) type = "STRUCT";

        if(fields.empty() || fields.find("FLAGS") != fields.end()) description->structValue->emplace("FLAGS", std::make_shared<BaseLib::Variable>(uiFlags));
        if(fields.empty() || fields.find("ID") != fields.end()) description->structValue->emplace("ID", std::make_shared<BaseLib::Variable>(name));
        if(fields.empty() || fields.find("OPERATIONS") != fields.end()) description->structValue->emplace("OPERATIONS", std::make_shared<BaseLib::Variable>(operations));
        if(fields.empty() || fields.find("TYPE") != fields.end()) description->structValue->emplace("TYPE", std::make_shared<BaseLib::Variable>(type));

        if(fields.empty() || fields.find("ROOM") != fields.end())
        {
            auto room = systemVariable->room;
            if(room != 0) description->structValue->emplace("ROOM", std::make_shared<BaseLib::Variable>(room));
        }

        if(fields.empty() || fields.find("CATEGORIES") != fields.end())
        {
            if(!systemVariable->categories.empty())
            {
                BaseLib::PVariable categoriesResult = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                categoriesResult->arrayValue->reserve(systemVariable->categories.size());
                for(auto category : systemVariable->categories)
                {
                    categoriesResult->arrayValue->push_back(std::make_shared<BaseLib::Variable>(category));
                }
                description->structValue->emplace("CATEGORIES", categoriesResult);
            }
        }

        if(fields.empty() || fields.find("ROLES") != fields.end())
        {
            if(!systemVariable->roles.empty())
            {
                BaseLib::PVariable rolesArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
                rolesArray->arrayValue->reserve(systemVariable->roles.size());
                for(auto role : systemVariable->roles)
                {
                    auto roleStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    roleStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(role.second.id));
                    roleStruct->structValue->emplace("direction", std::make_shared<BaseLib::Variable>((int32_t)role.second.direction));
                    if(role.second.invert) roleStruct->structValue->emplace("invert", std::make_shared<BaseLib::Variable>(role.second.invert));
                    rolesArray->arrayValue->emplace_back(std::move(roleStruct));
                }
                description->structValue->emplace("ROLES", rolesArray);
            }
        }

        return description;
    }
    catch(const std::exception& ex)
    {
        GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}