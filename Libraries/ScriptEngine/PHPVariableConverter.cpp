/* Copyright 2013-2015 Sathya Laufer
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

#ifdef SCRIPTENGINE
#include "PHPVariableConverter.h"
#include "../GD/GD.h"

PHPVariableConverter::PHPVariableConverter()
{
}

PHPVariableConverter::~PHPVariableConverter()
{
}

std::shared_ptr<BaseLib::RPC::Variable> PHPVariableConverter::getVariable(zval* value)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::Variable> variable;
		if(!value) return variable;
		if(Z_TYPE_P(value) == IS_LONG)
		{
			variable.reset(new BaseLib::RPC::Variable((int32_t)Z_LVAL_P(value)));
		}
		else if(Z_TYPE_P(value) == IS_DOUBLE)
		{
			variable.reset(new BaseLib::RPC::Variable((double)Z_DVAL_P(value)));
		}
		else if(Z_TYPE_P(value) == IS_BOOL)
		{
			variable.reset(new BaseLib::RPC::Variable((bool)Z_BVAL_P(value)));
		}
		else if(Z_TYPE_P(value) == IS_STRING)
		{
			if(Z_STRLEN_P(value) > 0) variable.reset(new BaseLib::RPC::Variable(std::string(Z_STRVAL_P(value), Z_STRLEN_P(value))));
			else variable.reset(new BaseLib::RPC::Variable(std::string("")));
		}
		else if(Z_TYPE_P(value) == IS_ARRAY)
		{
			zval** element = nullptr;
			HashPosition* pos = nullptr;
			HashTable* ht = Z_ARRVAL_P(value);
			char* key = nullptr;
			uint32_t keyLength = 0;
			unsigned long keyIndex = 0;
			for(zend_hash_internal_pointer_reset_ex(ht, pos); zend_hash_has_more_elements_ex(ht, pos) == SUCCESS; zend_hash_move_forward_ex(ht, pos))
			{
				int32_t type = zend_hash_get_current_key_ex(ht, &key, &keyLength, &keyIndex, 0, pos);
				if(!variable)
				{
					if(type == HASH_KEY_IS_STRING) variable.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
					else variable.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
				}
				if (zend_hash_get_current_data_ex(ht, (void**)&element, pos) == FAILURE) continue; //Should never fail
				std::shared_ptr<BaseLib::RPC::Variable> arrayElement = getVariable(*element);
				if(!arrayElement) continue;
				if(variable->type == BaseLib::RPC::VariableType::rpcStruct)
				{
					std::string keyName;
					if(keyLength > 0) keyName = std::string(key, keyLength - 1); //keyLength includes the trailing '\0'
					variable->structValue->insert(BaseLib::RPC::RPCStructElement(keyName, arrayElement));
				}
				else variable->arrayValue->push_back(arrayElement);
			}
		}
		else
		{
			variable.reset(new BaseLib::RPC::Variable(std::string("")));
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
	return std::shared_ptr<BaseLib::RPC::Variable>();
}

void PHPVariableConverter::getPHPVariable(std::shared_ptr<BaseLib::RPC::Variable> input, zval* output)
{
	try
	{
		if(!input || !output) return;

		if(input->type == BaseLib::RPC::VariableType::rpcArray)
		{
			array_init(output);
			for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = input->arrayValue->begin(); i != input->arrayValue->end(); ++i)
			{
				zval* element;
				MAKE_STD_ZVAL(element);
				getPHPVariable(*i, element);
				add_next_index_zval(output, element);
			}
			return;
		}
		else if(input->type == BaseLib::RPC::VariableType::rpcStruct)
		{
			array_init(output);
			for(std::map<std::string, std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = input->structValue->begin(); i != input->structValue->end(); ++i)
			{
				zval* element;
				MAKE_STD_ZVAL(element);
				getPHPVariable(i->second, element);
				add_assoc_zval_ex(output, i->first.c_str(), i->first.size() + 1, element);
			}
			return;
		}

		if(input->type == BaseLib::RPC::VariableType::rpcVoid)
		{
			ZVAL_NULL(output);
		}
		else if(input->type == BaseLib::RPC::VariableType::rpcBoolean)
		{
			ZVAL_BOOL(output, input->booleanValue);
		}
		else if(input->type == BaseLib::RPC::VariableType::rpcInteger)
		{
			ZVAL_LONG(output, input->integerValue);
		}
		else if(input->type == BaseLib::RPC::VariableType::rpcFloat)
		{
			ZVAL_DOUBLE(output, input->floatValue);
		}
		else if(input->type == BaseLib::RPC::VariableType::rpcString || input->type == BaseLib::RPC::VariableType::rpcBase64)
		{
			ZVAL_STRINGL(output, input->stringValue.c_str(), input->stringValue.size(), true);
		}
		else
		{
			ZVAL_STRINGL(output, "UNKNOWN", 7, true);
		}
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
}
#endif
