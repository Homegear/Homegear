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

#ifndef HOMEGEAR_SYSTEMVARIABLECONTROLLER_H
#define HOMEGEAR_SYSTEMVARIABLECONTROLLER_H

#include <homegear-base/BaseLib.h>

namespace Homegear
{

class SystemVariableController
{
private:
    std::mutex _systemVariableMutex;
    std::unordered_map<std::string, BaseLib::Database::PSystemVariable> _systemVariables;
    std::set<std::string> _specialSystemVariables;

    std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
    std::unique_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;
public:
    SystemVariableController();

    BaseLib::PVariable erase(std::string& variableId);

    BaseLib::PVariable getCategories(std::string& variableId);

    std::set<uint64_t> getCategoriesInternal(std::string& variableId);

    BaseLib::PVariable getRoles(std::string& variableId);

    BaseLib::PVariable getRolesInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, bool checkAcls);

    std::unordered_map<uint64_t, BaseLib::Role> getRolesInternal(std::string& variableId);

    BaseLib::PVariable getRoom(std::string& variableId);

    BaseLib::PVariable getValue(BaseLib::PRpcClientInfo clientInfo, std::string& variableId, bool checkAcls);

    BaseLib::Database::PSystemVariable getInternal(const std::string& variableId);

    BaseLib::PVariable getVariableDescription(BaseLib::PRpcClientInfo clientInfo, std::string name, const std::unordered_set<std::string>& fields, bool checkAcls);

    BaseLib::PVariable getVariablesInCategory(BaseLib::PRpcClientInfo clientInfo, uint64_t categoryId, bool checkAcls);

    BaseLib::PVariable getVariablesInRole(BaseLib::PRpcClientInfo clientInfo, uint64_t roleId, bool checkAcls);

    BaseLib::PVariable getVariablesInRoom(BaseLib::PRpcClientInfo clientInfo, uint64_t roomId, bool checkAcls);

    uint64_t getRoomInternal(std::string& variableId);

    BaseLib::PVariable getAll(BaseLib::PRpcClientInfo clientInfo, bool returnRoomsCategoriesRolesFlags, bool checkAcls);

    bool hasCategory(std::string& variableId, uint64_t categoryId);

    bool hasRole(std::string& variableId, uint64_t roleId);

    void removeCategory(uint64_t categoryId);

    void removeRole(uint64_t categoryId);

    void removeRoom(uint64_t roomId);

    BaseLib::PVariable setCategories(std::string& variableId, std::set<uint64_t>& categories);

    BaseLib::PVariable setRoles(std::string& variableId, std::unordered_map<uint64_t, BaseLib::Role>& roles);

    BaseLib::PVariable setRoom(std::string& variableId, uint64_t room);

    BaseLib::PVariable setValue(BaseLib::PRpcClientInfo clientInfo, std::string& variableId, BaseLib::PVariable& value, int32_t flags, bool checkAcls);
};

}
#endif //HOMEGEAR_SYSTEMVARIABLECONTROLLER_H
