/* Copyright 2013-2016 Sathya Laufer
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

#ifndef HOMEGEAR_PHP_SAPI_H_
#define HOMEGEAR_PHP_SAPI_H_

#include "php_config_fixes.h"
#include <homegear-base/BaseLib.h>
#include "PhpEvents.h"

#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <map>

#include <php.h>
#include <SAPI.h>
#include <php_main.h>
#include <php_variables.h>
#include <php_ini.h>
#include <zend_API.h>
#include <zend_ini.h>
#include <zend_exceptions.h>
#include <ext/standard/info.h>

namespace ScriptEngine
{
	class ScriptEngineClient;
}

typedef struct _zend_homegear_globals
{
	std::function<void(std::string& output)> outputCallback;
	std::function<void(BaseLib::PVariable& headers)> sendHeadersCallback;
	std::function<BaseLib::PVariable(std::string& methodName, BaseLib::PVariable& parameters)> rpcCallback;
	BaseLib::Http http;
	PScriptInfo scriptInfo;

	bool webRequest = false;
	bool commandLine = false;
	bool cookiesParsed = false;
	int64_t peerId = 0;

	// {{{ Needed by ScriptEngineClient
	int32_t id = 0;
	std::string token;
	bool executionStarted = false;
	// }}}
} zend_homegear_globals;

typedef struct _zend_homegear_superglobals
{
	BaseLib::Http* http;
	BaseLib::Gpio* gpio;
	std::mutex serialDevicesMutex;
	std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>> serialDevices;
} zend_homegear_superglobals;

zend_homegear_globals* php_homegear_get_globals();
void php_homegear_build_argv(std::vector<std::string>& arguments);
int php_homegear_init();
void php_homegear_shutdown();

#endif

