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

#include "php_config_fixes.h"
#include "../GD/GD.h"
#include "php_sapi.h"
#include "PhpVariableConverter.h"
#include "ScriptEngineClient.h"
#include "../../config.h"

#ifdef I2CSUPPORT
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#endif

static bool _disposed = true;
static zend_homegear_superglobals _superglobals;
static pthread_key_t pthread_key;
static zend_class_entry* homegear_class_entry = nullptr;
static zend_class_entry* homegear_gpio_class_entry = nullptr;
static zend_class_entry* homegear_serial_class_entry = nullptr;
#ifdef I2CSUPPORT
static zend_class_entry* homegear_i2c_class_entry = nullptr;
#endif
static zend_class_entry* homegear_exception_class_entry = nullptr;
static char* ini_path_override = nullptr;
static char* ini_entries = nullptr;
static const char HARDCODED_INI[] =
	"register_argc_argv=1\n"
	"max_execution_time=0\n"
	"max_input_time=-1\n\0";

static int php_homegear_startup(sapi_module_struct* sapi_module);
static int php_homegear_activate(TSRMLS_D);
static int php_homegear_deactivate(TSRMLS_D);
static size_t php_homegear_ub_write(const char* str, size_t length);
static void php_homegear_flush(void *server_context);
static int php_homegear_send_headers(sapi_headers_struct* sapi_headers);
static size_t php_homegear_read_post(char *buf, size_t count_bytes);
static char* php_homegear_read_cookies(TSRMLS_D);
static void php_homegear_register_variables(zval* track_vars_array);
static void php_homegear_log_message(char* message);
static PHP_MINIT_FUNCTION(homegear);
static PHP_MSHUTDOWN_FUNCTION(homegear);
static PHP_RINIT_FUNCTION(homegear);
static PHP_RSHUTDOWN_FUNCTION(homegear);
static PHP_MINFO_FUNCTION(homegear);

#define SEG(v) php_homegear_get_globals()->v

ZEND_FUNCTION(hg_get_script_id);
ZEND_FUNCTION(hg_register_thread);
ZEND_FUNCTION(hg_invoke);
ZEND_FUNCTION(hg_get_meta);
ZEND_FUNCTION(hg_get_system);
ZEND_FUNCTION(hg_get_value);
ZEND_FUNCTION(hg_set_meta);
ZEND_FUNCTION(hg_set_system);
ZEND_FUNCTION(hg_set_value);
ZEND_FUNCTION(hg_list_modules);
ZEND_FUNCTION(hg_load_module);
ZEND_FUNCTION(hg_unload_module);
ZEND_FUNCTION(hg_reload_module);
ZEND_FUNCTION(hg_auth);
ZEND_FUNCTION(hg_create_user);
ZEND_FUNCTION(hg_delete_user);
ZEND_FUNCTION(hg_update_user);
ZEND_FUNCTION(hg_user_exists);
ZEND_FUNCTION(hg_users);
ZEND_FUNCTION(hg_log);
ZEND_FUNCTION(hg_check_license);
ZEND_FUNCTION(hg_remove_license);
ZEND_FUNCTION(hg_get_license_states);
ZEND_FUNCTION(hg_poll_event);
ZEND_FUNCTION(hg_list_rpc_clients);
ZEND_FUNCTION(hg_peer_exists);
ZEND_FUNCTION(hg_subscribe_peer);
ZEND_FUNCTION(hg_unsubscribe_peer);
ZEND_FUNCTION(hg_shutting_down);
ZEND_FUNCTION(hg_gpio_open);
ZEND_FUNCTION(hg_gpio_close);
ZEND_FUNCTION(hg_gpio_set_direction);
ZEND_FUNCTION(hg_gpio_set_edge);
ZEND_FUNCTION(hg_gpio_get);
ZEND_FUNCTION(hg_gpio_set);
ZEND_FUNCTION(hg_gpio_poll);
ZEND_FUNCTION(hg_serial_open);
ZEND_FUNCTION(hg_serial_close);
ZEND_FUNCTION(hg_serial_read);
ZEND_FUNCTION(hg_serial_readline);
ZEND_FUNCTION(hg_serial_write);
#ifdef I2CSUPPORT
ZEND_FUNCTION(hg_i2c_open);
ZEND_FUNCTION(hg_i2c_close);
ZEND_FUNCTION(hg_i2c_read);
ZEND_FUNCTION(hg_i2c_write);
#endif

static const zend_function_entry homegear_functions[] = {
	ZEND_FE(hg_get_script_id, NULL)
	ZEND_FE(hg_register_thread, NULL)
	ZEND_FE(hg_invoke, NULL)
	ZEND_FE(hg_get_meta, NULL)
	ZEND_FE(hg_get_system, NULL)
	ZEND_FE(hg_get_value, NULL)
	ZEND_FE(hg_set_meta, NULL)
	ZEND_FE(hg_set_system, NULL)
	ZEND_FE(hg_set_value, NULL)
	ZEND_FE(hg_list_modules, NULL)
	ZEND_FE(hg_load_module, NULL)
	ZEND_FE(hg_unload_module, NULL)
	ZEND_FE(hg_reload_module, NULL)
	ZEND_FE(hg_auth, NULL)
	ZEND_FE(hg_create_user, NULL)
	ZEND_FE(hg_delete_user, NULL)
	ZEND_FE(hg_update_user, NULL)
	ZEND_FE(hg_user_exists, NULL)
	ZEND_FE(hg_users, NULL)
	ZEND_FE(hg_log, NULL)
	ZEND_FE(hg_check_license, NULL)
	ZEND_FE(hg_remove_license, NULL)
	ZEND_FE(hg_get_license_states, NULL)
	ZEND_FE(hg_poll_event, NULL)
	ZEND_FE(hg_list_rpc_clients, NULL)
	ZEND_FE(hg_peer_exists, NULL)
	ZEND_FE(hg_subscribe_peer, NULL)
	ZEND_FE(hg_unsubscribe_peer, NULL)
	ZEND_FE(hg_shutting_down, NULL)
	ZEND_FE(hg_gpio_open, NULL)
	ZEND_FE(hg_gpio_close, NULL)
	ZEND_FE(hg_gpio_set_direction, NULL)
	ZEND_FE(hg_gpio_set_edge, NULL)
	ZEND_FE(hg_gpio_get, NULL)
	ZEND_FE(hg_gpio_set, NULL)
	ZEND_FE(hg_gpio_poll, NULL)
	ZEND_FE(hg_serial_open, NULL)
	ZEND_FE(hg_serial_close, NULL)
	ZEND_FE(hg_serial_read, NULL)
	ZEND_FE(hg_serial_readline, NULL)
	ZEND_FE(hg_serial_write, NULL)
#ifdef I2CSUPPORT
	ZEND_FE(hg_i2c_open, NULL)
	ZEND_FE(hg_i2c_close, NULL)
	ZEND_FE(hg_i2c_read, NULL)
	ZEND_FE(hg_i2c_write, NULL)
#endif
	{NULL, NULL, NULL}
};

#define INI_DEFAULT(name,value)\
	ZVAL_NEW_STR(&tmp, zend_string_init(value, sizeof(value)-1, 1));\
	zend_hash_str_update(configuration_hash, name, sizeof(name)-1, &tmp);\

static void homegear_ini_defaults(HashTable *configuration_hash)
{
	zval tmp;
	INI_DEFAULT("date.timezone", "Europe/Berlin");
	INI_DEFAULT("report_zend_debug", "0");
	INI_DEFAULT("display_errors", "1");
}

static zend_module_entry homegear_module_entry = {
        STANDARD_MODULE_HEADER,
        "homegear",
        homegear_functions,
        PHP_MINIT(homegear),
        PHP_MSHUTDOWN(homegear),
        PHP_RINIT(homegear),
        PHP_RSHUTDOWN(homegear),
        PHP_MINFO(homegear),
        VERSION,
        STANDARD_MODULE_PROPERTIES
};

static sapi_module_struct php_homegear_sapi_module = {
	(char*)"homegear",                       /* name */
	(char*)"PHP Homegear Library",        /* pretty name */

	php_homegear_startup,              /* startup == MINIT. Called for each new thread. */
	php_module_shutdown_wrapper,   /* shutdown == MSHUTDOWN. Called for each new thread. */

	php_homegear_activate,	            /* activate == RINIT. Called for each request. */
	php_homegear_deactivate,  			/* deactivate == RSHUTDOWN. Called for each request. */

	php_homegear_ub_write,             /* unbuffered write */
	php_homegear_flush,                /* flush */
	NULL,                          /* get uid */
	NULL,                          /* getenv */

	php_error,                     /* error handler */

	NULL,                          /* header handler */
	php_homegear_send_headers,                          /* send headers handler */
	NULL,          /* send header handler */

	php_homegear_read_post,            /* read POST data */
	php_homegear_read_cookies,         /* read Cookies */

	php_homegear_register_variables,   /* register server variables */
	php_homegear_log_message,          /* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
};

void pthread_data_destructor(void* data)
{
	if(data) delete (zend_homegear_globals*)data;
}

zend_homegear_globals* php_homegear_get_globals()
{
	zend_homegear_globals* data = (zend_homegear_globals*)pthread_getspecific(pthread_key);
	if(!data)
	{
		data = new zend_homegear_globals();
		if(!data || pthread_setspecific(pthread_key, data) != 0)
		{
			GD::out.printCritical("Critical: Could not set PHP globals data - out of memory?.");
			if(data) delete data;
			data = nullptr;
			return data;
		}
		data->id = 0;
		data->webRequest = false;
		data->commandLine = false;
		data->cookiesParsed = false;
		data->peerId = 0;
	}
	return data;
}

void php_homegear_build_argv(std::vector<std::string>& arguments)
{
	if(_disposed) return;
	zval argc;
	zval argv;
	array_init(&argv);

	for(std::vector<std::string>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
	{
		zval arg;
		ZVAL_STR(&arg, zend_string_init(i->c_str(), i->size(), 0)); //i->size does not contain \0, so no "-1"
		if (zend_hash_next_index_insert(Z_ARRVAL_P(&argv), &arg) == NULL) {
			if (Z_TYPE(arg) == IS_STRING) {
				zend_string_release(Z_STR(arg));
			}
		}
	}

	ZVAL_LONG(&argc, arguments.size());
	zend_hash_update(&EG(symbol_table), zend_string_init("argv", sizeof("argv") - 1, 0), &argv);
	zend_hash_update(&EG(symbol_table), zend_string_init("argc", sizeof("argc") - 1, 0), &argc);

	//zval_ptr_dtor(&argc); //Not needed
	//zval_ptr_dtor(&argv);
}

static size_t php_homegear_read_post(char *buf, size_t count_bytes)
{
	if(_disposed || SEG(commandLine)) return 0;
	BaseLib::Http* http = &SEG(http);
	if(!http || http->getContentSize() == 0) return 0;
	size_t bytesRead = http->readContentStream(buf, count_bytes);
	if(GD::bl->debugLevel >= 5 && bytesRead > 0) GD::out.printDebug("Debug: Raw post data: " + std::string(buf, bytesRead));
	return bytesRead;
}

static char* php_homegear_read_cookies()
{
	if(_disposed || SEG(commandLine)) return 0;
	BaseLib::Http* http = &SEG(http);
	if(!http) return 0;
	if(!SEG(cookiesParsed))
	{
		SEG(cookiesParsed) = true;
		//Conversion from (const char*) to (char*) is ok, because PHP makes a copy before parsing and does not change the original data.
		if(!http->getHeader().cookie.empty()) return (char*)http->getHeader().cookie.c_str();
	}
	return NULL;
}

static size_t php_homegear_ub_write(const char* str, size_t length)
{
	if(length == 0 || _disposed) return 0;
	std::string output(str, length);
	zend_homegear_globals* globals = php_homegear_get_globals();
	if(globals->outputCallback) globals->outputCallback(output);
	else
	{
		if(SEG(peerId) != 0) GD::out.printMessage("Script output (peer id: " + std::to_string(SEG(peerId)) + "): " + output);
		else GD::out.printMessage("Script output: " + output);
	}
	return length;
}

static void php_homegear_flush(void *server_context)
{
	//We are storing all data, so no flush is needed.
}

static int php_homegear_send_headers(sapi_headers_struct* sapi_headers)
{
	if(_disposed || SEG(commandLine)) return SAPI_HEADER_SENT_SUCCESSFULLY;
	if(!sapi_headers || !SEG(sendHeadersCallback)) return SAPI_HEADER_SEND_FAILED;
	BaseLib::PVariable headers(new BaseLib::Variable(BaseLib::VariableType::tStruct));
	if(!SEG(webRequest)) return SAPI_HEADER_SENT_SUCCESSFULLY;
	if(sapi_headers->http_status_line)
	{
		std::string status(sapi_headers->http_status_line);
		std::vector<std::string> elements = BaseLib::HelperFunctions::splitAll(status, ' ');
		if(elements.size() < 2) headers->structValue->insert(BaseLib::StructElement("RESPONSE_CODE", BaseLib::PVariable(new BaseLib::Variable(BaseLib::Math::getNumber(elements.at(1))))));
		else headers->structValue->insert(BaseLib::StructElement("RESPONSE_CODE", BaseLib::PVariable(new BaseLib::Variable(500))));
	}
	else headers->structValue->insert(BaseLib::StructElement("RESPONSE_CODE", BaseLib::PVariable(new BaseLib::Variable(200))));
	zend_llist_element* element = sapi_headers->headers.head;
	while(element)
	{
		sapi_header_struct* header = (sapi_header_struct*)element->data;
		std::string temp(header->header, header->header_len);
		//PHP returns this sometimes
		if(temp.compare(0, 22, "Content-type: ext/html") == 0)
		{
			if(GD::bl->settings.devLog())
			{
				GD::out.printError("PHP Content-Type error.");
				if(SG(default_mimetype)) GD::out.printError("Default MIME type is: " + std::string(SG(default_mimetype)));
			}
			temp = "Content-Type: text/html; charset=UTF-8";
		}
		std::pair<std::string, std::string> dataPair = BaseLib::HelperFunctions::splitFirst(temp, ':');
		headers->structValue->insert(BaseLib::StructElement(BaseLib::HelperFunctions::trim(dataPair.first), BaseLib::PVariable(new BaseLib::Variable(BaseLib::HelperFunctions::trim(dataPair.second)))));
		element = element->next;
	}
	SEG(sendHeadersCallback)(headers);
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}

/*static int php_homegear_header_handler(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers)
{
	std::cerr << "php_homegear_header_handler: " << std::string(sapi_header.header, sapi_header.header_len) << std::endl;
	std::vector<char>* out = SEG(output);
	if(!out) return FAILURE;
	switch(op)
	{
		case SAPI_HEADER_DELETE:
			return SUCCESS;
		case SAPI_HEADER_DELETE_ALL:
			out->clear();
			return SUCCESS;
		case SAPI_HEADER_ADD:
		case SAPI_HEADER_REPLACE:
			if (!memchr(sapi_header.header, ':', sapi_header.header_len))
			{
				sapi_free_header(sapi_header);
				return SUCCESS;
			}
			if(out->size() + sapi_header.header_len + 4 > out->capacity()) out->reserve(out->capacity() + 1024);
			out->insert(out->end(), sapi_header.header, sapi_header.header + sapi_header.header_len);
			out->push_back('\r');
			out->push_back('\n');
			return SAPI_HEADER_ADD;
		default:
			return SUCCESS;
	}
}*/

/*static void php_homegear_send_header(sapi_header_struct *sapi_header, void *server_context)
{
	if(!sapi_header) return;
	std::vector<char>* out = SEG(output);
	if(out)
	{
		if(out->size() + sapi_header.header_len + 4 > out->capacity()) out->reserve(out->capacity() + 1024);
		out->insert(out->end(), sapi_header.header, sapi_header.header + sapi_header.header_len);
		out->push_back('\r');
		out->push_back('\n');
		std::cerr << "php_homegear_send_header: " << std::string(sapi_header.header, sapi_header.header_len) << std::endl;
	}
}*/

static void php_homegear_log_message(char* message)
{
	if(_disposed) return;
	std::string pathTranslated;
	if(SG(request_info).path_translated) pathTranslated = SG(request_info).path_translated;
	GD::out.printError("Scriptengine" + (SG(request_info).path_translated ? std::string(" (") + std::string(SG(request_info).path_translated) + "): " : ": ") + std::string(message));
}

static void php_homegear_register_variables(zval* track_vars_array)
{
	if(_disposed) return;
	if(SEG(commandLine))
	{
		if(SG(request_info).path_translated)
		{
			php_register_variable((char*)"PHP_SELF", (char*)SG(request_info).path_translated, track_vars_array);
			php_register_variable((char*)"SCRIPT_NAME", (char*)SG(request_info).path_translated, track_vars_array);
			php_register_variable((char*)"SCRIPT_FILENAME", (char*)SG(request_info).path_translated, track_vars_array);
			php_register_variable((char*)"PATH_TRANSLATED", (char*)SG(request_info).path_translated, track_vars_array);
		}
		php_import_environment_variables(track_vars_array);
	}
	else
	{
		BaseLib::Http* http = &SEG(http);
		if(!http) return;
		BaseLib::Http::Header& header = http->getHeader();
		BaseLib::Rpc::ServerInfo::Info* server = (BaseLib::Rpc::ServerInfo::Info*)SG(server_context);
		zval value;

		//PHP makes copies of all values, so conversion from const to non-const is ok.
		if(server)
		{
			if(server->ssl) php_register_variable_safe((char*)"HTTPS", (char*)"on", 2, track_vars_array);
			else php_register_variable_safe((char*)"HTTPS", (char*)"", 0, track_vars_array);
			std::string connection = (header.connection & BaseLib::Http::Connection::keepAlive) ? "keep-alive" : "close";
			php_register_variable_safe((char*)"HTTP_CONNECTION", (char*)connection.c_str(), connection.size(), track_vars_array);
			php_register_variable_safe((char*)"DOCUMENT_ROOT", (char*)server->contentPath.c_str(), server->contentPath.size(), track_vars_array);
			std::string filename = server->contentPath;
			filename += (!header.path.empty() && header.path.front() == '/') ? header.path.substr(1) : header.path;
			php_register_variable_safe((char*)"SCRIPT_FILENAME", (char*)filename.c_str(), filename.size(), track_vars_array);
			php_register_variable_safe((char*)"SERVER_NAME", (char*)server->name.c_str(), server->name.size(), track_vars_array);
			php_register_variable_safe((char*)"SERVER_ADDR", (char*)server->address.c_str(), server->address.size(), track_vars_array);
			ZVAL_LONG(&value, server->port);
			php_register_variable_ex((char*)"SERVER_PORT", &value, track_vars_array);
			std::string authType = (server->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::basic) ? "basic" : "none";
			php_register_variable_safe((char*)"AUTH_TYPE", (char*)authType.c_str(), authType.size(), track_vars_array);
			std::string webSocketAuthType = (server->websocketAuthType == BaseLib::Rpc::ServerInfo::Info::AuthType::basic) ? "basic" : ((server->websocketAuthType == BaseLib::Rpc::ServerInfo::Info::AuthType::session) ? "session" : "none");
			php_register_variable_safe((char*)"WEBSOCKET_AUTH_TYPE", (char*)webSocketAuthType.c_str(), webSocketAuthType.size(), track_vars_array);
		}

		std::string version = std::string("Homegear ") + VERSION;
		php_register_variable_safe((char*)"SERVER_SOFTWARE", (char*)version.c_str(), version.size(), track_vars_array);
		php_register_variable_safe((char*)"SCRIPT_NAME", (char*)header.path.c_str(), header.path.size(), track_vars_array);
		std::string phpSelf = header.path + header.pathInfo;
		php_register_variable_safe((char*)"PHP_SELF", (char*)phpSelf.c_str(), phpSelf.size(), track_vars_array);
		if(!header.pathInfo.empty())
		{
			php_register_variable_safe((char*)"PATH_INFO", (char*)header.pathInfo.c_str(), header.pathInfo.size(), track_vars_array);
			if(SG(request_info).path_translated) php_register_variable((char*)"PATH_TRANSLATED", (char*)SG(request_info).path_translated, track_vars_array);
		}
		php_register_variable_safe((char*)"HTTP_HOST", (char*)header.host.c_str(), header.host.size(), track_vars_array);
		php_register_variable_safe((char*)"QUERY_STRING", (char*)header.args.c_str(), header.args.size(), track_vars_array);
		php_register_variable_safe((char*)"SERVER_PROTOCOL", (char*)"HTTP/1.1", 8, track_vars_array);
		php_register_variable_safe((char*)"REMOTE_ADDR", (char*)header.remoteAddress.c_str(), header.remoteAddress.size(), track_vars_array);
		ZVAL_LONG(&value, header.remotePort);
		php_register_variable_ex((char*)"REMOTE_PORT", &value, track_vars_array);
		if(SG(request_info).request_method) php_register_variable((char*)"REQUEST_METHOD", (char*)SG(request_info).request_method, track_vars_array);
		if(SG(request_info).request_uri) php_register_variable((char*)"REQUEST_URI", SG(request_info).request_uri, track_vars_array);
		if(header.contentLength > 0)
		{
			ZVAL_LONG(&value, header.contentLength);
			php_register_variable_ex((char*)"CONTENT_LENGTH", &value, track_vars_array);
		}
		if(!header.contentType.empty()) php_register_variable_safe((char*)"CONTENT_TYPE", (char*)header.contentType.c_str(), header.contentType.size(), track_vars_array);
		for(std::map<std::string, std::string>::const_iterator i = header.fields.begin(); i != header.fields.end(); ++i)
		{
			std::string name = "HTTP_" + i->first;
			BaseLib::HelperFunctions::stringReplace(name, "-", "_");
			BaseLib::HelperFunctions::toUpper(name);
			php_register_variable_safe((char*)name.c_str(), (char*)i->second.c_str(), i->second.size(), track_vars_array);
		}
		zval_ptr_dtor(&value);
	}
}

void php_homegear_invoke_rpc(std::string& methodName, BaseLib::PVariable& parameters, zval* return_value)
{
	if(!SEG(rpcCallback)) RETURN_FALSE;
	if(!parameters) parameters.reset(new BaseLib::Variable(BaseLib::VariableType::tArray));
	BaseLib::PVariable result = SEG(rpcCallback)(methodName, parameters);
	if(result->errorStruct)
	{
		zend_throw_exception(homegear_exception_class_entry, result->structValue->at("faultString")->stringValue.c_str(), result->structValue->at("faultCode")->integerValue);
		RETURN_NULL()
	}
	PhpVariableConverter::getPHPVariable(result, return_value);
}

/* RPC functions */
ZEND_FUNCTION(hg_get_script_id)
{
	std::string id = std::to_string(SEG(id)) + ',' + SEG(token);
	ZVAL_STRINGL(return_value, id.c_str(), id.size());
}

ZEND_FUNCTION(hg_register_thread)
{
	if(_disposed) RETURN_FALSE;
	char* pTokenPair = nullptr;
	int tokenPairLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pTokenPair, &tokenPairLength) != SUCCESS) RETURN_FALSE;
	int32_t scriptId;
	std::string tokenPairString(pTokenPair, tokenPairLength);
	std::pair<std::string, std::string> tokenPair = BaseLib::HelperFunctions::splitFirst(tokenPairString, ',');
	scriptId = BaseLib::Math::getNumber(tokenPair.first, false);
	std::string token = tokenPair.second;
	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::lock_guard<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(scriptId);
		if(eventsIterator == PhpEvents::eventsMap.end() || !eventsIterator->second || eventsIterator->second->getToken().empty() || eventsIterator->second->getToken() != token) RETURN_FALSE
		phpEvents = eventsIterator->second;
	}
	SEG(id) = scriptId;
	SEG(token) = token;
	SEG(outputCallback) = phpEvents->getOutputCallback();
	SEG(rpcCallback) = phpEvents->getRpcCallback();
	RETURN_TRUE
}

ZEND_FUNCTION(hg_invoke)
{
	if(_disposed) RETURN_NULL();
	if(SEG(id) == 0)
	{
		zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
		RETURN_FALSE
	}
	char* pMethodName = nullptr;
	int methodNameLength = 0;
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s*", &pMethodName, &methodNameLength, &args, &argc) != SUCCESS) RETURN_NULL();
	if(methodNameLength == 0) RETURN_NULL();
	std::string methodName(pMethodName, methodNameLength);
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	for(int32_t i = 0; i < argc; i++)
	{
		BaseLib::PVariable parameter = PhpVariableConverter::getVariable(&args[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_meta)
{
	if(_disposed) RETURN_NULL();
	unsigned long id = 0;
	char* pName = nullptr;
	int nameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &id, &pName, &nameLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_NULL();
	std::string methodName("getMetadata");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_system)
{
	if(_disposed) RETURN_NULL();
	char* pName = nullptr;
	int nameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s",&pName, &nameLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_NULL();
	std::string methodName("getSystemVariable");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_value)
{
	if(_disposed) RETURN_NULL();
	unsigned long id = 0;
	long channel = -1;
	char* pParameterName = nullptr;
	int parameterNameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lls", &id, &channel, &pParameterName, &parameterNameLength) != SUCCESS) RETURN_NULL();
	if(parameterNameLength == 0) RETURN_NULL();
	std::string methodName("getValue");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)channel)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pParameterName, parameterNameLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_set_meta)
{
	if(_disposed) RETURN_NULL();
	unsigned long id = 0;
	char* pName = nullptr;
	int nameLength = 0;
	zval* newValue = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lsz", &id, &pName, &nameLength, &newValue) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_FALSE;
	std::string methodName("setMetadata");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	BaseLib::PVariable parameter = PhpVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_set_system)
{
	if(_disposed) RETURN_NULL();
	char* pName = nullptr;
	int nameLength = 0;
	zval* newValue = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &pName, &nameLength, &newValue) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_FALSE;
	std::string methodName("setSystemVariable");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	BaseLib::PVariable parameter = PhpVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_set_value)
{
	if(_disposed) RETURN_NULL();
	unsigned long id = 0;
	long channel = -1;
	char* pParameterName = nullptr;
	int parameterNameLength = 0;
	zval* newValue = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llsz", &id, &channel, &pParameterName, &parameterNameLength, &newValue) != SUCCESS) RETURN_NULL();
	if(parameterNameLength == 0) RETURN_FALSE;
	std::string methodName("setValue");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)channel)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pParameterName, parameterNameLength))));
	BaseLib::PVariable parameter = PhpVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

// {{{ Module functions
	ZEND_FUNCTION(hg_list_modules)
	{
		if(_disposed) RETURN_NULL();

		std::string methodName("listModules");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_load_module)
	{
		if(_disposed) RETURN_NULL();
		char* pFilename = nullptr;
		int filenameLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pFilename, &filenameLength) != SUCCESS) RETURN_NULL();
		if(filenameLength == 0)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		std::string methodName("loadModule");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pFilename, filenameLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_unload_module)
	{
		if(_disposed) RETURN_NULL();
		char* pFilename = nullptr;
		int filenameLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pFilename, &filenameLength) != SUCCESS) RETURN_NULL();
		if(filenameLength == 0)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		std::string methodName("unloadModule");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pFilename, filenameLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_reload_module)
	{
		if(_disposed) RETURN_NULL();
		char* pFilename = nullptr;
		int filenameLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pFilename, &filenameLength) != SUCCESS) RETURN_NULL();
		if(filenameLength == 0)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		std::string methodName("reloadModule");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pFilename, filenameLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

// }}}

// {{{ User functions

	ZEND_FUNCTION(hg_auth)
	{
		if(_disposed) RETURN_NULL();
		char* pName = nullptr;
		int nameLength = 0;
		char* pPassword = nullptr;
		int passwordLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
		if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
		std::string methodName("auth");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pPassword, passwordLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_create_user)
	{
		if(_disposed) RETURN_NULL();
		char* pName = nullptr;
		int nameLength = 0;
		char* pPassword = nullptr;
		int passwordLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
		if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
		std::string methodName("createUser");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pPassword, passwordLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_delete_user)
	{
		if(_disposed) RETURN_NULL();
		char* pName = nullptr;
		int nameLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pName, &nameLength) != SUCCESS) RETURN_NULL();
		if(nameLength == 0) RETURN_FALSE;
		std::string methodName("deleteUser");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_update_user)
	{
		if(_disposed) RETURN_NULL();
		char* pName = nullptr;
		int nameLength = 0;
		char* pPassword = nullptr;
		int passwordLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
		if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
		std::string methodName("updateUser");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pPassword, passwordLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_user_exists)
	{
		if(_disposed) RETURN_NULL();
		char* pName = nullptr;
		int nameLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pName, &nameLength) != SUCCESS) RETURN_NULL();
		if(nameLength == 0) RETURN_FALSE;
		std::string methodName("userExists");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}

	ZEND_FUNCTION(hg_users)
	{
		if(_disposed) RETURN_NULL();
		std::string methodName("listUsers");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		php_homegear_invoke_rpc(methodName, parameters, return_value);
	}
// }}}

ZEND_FUNCTION(hg_poll_event)
{
	if(_disposed) RETURN_NULL();
	if(SEG(id) == 0)
	{
		zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
		RETURN_FALSE
	}
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	int32_t timeout = -1;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::pollEvent().");
	else if(argc >= 1)
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "timeout is not of type int.");
		else timeout = Z_LVAL(args[0]);
	}

	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::lock_guard<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end())
		{
			eventsMapGuard.~lock_guard();
			zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
			RETURN_FALSE
		}
		if(!eventsIterator->second) eventsIterator->second.reset(new PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
		phpEvents = eventsIterator->second;
	}
	std::shared_ptr<PhpEvents::EventData> eventData = phpEvents->poll(timeout);
	if(eventData)
	{
		array_init(return_value);
		zval element;

		if(!eventData->type.empty())
		{
			ZVAL_STRINGL(&element, eventData->type.c_str(), eventData->type.size());
			add_assoc_zval_ex(return_value, "TYPE", sizeof("TYPE") - 1, &element);
		}

		if(eventData->id != 0)
		{
			ZVAL_LONG(&element, eventData->id);
			add_assoc_zval_ex(return_value, "PEERID", sizeof("PEERID") - 1, &element);
		}

		if(eventData->channel > -1)
		{
			ZVAL_LONG(&element, eventData->channel);
			add_assoc_zval_ex(return_value, "CHANNEL", sizeof("CHANNEL") - 1, &element);
		}

		if(!eventData->variable.empty())
		{
			ZVAL_STRINGL(&element, eventData->variable.c_str(), eventData->variable.size());
			add_assoc_zval_ex(return_value, "VARIABLE", sizeof("VARIABLE") - 1, &element);
		}

		if(eventData->hint != -1)
		{
			ZVAL_LONG(&element, eventData->hint);
			add_assoc_zval_ex(return_value, "HINT", sizeof("HINT") - 1, &element);
		}

		if(eventData->value)
		{
			PhpVariableConverter::getPHPVariable(eventData->value, &element);
			add_assoc_zval_ex(return_value, "VALUE", sizeof("VALUE") - 1, &element);
		}
	}
	else RETURN_FALSE
}

ZEND_FUNCTION(hg_list_rpc_clients)
{
	if(_disposed) RETURN_NULL();
	std::string methodName("listRpcClients");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_peer_exists)
{
	if(_disposed) RETURN_NULL();
	unsigned long peerId = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &peerId) != SUCCESS) RETURN_NULL();
	std::string methodName("peerExists");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)peerId)));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_subscribe_peer)
{
	if(_disposed) RETURN_NULL();
	if(SEG(id) == 0)
	{
		zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
		RETURN_FALSE
	}
	long peerId = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &peerId) != SUCCESS) RETURN_NULL();
	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::lock_guard<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end())
		{
			eventsMapGuard.~lock_guard();
			zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
		}
		if(!eventsIterator->second) eventsIterator->second.reset(new PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
		phpEvents = eventsIterator->second;
	}
	phpEvents->addPeer(peerId);
}

ZEND_FUNCTION(hg_unsubscribe_peer)
{
	if(_disposed) RETURN_NULL();
	if(SEG(id) == 0)
	{
		zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
		RETURN_FALSE
	}
	long peerId = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &peerId) != SUCCESS) RETURN_NULL();
	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::lock_guard<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end())
		{
			eventsMapGuard.~lock_guard();
			zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
		}
		if(!eventsIterator->second) eventsIterator->second.reset(new PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
		phpEvents = eventsIterator->second;
	}
	phpEvents->removePeer(peerId);
}

ZEND_FUNCTION(hg_log)
{
	if(_disposed) RETURN_NULL();
	long debugLevel = 3;
	char* pMessage = nullptr;
	int messageLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &debugLevel, &pMessage, &messageLength) != SUCCESS) RETURN_NULL();
	if(messageLength == 0) RETURN_FALSE;
	if(SEG(peerId) != 0) GD::out.printMessage("Script log (peer id: " + std::to_string(SEG(peerId)) + "): " + std::string(pMessage, messageLength), debugLevel, true);
	else GD::out.printMessage("Script log: " + std::string(pMessage, messageLength), debugLevel, true);
	RETURN_TRUE;
}

ZEND_FUNCTION(hg_check_license)
{
	if(_disposed) RETURN_NULL();
	long moduleId = -1;
	long familyId = -1;
	long deviceId = -1;
	char* pLicenseKey = nullptr;
	int licenseKeyLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "llls", &moduleId, &familyId, &deviceId, &pLicenseKey, &licenseKeyLength) != SUCCESS) RETURN_NULL();
	std::string methodName("checkLicense");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)moduleId)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)familyId)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)deviceId)));
	if(licenseKeyLength > 0) parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pLicenseKey, licenseKeyLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_remove_license)
{
	if(_disposed) RETURN_NULL();
	long moduleId = -1;
	long familyId = -1;
	long deviceId = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "lll", &moduleId, &familyId, &deviceId) != SUCCESS) RETURN_NULL();
	std::string methodName("removeLicense");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)moduleId)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)familyId)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)deviceId)));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_license_states)
{
	if(_disposed) RETURN_NULL();
	long moduleId = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &moduleId) != SUCCESS) RETURN_NULL();
	std::string methodName("getLicenseStates");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)moduleId)));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_shutting_down)
{
	if(_disposed || GD::bl->shuttingDown) RETURN_TRUE
	RETURN_FALSE
}

ZEND_FUNCTION(hg_gpio_open)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->openDevice(gpio, false);
}

ZEND_FUNCTION(hg_gpio_close)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->closeDevice(gpio);
}

ZEND_FUNCTION(hg_gpio_set_direction)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	long direction = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &gpio, &direction) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->setDirection(gpio, (BaseLib::Gpio::GpioDirection::Enum)direction);
}

ZEND_FUNCTION(hg_gpio_set_edge)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	long edge = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &gpio, &edge) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->setEdge(gpio, (BaseLib::Gpio::GpioEdge::Enum)edge);
}

ZEND_FUNCTION(hg_gpio_get)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
	if(_superglobals.gpio->get(gpio))
	{
		RETURN_TRUE;
	}
	else
	{
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(hg_gpio_set)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	zend_bool value = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "lb", &gpio, &value) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->set(gpio, (bool)value);
}

ZEND_FUNCTION(hg_gpio_poll)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	long timeout = -1;
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll*", &gpio, &timeout, &args, &argc) != SUCCESS) RETURN_NULL();

	bool debounce = false;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearGpio::poll().");
	else if(argc >= 1)
	{
		if(Z_TYPE(args[0]) != IS_TRUE && Z_TYPE(args[0]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "debounce is not of type bool.");
		else debounce = Z_TYPE(args[0]) == IS_TRUE;
	}

	if(gpio < 0 || timeout < 0)
	{
		ZVAL_LONG(return_value, -1);
		return;
	}

	ZVAL_LONG(return_value, _superglobals.gpio->poll(gpio, timeout, debounce));
}

ZEND_FUNCTION(hg_serial_open)
{
	try
	{
		if(_disposed) RETURN_NULL();
		char* pDevice = nullptr;
		int deviceLength = 0;
		long baudrate = 38400;
		bool evenParity = false;
		bool oddParity = false;
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "s*", &pDevice, &deviceLength, &args, &argc) != SUCCESS) RETURN_NULL();
		if(deviceLength == 0)
		{
			RETURN_FALSE;
		}
		std::string device(pDevice, deviceLength);
		if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearSerial::open().");
		else if(argc >= 1)
		{
			if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "baudrate is not of type integer.");
			else baudrate = Z_LVAL(args[0]);

			if(argc >= 2)
			{
				if(Z_TYPE(args[1]) != IS_TRUE && Z_TYPE(args[1]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "evenParity is not of type boolean.");
				else
				{
					evenParity = Z_TYPE(args[1]) == IS_TRUE;
				}

				if(argc == 3)
				{
					if(Z_TYPE(args[2]) != IS_TRUE && Z_TYPE(args[2]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "oddParity is not of type boolean.");
					else
					{
						oddParity = Z_TYPE(args[2]) == IS_TRUE;
					}
				}
			}
		}

		std::shared_ptr<BaseLib::SerialReaderWriter> serialDevice(new BaseLib::SerialReaderWriter(GD::bl.get(), device, baudrate, 0, true, -1));
		serialDevice->openDevice(evenParity, oddParity, false);
		if(serialDevice->isOpen())
		{
			int32_t descriptor = serialDevice->fileDescriptor()->descriptor;
			_superglobals.serialDevicesMutex.lock();
			_superglobals.serialDevices.insert(std::pair<int, std::shared_ptr<BaseLib::SerialReaderWriter>>(descriptor, serialDevice));
			_superglobals.serialDevicesMutex.unlock();
			ZVAL_LONG(return_value, descriptor);
		}
		else
		{
			RETURN_FALSE;
		}
	}
	catch(BaseLib::SerialReaderWriterException& ex)
	{
		GD::out.printError("Script engine: " + ex.what());
		ZVAL_LONG(return_value, -1);
	}
}

ZEND_FUNCTION(hg_serial_close)
{
	try
	{
		if(_disposed) RETURN_NULL();
		long id = -1;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &id) != SUCCESS) RETURN_NULL();
		_superglobals.serialDevicesMutex.lock();
		std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = _superglobals.serialDevices.find(id);
		if(deviceIterator != _superglobals.serialDevices.end())
		{
			if(deviceIterator->second) deviceIterator->second->closeDevice();
			_superglobals.serialDevices.erase(id);
		}
		_superglobals.serialDevicesMutex.unlock();
		RETURN_TRUE;
	}
	catch(BaseLib::SerialReaderWriterException& ex)
	{
		GD::out.printError("Script engine: " + ex.what());
		RETURN_FALSE;
	}
}

ZEND_FUNCTION(hg_serial_read)
{
	try
	{
		if(_disposed) RETURN_NULL();
		long id = -1;
		long timeout = -1;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &id, &timeout) != SUCCESS) RETURN_NULL();
		if(timeout < 0)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		if(timeout > 5000) timeout = 5000;
		std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;
		_superglobals.serialDevicesMutex.lock();
		std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = _superglobals.serialDevices.find(id);
		if(deviceIterator != _superglobals.serialDevices.end())
		{
			if(deviceIterator->second) serialReaderWriter = deviceIterator->second;
		}
		_superglobals.serialDevicesMutex.unlock();
		if(!serialReaderWriter)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		char data;
		int32_t result = serialReaderWriter->readChar(data, timeout * 1000);
		if(result == -1)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		else if(result == 1)
		{
			ZVAL_LONG(return_value, -2);
			return;
		}
		ZVAL_STRINGL(return_value, &data, 1);
	}
	catch(BaseLib::SerialReaderWriterException& ex)
	{
		GD::out.printError("Script engine: " + ex.what());
		ZVAL_LONG(return_value, -1);
	}
}

ZEND_FUNCTION(hg_serial_readline)
{
	try
	{
		if(_disposed) RETURN_NULL();
		long id = -1;
		long timeout = -1;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &id, &timeout) != SUCCESS) RETURN_NULL();
		if(timeout < 0)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		if(timeout > 5000) timeout = 5000;
		std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;
		_superglobals.serialDevicesMutex.lock();
		std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = _superglobals.serialDevices.find(id);
		if(deviceIterator != _superglobals.serialDevices.end())
		{
			if(deviceIterator->second) serialReaderWriter = deviceIterator->second;
		}
		_superglobals.serialDevicesMutex.unlock();
		if(!serialReaderWriter)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		std::string data;
		int32_t result = serialReaderWriter->readLine(data, timeout * 1000);
		if(result == -1)
		{
			ZVAL_LONG(return_value, -1);
			return;
		}
		else if(result == 1)
		{
			ZVAL_LONG(return_value, -2);
			return;
		}
		if(data.empty()) ZVAL_STRINGL(return_value, "", 0); //At least once, input->stringValue.c_str() on an empty string was a nullptr causing a segementation fault, so check for empty string
		else ZVAL_STRINGL(return_value, data.c_str(), data.size());
	}
	catch(BaseLib::SerialReaderWriterException& ex)
	{
		GD::out.printError("Script engine: " + ex.what());
		ZVAL_LONG(return_value, -1);
	}
}

ZEND_FUNCTION(hg_serial_write)
{
	try
	{
		if(_disposed) RETURN_NULL();
		long id = -1;
		char* pData = nullptr;
		int dataLength = 0;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &id, &pData, &dataLength) != SUCCESS) RETURN_NULL();
		std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;
		_superglobals.serialDevicesMutex.lock();
		std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = _superglobals.serialDevices.find(id);
		if(deviceIterator != _superglobals.serialDevices.end())
		{
			if(deviceIterator->second) serialReaderWriter = deviceIterator->second;
		}
		_superglobals.serialDevicesMutex.unlock();
		if(!serialReaderWriter) RETURN_FALSE;
		std::string data(pData, dataLength);
		if(data.size() > 0) serialReaderWriter->writeLine(data);
		RETURN_TRUE;
	}
	catch(BaseLib::SerialReaderWriterException& ex)
	{
		GD::out.printError("Script engine: " + ex.what());
		RETURN_FALSE;
	}
}

#ifdef I2CSUPPORT
ZEND_FUNCTION(hg_i2c_open)
{
	if(_disposed) RETURN_NULL();
	char* pDevice = nullptr;
	int deviceLength = 0;
	long address = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "sl", &pDevice, &deviceLength, &address) != SUCCESS) RETURN_NULL();
	std::string device(pDevice, deviceLength);
	int32_t descriptor = open(device.c_str(), O_RDWR);
	if(descriptor == -1) RETURN_FALSE;
	if (ioctl(descriptor, I2C_SLAVE, address) == -1)
	{
		close(descriptor);
		RETURN_FALSE;
	}
	ZVAL_LONG(return_value, descriptor);
}

ZEND_FUNCTION(hg_i2c_close)
{
	if(_disposed) RETURN_NULL();
	long descriptor = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &descriptor) != SUCCESS) RETURN_NULL();
	if(descriptor != -1) close(descriptor);
	RETURN_TRUE;
}

ZEND_FUNCTION(hg_i2c_read)
{
	if(_disposed) RETURN_NULL();
	long descriptor = -1;
	long length = 1;
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll*", &descriptor, &length, &args, &argc) != SUCCESS) RETURN_NULL();
	if(length < 0) RETURN_FALSE;

	bool binary = false;
	if(argc == 1)
	{
		if(Z_TYPE(args[0]) != IS_TRUE && Z_TYPE(args[0]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "returnBinary is not of type bool.");
		else binary = (Z_TYPE(args[0]) == IS_TRUE);
	}
	else if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearI2c::read().");

	std::vector<uint8_t> buffer(length, 0);
	if(read(descriptor, &buffer.at(0), length) != length) RETURN_FALSE;

	if(binary) ZVAL_STRINGL(return_value, (char*)(&buffer[0]), buffer.size());
	else
	{
		std::string hex = BaseLib::HelperFunctions::getHexString(buffer);
		if(hex.empty()) ZVAL_STRINGL(return_value, "", 0); //At least once, input->stringValue.c_str() on an empty string was a nullptr causing a segementation fault, so check for empty string
		else ZVAL_STRINGL(return_value, hex.c_str(), hex.size());
	}
}

ZEND_FUNCTION(hg_i2c_write)
{
	if(_disposed) RETURN_NULL();
	long descriptor = -1;
	char* pData = nullptr;
	int dataLength = 0;
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ls*", &descriptor, &pData, &dataLength, &args, &argc) != SUCCESS) RETURN_NULL();

	bool binary = false;
	if(argc == 1)
	{
		if(Z_TYPE(args[0]) != IS_TRUE && Z_TYPE(args[0]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "dataIsBinary is not of type bool.");
		else binary = (Z_TYPE(args[0]) == IS_TRUE);
	}
	else if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearI2c::write().");

	std::vector<uint8_t> data;
	if(binary) data = std::vector<uint8_t>(pData, pData + dataLength);
	else
	{
		std::string hexData(pData, dataLength);
		data = GD::bl->hf.getUBinary(hexData);
	}
	if (write(descriptor, &data.at(0), data.size()) != (signed)data.size()) RETURN_FALSE;
	RETURN_TRUE;
}
#endif

ZEND_METHOD(Homegear, __call)
{
	if(_disposed) RETURN_NULL();
	char* pMethodName = nullptr;
	int methodNameLength = 0;
	zval* args = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &pMethodName, &methodNameLength, &args) != SUCCESS) RETURN_NULL();
	std::string methodName(std::string(pMethodName, methodNameLength));
	BaseLib::PVariable parameters = PhpVariableConverter::getVariable(args);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_METHOD(Homegear, __callStatic)
{
	if(_disposed) RETURN_NULL();
	char* pMethodName = nullptr;
	int methodNameLength = 0;
	zval* args = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &pMethodName, &methodNameLength, &args) != SUCCESS) RETURN_NULL();
	std::string methodName(std::string(pMethodName, methodNameLength));
	BaseLib::PVariable parameters = PhpVariableConverter::getVariable(args);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_BEGIN_ARG_INFO_EX(php_homegear_two_args, 0, 0, 2)
	ZEND_ARG_INFO(0, arg1)
	ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

static const zend_function_entry homegear_methods[] = {
	ZEND_ME(Homegear, __call, php_homegear_two_args, ZEND_ACC_PUBLIC)
	ZEND_ME(Homegear, __callStatic, php_homegear_two_args, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getScriptId, hg_get_script_id, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(registerThread, hg_register_thread, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(log, hg_log, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(pollEvent, hg_poll_event, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(subscribePeer, hg_subscribe_peer, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(unsubscribePeer, hg_unsubscribe_peer, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(shuttingDown, hg_shutting_down, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

static const zend_function_entry homegear_gpio_methods[] = {
	ZEND_ME_MAPPING(open, hg_gpio_open, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(close, hg_gpio_close, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(setDirection, hg_gpio_set_direction, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(setEdge, hg_gpio_set_edge, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(get, hg_gpio_get, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(set, hg_gpio_set, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(poll, hg_gpio_poll, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

static const zend_function_entry homegear_serial_methods[] = {
	ZEND_ME_MAPPING(open, hg_serial_open, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(close, hg_serial_close, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(read, hg_serial_read, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(readline, hg_serial_readline, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(write, hg_serial_write, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

#ifdef I2CSUPPORT
static const zend_function_entry homegear_i2c_methods[] = {
	ZEND_ME_MAPPING(open, hg_i2c_open, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(close, hg_i2c_close, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(read, hg_i2c_read, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(write, hg_i2c_write, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};
#endif

int php_homegear_init()
{
	_superglobals.http = new BaseLib::Http();
	_superglobals.gpio = new BaseLib::Gpio(GD::bl.get());
	_disposed = false;
	pthread_key_create(&pthread_key, pthread_data_destructor);
	tsrm_startup(20, 1, 0, NULL);
	php_homegear_sapi_module.ini_defaults = homegear_ini_defaults;
	ini_path_override = strndup(GD::bl->settings.phpIniPath().c_str(), GD::bl->settings.phpIniPath().size());
	php_homegear_sapi_module.php_ini_path_override = ini_path_override;
	php_homegear_sapi_module.phpinfo_as_text = 0;
	php_homegear_sapi_module.php_ini_ignore_cwd = 1;
	sapi_startup(&php_homegear_sapi_module);

	ini_entries = (char*)malloc(sizeof(HARDCODED_INI));
	memcpy(ini_entries, HARDCODED_INI, sizeof(HARDCODED_INI));
	php_homegear_sapi_module.ini_entries = ini_entries;

	sapi_module.startup(&php_homegear_sapi_module);

	return SUCCESS;
}

void php_homegear_shutdown()
{
	_disposed = true;
	if(_disposed) return;
	php_homegear_sapi_module.shutdown(&php_homegear_sapi_module);
	sapi_shutdown();

	ts_free_worker_threads();

	tsrm_shutdown();
	if(ini_path_override) free(ini_path_override);
	if(ini_entries) free(ini_entries);
	if(_superglobals.http)
	{
		delete(_superglobals.http);
		_superglobals.http = nullptr;
	}
	if(_superglobals.gpio)
	{
		delete(_superglobals.gpio);
		_superglobals.gpio = nullptr;
	}
	_superglobals.serialDevicesMutex.lock();
	_superglobals.serialDevices.clear();
	_superglobals.serialDevicesMutex.unlock();
}

static int php_homegear_startup(sapi_module_struct* sapi_module)
{
	if(php_module_startup(sapi_module, &homegear_module_entry, 1) == FAILURE) return FAILURE;
	return SUCCESS;
}

static int php_homegear_activate(TSRMLS_D)
{
	return SUCCESS;
}

static int php_homegear_deactivate(TSRMLS_D)
{
	return SUCCESS;
}

static PHP_MINIT_FUNCTION(homegear)
{
	zend_class_entry homegearExceptionCe;
	INIT_CLASS_ENTRY(homegearExceptionCe, "Homegear\\HomegearException", NULL);
	//Register child class inherited from Exception (fetched with "zend_exception_get_default")
	homegear_exception_class_entry = zend_register_internal_class_ex(&homegearExceptionCe, zend_exception_get_default());
	zend_declare_class_constant_long(homegear_exception_class_entry, "UNKNOWN_DEVICE", sizeof("UNKNOWN_DEVICE"), -2);

	zend_class_entry homegearCe;
	INIT_CLASS_ENTRY(homegearCe, "Homegear\\Homegear", homegear_methods);
	homegear_class_entry = zend_register_internal_class(&homegearCe);

	zend_class_entry homegearGpioCe;
	INIT_CLASS_ENTRY(homegearGpioCe, "Homegear\\HomegearGpio", homegear_gpio_methods);
	homegear_gpio_class_entry = zend_register_internal_class(&homegearGpioCe);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "DIRECTION_IN", 12, 0);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "DIRECTION_OUT", 13, 1);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "EDGE_RISING", 11, 0);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "EDGE_FALLING", 12, 1);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "EDGE_BOTH", 9, 2);

	zend_class_entry homegearSerialCe;
	INIT_CLASS_ENTRY(homegearSerialCe, "Homegear\\HomegearSerial", homegear_serial_methods);
	homegear_serial_class_entry = zend_register_internal_class(&homegearSerialCe);

#ifdef I2CSUPPORT
	zend_class_entry homegearI2cCe;
	INIT_CLASS_ENTRY(homegearI2cCe, "Homegear\\HomegearI2c", homegear_i2c_methods);
	homegear_i2c_class_entry = zend_register_internal_class(&homegearI2cCe);
#endif

    return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(homegear)
{
    return SUCCESS;
}

static PHP_RINIT_FUNCTION(homegear)
{
	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(homegear)
{
	return SUCCESS;
}

static PHP_MINFO_FUNCTION(homegear)
{
    php_info_print_table_start();
	php_info_print_table_row(2, "Homegear support", "enabled");
	php_info_print_table_row(2, "Homegear version", VERSION);
	php_info_print_table_end();
}
