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

#ifndef HOMEGEAR_PHP_GLOBALS_H_
#define HOMEGEAR_PHP_GLOBALS_H_

#include <homegear-base/BaseLib.h>
#include "../../config.h"
#include "php_config_fixes.h"

#include <zend_types.h>

__attribute__((unused)) static zend_class_entry* homegear_exception_class_entry = nullptr;

typedef struct _zend_homegear_globals
{
	std::function<void(std::string& output)> outputCallback;
	std::function<void(BaseLib::PVariable& headers)> sendHeadersCallback;
	std::function<BaseLib::PVariable(std::string& methodName, BaseLib::PVariable& parameters, bool wait)> rpcCallback;
	BaseLib::Http http;
	BaseLib::ScriptEngine::PScriptInfo scriptInfo;

	bool webRequest = false;
	bool commandLine = false;
	bool cookiesParsed = false;
	int64_t peerId = 0;
	int32_t logLevel = -1;

	// {{{ Needed by ScriptEngineClient
	int32_t id = 0;
	std::string token;
	bool executionStarted = false;
	// }}}

	// {{{ Needed for nodes
	std::string nodeId;
	std::string flowId;
	// }}}
} zend_homegear_globals;

typedef struct _zend_homegear_superglobals
{
	BaseLib::Http* http;
	BaseLib::LowLevel::Gpio* gpio;
	std::mutex serialDevicesMutex;
	std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>> serialDevices;
} zend_homegear_superglobals;

zend_homegear_globals* php_homegear_get_globals();
pthread_key_t* php_homegear_get_pthread_key();

#endif
