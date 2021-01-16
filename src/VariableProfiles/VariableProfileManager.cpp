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

namespace Homegear {

VariableProfileManager::VariableProfileManager() {
  _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));

  _profileManagerClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
  _profileManagerClientInfo->initInterfaceId = "profileManager";
  _profileManagerClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  std::vector<uint64_t> groups{2};
  _profileManagerClientInfo->acls->fromGroups(groups);
  _profileManagerClientInfo->user = "SYSTEM (2)";
}

void VariableProfileManager::load() {
  try {
    std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
    _variableProfiles.clear();
    auto rows = GD::bl->db->getVariableProfiles();
    for (auto &row : *rows) {
      auto variableProfile = std::make_shared<VariableProfile>();
      variableProfile->id = (uint64_t)row.second.at(0)->intValue;
      variableProfile->name = _rpcDecoder->decodeResponse(*row.second.at(1)->binaryValue);
      variableProfile->profileStruct = _rpcDecoder->decodeResponse(*row.second.at(2)->binaryValue);

      if (variableProfile->id == 0 || variableProfile->name->structValue->empty()) continue;

      size_t valuesArraySize = 0;

      auto valuesIterator = variableProfile->profileStruct->structValue->find("values");
      if (valuesIterator != variableProfile->profileStruct->structValue->end()) {
        valuesArraySize = valuesIterator->second->arrayValue->size();
      }

      {
        auto roleVariables = getRoleVariables(variableProfile->id, variableProfile->profileStruct);
        if (!roleVariables.empty()) {
          variableProfile->values.reserve(roleVariables.size() + valuesArraySize);
          for (auto &roleVariable : roleVariables) {
            variableProfile->values.emplace_back(roleVariable);
          }
        }
      }

      if (valuesIterator != variableProfile->profileStruct->structValue->end()) {
        for (auto &valueEntry : *valuesIterator->second->arrayValue) {
          auto peerIdIterator = valueEntry->structValue->find("peerId");
          auto channelIterator = valueEntry->structValue->find("channel");
          auto variableIterator = valueEntry->structValue->find("variable");
          auto valueIterator = valueEntry->structValue->find("value");
          auto waitIterator = valueEntry->structValue->find("wait");
          auto ignoreValueFromDeviceIterator = valueEntry->structValue->find("ignoreValueFromDevice");
          auto deviceRefractoryPeriodIterator = valueEntry->structValue->find("deviceRefractoryPeriod");

          if (peerIdIterator == valueEntry->structValue->end() || channelIterator == valueEntry->structValue->end() || variableIterator == valueEntry->structValue->end() || valueIterator == valueEntry->structValue->end()) {
            GD::out.printWarning("Warning: Value entry of profile " + std::to_string(variableProfile->id) + " is missing required elements.");
            continue;
          }

          auto profileValue = std::make_shared<VariableProfileValue>();
          profileValue->peerId = peerIdIterator->second->integerValue64;
          profileValue->channel = channelIterator->second->integerValue;
          profileValue->variable = variableIterator->second->stringValue;
          profileValue->value = valueIterator->second;
          if (!profileValue->value) continue;
          profileValue->wait = (waitIterator != valueEntry->structValue->end() && waitIterator->second->booleanValue && profileValue->value->type == BaseLib::VariableType::tBoolean);

          if (ignoreValueFromDeviceIterator != valueEntry->structValue->end()) profileValue->ignoreValueFromDevice = ignoreValueFromDeviceIterator->second->booleanValue;
          if (deviceRefractoryPeriodIterator != valueEntry->structValue->end()) profileValue->deviceRefractoryPeriod = deviceRefractoryPeriodIterator->second->integerValue;
          variableProfile->values.emplace_back(std::move(profileValue));
        }
      }

      _variableProfiles.emplace(variableProfile->id, variableProfile);

      variableProfile->isActive = !variableProfile->values.empty();
      for (auto &valueEntry : variableProfile->values) {
        auto profileAssociation = std::make_shared<VariableProfileAssociation>();
        profileAssociation->profileId = variableProfile->id;
        profileAssociation->profileValue = valueEntry->value;
        profileAssociation->ignoreValueFromDevice = valueEntry->ignoreValueFromDevice;
        profileAssociation->deviceRefractoryPeriod = valueEntry->deviceRefractoryPeriod;

        {
          auto parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          parameters->arrayValue->reserve(3);
          parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->peerId));
          parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->channel));
          parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->variable));
          auto response = GD::rpcServers.begin()->second->callMethod(_profileManagerClientInfo, "getValue", parameters);
          if (!response->errorStruct) profileAssociation->currentValue = response;
        }

        if (!profileAssociation->currentValue) profileAssociation->currentValue = std::make_shared<BaseLib::Variable>();

        variableProfile->variableCount++;

        if (*profileAssociation->currentValue != *profileAssociation->profileValue) {
          variableProfile->isActive = false;
          profileAssociation->isActive = false;
        } else variableProfile->activeVariables++;

        _profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable][variableProfile->id] = profileAssociation;
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void VariableProfileManager::variableEvent(const std::string &source, uint64_t id, int32_t channel, const std::shared_ptr<std::vector<std::string>> &variables, const BaseLib::PArray &values) {
  try {
    std::list<std::pair<uint64_t, bool>> updates;

    {
      std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
      auto peerIdIterator = _profilesByVariable.find(id);
      if (peerIdIterator != _profilesByVariable.end()) {
        auto channelIterator = peerIdIterator->second.find(channel);
        if (channelIterator != peerIdIterator->second.end()) {
          for (int32_t i = 0; i < (signed)variables->size(); i++) {
            auto variable = variables->at(i);
            auto value = values->at(i);

            auto variableIterator = channelIterator->second.find(variable);
            if (variableIterator != channelIterator->second.end()) {
              for (auto &profileAssociation : variableIterator->second) {
                if (source.compare(0, 7, "device-") == 0) {
                  if (profileAssociation.second->ignoreValueFromDevice ||
                      (signed)profileAssociation.first + (profileAssociation.second->deviceRefractoryPeriod * 1000) < BaseLib::HelperFunctions::getTime()) {
                    continue;
                  }
                }

                auto profileIterator = _variableProfiles.find(profileAssociation.first);
                if (profileIterator == _variableProfiles.end()) continue;

                if (*value != *profileAssociation.second->currentValue) {
                  profileAssociation.second->currentValue = value;

                  if (*profileAssociation.second->currentValue == *profileAssociation.second->profileValue) {
                    if (!profileAssociation.second->isActive) {
                      profileAssociation.second->isActive = true;
                      profileIterator->second->activeVariables++;
                      if (profileIterator->second->activeVariables > profileIterator->second->variableCount) profileIterator->second->activeVariables = profileIterator->second->variableCount;
                    }
                  } else {
                    if (profileAssociation.second->isActive) {
                      profileAssociation.second->isActive = false;
                      profileIterator->second->activeVariables--;
                      if (profileIterator->second->activeVariables < 0) profileIterator->second->activeVariables = 0;
                    }
                  }

                  if (profileIterator->second->activeVariables == profileIterator->second->variableCount) {
                    profileIterator->second->isActive = true;
                    updates.emplace_back(profileIterator->first, true);
                  } else if (profileIterator->second->isActive) {
                    profileIterator->second->isActive = false;
                    updates.emplace_back(profileIterator->first, false);
                  }
                }
              }
            }
          }
        }
      }
    }

    for (auto &update : updates) {
      if (GD::nodeBlueServer) GD::nodeBlueServer->broadcastVariableProfileStateChanged(update.first, update.second);
      if (GD::scriptEngineServer) GD::scriptEngineServer->broadcastVariableProfileStateChanged(update.first, update.second);
      if (GD::ipcServer) GD::ipcServer->broadcastVariableProfileStateChanged(update.first, update.second);
      GD::rpcClient->broadcastVariableProfileStateChanged(update.first, update.second);
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable VariableProfileManager::activateVariableProfile(const BaseLib::PRpcClientInfo &clientInfo, uint64_t profileId) {
  try {
    std::vector<PVariableProfileValue> profileValues;

    {
      std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
      auto variableProfileIterator = _variableProfiles.find(profileId);
      if (variableProfileIterator == _variableProfiles.end()) return BaseLib::Variable::createError(-1, "Variable profile not found.");
      profileValues = variableProfileIterator->second->values;
      variableProfileIterator->second->lastActivation = BaseLib::HelperFunctions::getTime();
    }

    GD::out.printInfo("Info: Executing variable profile with ID " + std::to_string(profileId));

    bool returnValue = true;

    for (auto &valueEntry : profileValues) {
      auto result = setValue(clientInfo, profileId, valueEntry->peerId, valueEntry->channel, valueEntry->variable, valueEntry->value, valueEntry->wait);
      if (!result) returnValue = false;
    }

    return std::make_shared<BaseLib::Variable>(returnValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool VariableProfileManager::setValue(const BaseLib::PRpcClientInfo &clientInfo, uint64_t profileId, uint64_t peerId, int32_t channel, const std::string &variable, const BaseLib::PVariable &value, bool wait) {
  try {
    //{{{ This is basically a reimplementation of setValue(). We need this to check the ACLs, because we switch clientInfo when calling setValue on the central.
    if (!clientInfo || !clientInfo->acls->checkMethodAccess("setValue")) return false;
    bool checkAcls = clientInfo->acls->variablesRoomsCategoriesRolesDevicesWriteSet();

    if (peerId == 0 && channel < 0) //System variable?
    {
      auto requestParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      requestParameters->arrayValue->reserve(2);
      requestParameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(variable));
      requestParameters->arrayValue->emplace_back(value);
      std::string methodName = "setSystemVariable";
      auto result = GD::rpcServers.begin()->second->callMethod(_profileManagerClientInfo, methodName, requestParameters);
      if (result->errorStruct) {
        GD::out.printWarning("Warning: Error setting system variable \"" + variable + "\" of profile " + std::to_string(profileId) + ": " + result->structValue->at("faultString")->stringValue);
        return false;
      }
    } else if (peerId != 0 && channel < 0) //Metadata?
    {
      auto requestParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      requestParameters->arrayValue->reserve(3);
      requestParameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(peerId));
      requestParameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(variable));
      requestParameters->arrayValue->push_back(value);
      std::string methodName = "setMetadata";
      auto result = GD::rpcServers.begin()->second->callMethod(_profileManagerClientInfo, methodName, requestParameters);
      if (result->errorStruct) {
        GD::out.printWarning("Warning: Error metadata variable \"" + variable + "\" of profile " + std::to_string(profileId) + ": " + result->structValue->at("faultString")->stringValue);
        return false;
      }
    } else //Device variable
    {
      auto families = GD::familyController->getFamilies();
      for (auto &family : families) {
        std::shared_ptr<BaseLib::Systems::ICentral> central = family.second->getCentral();
        if (central) {
          if (!central->peerExists(peerId)) continue;

          if (checkAcls) {
            auto peer = central->getPeer(peerId);
            if (!peer || !clientInfo->acls->checkVariableWriteAccess(peer, channel, variable)) return false;
          }

          auto result = central->setValue(_profileManagerClientInfo, peerId, channel, variable, value, wait);
          if (result->errorStruct) {
            GD::out.printWarning("Warning: Error setting device variable \"" + variable + "\" of profile " + std::to_string(profileId) + ": " + result->structValue->at("faultString")->stringValue);
            return false;
          }
        }
      }
    }
    //}}}
    return true;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

std::list<PVariableProfileValue> VariableProfileManager::getRoleVariables(uint64_t profileId, const BaseLib::PVariable &profile) {
  try {
    std::list<PVariableProfileValue> roleVariables;

    auto rolesIterator = profile->structValue->find("roles");
    if (rolesIterator != profile->structValue->end()) {
      for (auto &valueEntry : *rolesIterator->second->arrayValue) {
        auto roleIterator = valueEntry->structValue->find("role");
        auto valueIterator = valueEntry->structValue->find("value");
        auto waitIterator = valueEntry->structValue->find("wait");
        auto ignoreValueFromDeviceIterator = valueEntry->structValue->find("ignoreValueFromDevice");
        auto deviceRefractoryPeriodIterator = valueEntry->structValue->find("deviceRefractoryPeriod");

        if (roleIterator != valueEntry->structValue->end()) {
          if (valueIterator == valueEntry->structValue->end()) {
            GD::out.printWarning("Warning: Value entry of profile " + std::to_string(profileId) + " is missing required elements.");
            continue;
          }

          BaseLib::PVariable value = valueIterator->second;
          if (!value) continue;
          bool wait = (waitIterator != valueEntry->structValue->end() && waitIterator->second->booleanValue);
          bool ignoreValueFromDevice = true;
          if (ignoreValueFromDeviceIterator != valueEntry->structValue->end()) ignoreValueFromDevice = ignoreValueFromDeviceIterator->second->booleanValue;
          uint32_t deviceRefractoryPeriod = 60;
          if (deviceRefractoryPeriodIterator != valueEntry->structValue->end()) deviceRefractoryPeriod = deviceRefractoryPeriodIterator->second->integerValue64;

          auto requestParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(roleIterator->second->integerValue64));
          std::string methodName = "getVariablesInRole";
          auto result = GD::rpcServers.begin()->second->callMethod(_profileManagerClientInfo, methodName, requestParameters);
          if (result->errorStruct) continue;

          for (auto &peerStructElement : *result->structValue) {
            for (auto &channelStructElement : *peerStructElement.second->structValue) {
              for (auto &variableElement : *channelStructElement.second->structValue) {
                auto directionIterator = variableElement.second->structValue->find("direction");
                if (directionIterator != variableElement.second->structValue->end()) {
                  if ((BaseLib::RoleDirection)directionIterator->second->integerValue64 == BaseLib::RoleDirection::input) {
                    continue;
                  }
                }

                auto profileValue = std::make_shared<VariableProfileValue>();
                profileValue->peerId = BaseLib::Math::getUnsignedNumber64(peerStructElement.first);
                profileValue->channel = BaseLib::Math::getNumber(channelStructElement.first);
                profileValue->variable = variableElement.first;
                profileValue->value = value;
                profileValue->wait = wait;
                profileValue->ignoreValueFromDevice = ignoreValueFromDevice;
                profileValue->deviceRefractoryPeriod = deviceRefractoryPeriod;

                roleVariables.emplace_back(std::move(profileValue));
              }
            }
          }
        }
      }
    }

    return roleVariables;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::list<PVariableProfileValue>();
}

BaseLib::PVariable VariableProfileManager::addVariableProfile(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PVariable &translations, const BaseLib::PVariable &profile) {
  try {
    if (translations->structValue->empty() || profile->structValue->empty()) {
      return BaseLib::Variable::createError(-1, "translations or profile is empty or not a valid Struct.");
    }

    uint64_t id = GD::bl->db->addVariableProfile(translations, profile);
    if (id == 0) return BaseLib::Variable::createError(-1, "Could not insert profile into database.");

    auto variableProfile = std::make_shared<VariableProfile>();
    variableProfile->id = id;
    variableProfile->name = translations;
    variableProfile->profileStruct = profile;

    size_t valuesArraySize = 0;

    auto valuesIterator = variableProfile->profileStruct->structValue->find("values");
    if (valuesIterator != variableProfile->profileStruct->structValue->end()) {
      valuesArraySize = valuesIterator->second->arrayValue->size();
    }

    {
      auto roleVariables = getRoleVariables(id, profile);
      if (!roleVariables.empty()) {
        variableProfile->values.reserve(roleVariables.size() + valuesArraySize);
        for (auto &roleVariable : roleVariables) {
          variableProfile->values.emplace_back(roleVariable);
        }
      }
    }

    if (valuesIterator != variableProfile->profileStruct->structValue->end()) {
      for (auto &valueEntry : *valuesIterator->second->arrayValue) {
        auto peerIdIterator = valueEntry->structValue->find("peerId");
        auto channelIterator = valueEntry->structValue->find("channel");
        auto variableIterator = valueEntry->structValue->find("variable");
        auto valueIterator = valueEntry->structValue->find("value");
        auto waitIterator = valueEntry->structValue->find("wait");
        auto ignoreValueFromDeviceIterator = valueEntry->structValue->find("ignoreValueFromDevice");
        auto deviceRefractoryPeriodIterator = valueEntry->structValue->find("deviceRefractoryPeriod");

        if (peerIdIterator == valueEntry->structValue->end() || channelIterator == valueEntry->structValue->end() || variableIterator == valueEntry->structValue->end() || valueIterator == valueEntry->structValue->end()) {
          GD::out.printWarning("Warning: Value entry of profile " + std::to_string(variableProfile->id) + " is missing required elements.");
          continue;
        }

        auto profileValue = std::make_shared<VariableProfileValue>();
        profileValue->peerId = peerIdIterator->second->integerValue64;
        profileValue->channel = channelIterator->second->integerValue;
        profileValue->variable = variableIterator->second->stringValue;
        profileValue->value = valueIterator->second;
        if (!profileValue->value) continue;
        profileValue->wait = (waitIterator != valueEntry->structValue->end() && waitIterator->second->booleanValue && profileValue->value->type == BaseLib::VariableType::tBoolean);
        if (ignoreValueFromDeviceIterator != valueEntry->structValue->end()) profileValue->ignoreValueFromDevice = ignoreValueFromDeviceIterator->second->booleanValue;
        if (deviceRefractoryPeriodIterator != valueEntry->structValue->end()) profileValue->deviceRefractoryPeriod = ignoreValueFromDeviceIterator->second->integerValue;
        variableProfile->values.emplace_back(std::move(profileValue));
      }
    }

    std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
    _variableProfiles.emplace(variableProfile->id, variableProfile);

    variableProfile->isActive = !variableProfile->values.empty();
    for (auto &valueEntry : variableProfile->values) {
      auto profileAssociation = std::make_shared<VariableProfileAssociation>();
      profileAssociation->profileId = variableProfile->id;
      profileAssociation->profileValue = valueEntry->value;
      profileAssociation->ignoreValueFromDevice = valueEntry->ignoreValueFromDevice;
      profileAssociation->deviceRefractoryPeriod = valueEntry->deviceRefractoryPeriod;

      {
        auto parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        parameters->arrayValue->reserve(3);
        parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->peerId));
        parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->channel));
        parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->variable));
        auto response = GD::rpcServers.begin()->second->callMethod(_profileManagerClientInfo, "getValue", parameters);
        if (!response->errorStruct) profileAssociation->currentValue = response;
      }

      if (!profileAssociation->currentValue) profileAssociation->currentValue = std::make_shared<BaseLib::Variable>();

      variableProfile->variableCount++;

      if (*profileAssociation->currentValue != *profileAssociation->profileValue) {
        variableProfile->isActive = false;
        profileAssociation->isActive = false;
      } else variableProfile->activeVariables++;

      _profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable][variableProfile->id] = profileAssociation;
    }

    return std::make_shared<BaseLib::Variable>(id);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::deleteVariableProfile(uint64_t profileId) {
  try {
    GD::bl->db->deleteVariableProfile(profileId);

    std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
    auto variableProfileIterator = _variableProfiles.find(profileId);
    if (variableProfileIterator != _variableProfiles.end()) {
      auto variableProfile = variableProfileIterator->second;

      for (auto &valueEntry : variableProfile->values) {
        _profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable].erase(variableProfile->id);
        if (_profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable].empty()) {
          _profilesByVariable[valueEntry->peerId][valueEntry->channel].erase(valueEntry->variable);
          if (_profilesByVariable[valueEntry->peerId][valueEntry->channel].empty()) {
            _profilesByVariable[valueEntry->peerId].erase(valueEntry->channel);
            if (_profilesByVariable[valueEntry->peerId].empty()) {
              _profilesByVariable.erase(valueEntry->peerId);
            }
          }
        }
      }

      _variableProfiles.erase(profileId);
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::getAllVariableProfiles(const std::string &languageCode) {
  try {
    auto profiles = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

    std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
    profiles->arrayValue->reserve(_variableProfiles.size());
    for (auto &profile : _variableProfiles) {
      profile.second->profileStruct->structValue->erase("translations");
      profile.second->profileStruct->structValue->erase("name");
      if (languageCode.empty()) {
        profile.second->profileStruct->structValue->emplace("translations", profile.second->name);
      } else {
        std::string name;
        auto nameIterator = profile.second->name->structValue->find(languageCode);
        if (nameIterator == profile.second->name->structValue->end()) nameIterator = profile.second->name->structValue->find("en");
        if (nameIterator == profile.second->name->structValue->end()) nameIterator = profile.second->name->structValue->find("en-US");
        if (nameIterator == profile.second->name->structValue->end()) nameIterator = profile.second->name->structValue->begin();
        if (nameIterator != profile.second->name->structValue->end()) name = nameIterator->second->stringValue;
        profile.second->profileStruct->structValue->emplace("name", std::make_shared<BaseLib::Variable>(name));
      }

      profile.second->profileStruct->structValue->erase("id");
      profile.second->profileStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(profile.first));
      profile.second->profileStruct->structValue->erase("isActive");
      profile.second->profileStruct->structValue->emplace("isActive", std::make_shared<BaseLib::Variable>(profile.second->isActive));
      profile.second->profileStruct->structValue->erase("totalVariableCount");
      profile.second->profileStruct->structValue->emplace("totalVariableCount", std::make_shared<BaseLib::Variable>(profile.second->variableCount));
      profile.second->profileStruct->structValue->erase("activeVariableCount");
      profile.second->profileStruct->structValue->emplace("activeVariableCount", std::make_shared<BaseLib::Variable>(profile.second->activeVariables));
      profiles->arrayValue->emplace_back(profile.second->profileStruct);
    }

    return profiles;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::getVariableProfile(uint64_t id, const std::string &languageCode) {
  try {
    std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
    auto variableProfileIterator = _variableProfiles.find(id);
    if (variableProfileIterator == _variableProfiles.end()) return BaseLib::Variable::createError(-1, "Variable profile not found.");

    variableProfileIterator->second->profileStruct->structValue->erase("translations");
    variableProfileIterator->second->profileStruct->structValue->erase("name");
    if (languageCode.empty()) {
      variableProfileIterator->second->profileStruct->structValue->emplace("translations", variableProfileIterator->second->name);
    } else {
      std::string name;
      auto nameIterator = variableProfileIterator->second->name->structValue->find(languageCode);
      if (nameIterator == variableProfileIterator->second->name->structValue->end()) nameIterator = variableProfileIterator->second->name->structValue->find("en");
      if (nameIterator == variableProfileIterator->second->name->structValue->end()) nameIterator = variableProfileIterator->second->name->structValue->find("en-US");
      if (nameIterator == variableProfileIterator->second->name->structValue->end()) nameIterator = variableProfileIterator->second->name->structValue->begin();
      if (nameIterator != variableProfileIterator->second->name->structValue->end()) name = nameIterator->second->stringValue;
      variableProfileIterator->second->profileStruct->structValue->emplace("name", std::make_shared<BaseLib::Variable>(name));
    }

    variableProfileIterator->second->profileStruct->structValue->erase("id");
    variableProfileIterator->second->profileStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(variableProfileIterator->first));
    variableProfileIterator->second->profileStruct->structValue->erase("isActive");
    variableProfileIterator->second->profileStruct->structValue->emplace("isActive", std::make_shared<BaseLib::Variable>(variableProfileIterator->second->isActive));
    variableProfileIterator->second->profileStruct->structValue->erase("totalVariableCount");
    variableProfileIterator->second->profileStruct->structValue->emplace("totalVariableCount", std::make_shared<BaseLib::Variable>(variableProfileIterator->second->variableCount));
    variableProfileIterator->second->profileStruct->structValue->erase("activeVariableCount");
    variableProfileIterator->second->profileStruct->structValue->emplace("activeVariableCount", std::make_shared<BaseLib::Variable>(variableProfileIterator->second->activeVariables));

    return variableProfileIterator->second->profileStruct;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable VariableProfileManager::updateVariableProfile(uint64_t profileId, const BaseLib::PVariable &translations, const BaseLib::PVariable &profile) {
  try {
    if (translations->structValue->empty() && profile->structValue->empty()) {
      return BaseLib::Variable::createError(-1, "translations and profile are empty or not a valid Struct.");
    }

    auto newProfile = profile;

    {
      std::lock_guard<std::mutex> variableProfilesGuard(_variableProfilesMutex);
      auto variableProfileIterator = _variableProfiles.find(profileId);
      if (variableProfileIterator == _variableProfiles.end()) return BaseLib::Variable::createError(-1, "Variable profile not found.");
      auto variableProfile = variableProfileIterator->second;

      if (!translations->structValue->empty()) variableProfile->name = translations;
      if (newProfile->structValue->empty()) newProfile = variableProfile->profileStruct;

      //{{{ Remove old variable associations
      {
        for (auto &valueEntry : variableProfile->values) {
          _profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable].erase(variableProfile->id);
          if (_profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable].empty()) {
            _profilesByVariable[valueEntry->peerId][valueEntry->channel].erase(valueEntry->variable);
            if (_profilesByVariable[valueEntry->peerId][valueEntry->channel].empty()) {
              _profilesByVariable[valueEntry->peerId].erase(valueEntry->channel);
              if (_profilesByVariable[valueEntry->peerId].empty()) {
                _profilesByVariable.erase(valueEntry->peerId);
              }
            }
          }
        }
      }
      //}}}

      variableProfile->profileStruct = newProfile;
      variableProfile->values.clear();

      size_t valuesArraySize = 0;

      auto valuesIterator = variableProfile->profileStruct->structValue->find("values");
      if (valuesIterator != variableProfile->profileStruct->structValue->end()) {
        valuesArraySize = valuesIterator->second->arrayValue->size();
      }

      {
        auto roleVariables = getRoleVariables(variableProfile->id, variableProfile->profileStruct);
        if (!roleVariables.empty()) {
          variableProfile->values.reserve(roleVariables.size() + valuesArraySize);
          for (auto &roleVariable : roleVariables) {
            variableProfile->values.emplace_back(roleVariable);
          }
        }
      }

      //{{{ Add new variable associations
      {
        variableProfile->variableCount = 0;
        variableProfile->activeVariables = 0;
        auto valuesIterator = variableProfile->profileStruct->structValue->find("values");
        if (valuesIterator != variableProfile->profileStruct->structValue->end()) {
          for (auto &valueEntry : *valuesIterator->second->arrayValue) {
            auto roleIterator = valueEntry->structValue->find("role");
            auto peerIdIterator = valueEntry->structValue->find("peerId");
            auto channelIterator = valueEntry->structValue->find("channel");
            auto variableIterator = valueEntry->structValue->find("variable");
            auto valueIterator = valueEntry->structValue->find("value");
            auto waitIterator = valueEntry->structValue->find("wait");
            auto ignoreValueFromDeviceIterator = valueEntry->structValue->find("ignoreValueFromDevice");
            auto deviceRefractoryPeriodIterator = valueEntry->structValue->find("deviceRefractoryPeriod");

            //Ignore role entry
            if (roleIterator != valueEntry->structValue->end() && peerIdIterator == valueEntry->structValue->end()) continue;

            if (peerIdIterator == valueEntry->structValue->end() || channelIterator == valueEntry->structValue->end() || variableIterator == valueEntry->structValue->end() || valueIterator == valueEntry->structValue->end()) {
              GD::out.printWarning("Warning: Value entry of profile " + std::to_string(variableProfile->id) + " is missing required elements.");
              continue;
            }

            auto profileValue = std::make_shared<VariableProfileValue>();
            profileValue->peerId = peerIdIterator->second->integerValue64;
            profileValue->channel = channelIterator->second->integerValue;
            profileValue->variable = variableIterator->second->stringValue;
            profileValue->value = valueIterator->second;
            if (!profileValue->value) continue;
            profileValue->wait = (waitIterator != valueEntry->structValue->end() && waitIterator->second->booleanValue && profileValue->value->type == BaseLib::VariableType::tBoolean);
            if (ignoreValueFromDeviceIterator != valueEntry->structValue->end()) profileValue->ignoreValueFromDevice = ignoreValueFromDeviceIterator->second->booleanValue;
            if (deviceRefractoryPeriodIterator != valueEntry->structValue->end()) profileValue->deviceRefractoryPeriod = ignoreValueFromDeviceIterator->second->integerValue;
            variableProfile->values.emplace_back(std::move(profileValue));
          }
        }
      }
      //}}}

      variableProfile->isActive = !variableProfile->values.empty();
      for (auto &valueEntry : variableProfile->values) {
        auto profileAssociation = std::make_shared<VariableProfileAssociation>();
        profileAssociation->profileId = variableProfile->id;
        profileAssociation->profileValue = valueEntry->value;
        profileAssociation->ignoreValueFromDevice = valueEntry->ignoreValueFromDevice;
        profileAssociation->deviceRefractoryPeriod = valueEntry->deviceRefractoryPeriod;

        {
          auto parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          parameters->arrayValue->reserve(3);
          parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->peerId));
          parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->channel));
          parameters->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(valueEntry->variable));
          auto response = GD::rpcServers.begin()->second->callMethod(_profileManagerClientInfo, "getValue", parameters);
          if (!response->errorStruct) profileAssociation->currentValue = response;
        }

        if (!profileAssociation->currentValue) profileAssociation->currentValue = std::make_shared<BaseLib::Variable>();

        variableProfile->variableCount++;

        if (*profileAssociation->currentValue != *profileAssociation->profileValue) {
          variableProfile->isActive = false;
          profileAssociation->isActive = false;
        } else variableProfile->activeVariables++;

        _profilesByVariable[valueEntry->peerId][valueEntry->channel][valueEntry->variable][variableProfile->id] = profileAssociation;
      }
    }

    auto result = GD::bl->db->updateVariableProfile(profileId, translations, newProfile);

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}