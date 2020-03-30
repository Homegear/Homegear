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

#include "VariableProfileManager.h"
#include "../GD/GD.h"

namespace Homegear
{

VariableProfileManager::VariableProfileManager()
{
    _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
}

void VariableProfileManager::load()
{
    try
    {
        std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
        _variableProfiles.clear();
        auto rows = GD::bl->db->getVariableProfiles();
        for(auto& row : *rows)
        {
            auto variableProfile = std::make_shared<VariableProfile>();
            variableProfile->id = (uint64_t) row.second.at(0)->intValue;
            variableProfile->name = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
            variableProfile->profile = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);

            if(variableProfile->id == 0 || variableProfile->name->structValue->empty()) continue;

            _variableProfiles.emplace(variableProfile->id, std::move(variableProfile));
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

BaseLib::PVariable VariableProfileManager::addVariableProfile(const BaseLib::PVariable& translations, const BaseLib::PVariable& profile)
{
    try
    {
        if(translations->structValue->empty() || profile->structValue->empty())
        {
            return BaseLib::Variable::createError(-1, "translations or profile is empty or not a valid Struct.");
        }

        uint64_t id = GD::bl->db->addVariableProfile(translations, profile);
        if(id == 0) return BaseLib::Variable::createError(-1, "Could not insert profile into database.");

        auto variableProfile = std::make_shared<VariableProfile>();
        variableProfile->id = id;
        variableProfile->name = translations;
        variableProfile->profile = profile;

        std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
        _variableProfiles.emplace(variableProfile->id, std::move(variableProfile));

        return std::make_shared<BaseLib::Variable>(id);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::deleteVariableProfile(uint64_t profileId)
{
    try
    {
        GD::bl->db->deleteVariableProfile(profileId);

        std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
        _variableProfiles.erase(profileId);

        return std::make_shared<BaseLib::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::executeVariableProfile(const BaseLib::PRpcClientInfo& clientInfo, uint64_t profileId)
{
    try
    {
        BaseLib::PVariable profile;

        {
            std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
            auto variableProfileIterator = _variableProfiles.find(profileId);
            if(variableProfileIterator == _variableProfiles.end()) return BaseLib::Variable::createError(-1, "Variable profile not found.");
            profile = variableProfileIterator->second->profile;
        }

        GD::out.printInfo("Info: Executing variable profile with ID " + std::to_string(profileId));

        bool returnValue = true;

        auto valuesIterator = profile->structValue->find("values");
        if(valuesIterator != profile->structValue->end())
        {
            for(auto& value : *valuesIterator->second->arrayValue)
            {
                if(value->arrayValue->size() != 4 && value->arrayValue->size() != 5)
                {
                    GD::out.printWarning("Warning: Value entry of profile " + std::to_string(profileId) + " has wrong number of array elements.");
                    continue;
                }

                BaseLib::PVariable parametersCopy = std::make_shared<BaseLib::Variable>();
                *parametersCopy = *value;
                if(parametersCopy->arrayValue->size() == 4) parametersCopy->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(false));

                std::string methodName = "setValue";
                auto result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parametersCopy);
                if(result->errorStruct)
                {
                    GD::out.printWarning("Warning: Error executing profile " + std::to_string(profileId) + ": " + result->structValue->at("faultString")->stringValue);
                    returnValue = false;
                }
            }
        }

        return std::make_shared<BaseLib::Variable>(returnValue);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::getAllVariableProfiles(const std::string& languageCode)
{
    try
    {
        auto profiles = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

        std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
        profiles->arrayValue->reserve(_variableProfiles.size());
        for(auto& profile : _variableProfiles)
        {
            std::string name;
            auto nameIterator = profile.second->name->structValue->find(languageCode);
            if(nameIterator == profile.second->name->structValue->end()) nameIterator = profile.second->name->structValue->find("en-US");
            if(nameIterator != profile.second->name->structValue->end()) name = nameIterator->second->stringValue;

            profile.second->profile->structValue->erase("id");
            profile.second->profile->structValue->emplace("id", std::make_shared<BaseLib::Variable>(profile.first));
            profile.second->profile->structValue->erase("name");
            profile.second->profile->structValue->emplace("name", std::make_shared<BaseLib::Variable>(name));
            profiles->arrayValue->emplace_back(profile.second->profile);
        }

        return profiles;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::getVariableProfile(uint64_t id, const std::string& languageCode)
{
    try
    {
        std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
        auto variableProfileIterator = _variableProfiles.find(id);
        if(variableProfileIterator == _variableProfiles.end()) return BaseLib::Variable::createError(-1, "Variable profile not found.");

        std::string name;
        auto nameIterator = variableProfileIterator->second->name->structValue->find(languageCode);
        if(nameIterator == variableProfileIterator->second->name->structValue->end()) nameIterator = variableProfileIterator->second->name->structValue->find("en-US");
        if(nameIterator != variableProfileIterator->second->name->structValue->end()) name = nameIterator->second->stringValue;

        variableProfileIterator->second->profile->structValue->erase("id");
        variableProfileIterator->second->profile->structValue->emplace("id", std::make_shared<BaseLib::Variable>(variableProfileIterator->first));
        variableProfileIterator->second->profile->structValue->erase("name");
        variableProfileIterator->second->profile->structValue->emplace("name", std::make_shared<BaseLib::Variable>(name));

        return variableProfileIterator->second->profile;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::updateVariableProfile(uint64_t profileId, const BaseLib::PVariable& translations, const BaseLib::PVariable& profile)
{
    try
    {
        if(translations->structValue->empty() && profile->structValue->empty())
        {
            return BaseLib::Variable::createError(-1, "translations and profile are empty or not a valid Struct.");
        }

        auto result = GD::bl->db->updateVariableProfile(profileId, translations, profile);

        return std::make_shared<BaseLib::Variable>(result);
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}