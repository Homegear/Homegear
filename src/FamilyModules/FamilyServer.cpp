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

#include "FamilyServer.h"
#include "../GD/GD.h"

namespace Homegear
{

FamilyServer::FamilyServer()
{
    _out.init(GD::bl.get());
    _out.setPrefix("Family Server: ");

    _rpcMethods.emplace(std::pair<std::string, std::function<BaseLib::PVariable(BaseLib::PRpcClientInfo& clientInfo, BaseLib::PArray& parameters)>>("rpcTest", std::bind(&FamilyServer::rpcTest, this, std::placeholders::_1, std::placeholders::_2)));
}

bool FamilyServer::methodExists(BaseLib::PRpcClientInfo clientInfo, std::string& methodName)
{
    try
    {
        if(!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return false;

        auto methodIterator = _rpcMethods.find(methodName);
        return methodIterator != _rpcMethods.end();
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return false;
}

BaseLib::PVariable FamilyServer::callRpcMethod(BaseLib::PRpcClientInfo clientInfo, std::string& methodName, BaseLib::PArray& parameters)
{
    try
    {
        if(!clientInfo || !clientInfo->acls->checkMethodAccess(methodName)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

        auto methodIterator = _rpcMethods.find(methodName);
        if(methodIterator == _rpcMethods.end())
        {
            _out.printError("Warning: RPC method not found: " + methodName);
            return BaseLib::Variable::createError(-32601, "Requested method not found.");
        }

        return methodIterator->second(clientInfo, parameters);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

// {{{ RPC methods
BaseLib::PVariable FamilyServer::rpcTest(BaseLib::PRpcClientInfo& clientInfo, BaseLib::PArray& parameters)
{
    try
    {
        return std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::getTime());
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}
