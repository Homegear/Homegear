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

#ifndef HOMEGEAR_VARIABLEPROFILEMANAGER_H
#define HOMEGEAR_VARIABLEPROFILEMANAGER_H

#include <homegear-base/Variable.h>
#include <homegear-base/Encoding/RpcDecoder.h>
#include <homegear-base/Sockets/RpcClientInfo.h>
#include <mutex>

namespace Homegear {

struct VariableProfileValue {
  uint64_t peerId = 0;
  int32_t channel = -1;
  std::string variable;
  BaseLib::PVariable value;
  bool wait = false;
  bool ignoreValueFromDevice = true;
  uint32_t deviceRefractoryPeriod = 60;
};
typedef std::shared_ptr<VariableProfileValue> PVariableProfileValue;

struct VariableProfile {
  uint64_t id;
  BaseLib::PVariable name;
  int64_t lastActivation = 0;
  std::vector<PVariableProfileValue> values;
  BaseLib::PVariable profileStruct;
  bool isActive = false;
  uint32_t activeVariables = 0;
  uint32_t variableCount = 0;
};
typedef std::shared_ptr<VariableProfile> PVariableProfile;

struct VariableProfileAssociation {
  bool isActive = true;
  uint64_t profileId = 0;
  bool ignoreValueFromDevice = true;
  uint32_t deviceRefractoryPeriod = 60;
  BaseLib::PVariable profileValue;
  BaseLib::PVariable currentValue;
};
typedef std::shared_ptr<VariableProfileAssociation> PVariableProfileAssociation;

class VariableProfileManager {
 private:
  std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;

  BaseLib::PRpcClientInfo _profileManagerClientInfo;

  std::mutex _variableProfilesMutex;
  std::unordered_map<uint64_t, PVariableProfile> _variableProfiles;

  /**
   * Peer ID, channel, variable, profile ID
   */
  std::unordered_map<uint64_t, std::unordered_map<int32_t, std::unordered_map<std::string, std::unordered_map<uint64_t, PVariableProfileAssociation>>>> _profilesByVariable;

  bool setValue(const BaseLib::PRpcClientInfo &clientInfo, uint64_t profileId, uint64_t peerId, int32_t channel, const std::string &variable, const BaseLib::PVariable &value, bool wait);
  std::list<PVariableProfileValue> getRoleVariables(uint64_t profileId, const BaseLib::PVariable &profile);
 public:
  VariableProfileManager();

  void variableEvent(const std::string &source, uint64_t id, int32_t channel, const std::shared_ptr<std::vector<std::string>> &variables, const BaseLib::PArray &values);

  void load();
  BaseLib::PVariable activateVariableProfile(const BaseLib::PRpcClientInfo &clientInfo, uint64_t profileId);
  BaseLib::PVariable addVariableProfile(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PVariable &translations, const BaseLib::PVariable &profile);
  BaseLib::PVariable deleteVariableProfile(uint64_t profileId);
  BaseLib::PVariable getAllVariableProfiles(const std::string &languageCode);
  BaseLib::PVariable getVariableProfile(uint64_t id, const std::string &languageCode);
  BaseLib::PVariable updateVariableProfile(uint64_t profileId, const BaseLib::PVariable &translations, const BaseLib::PVariable &profile);
};

}

#endif //HOMEGEAR_VARIABLEPROFILEMANAGER_H
