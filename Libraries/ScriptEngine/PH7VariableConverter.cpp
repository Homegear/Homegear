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

#include "PH7VariableConverter.h"
#include "../GD/GD.h"

int32_t arrayWalkCallback(ph7_value* key, ph7_value* value, void* userData)
{
	if(!userData) return PH7_ABORT;
	BaseLib::RPC::RPCVariable* rpcArray = (BaseLib::RPC::RPCVariable*)userData;

	std::shared_ptr<BaseLib::RPC::RPCVariable> element = PH7VariableConverter::getRPCVariable(value);
	if(ph7_value_is_int(key))
	{
		int32_t keyInt = ph7_value_to_int(key);
		if(rpcArray->type == BaseLib::RPC::RPCVariableType::rpcVoid) rpcArray->type = BaseLib::RPC::RPCVariableType::rpcArray;
		//In case keyInt skips values
		while(rpcArray->arrayValue->size() < keyInt) rpcArray->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable()));
		if(keyInt < rpcArray->arrayValue->size()) rpcArray->arrayValue->at(keyInt) = element;
		else rpcArray->arrayValue->push_back(element);
	}
	else if(ph7_value_is_string(key))
	{
		rpcArray->type = BaseLib::RPC::RPCVariableType::rpcStruct; //Always set to struct, when key is of type String
		int32_t length = 0;
		const char* string = ph7_value_to_string(key, &length);
		std::string keyString(string, string + length);
		if(!keyString.empty()) rpcArray->structValue->insert(BaseLib::RPC::RPCStructElement(keyString, element));
	}
	else
	{
		GD::out.printWarning("Warning: Script engine: Key of array is not of type String of Integer.");
		return PH7_ABORT;
	}

	if(!element) return PH7_ABORT;

	return PH7_OK;
}

PH7VariableConverter::PH7VariableConverter()
{
}

PH7VariableConverter::~PH7VariableConverter()
{
}

std::shared_ptr<BaseLib::RPC::RPCVariable> PH7VariableConverter::getRPCVariable(ph7_value* value)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::RPCVariable> variable;
		if(!value) return variable;
		if(ph7_value_is_int(value))
		{
			variable.reset(new BaseLib::RPC::RPCVariable((int32_t)ph7_value_to_int(value)));
		}
		else if(ph7_value_is_float(value))
		{
			variable.reset(new BaseLib::RPC::RPCVariable(ph7_value_to_double(value)));
		}
		else if(ph7_value_is_bool(value))
		{
			variable.reset(new BaseLib::RPC::RPCVariable((bool)ph7_value_to_bool(value)));
		}
		else if(ph7_value_is_string(value))
		{
			int32_t length = 0;
			const char* string = ph7_value_to_string(value, &length);
			if(length > 0) variable.reset(new BaseLib::RPC::RPCVariable(std::string(string, string + length)));
			else variable.reset(new BaseLib::RPC::RPCVariable(std::string("")));
		}
		else if(ph7_value_is_array(value))
		{
			variable.reset(new BaseLib::RPC::RPCVariable());
			if(ph7_array_walk(value, arrayWalkCallback, variable.get()) != PH7_OK)
			{
				GD::out.printWarning("Warning: Script engine: Could not convert array to RPC variable.");
			}
		}
		else
		{
			variable.reset(new BaseLib::RPC::RPCVariable(std::string("")));
		}

		return variable;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return std::shared_ptr<BaseLib::RPC::RPCVariable>();
}

ph7_value* PH7VariableConverter::getPH7Variable(ph7_context* context, std::shared_ptr<BaseLib::RPC::RPCVariable> value)
{
	try
	{
		if(!value) return nullptr;

		if(value->type == BaseLib::RPC::RPCVariableType::rpcArray)
		{
			ph7_value* pArray = ph7_context_new_array(context);
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = value->arrayValue->begin(); i != value->arrayValue->end(); ++i)
			{
				ph7_value* pValue = getPH7Variable(context, *i);
				if(pValue) ph7_array_add_elem(pArray, 0, pValue);
			}
			return pArray;
		}
		else if(value->type == BaseLib::RPC::RPCVariableType::rpcStruct)
		{
			ph7_value* pStruct = ph7_context_new_array(context);
			for(std::map<std::string, std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = value->structValue->begin(); i != value->structValue->end(); ++i)
			{
				ph7_value* pKey = ph7_context_new_scalar(context);
				ph7_value_string(pKey, i->first.c_str(), -1);
				ph7_value* pValue = getPH7Variable(context, i->second);
				if(pValue) ph7_array_add_elem(pStruct, pKey, pValue);
			}
			return pStruct;
		}

		ph7_value* pValue = ph7_context_new_scalar(context);
		if(value->type == BaseLib::RPC::RPCVariableType::rpcVoid)
		{
			ph7_value_null(pValue);
		}
		else if(value->type == BaseLib::RPC::RPCVariableType::rpcBoolean)
		{
			ph7_value_bool(pValue, value->booleanValue);
		}
		else if(value->type == BaseLib::RPC::RPCVariableType::rpcInteger)
		{
			ph7_value_int(pValue, value->integerValue);
		}
		else if(value->type == BaseLib::RPC::RPCVariableType::rpcFloat)
		{
			ph7_value_double(pValue, value->floatValue);
		}
		else if(value->type == BaseLib::RPC::RPCVariableType::rpcString || value->type == BaseLib::RPC::RPCVariableType::rpcBase64)
		{
			ph7_value_string(pValue, value->stringValue.c_str(), -1);
		}
		else
		{
			ph7_value_string(pValue, "UNKNOWN", 7);
		}

		return pValue;
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return nullptr;
}
