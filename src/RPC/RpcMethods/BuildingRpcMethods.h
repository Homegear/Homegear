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

#ifndef HOMEGEAR_BUILDINGRPCMETHODS_H
#define HOMEGEAR_BUILDINGRPCMETHODS_H

#include <homegear-base/Variable.h>
#include <homegear-base/Encoding/RpcMethod.h>

namespace Homegear
{
namespace RpcMethods
{

class RPCAddStoryToBuilding : public BaseLib::Rpc::RpcMethod
{
public:
    RPCAddStoryToBuilding()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCCreateBuilding : public BaseLib::Rpc::RpcMethod
{
public:
    RPCCreateBuilding()
    {
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCDeleteBuilding : public BaseLib::Rpc::RpcMethod
{
public:
    RPCDeleteBuilding()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetStoriesInBuilding : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetStoriesInBuilding()
    {
        addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetBuildings : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetBuildings()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCGetBuildingMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCGetBuildingMetadata()
    {
        addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCRemoveStoryFromBuilding : public BaseLib::Rpc::RpcMethod
{
public:
    RPCRemoveStoryFromBuilding()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCSetBuildingMetadata : public BaseLib::Rpc::RpcMethod
{
public:
    RPCSetBuildingMetadata()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

class RPCUpdateBuilding : public BaseLib::Rpc::RpcMethod
{
public:
    RPCUpdateBuilding()
    {
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});
        addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct});
    }

    BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters);
};

}
}


#endif //HOMEGEAR_BUILDINGRPCMETHODS_H
