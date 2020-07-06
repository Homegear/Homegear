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

#ifndef HOMEGEAR_ROLES_H
#define HOMEGEAR_ROLES_H

#include <homegear-base/BaseLib.h>

namespace Homegear
{

namespace Rpc
{

enum class RoleAggregationType
{
    countTrue = 1,
    countDistinct = 2,
    countMinimum = 3,
    countMaximum = 4,
    countBelowThreshold = 5,
    countAboveThreshold = 6
};

struct RoleVariableInfo
{
    RoleVariableInfo() = default;
    RoleVariableInfo(const std::string& name) : name(name) {}

    std::string name;
};

class Roles
{
public:
    static BaseLib::PVariable aggregate(const BaseLib::PRpcClientInfo& clientInfo, RoleAggregationType aggregationType, const BaseLib::PVariable& aggregationParameters, const BaseLib::PArray& roles, uint64_t roomId);

private:
    Roles() = default;

    static BaseLib::PVariable countTrue(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::list<RoleVariableInfo>>>& variables);
    static BaseLib::PVariable countDistinct(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::list<RoleVariableInfo>>>& variables);
    static BaseLib::PVariable countMinimum(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::list<RoleVariableInfo>>>& variables);
    static BaseLib::PVariable countMaximum(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::list<RoleVariableInfo>>>& variables);
    static BaseLib::PVariable countBelowThreshold(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::list<RoleVariableInfo>>>& variables, const BaseLib::PVariable& aggregationParameters);
    static BaseLib::PVariable countAboveThreshold(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::list<RoleVariableInfo>>>& variables, const BaseLib::PVariable& aggregationParameters);
};

}

}

#endif //HOMEGEAR_ROLES_H
