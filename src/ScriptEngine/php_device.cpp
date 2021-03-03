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

#ifndef NO_SCRIPTENGINE

#include "php_config_fixes.h"
#include "../GD/GD.h"
#include "php_device.h"
#include "PhpVariableConverter.h"

#define SEG(v) php_homegear_get_globals()->v

void php_homegear_device_invoke_rpc(std::string &methodName, BaseLib::PVariable &parameters, zval *return_value, bool wait) {
  if (SEG(id) == 0) {
    zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Please call \"registerThread\" before calling any Homegear specific method within threads.", -1);
    RETURN_FALSE;
  }
  if (!SEG(rpcCallback)) RETURN_FALSE;
  if (!parameters) parameters.reset(new BaseLib::Variable(BaseLib::VariableType::tArray));
  BaseLib::PVariable result = SEG(rpcCallback)(methodName, parameters, wait);
  if (result->errorStruct) {
    zend_throw_exception(homegear_exception_class_entry, result->structValue->at("faultString")->stringValue.c_str(), result->structValue->at("faultCode")->integerValue);
    RETURN_NULL();
  }
  Homegear::PhpVariableConverter::getPHPVariable(result, return_value);
}

static const zend_function_entry homegear_device_base_methods[] = {
    {NULL, NULL, NULL}
};

void php_device_startup() {
  zend_class_entry homegearDeviceBaseCe{};
  INIT_CLASS_ENTRY(homegearDeviceBaseCe, "HomegearDeviceBase", homegear_device_base_methods);
  homegear_device_base_class_entry = zend_register_internal_class(&homegearDeviceBaseCe);
  std::string peerIdString = std::to_string(SEG(peerId));
  zend_declare_class_constant_stringl(homegear_device_base_class_entry, "PEER_ID", sizeof("PEER_ID") - 1, peerIdString.c_str(), peerIdString.size());
}

bool php_init_device(PScriptInfo scriptInfo, zend_class_entry *homegearDeviceClassEntry, zval *homegearDeviceObject) {
  try {
    zend_object *homegearDevice = nullptr;

    {
      zend_string *className = zend_string_init("HomegearDevice", sizeof("HomegearDevice") - 1, 0);
      homegearDeviceClassEntry = zend_lookup_class(className);
      zend_string_release(className);
    }

    if (!homegearDeviceClassEntry) {
      Homegear::GD::out.printError("Error: Class HomegearDevice not found in file: " + scriptInfo->fullPath);
      return false;
    }

    //Don't use pefree to free the allocated memory => double free in php_request_shutdown (invisible in valgrind and not crashing on at least amd64!).
    homegearDevice = (zend_object *)ecalloc(1, sizeof(zend_object) + zend_object_properties_size(homegearDeviceClassEntry));
    zend_object_std_init(homegearDevice, homegearDeviceClassEntry);
    object_properties_init(homegearDevice, homegearDeviceClassEntry);
    homegearDevice->handlers = zend_get_std_object_handlers();
    ZVAL_OBJ(homegearDeviceObject, homegearDevice);

    bool stop = false;
    if (!zend_hash_str_find_ptr(&(homegearDeviceClassEntry->function_table), "init", sizeof("init") - 1)) {
      Homegear::GD::out.printError("Error: Mandatory method \"init\" not found in class \"HomegearDevice\". File: " + scriptInfo->fullPath);
      stop = true;
    }
    if (!zend_hash_str_find_ptr(&(homegearDeviceClassEntry->function_table), "start", sizeof("start") - 1)) {
      Homegear::GD::out.printError("Error: Mandatory method \"start\" not found in class \"HomegearDevice\". File: " + scriptInfo->fullPath);
      stop = true;
    }
    if (!zend_hash_str_find_ptr(&(homegearDeviceClassEntry->function_table), "stop", sizeof("stop") - 1)) {
      Homegear::GD::out.printError("Error: Mandatory method \"stop\" not found in class \"HomegearDevice\". File: " + scriptInfo->fullPath);
      stop = true;
    }
    if (!zend_hash_str_find_ptr(&(homegearDeviceClassEntry->function_table), "waitforstop", sizeof("waitforstop") - 1)) {
      Homegear::GD::out.printError("Error: Mandatory method \"waitForStop\" not found in class \"HomegearDevice\". File: " + scriptInfo->fullPath);
      stop = true;
    }
    if (stop) return false;

    if (zend_hash_str_find_ptr(&(homegearDeviceClassEntry->function_table), "__construct", sizeof("__construct") - 1)) {
      zval returnValue;
      zval function;

      ZVAL_STRINGL(&function, "__construct", sizeof("__construct") - 1);
      int result = 0;

      zend_try
          {
            result = call_user_function(&(homegearDeviceClassEntry->function_table), homegearDeviceObject, &function, &returnValue, 0, nullptr);
          }
      zend_end_try();

      if (result != 0) Homegear::GD::out.printError("Error calling function \"__construct\" in file: " + scriptInfo->fullPath);
      zval_ptr_dtor(&function);
      zval_ptr_dtor(&returnValue); //Not really necessary as returnValue is of primitive type
    }

    return true;
  }
  catch (const std::exception &ex) {
    Homegear::GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

BaseLib::PVariable php_device_object_invoke_local(PScriptInfo &scriptInfo, zval *homegearDeviceObject, std::string &methodName, BaseLib::PArray &methodParameters) {
  try {
    std::string methodName2 = methodName;
    BaseLib::HelperFunctions::toLower(methodName2);
    if (!zend_hash_str_find_ptr(&(Z_OBJ_P(homegearDeviceObject)->ce->function_table), methodName2.c_str(), methodName2.size())) {
      if (methodName != "__destruct" && methodName != "waitForStop") {
        return BaseLib::Variable::createError(-1, "Unknown method.");
      } else return std::make_shared<BaseLib::Variable>();
    }

    zval returnValue;
    zval function;
    ZVAL_STRINGL(&function, methodName.c_str(), methodName.size());
    int result = 0;
    if (methodParameters->size() == 0) {
      zend_try
          {
            result = call_user_function(&(Z_OBJ_P(homegearDeviceObject)->ce->function_table), homegearDeviceObject, &function, &returnValue, 0, nullptr);
          }
      zend_end_try();
    } else {
      zval parameters[methodParameters->size()];
      for (uint32_t i = 0; i < methodParameters->size(); i++) {
        Homegear::PhpVariableConverter::getPHPVariable(methodParameters->at(i), &parameters[i]);
      }

      zend_try
          {
            result = call_user_function(&(Z_OBJ_P(homegearDeviceObject)->ce->function_table), homegearDeviceObject, &function, &returnValue, methodParameters->size(), parameters);
          }
      zend_end_try();

      for (uint32_t i = 0; i < methodParameters->size(); i++) {
        zval_ptr_dtor(&parameters[i]);
      }
    }
    BaseLib::PVariable response;
    if (result != 0) {
      Homegear::GD::out.printError("Error calling function \"" + methodName + "\" in file: " + scriptInfo->fullPath);
      response = BaseLib::Variable::createError(-3, "Error calling method: " + methodName);
    } else response = Homegear::PhpVariableConverter::getVariable(&returnValue);
    zval_ptr_dtor(&function);
    zval_ptr_dtor(&returnValue); //Not really necessary as returnValue is of primitive type
    return response;
  }
  catch (const std::exception &ex) {
    Homegear::GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

#endif
