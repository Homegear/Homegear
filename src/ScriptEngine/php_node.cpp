/* Copyright 2013-2017 Sathya Laufer
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

#include "php_config_fixes.h"
#include "../GD/GD.h"
#include "php_node.h"
#include "PhpVariableConverter.h"

#define SEG(v) php_homegear_get_globals()->v

void php_homegear_node_invoke_rpc(std::string& methodName, BaseLib::PVariable& parameters, zval* return_value, bool wait)
{
	if(SEG(id) == 0)
	{
		zend_throw_exception(SEG(homegear_exception_class_entry), "Script id is unset. Please call \"registerThread\" before calling any Homegear specific method within threads.", -1);
		RETURN_FALSE
	}
	if(!SEG(rpcCallback)) RETURN_FALSE;
	if(!parameters) parameters.reset(new BaseLib::Variable(BaseLib::VariableType::tArray));
	BaseLib::PVariable result = SEG(rpcCallback)(methodName, parameters, wait);
	if(result->errorStruct)
	{
		zend_throw_exception(SEG(homegear_exception_class_entry), result->structValue->at("faultString")->stringValue.c_str(), result->structValue->at("faultCode")->integerValue);
		RETURN_NULL()
	}
	PhpVariableConverter::getPHPVariable(result, return_value);
}

ZEND_FUNCTION(hg_node_log);
ZEND_FUNCTION(hg_node_invoke_node_method);
ZEND_FUNCTION(hg_node_output);
ZEND_FUNCTION(hg_node_node_event);
ZEND_FUNCTION(hg_node_get_node_data);
ZEND_FUNCTION(hg_node_set_node_data);
ZEND_FUNCTION(hg_node_get_flow_data);
ZEND_FUNCTION(hg_node_set_flow_data);
ZEND_FUNCTION(hg_node_get_global_data);
ZEND_FUNCTION(hg_node_set_global_data);
ZEND_FUNCTION(hg_node_get_config_parameter);

ZEND_FUNCTION(hg_node_log)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	int32_t logLevel = 3;
	std::string message;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::log().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::log().");
	else
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "logLevel is not of type integer.");
		else
		{
			logLevel = Z_LVAL(args[0]);
		}

		if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "message is not of type string.");
		else
		{
			if(Z_STRLEN(args[1]) > 0) message = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
		}
	}
	if(message.empty()) RETURN_FALSE;

	std::string methodName("executePhpNodeBaseMethod");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>("log"));
	BaseLib::PVariable innerParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	innerParameters->arrayValue->reserve(3);
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(logLevel));
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(message));
	parameters->arrayValue->push_back(innerParameters);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, false);

	RETURN_TRUE;
}

ZEND_FUNCTION(hg_node_invoke_node_method)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string nodeId;
	std::string nodeMethodName;
	BaseLib::PVariable nodeMethodParameters;
	if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::invokeNodeMethod().");
	else if(argc < 3) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::invokeNodeMethod().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "nodeId is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) nodeId = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "methodName is not of type string.");
		else
		{
			if(Z_STRLEN(args[1]) > 0) nodeMethodName = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
		}

		if(Z_TYPE(args[2]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "parameters are not of type array.");
		else
		{
			nodeMethodParameters = PhpVariableConverter::getVariable(&(args[2]));
		}
	}
	if(nodeId.empty() || nodeMethodName.empty()) RETURN_FALSE;

	std::string methodName("executePhpNodeBaseMethod");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>("invokeNodeMethod"));
	BaseLib::PVariable innerParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	innerParameters->arrayValue->reserve(3);
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(nodeId));
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(nodeMethodName));
	innerParameters->arrayValue->push_back(nodeMethodParameters);
	parameters->arrayValue->push_back(innerParameters);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_node_output)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	int32_t outputIndex = 0;
	BaseLib::PVariable message;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::output().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::output().");
	else
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "outputIndex is not of type integer.");
		else
		{
			outputIndex = Z_LVAL(args[0]);
		}

		message = PhpVariableConverter::getVariable(&(args[1]));
	}

	std::string methodName("nodeOutput");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(outputIndex));
	parameters->arrayValue->push_back(message);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, false);
}

ZEND_FUNCTION(hg_node_node_event)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	BaseLib::PVariable value;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::nodeEvent().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::nodeEvent().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "topic is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		value = PhpVariableConverter::getVariable(&(args[1]));
	}

	std::string methodName("executePhpNodeBaseMethod");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>("nodeEvent"));
	BaseLib::PVariable innerParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	innerParameters->arrayValue->reserve(3);
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	innerParameters->arrayValue->push_back(value);
	parameters->arrayValue->push_back(innerParameters);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, false);
}

ZEND_FUNCTION(hg_node_get_node_data)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::getNodeData().");
	else if(argc < 1) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::getNodeData().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "key is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}
	}

	std::string methodName("getNodeData");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(2);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_node_set_node_data)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	BaseLib::PVariable value;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::setNodeData().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::setNodeData().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "key is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		value = PhpVariableConverter::getVariable(&(args[1]));
	}

	std::string methodName("setNodeData");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(nodeId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	parameters->arrayValue->push_back(value);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, false);
}

ZEND_FUNCTION(hg_node_get_flow_data)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::getFlowData().");
	else if(argc < 1) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::getFlowData().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "key is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}
	}

	std::string methodName("getFlowData");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(2);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(flowId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_node_set_flow_data)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	BaseLib::PVariable value;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::setFlowData().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::setFlowData().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "key is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		value = PhpVariableConverter::getVariable(&(args[1]));
	}

	std::string methodName("setFlowData");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(SEG(flowId)));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	parameters->arrayValue->push_back(value);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, false);
}

ZEND_FUNCTION(hg_node_get_global_data)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::getGlobalData().");
	else if(argc < 1) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::getGlobalData().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "key is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}
	}

	std::string methodName("getGlobalData");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_node_set_global_data)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string topic;
	BaseLib::PVariable value;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::setGlobalData().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::setGlobalData().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "key is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) topic = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		value = PhpVariableConverter::getVariable(&(args[1]));
	}

	std::string methodName("setGlobalData");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(2);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(topic));
	parameters->arrayValue->push_back(value);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, false);
}

ZEND_FUNCTION(hg_node_get_config_parameter)
{
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string nodeId;
	std::string name;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearNode::getConfigParameter().");
	else if(argc < 2) php_error_docref(NULL, E_WARNING, "Not enough arguments passed to HomegearNode::getConfigParameter().");
	else
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "nodeId is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) nodeId = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
		else
		{
			if(Z_STRLEN(args[1]) > 0) name = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
		}
	}

	std::string methodName("executePhpNodeBaseMethod");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->reserve(3);
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(nodeId));
	parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>("getConfigParameter"));
	BaseLib::PVariable innerParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
	parameters->arrayValue->push_back(innerParameters);
	php_homegear_node_invoke_rpc(methodName, parameters, return_value, true);
}

static const zend_function_entry homegear_node_base_methods[] = {
	ZEND_ME_MAPPING(log, hg_node_log, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(invokeNodeMethod, hg_node_invoke_node_method, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(output, hg_node_output, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(nodeEvent, hg_node_node_event, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getNodeData, hg_node_get_node_data, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(setNodeData, hg_node_set_node_data, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getFlowData, hg_node_get_node_data, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(setFlowData, hg_node_set_node_data, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getGlobalData, hg_node_get_node_data, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(setGlobalData, hg_node_set_node_data, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getConfigParameter, hg_node_get_config_parameter, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

void php_node_startup()
{
	zend_class_entry homegearNodeBaseCe;
	INIT_CLASS_ENTRY(homegearNodeBaseCe, "HomegearNodeBase", homegear_node_base_methods);
	SEG(homegear_node_base_class_entry) = zend_register_internal_class(&homegearNodeBaseCe);
	zend_declare_class_constant_stringl(SEG(homegear_node_base_class_entry), "NODE_ID", sizeof("NODE_ID") - 1, SEG(nodeId).c_str(), SEG(nodeId).size());
}

bool php_init_node(PScriptInfo scriptInfo, zend_class_entry* homegearNodeClassEntry, zval* homegearNodeObject)
{
	try
	{
		zend_object* homegearNode = nullptr;

		{
			zend_string* className = zend_string_init("HomegearNode", sizeof("HomegearNode") - 1, 0);
			homegearNodeClassEntry = zend_lookup_class(className);
			zend_string_release(className);
		}

		if(!homegearNodeClassEntry)
		{
			GD::out.printError("Error: Class HomegearNode not found in file: " + scriptInfo->fullPath);
			return false;
		}

		//Don't use pefree to free the allocated memory => double free in php_request_shutdown (invisible in valgrind and not crashing on at least amd64!).
		homegearNode = (zend_object*)ecalloc(1, sizeof(zend_object) + zend_object_properties_size(homegearNodeClassEntry));
		zend_object_std_init(homegearNode, homegearNodeClassEntry);
		object_properties_init(homegearNode, homegearNodeClassEntry);
		homegearNode->handlers = zend_get_std_object_handlers();
		ZVAL_OBJ(homegearNodeObject, homegearNode);

		bool stop = false;
		if(scriptInfo->getType() == BaseLib::ScriptEngine::ScriptInfo::ScriptType::statefulNode)
		{
			if(!zend_hash_str_find_ptr(&(homegearNodeClassEntry->function_table), "init", sizeof("init") - 1))
			{
				GD::out.printError("Error: Mandatory method \"init\" not found in class \"HomegearNode\". File: " + scriptInfo->fullPath);
				stop = true;
			}
			if(!zend_hash_str_find_ptr(&(homegearNodeClassEntry->function_table), "start", sizeof("start") - 1))
			{
				GD::out.printError("Error: Mandatory method \"start\" not found in class \"HomegearNode\". File: " + scriptInfo->fullPath);
				stop = true;
			}
			if(!zend_hash_str_find_ptr(&(homegearNodeClassEntry->function_table), "stop", sizeof("stop") - 1))
			{
				GD::out.printError("Error: Mandatory method \"stop\" not found in class \"HomegearNode\". File: " + scriptInfo->fullPath);
				stop = true;
			}
			if(!zend_hash_str_find_ptr(&(homegearNodeClassEntry->function_table), "waitForStop", sizeof("stop") - 1))
			{
				GD::out.printError("Error: Mandatory method \"waitForStop\" not found in class \"HomegearNode\". File: " + scriptInfo->fullPath);
				stop = true;
			}
		}
		else if(scriptInfo->getType() == BaseLib::ScriptEngine::ScriptInfo::ScriptType::simpleNode)
		{
			if(!zend_hash_str_find_ptr(&(homegearNodeClassEntry->function_table), "input", sizeof("input") - 1))
			{
				GD::out.printError("Error: Mandatory method \"input\" not found in class \"HomegearNode\". File: " + scriptInfo->fullPath);
				stop = true;
			}
		}

		if(stop) return false;

		if(zend_hash_str_find_ptr(&(homegearNodeClassEntry->function_table), "__construct", sizeof("__construct") - 1))
		{
			zval returnValue;
			zval function;

			ZVAL_STRINGL(&function, "__construct", sizeof("__construct") - 1);
			int result = 0;

			zend_try
			{
				result = call_user_function(&(homegearNodeClassEntry->function_table), homegearNodeObject, &function, &returnValue, 0, nullptr);
			}
			zend_end_try();

			if(result != 0) GD::out.printError("Error calling function \"__construct\" in file: " + scriptInfo->fullPath);
			zval_ptr_dtor(&function);
			zval_ptr_dtor(&returnValue); //Not really necessary as returnValue is of primitive type
		}

		return true;
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
	return false;
}

BaseLib::PVariable php_node_object_invoke_local(PScriptInfo& scriptInfo, zval* homegearNodeObject, std::string& methodName, BaseLib::PArray& methodParameters)
{
	try
	{
		std::string methodName2 = methodName;
		BaseLib::HelperFunctions::toLower(methodName2);
		if(!zend_hash_str_find_ptr(&(Z_OBJ_P(homegearNodeObject)->ce->function_table), methodName2.c_str(), methodName2.size()))
		{
			if(methodName != "__destruct" && methodName != "configNodesStarted" && methodName != "startUpComplete" && methodName != "variableEvent" && methodName != "setNodeVariable" && methodName != "waitForStop")
			{
				return BaseLib::Variable::createError(-1, "Unknown method.");
			}
			else return std::make_shared<BaseLib::Variable>();
		}

		zval returnValue;
		zval function;
		ZVAL_STRINGL(&function, methodName.c_str(), methodName.size());
		int result = 0;
		if(methodParameters->size() == 0)
		{
			zend_try
			{
				result = call_user_function(&(Z_OBJ_P(homegearNodeObject)->ce->function_table), homegearNodeObject, &function, &returnValue, 0, nullptr);
			}
			zend_end_try();
		}
		else
		{
			zval parameters[methodParameters->size()];
			for(uint32_t i = 0; i < methodParameters->size(); i++)
			{
				PhpVariableConverter::getPHPVariable(methodParameters->at(i), &parameters[i]);
			}

			zend_try
			{
				result = call_user_function(&(Z_OBJ_P(homegearNodeObject)->ce->function_table), homegearNodeObject, &function, &returnValue, methodParameters->size(), parameters);
			}
			zend_end_try();

			for(uint32_t i = 0; i < methodParameters->size(); i++)
			{
				zval_ptr_dtor(&parameters[i]);
			}
		}
		BaseLib::PVariable response;
		if(result != 0)
		{
			GD::out.printError("Error calling function \"" + methodName + "\" in file: " + scriptInfo->fullPath);
			response = BaseLib::Variable::createError(-3, "Error calling method: " + methodName);
		}
		else response = PhpVariableConverter::getVariable(&returnValue);
		zval_ptr_dtor(&function);
		zval_ptr_dtor(&returnValue); //Not really necessary as returnValue is of primitive type
		return response;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

