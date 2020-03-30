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

namespace Homegear
{

struct VariableProfile
{
    uint64_t id;
    BaseLib::PVariable name;
    BaseLib::PVariable profile;
};
typedef std::shared_ptr<VariableProfile> PVariableProfile;

class VariableProfileManager
{
private:
    std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;

    std::mutex _variableProfilesMutex;
    std::unordered_map<uint64_t, PVariableProfile> _variableProfiles;
public:
    VariableProfileManager();

    void load();
    BaseLib::PVariable addVariableProfile(const BaseLib::PVariable& translations, const BaseLib::PVariable& profile);
    BaseLib::PVariable deleteVariableProfile(uint64_t profileId);
    BaseLib::PVariable executeVariableProfile(const BaseLib::PRpcClientInfo& clientInfo, uint64_t profileId);
    BaseLib::PVariable getAllVariableProfiles(const std::string& languageCode);
    BaseLib::PVariable getVariableProfile(uint64_t id, const std::string& languageCode);
    BaseLib::PVariable updateVariableProfile(uint64_t profileId, const BaseLib::PVariable& translations, const BaseLib::PVariable& profile);
};

}

#endif //HOMEGEAR_VARIABLEPROFILEMANAGER_H
