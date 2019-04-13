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

#include "Roles.h"
#include "../GD/GD.h"

namespace Homegear
{

namespace Rpc
{

BaseLib::PVariable Roles::aggregate(const BaseLib::PRpcClientInfo& clientInfo, RoleAggregationType aggregationType, const BaseLib::PVariable& aggregationParameters, const BaseLib::PArray& roles, uint64_t roomId)
{
    try
    {
        std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>> variablesInRoom;
        //{{{ Get variables in room
            if(roomId != 0)
            {
                if(!GD::bl->db->roomExists(roomId)) return BaseLib::Variable::createError(-1, "Unknown room.");

                BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(roomId));
                std::string methodName = "getVariablesInRoom";
                auto result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                if(result->errorStruct) return BaseLib::Variable::createError(-1, "Error getting variables in room: " + result->structValue->at("faultString")->stringValue);

                for(auto& peerStructElement : *result->structValue)
                {
                    for(auto& channelStructElement : *peerStructElement.second->structValue)
                    {
                        for(auto& variableElement : *channelStructElement.second->arrayValue)
                        {
                            uint64_t peerId = BaseLib::Math::getUnsignedNumber64(peerStructElement.first);
                            int32_t channel = BaseLib::Math::getNumber(channelStructElement.first);
                            std::string variable = variableElement->stringValue;

                            variablesInRoom[peerId][channel].emplace(variable);
                        }
                    }
                }
            }
        //}}}

        //{{{ Get variables in role filtered by room
            std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>> variables;
            for(auto& role : *roles)
            {
                if(!GD::bl->db->roleExists((uint64_t)role->integerValue64)) return BaseLib::Variable::createError(-1, "At least one role ID is unknown.");

                BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                requestParameters->arrayValue->push_back(role);
                std::string methodName = "getVariablesInRole";
                auto result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                if(result->errorStruct) continue;

                for(auto& peerStructElement : *result->structValue)
                {
                    for(auto& channelStructElement : *peerStructElement.second->structValue)
                    {
                        for(auto& variableElement : *channelStructElement.second->arrayValue)
                        {
                            uint64_t peerId = BaseLib::Math::getUnsignedNumber64(peerStructElement.first);
                            int32_t channel = BaseLib::Math::getNumber(channelStructElement.first);
                            std::string variable = variableElement->stringValue;

                            if(roomId != 0) //Check if role variable is in room
                            {
                                auto peerIterator = variablesInRoom.find(peerId);
                                if(peerIterator == variablesInRoom.end()) continue;
                                auto channelIterator = peerIterator->second.find(channel);
                                if(channelIterator == peerIterator->second.end()) continue;
                                auto variableIterator = channelIterator->second.find(variable);
                                if(variableIterator == channelIterator->second.end()) continue;
                            }

                            variables[peerId][channel].emplace(variable);
                        }
                    }
                }
            }
        //}}}

        if(aggregationType == RoleAggregationType::countTrue) return countTrue(clientInfo, variables);
        else if(aggregationType == RoleAggregationType::countDistinct) return countDistinct(clientInfo, variables);
        else if(aggregationType == RoleAggregationType::countMinimum) return countMinimum(clientInfo, variables);
        else if(aggregationType == RoleAggregationType::countMaximum) return countMaximum(clientInfo, variables);
        else if(aggregationType == RoleAggregationType::countBelowThreshold) return countBelowThreshold(clientInfo, variables, aggregationParameters);
        else if(aggregationType == RoleAggregationType::countAboveThreshold) return countAboveThreshold(clientInfo, variables, aggregationParameters);
        else return BaseLib::Variable::createError(-1, "Unknown aggregation type.");
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Roles::countTrue(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>& variables)
{
    try
    {
        uint64_t variableCount = 0;
        uint64_t trueCount = 0;

        for(auto& peerIterator : variables)
        {
            for(auto& channelIterator : peerIterator.second)
            {
                for(auto& variable : channelIterator.second)
                {
                    BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    requestParameters->arrayValue->reserve(3);
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                    std::string methodName = "getValue";
                    auto result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(result->errorStruct) continue;
                    variableCount++;
                    bool isTrue = (bool)(*result);
                    if(isTrue) trueCount++;
                }
            }
        }

        auto response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        response->structValue->emplace("variableCount", std::make_shared<BaseLib::Variable>(variableCount));
        response->structValue->emplace("trueCount", std::make_shared<BaseLib::Variable>(trueCount));
        response->structValue->emplace("falseCount", std::make_shared<BaseLib::Variable>(variableCount - trueCount));
        return response;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Roles::countDistinct(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>& variables)
{
    try
    {
        uint64_t variableCount = 0;
        std::unordered_map<uint64_t, uint64_t> counts;

        for(auto& peerIterator : variables)
        {
            for(auto& channelIterator : peerIterator.second)
            {
                for(auto& variable : channelIterator.second)
                {
                    BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    requestParameters->arrayValue->reserve(3);
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                    std::string methodName = "getValue";
                    auto result = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(result->errorStruct) continue;
                    variableCount++;
                    counts[result->integerValue64]++;
                }
            }
        }

        auto response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        response->structValue->emplace("variableCount", std::make_shared<BaseLib::Variable>(variableCount));
        for(auto element : counts)
        {
            response->structValue->emplace(std::to_string(element.first), std::make_shared<BaseLib::Variable>(element.second));
        }
        return response;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Roles::countMinimum(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>& variables)
{
    try
    {
        uint64_t variableCount = 0;
        uint64_t minCount = 0;

        for(auto& peerIterator : variables)
        {
            for(auto& channelIterator : peerIterator.second)
            {
                for(auto& variable : channelIterator.second)
                {
                    BaseLib::PVariable value;

                    {
                        BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                        requestParameters->arrayValue->reserve(3);
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                        std::string methodName = "getValue";
                        value = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                        if(value->errorStruct) continue;
                    }

                    BaseLib::PVariable description;

                    if(value->type == BaseLib::VariableType::tInteger || value->type == BaseLib::VariableType::tInteger64 || value->type == BaseLib::VariableType::tFloat)
                    {
                        BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                        requestParameters->arrayValue->reserve(3);
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                        std::string methodName = "getVariableDescription";
                        description = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                        if(description->errorStruct) continue;
                    }

                    bool isMin = false;
                    if(value->type == BaseLib::VariableType::tBoolean)
                    {
                        isMin = (value->booleanValue == false);
                    }
                    else if(value->type == BaseLib::VariableType::tInteger || value->type == BaseLib::VariableType::tInteger64)
                    {
                        isMin = (value->integerValue64 == description->structValue->at("MIN")->integerValue64);
                    }
                    else if(value->type == BaseLib::VariableType::tFloat)
                    {
                        isMin = (value->floatValue == description->structValue->at("MIN")->floatValue);
                    }
                    else continue;

                    variableCount++;
                    if(isMin) minCount++;
                }
            }
        }

        auto response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        response->structValue->emplace("variableCount", std::make_shared<BaseLib::Variable>(variableCount));
        response->structValue->emplace("minimumCount", std::make_shared<BaseLib::Variable>(minCount));
        response->structValue->emplace("aboveMinimumCount", std::make_shared<BaseLib::Variable>(variableCount - minCount));
        return response;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Roles::countMaximum(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>& variables)
{
    try
    {
        uint64_t variableCount = 0;
        uint64_t maxCount = 0;

        for(auto& peerIterator : variables)
        {
            for(auto& channelIterator : peerIterator.second)
            {
                for(auto& variable : channelIterator.second)
                {
                    BaseLib::PVariable value;

                    {
                        BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                        requestParameters->arrayValue->reserve(3);
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                        std::string methodName = "getValue";
                        value = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                        if(value->errorStruct) continue;
                    }

                    BaseLib::PVariable description;

                    if(value->type == BaseLib::VariableType::tInteger || value->type == BaseLib::VariableType::tInteger64 || value->type == BaseLib::VariableType::tFloat)
                    {
                        BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                        requestParameters->arrayValue->reserve(3);
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                        requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                        std::string methodName = "getVariableDescription";
                        description = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                        if(description->errorStruct) continue;
                    }

                    bool isMax = false;
                    if(value->type == BaseLib::VariableType::tBoolean)
                    {
                        isMax = !value->booleanValue;
                    }
                    else if(value->type == BaseLib::VariableType::tInteger || value->type == BaseLib::VariableType::tInteger64)
                    {
                        isMax = (value->integerValue64 == description->structValue->at("MAX")->integerValue64);
                    }
                    else if(value->type == BaseLib::VariableType::tFloat)
                    {
                        isMax = (value->floatValue == description->structValue->at("MAX")->floatValue);
                    }
                    else continue;

                    variableCount++;
                    if(isMax) maxCount++;
                }
            }
        }

        auto response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        response->structValue->emplace("variableCount", std::make_shared<BaseLib::Variable>(variableCount));
        response->structValue->emplace("maximumCount", std::make_shared<BaseLib::Variable>(maxCount));
        response->structValue->emplace("belowMaximumCount", std::make_shared<BaseLib::Variable>(variableCount - maxCount));
        return response;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Roles::countBelowThreshold(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>& variables, const BaseLib::PVariable& aggregationParameters)
{
    try
    {
        BaseLib::PVariable threshold;

        {
            auto thresholdIterator = aggregationParameters->structValue->find("threshold");
            if(thresholdIterator == aggregationParameters->structValue->end()) return BaseLib::Variable::createError(-1, R"("threshold" not set in aggregationParameters.)");
            threshold = thresholdIterator->second;
        }

        uint64_t variableCount = 0;
        uint64_t belowThresholdCount = 0;

        for(auto& peerIterator : variables)
        {
            for(auto& channelIterator : peerIterator.second)
            {
                for(auto& variable : channelIterator.second)
                {

                    BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    requestParameters->arrayValue->reserve(3);
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                    std::string methodName = "getValue";
                    auto value = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(value->errorStruct) continue;

                    bool isBelowThreshold = false;
                    if(value->type == BaseLib::VariableType::tBoolean)
                    {
                        if(threshold->type == BaseLib::VariableType::tInteger || threshold->type == BaseLib::VariableType::tInteger64)
                        {
                            isBelowThreshold = ((int64_t)value->booleanValue < threshold->integerValue64);
                        }
                        else if(threshold->type == BaseLib::VariableType::tFloat)
                        {
                            isBelowThreshold = ((double)value->booleanValue < threshold->floatValue);
                        }
                    }
                    else if(value->type == BaseLib::VariableType::tInteger || value->type == BaseLib::VariableType::tInteger64)
                    {
                        if(threshold->type == BaseLib::VariableType::tInteger || threshold->type == BaseLib::VariableType::tInteger64)
                        {
                            isBelowThreshold = (value->integerValue64 < threshold->integerValue64);
                        }
                        else if(threshold->type == BaseLib::VariableType::tFloat)
                        {
                            isBelowThreshold = ((double)value->integerValue64 < threshold->floatValue);
                        }
                    }
                    else if(value->type == BaseLib::VariableType::tFloat)
                    {
                        if(threshold->type == BaseLib::VariableType::tInteger || threshold->type == BaseLib::VariableType::tInteger64)
                        {
                            isBelowThreshold = ((int64_t)value->floatValue < threshold->integerValue64);
                        }
                        else if(threshold->type == BaseLib::VariableType::tFloat)
                        {
                            isBelowThreshold = (value->floatValue < threshold->floatValue);
                        }
                    }
                    else continue;

                    variableCount++;
                    if(isBelowThreshold) belowThresholdCount++;
                }
            }
        }

        auto response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        response->structValue->emplace("variableCount", std::make_shared<BaseLib::Variable>(variableCount));
        response->structValue->emplace("belowThresholdCount", std::make_shared<BaseLib::Variable>(belowThresholdCount));
        response->structValue->emplace("aboveOrEqualThresholdCount", std::make_shared<BaseLib::Variable>(variableCount - belowThresholdCount));
        return response;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable Roles::countAboveThreshold(const BaseLib::PRpcClientInfo& clientInfo, const std::unordered_map<uint64_t, std::unordered_map<int32_t, std::set<std::string>>>& variables, const BaseLib::PVariable& aggregationParameters)
{
    try
    {
        BaseLib::PVariable threshold;

        {
            auto thresholdIterator = aggregationParameters->structValue->find("threshold");
            if(thresholdIterator == aggregationParameters->structValue->end()) return BaseLib::Variable::createError(-1, R"("threshold" not set in aggregationParameters.)");
            threshold = thresholdIterator->second;
        }

        uint64_t variableCount = 0;
        uint64_t aboveThresholdCount = 0;

        for(auto& peerIterator : variables)
        {
            for(auto& channelIterator : peerIterator.second)
            {
                for(auto& variable : channelIterator.second)
                {

                    BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    requestParameters->arrayValue->reserve(3);
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(peerIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(channelIterator.first));
                    requestParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(variable));
                    std::string methodName = "getValue";
                    auto value = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
                    if(value->errorStruct) continue;

                    bool isAboveThreshold = false;
                    if(value->type == BaseLib::VariableType::tBoolean)
                    {
                        if(threshold->type == BaseLib::VariableType::tInteger || threshold->type == BaseLib::VariableType::tInteger64)
                        {
                            isAboveThreshold = ((int64_t)value->booleanValue > threshold->integerValue64);
                        }
                        else if(threshold->type == BaseLib::VariableType::tFloat)
                        {
                            isAboveThreshold = ((double)value->booleanValue > threshold->floatValue);
                        }
                    }
                    else if(value->type == BaseLib::VariableType::tInteger || value->type == BaseLib::VariableType::tInteger64)
                    {
                        if(threshold->type == BaseLib::VariableType::tInteger || threshold->type == BaseLib::VariableType::tInteger64)
                        {
                            isAboveThreshold = (value->integerValue64 > threshold->integerValue64);
                        }
                        else if(threshold->type == BaseLib::VariableType::tFloat)
                        {
                            isAboveThreshold = ((double)value->integerValue64 > threshold->floatValue);
                        }
                    }
                    else if(value->type == BaseLib::VariableType::tFloat)
                    {
                        if(threshold->type == BaseLib::VariableType::tInteger || threshold->type == BaseLib::VariableType::tInteger64)
                        {
                            isAboveThreshold = ((int64_t)value->floatValue > threshold->integerValue64);
                        }
                        else if(threshold->type == BaseLib::VariableType::tFloat)
                        {
                            isAboveThreshold = (value->floatValue > threshold->floatValue);
                        }
                    }
                    else continue;

                    variableCount++;
                    if(isAboveThreshold) aboveThresholdCount++;
                }
            }
        }

        auto response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        response->structValue->emplace("variableCount", std::make_shared<BaseLib::Variable>(variableCount));
        response->structValue->emplace("aboveThresholdCount", std::make_shared<BaseLib::Variable>(aboveThresholdCount));
        response->structValue->emplace("belowOrEqualThresholdCount", std::make_shared<BaseLib::Variable>(variableCount - aboveThresholdCount));
        return response;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}

}