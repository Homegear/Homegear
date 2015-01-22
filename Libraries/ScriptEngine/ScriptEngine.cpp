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

#include "ScriptEngine.h"
#include "../GD/GD.h"

int32_t logScriptOutput(const void *output, unsigned int outputLen, void *userData)
{
	if(!output || outputLen == 0) return PH7_OK;
	std::string stringOutput((const char*)output, outputLen);
	GD::out.printMessage("Script output: " + stringOutput);
    return PH7_OK;
}

int32_t logScriptError(const char* message, int32_t messageType, const char* destination, const char* header)
{
	if(!message) return PH7_OK;
	std::string stringMessage(message);
	GD::out.printError("Script output: Error (type " + std::to_string(messageType) + "): " + stringMessage);
    return PH7_OK;
}

int32_t storeScriptOutput(const void *output, unsigned int outputLen, void *userData)
{
	if(!output || outputLen == 0) return PH7_OK;
	if(!userData || !((ScriptEngine::ScriptInfo*)userData)->engine) return PH7_OK;
	if(((ScriptEngine::ScriptInfo*)userData)->died) return PH7_ABORT;
	((ScriptEngine::ScriptInfo*)userData)->engine->appendOutput(((ScriptEngine::ScriptInfo*)userData)->path, (const char*)output, outputLen);
    return PH7_OK;
}

int32_t fillArray(ph7_value* pKey, ph7_value* pValue, void* pUserData)
{
	((std::map<std::string, std::string>*)pUserData)->operator [](std::string(ph7_value_to_string(pKey, 0))) = std::string(ph7_value_to_string(pValue, 0));
	return PH7_OK;
}

int32_t abort(ph7_context* context, int32_t argc, ph7_value** argv)
{
	ScriptEngine::ScriptInfo* info = (ScriptEngine::ScriptInfo*)ph7_context_user_data(context);
	if(!info) return PH7_ABORT;
	info->engine->resetOutput(info->path);
	info->died = true;
	if(argc > 0)
	{
		if(ph7_value_is_string(argv[0]))
		{
			const char* message;
			int32_t length = 0;
			message = ph7_value_to_string(argv[0], &length);
			info->engine->appendOutput(info->path, message, length);
		}
	}
	return PH7_ABORT;
}

int32_t header(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 1)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ScriptEngine::ScriptInfo* info = (ScriptEngine::ScriptInfo*)ph7_context_user_data(context);
	if(!info || !info->headers) return PH7_OK;

	int32_t length = 0;
	const char* header = ph7_value_to_string(argv[0], &length);
	info->headers->push_back(std::string(header, length));

	return PH7_OK;
}

int32_t session_start(ph7_context* context, int32_t argc, ph7_value** argv)
{
	ph7_result_bool(context, 1);
	ScriptEngine::ScriptInfo* info = (ScriptEngine::ScriptInfo*)ph7_context_user_data(context);
	if(!info) return PH7_OK;
	if(!info->sessionID.empty())
	{
		std::shared_ptr<ScriptEngine::Session> session = info->engine->getSession(info->sessionID);
		if(session)
		{
			for(std::map<std::string, std::string>::iterator i = session->data.begin(); i != session->data.end(); ++i)
			{
				ph7_vm_config(info->compiledProgram, PH7_VM_CONFIG_SESSION_ATTR, i->first.c_str(), i->second.c_str(), i->second.size());
			}
			return PH7_OK;
		}
	}
	std::vector<uint8_t> id;
	id.reserve(16);
	for(int i = 0; i < 16; i++)
	{
		id.push_back((uint8_t)BaseLib::HelperFunctions::getRandomNumber(0, 255));
	}
	ph7_vm_config(info->compiledProgram, PH7_VM_CONFIG_COOKIE_ATTR, "PH7SESSID", BaseLib::HelperFunctions::getHexString(id).c_str(), 32);
	return PH7_OK;
}

int32_t shell_exec(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 1)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	int32_t length = 0;
	const char* cmd = ph7_value_to_string(argv[0], &length);

	std::string result;
	if(BaseLib::HelperFunctions::exec(std::string(cmd, length), result) != 0)
	{
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_result_string(context, result.c_str(), result.size());
	return PH7_OK;
}

int32_t hg_auth(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 2)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	int32_t lengthUser = 0;
	const char* user = ph7_value_to_string(argv[0], &lengthUser);
	int32_t lengthPassword = 0;
	const char* password = ph7_value_to_string(argv[1], &lengthPassword);

	if(User::verify(std::string(user, lengthUser), std::string(password, lengthPassword)))
	{
		ph7_result_bool(context, 1);
		return PH7_OK;
	}

	ph7_result_bool(context, 0);
	return PH7_OK;
}

int32_t hg_create_user(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 2)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	int32_t lengthUser = 0;
	const char* user = ph7_value_to_string(argv[0], &lengthUser);
	int32_t lengthPassword = 0;
	const char* password = ph7_value_to_string(argv[1], &lengthPassword);

	if(User::create(std::string(user, lengthUser), std::string(password, lengthPassword)))
	{
		ph7_result_bool(context, 1);
		return PH7_OK;
	}

	ph7_result_bool(context, 0);
	return PH7_OK;
}

int32_t hg_delete_user(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 1)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	int32_t lengthUser = 0;
	const char* user = ph7_value_to_string(argv[0], &lengthUser);

	if(User::remove(std::string(user, lengthUser)))
	{
		ph7_result_bool(context, 1);
		return PH7_OK;
	}

	ph7_result_bool(context, 0);
	return PH7_OK;
}

int32_t hg_update_user(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 2)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	int32_t lengthUser = 0;
	const char* user = ph7_value_to_string(argv[0], &lengthUser);
	int32_t lengthPassword = 0;
	const char* password = ph7_value_to_string(argv[1], &lengthPassword);

	if(User::update(std::string(user, lengthUser), std::string(password, lengthPassword)))
	{
		ph7_result_bool(context, 1);
		return PH7_OK;
	}

	ph7_result_bool(context, 0);
	return PH7_OK;
}

int32_t hg_user_exists(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 1)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	int32_t lengthUser = 0;
	const char* user = ph7_value_to_string(argv[0], &lengthUser);

	if(User::exists(std::string(user, lengthUser)))
	{
		ph7_result_bool(context, 1);
		return PH7_OK;
	}

	ph7_result_bool(context, 0);
	return PH7_OK;
}

int32_t hg_users(ph7_context* context, int32_t argc, ph7_value** argv)
{
	std::map<uint64_t, std::string> users;
	User::getAll(users);

	ph7_value* pArray = ph7_context_new_array(context);
	ph7_value* pKey = ph7_context_new_scalar(context);
	ph7_value* pValue = ph7_context_new_scalar(context);
	if(!pArray || !pKey || !pValue)
	{
		ph7_result_bool(context, 0);
		return PH7_ABORT;
	}

	for(std::map<uint64_t, std::string>::iterator i = users.begin(); i != users.end(); ++i)
	{
		ph7_value_int(pKey, i->first);
		ph7_value_string(pValue, i->second.c_str(), i->second.size());
		ph7_array_add_elem(pArray, pKey, pValue);
		ph7_value_reset_string_cursor(pValue);
	}

	ph7_result_value(context, pArray);
	return PH7_OK;
}

int32_t hg_set_system(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 2)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 0; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod("setSystemVariable", parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_result_null(context);
	return PH7_OK;
}

int32_t hg_get_system(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 1)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 0; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod("getSystemVariable", parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_value* ph7Result = PH7VariableConverter::getPH7Variable(context, result);
	if(ph7Result) ph7_result_value(context, ph7Result);
	return PH7_OK;
}

int32_t hg_set_meta(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 3)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_int(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no integer.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 0; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod("setMetadata", parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_result_null(context);
	return PH7_OK;
}

int32_t hg_get_meta(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 2)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_int(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no integer.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 0; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod("getMetadata", parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_value* ph7Result = PH7VariableConverter::getPH7Variable(context, result);
	if(ph7Result) ph7_result_value(context, ph7Result);
	return PH7_OK;
}

int32_t hg_set_value(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 4)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_int(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no integer.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_int(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no integer.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[2]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Third parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 0; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod("setValue", parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_result_null(context);
	return PH7_OK;
}

int32_t hg_get_value(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc != 3)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Wrong parameter count.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_int(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no integer.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_int(argv[1]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Second parameter is no integer.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[2]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Third parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 0; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod("getValue", parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_value* ph7Result = PH7VariableConverter::getPH7Variable(context, result);
	if(ph7Result) ph7_result_value(context, ph7Result);
	return PH7_OK;
}

int32_t hg_invoke(ph7_context* context, int32_t argc, ph7_value** argv)
{
	if(argc == 0)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Missing method name.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	if(!ph7_value_is_string(argv[0]))
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "First parameter is no string.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	int32_t length = 0;
	const char* pMethodName = ph7_value_to_string(argv[0], &length);
	if(length <= 0)
	{
		ph7_context_throw_error(context, PH7_CTX_WARNING, "Missing method name.");
		ph7_result_bool(context, 0);
		return PH7_OK;
	}
	std::string methodName(pMethodName, length);
	std::shared_ptr<BaseLib::RPC::Variable> parameters(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
	for(int32_t i = 1; i < argc; i++)
	{
		std::shared_ptr<BaseLib::RPC::Variable> parameter = PH7VariableConverter::getVariable(argv[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	std::shared_ptr<BaseLib::RPC::Variable> result = GD::rpcServers.begin()->second.callMethod(methodName, parameters);
	if(result->errorStruct)
	{
		std::string errorString("RPC error (Code " + std::to_string(result->structValue->at("faultCode")->integerValue) + "): " + result->structValue->at("faultString")->stringValue);
		ph7_context_throw_error(context, PH7_CTX_WARNING, errorString.c_str());
		ph7_result_bool(context, 0);
		return PH7_OK;
	}

	ph7_value* ph7Result = PH7VariableConverter::getPH7Variable(context, result);
	if(ph7Result) ph7_result_value(context, ph7Result);
	return PH7_OK;
}

ScriptEngine::ScriptEngine()
{
	try
	{
		if(ph7_init(&_engine) != PH7_OK)
		{
			GD::out.printError("Error while allocating a new PH7 engine instance.");
			ph7_lib_shutdown();
			_engine = nullptr;
			return;
		}
		if(!ph7_lib_is_threadsafe())
		{
			GD::out.printError("Error: PH7 library is not thread safe.");
			ph7_release(_engine);
			_engine = nullptr;
			ph7_lib_shutdown();
		}
		ph7_lib_config(PH7_LIB_CONFIG_THREAD_LEVEL_MULTI);
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

ScriptEngine::~ScriptEngine()
{
	dispose();
}

void ScriptEngine::dispose()
{
	if(_disposing) return;
	_disposing = true;
	_noExecution = true;
	ph7_release(_engine);
	_engine = nullptr;
	ph7_lib_shutdown();
	clearPrograms();
}

void ScriptEngine::clearPrograms()
{
	_noExecution = true;
	while(_executionCount > 0) std::this_thread::sleep_for(std::chrono::milliseconds(10));
	_programsMutex.lock();
	for(std::map<std::string, ScriptInfo>::iterator i = _programs.begin(); i != _programs.end(); ++i)
	{
		ph7_vm_release(i->second.compiledProgram);
		i->second.compiledProgram = nullptr;
	}
	_programs.clear();
	_programsMutex.unlock();
	_noExecution = false;
}

void ScriptEngine::appendOutput(std::string& path, const char* output, uint32_t outputLength)
{
	_outputMutex.lock();
	try
	{
		std::vector<char>* out = _outputs[path];
		if(!out)
		{
			_outputMutex.unlock();
			return;
		}
		if(out->size() + outputLength > out->capacity()) out->reserve(out->capacity() + 1024);
		out->insert(out->end(), output, output + outputLength);
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
	_outputMutex.unlock();
}

void ScriptEngine::resetOutput(std::string& path)
{
	_outputMutex.lock();
	try
	{
		std::vector<char>* out = _outputs[path];
		if(!out)
		{
			_outputMutex.unlock();
			return;
		}
		out->clear();
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
	_outputMutex.unlock();
}

ScriptEngine::ScriptInfo* ScriptEngine::addProgram(std::string path, bool storeOutput)
{
	try
	{
		if(!_engine)
		{
			GD::out.printError("Error: Script engine is nullptr.");
			return nullptr;
		}
		std::string content;
		try
		{
			content = GD::bl->hf.getFileContent(path);
		}
		catch(BaseLib::Exception& ex)
		{
			GD::bl->out.printError("Error in script engine: Script \"" + path + "\" not found.");
			return nullptr;
		}
		if(content.empty())
		{
			GD::out.printError("Error: Script file \"" + path + "\" is empty.");
			return nullptr;
		}
		ph7_vm* compiledProgram = nullptr;
		int32_t result = ph7_compile_v2(_engine, content.c_str(), -1, &compiledProgram, 0);
		if(result != PH7_OK)
		{
			if(result == PH7_COMPILE_ERR)
			{
				const char* errorLog;
				int32_t length;
				ph7_config(_engine, PH7_CONFIG_ERR_LOG,	&errorLog, &length);
				if(length > 0) GD::out.printError("Error compiling script \"" + path + "\": " + std::string(errorLog));
			}
			return nullptr;
		}

		result = ph7_create_function(compiledProgram, "shell_exec", shell_exec, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_auth", hg_auth, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_create_user", hg_create_user, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_delete_user", hg_delete_user, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_update_user", hg_update_user, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_user_exists", hg_user_exists, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_users", hg_users, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_invoke", hg_invoke, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_set_meta", hg_set_meta, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_get_meta", hg_get_meta, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_set_system", hg_set_system, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_get_system", hg_get_system, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_set_value", hg_set_value, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}
		result = ph7_create_function(compiledProgram, "hg_get_value", hg_get_value, 0);
		if(result != PH7_OK)
		{
			GD::out.printError("Error adding functions to script \"" + path + "\".");
			ph7_vm_release(compiledProgram);
			return nullptr;
		}

		_programsMutex.lock();
		if(_disposing)
		{
			_programsMutex.unlock();
			ph7_vm_release(compiledProgram);
			compiledProgram = nullptr;
			return nullptr;
		}
		if(_programs.find(path) != _programs.end())
		{
			_executeMutexes.at(path)->lock(); //Don't release program while executing it
			ph7_vm_release(_programs.at(path).compiledProgram);
			_programs.at(path).compiledProgram = nullptr;
			_programs.erase(path);
			_executeMutexes.at(path)->unlock();
		}
		ScriptInfo* info = nullptr;
		if(!_disposing)
		{
			_executeMutexes.insert(std::pair<std::string, std::unique_ptr<std::mutex>>(path, std::unique_ptr<std::mutex>(new std::mutex())));
			info = &_programs[path];

			result = ph7_create_function(compiledProgram, "abort", abort, info);
			if(result != PH7_OK)
			{
				GD::out.printError("Error adding functions to script \"" + path + "\".");
				ph7_vm_release(compiledProgram);
				_programsMutex.unlock();
				return nullptr;
			}
			result = ph7_create_function(compiledProgram, "header", header, info);
			if(result != PH7_OK)
			{
				GD::out.printError("Error adding functions to script \"" + path + "\".");
				ph7_vm_release(compiledProgram);
				_programsMutex.unlock();
				return nullptr;
			}
			result = ph7_create_function(compiledProgram, "session_start", session_start, info);
			if(result != PH7_OK)
			{
				GD::out.printError("Error adding functions to script \"" + path + "\".");
				ph7_vm_release(compiledProgram);
				_programsMutex.unlock();
				return nullptr;
			}

			info->compiledProgram = compiledProgram;
			info->lastModified = GD::bl->hf.getFileLastModifiedTime(path);
			info->path = path;
			info->engine = this;

			if(storeOutput) result = ph7_vm_config(compiledProgram, PH7_VM_CONFIG_OUTPUT, storeScriptOutput, info);
			else result = ph7_vm_config(compiledProgram, PH7_VM_CONFIG_OUTPUT, logScriptOutput, 0);
			if(result != PH7_OK)
			{
				GD::out.printError("Error while installing the VM output consumer callback for script \"" + path + "\".");
				ph7_vm_release(compiledProgram);
				return nullptr;
			}

			result = ph7_vm_config(compiledProgram, PH7_VM_CONFIG_ERR_LOG_HANDLER, logScriptError);
			if(result != PH7_OK)
			{
				GD::out.printError("Error while installing the VM error consumer callback for script \"" + path + "\".");
				ph7_vm_release(compiledProgram);
				return nullptr;
			}

			result = ph7_vm_config(compiledProgram, PH7_VM_CONFIG_ERR_REPORT);
			if(result != PH7_OK)
			{
				GD::out.printError("Error enabling VM error reporting for script \"" + path + "\".");
				ph7_vm_release(compiledProgram);
				return nullptr;
			}
		}
		else
		{
			ph7_vm_release(compiledProgram);
			compiledProgram = nullptr;
		}
		_programsMutex.unlock();
		return info;
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
	_programsMutex.unlock();
	return nullptr;
}

ScriptEngine::ScriptInfo* ScriptEngine::getProgram(std::string path, bool storeOutput)
{
	try
	{
		ScriptInfo* scriptInfo = nullptr;
		_programsMutex.lock();
		if(!_disposing && _programs.find(path) != _programs.end())
		{
			if(GD::bl->hf.getFileLastModifiedTime(path) > _programs.at(path).lastModified)
			{
				_programsMutex.unlock();
				return addProgram(path, storeOutput);
			}
			else
			{
				scriptInfo = &_programs.at(path);
				_programsMutex.unlock();
			}
		}
		else _programsMutex.unlock();
		return scriptInfo;
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
	_programsMutex.unlock();
	return nullptr;
}

void ScriptEngine::removeProgram(std::string path)
{
	try
	{
		_programsMutex.lock();
		if(_programs.find(path) != _programs.end())
		{
			_executeMutexes.at(path)->lock(); //Don't release program while executing it
			ph7_vm_release(_programs.at(path).compiledProgram);
			_programs.at(path).compiledProgram = nullptr;
			_programs.erase(path);
			_executeMutexes.at(path)->unlock();
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
	_programsMutex.unlock();
}

bool ScriptEngine::isValid(const std::string& path, ph7_vm* compiledProgram)
{
	try
	{
		_programsMutex.lock();
		if(_programs.find(path) != _programs.end())
		{
			if(_programs.at(path).compiledProgram == compiledProgram)
			{
				_programsMutex.unlock();
				return true;
			}
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
	_programsMutex.unlock();
	return false;
}

int32_t ScriptEngine::execute(const std::string& path, const std::string& arguments)
{
	try
	{
		if(_disposing || _noExecution) return 1;
		_executionCount++;
		ScriptInfo* scriptInfo = getProgram(path, false);
		if(!scriptInfo) scriptInfo = addProgram(path, false);
		if(!scriptInfo)
		{
			_executionCount--;
			return 1;
		}
		ph7_vm* compiledProgram = scriptInfo->compiledProgram;
		if(!compiledProgram)
		{
			_executionCount--;
			return 1;
		}
		_executeMutexes.at(path)->lock();
		if(_disposing)
		{
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			return 1;
		}
		if(!isValid(path, compiledProgram)) //Check if compiledProgram is valid within mutex
		{
			GD::out.printError("Error: Script changed during execution: " + path);
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			return 1;
		}

		scriptInfo->sessionID = "";
		scriptInfo->cookie = nullptr;

		std::vector<std::string> stringArgv = getArgs(path, arguments);
		ph7_value* argument = ph7_new_scalar(compiledProgram);
		ph7_value* argv = ph7_new_array(compiledProgram);
		for(std::vector<std::string>::iterator i = stringArgv.begin(); i != stringArgv.end(); i++)
		{
			if(ph7_value_string(argument, (*i).c_str(), -1) != PH7_OK)
			{
				GD::out.printError("Error setting argv for script \"" + path + "\".");
				ph7_release_value(compiledProgram, argument);
				ph7_release_value(compiledProgram, argv);
				_executeMutexes.at(path)->unlock();
				_executionCount--;
				return 1;
			}
			if(ph7_array_add_elem(argv, 0, argument) != PH7_OK)
			{
				GD::out.printError("Error setting argv for script \"" + path + "\".");
				ph7_release_value(compiledProgram, argument);
				ph7_release_value(compiledProgram, argv);
				_executeMutexes.at(path)->unlock();
				_executionCount--;
				return 1;
			}
			ph7_value_reset_string_cursor(argument);
		}
		if(ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "argv", argv) != PH7_OK)
		{
			GD::out.printError("Error setting argv for script \"" + path + "\".");
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			return 1;
		}
		ph7_release_value(compiledProgram, argument);
		ph7_release_value(compiledProgram, argv);

		int32_t exitStatus = 0;
		if(ph7_vm_exec(compiledProgram, &exitStatus) != PH7_OK)
		{
			GD::out.printError("Error executing script \"" + path + "\".");
		}
		if(ph7_vm_reset(compiledProgram) != PH7_OK)
		{
			GD::out.printError("Error resetting script \"" + path + "\".");
		}
		_executeMutexes.at(path)->unlock();
		_executionCount--;
		return exitStatus;
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
	_executeMutexes.at(path)->unlock();
	_executionCount--;
	return 1;
}

void ScriptEngine::cleanUpSessions()
{
	try
	{
		_sessionsMutex.lock();
		std::vector<std::string> sessionsToDelete;
		for(std::map<std::string, std::shared_ptr<Session>>::iterator i = _sessions.begin(); i != _sessions.end(); ++i)
		{
			if(i->second->lastAccess < BaseLib::HelperFunctions::getTime() - 3600000) sessionsToDelete.push_back(i->first);
		}
		for(std::vector<std::string>::iterator i = sessionsToDelete.begin(); i != sessionsToDelete.end(); ++i)
		{
			_sessions.erase(*i);
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
	_sessionsMutex.unlock();
}

std::shared_ptr<ScriptEngine::Session> ScriptEngine::getSession(std::string id)
{
	try
	{
		std::shared_ptr<ScriptEngine::Session> session;
		if(id.empty()) return session;
		_sessionsMutex.lock();
		if(_sessions.find(id) != _sessions.end()) session = _sessions[id];
		_sessionsMutex.unlock();
		return session;
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
	_sessionsMutex.unlock();
	return std::shared_ptr<ScriptEngine::Session>();
}

int32_t ScriptEngine::executeWebRequest(const std::string& path, const std::vector<char>& request, std::vector<char>& output, std::vector<std::string>& headers)
{
	try
	{
		if(_disposing || _noExecution) return 1;
		if(request.empty()) return 1;
		_executionCount++;
		if(_disposing)
		{
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}
		cleanUpSessions();
		std::string importPath;
		uint32_t pos = path.find_last_of('/');
		if(pos != std::string::npos) importPath = path.substr(0, pos);
		ScriptInfo* scriptInfo = getProgram(path, true);
		if(!scriptInfo) scriptInfo = addProgram(path, true);
		if(!scriptInfo)
		{
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}
		ph7_vm* compiledProgram = scriptInfo->compiledProgram;
		if(!compiledProgram)
		{
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}
		_executeMutexes.at(path)->lock();
		_outputMutex.lock();
		_outputs[path] = &output;
		_outputMutex.unlock();
		if(_disposing)
		{
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}
		if(!isValid(path, compiledProgram)) //Check if compiledProgram is valid within mutex
		{
			GD::out.printError("Error: Script changed during execution: " + path);
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}

		ph7_vm_config(compiledProgram, PH7_VM_CONFIG_IMPORT_PATH, importPath.c_str());
		scriptInfo->sessionID = "";
		scriptInfo->cookie = nullptr;
		ph7_value* nullValue1 = ph7_new_array(compiledProgram);
		ph7_value* nullValue2 = ph7_new_array(compiledProgram);
		ph7_value* nullValue3 = ph7_new_array(compiledProgram);
		ph7_value* nullValue4 = ph7_new_array(compiledProgram);
		ph7_value* cookieValue = ph7_new_array(compiledProgram);
		ph7_value* sessionValue = ph7_new_array(compiledProgram);
		if(ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "_GET", nullValue1) != PH7_OK ||
		   ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "_POST", nullValue2) != PH7_OK ||
		   ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "_REQUEST", nullValue3) != PH7_OK ||
		   ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "_SERVER", nullValue4) != PH7_OK ||
		   ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "_COOKIE", cookieValue) != PH7_OK ||
		   ph7_vm_config(compiledProgram, PH7_VM_CONFIG_CREATE_SUPER, "_SESSION", sessionValue) != PH7_OK)
		{
			GD::out.printError("Error resetting HTTP request information for script \"" + path + "\".");
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}
		ph7_release_value(compiledProgram, nullValue1);
		ph7_release_value(compiledProgram, nullValue2);
		ph7_release_value(compiledProgram, nullValue3);
		ph7_release_value(compiledProgram, nullValue4);

		if(ph7_vm_config(compiledProgram, PH7_VM_CONFIG_HTTP_REQUEST, &request.at(0), request.size()) != PH7_OK)
		{
			GD::out.printError("Error parsing request for script \"" + path + "\".");
			_executeMutexes.at(path)->unlock();
			_executionCount--;
			std::string error("Error executing script. Check Homegear log for more details.");
			output.insert(output.end(), error.begin(), error.end());
			return 1;
		}

		ph7_value* pSessionID = ph7_array_fetch(cookieValue, "PH7SESSID", 9);
		if(pSessionID)
		{
			scriptInfo->sessionID = std::string(ph7_value_to_string(pSessionID, 0));
		}
		scriptInfo->cookie = cookieValue;
		scriptInfo->headers = &headers;
		scriptInfo->died = false;

		int32_t exitStatus = 0;
		if(ph7_vm_exec(compiledProgram, &exitStatus) != PH7_OK)
		{
			GD::out.printError("Error executing script \"" + path + "\".");
		}
		pSessionID = ph7_array_fetch(cookieValue, "PH7SESSID", 9);
		if(pSessionID)
		{
			std::string sessionID(ph7_value_to_string(pSessionID, 0));
			if(!sessionID.empty())
			{
				_sessionsMutex.lock();
				std::shared_ptr<Session> session;
				if(_sessions.find(sessionID) == _sessions.end())
				{
					session.reset(new Session());
					_sessions[sessionID] = session;
				}
				else session = _sessions[sessionID];
				session->lastAccess = BaseLib::HelperFunctions::getTime();
				ph7_array_walk(sessionValue, fillArray, (void*)&(session->data));
				_sessionsMutex.unlock();
				headers.push_back("Set-Cookie: PH7SESSID=" + sessionID + ";\r\nexpires=" + BaseLib::HelperFunctions::getTimeString("a, d-b-Y H:I:S GMT", BaseLib::HelperFunctions::getTime() + 3600000) + ";\r\nMax-Age=2592000;\r\nPath=/");
			}
		}
		ph7_release_value(compiledProgram, cookieValue);
		ph7_release_value(compiledProgram, sessionValue);
		if(ph7_vm_reset(compiledProgram) != PH7_OK)
		{
			GD::out.printError("Error resetting script \"" + path + "\".");
		}
		_outputMutex.lock();
		_outputs.erase(path);
		_outputMutex.unlock();
		_executeMutexes.at(path)->unlock();
		_executionCount--;
		return exitStatus;
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
	_executeMutexes.at(path)->unlock();
	_executionCount--;
	std::string error("Error executing script. Check Homegear log for more details.");
	output.insert(output.end(), error.begin(), error.end());
	return 1;
}

std::vector<std::string> ScriptEngine::getArgs(const std::string& path, const std::string& args)
{
	std::vector<std::string> argv;
	if(!path.empty() && path.back() != '/') argv.push_back(path.substr(path.find_last_of('/') + 1));
	else argv.push_back(path);
	wordexp_t p;
	wordexp(args.c_str(), &p, 0);
	for (size_t i = 0; i < p.we_wordc; i++)
	{
		argv.push_back(std::string(p.we_wordv[i]));
	}
	wordfree(&p);
	return argv;
}
