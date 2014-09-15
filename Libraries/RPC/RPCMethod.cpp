/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "RPCMethod.h"
#include "../GD/GD.h"

namespace RPC
{
std::shared_ptr<BaseLib::RPC::Variable> RPCMethod::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable());
}

RPCMethod::ParameterError::Enum RPCMethod::checkParameters(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters, std::vector<BaseLib::RPC::VariableType> types)
{
	if(types.size() != parameters->size())
	{
		return RPCMethod::ParameterError::Enum::wrongCount;
	}
	for(uint32_t i = 0; i < types.size(); i++)
	{
		if(types.at(i) == BaseLib::RPC::VariableType::rpcVariant && parameters->at(i)->type != BaseLib::RPC::VariableType::rpcVoid) continue;
		if(types.at(i) != parameters->at(i)->type) return RPCMethod::ParameterError::Enum::wrongType;
	}
	return RPCMethod::ParameterError::Enum::noError;
}

RPCMethod::ParameterError::Enum RPCMethod::checkParameters(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters, std::vector<std::vector<BaseLib::RPC::VariableType>> types)
{
	RPCMethod::ParameterError::Enum error = RPCMethod::ParameterError::Enum::wrongCount;
	for(std::vector<std::vector<BaseLib::RPC::VariableType>>::iterator i = types.begin(); i != types.end(); ++i)
	{
		RPCMethod::ParameterError::Enum result = checkParameters(parameters, *i);
		if(result == RPCMethod::ParameterError::Enum::noError) return result;
		if(result != RPCMethod::ParameterError::Enum::wrongCount) error = result; //Priority of type error is higher than wrong count
	}
	return error;
}

std::shared_ptr<BaseLib::RPC::Variable> RPCMethod::getError(RPCMethod::ParameterError::Enum error)
{
	if(error == ParameterError::Enum::wrongCount) return BaseLib::RPC::Variable::createError(-1, "Wrong parameter count.");
	else if(error == ParameterError::Enum::wrongType) return BaseLib::RPC::Variable::createError(-1, "Type error.");
	return BaseLib::RPC::Variable::createError(-1, "Unknown parameter error.");
}

void RPCMethod::setHelp(std::string help)
{
	try
	{
		_help.reset(new BaseLib::RPC::Variable(help));
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RPCMethod::addSignature(BaseLib::RPC::VariableType returnType, std::vector<BaseLib::RPC::VariableType> parameterTypes)
{
	try
	{
		if(!_signatures) _signatures.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));

		std::shared_ptr<BaseLib::RPC::Variable> element(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));

		element->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::Variable::getTypeString(returnType))));

		for(std::vector<BaseLib::RPC::VariableType>::iterator i = parameterTypes.begin(); i != parameterTypes.end(); ++i)
		{
			element->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::Variable::getTypeString(*i))));
		}
		_signatures->arrayValue->push_back(element);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

} /* namespace RPC */
