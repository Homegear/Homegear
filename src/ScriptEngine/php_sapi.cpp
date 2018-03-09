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

#ifndef NO_SCRIPTENGINE

#include "php_config_fixes.h"
#include "../GD/GD.h"
#include "php_sapi.h"
#include "PhpVariableConverter.h"
#include "../../config.h"

#ifdef I2CSUPPORT
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#endif

#if PHP_VERSION_ID < 70100
#error "PHP 7.2 is required as ZTS in versions 7.0 and 7.1 is broken."
#endif
#if PHP_VERSION_ID >= 70300
#error "PHP 7.3 or greater is not officially supported yet. Please check the following points (only visible in source code) before removing this line."
/*
 * 1. Compare initialization with the initialization in one of the SAPI modules (e. g. "php_embed_init()" in "sapi/embed/php_embed.c").
 * 2. Check if content of hg_stream_open() equals zend_stream_open() in zend_stream.c
 */
#endif

static bool _disposed = true;
static zend_homegear_superglobals _superglobals;
static std::mutex _scriptCacheMutex;
static std::map<std::string, std::shared_ptr<ScriptEngine::CacheInfo>> _scriptCache;

static zend_class_entry* homegear_class_entry = nullptr;
static zend_class_entry* homegear_gpio_class_entry = nullptr;
static zend_class_entry* homegear_serial_class_entry = nullptr;
#ifdef I2CSUPPORT
static zend_class_entry* homegear_i2c_class_entry = nullptr;
#endif

static char* ini_path_override = nullptr;
static char* ini_entries = nullptr;
static const char HARDCODED_INI[] =
	"register_argc_argv=1\n"
	"max_execution_time=0\n"
	"max_input_time=-1\n\0";

static int php_homegear_startup(sapi_module_struct* sapi_globals);
static int php_homegear_shutdown(sapi_module_struct* sapi_globals);
static int php_homegear_activate();
static int php_homegear_deactivate();
static size_t php_homegear_ub_write_string(std::string& string);
static size_t php_homegear_ub_write(const char* str, size_t length);
static void php_homegear_flush(void *server_context);
static int php_homegear_send_headers(sapi_headers_struct* sapi_headers);
static size_t php_homegear_read_post(char *buf, size_t count_bytes);
static char* php_homegear_read_cookies();
static void php_homegear_register_variables(zval* track_vars_array);
#if PHP_VERSION_ID >= 70100
	static void php_homegear_log_message(char* message, int syslog_type_int);
#else
	static void php_homegear_log_message(char* message);
#endif
static PHP_MINIT_FUNCTION(homegear);
static PHP_MSHUTDOWN_FUNCTION(homegear);
static PHP_RINIT_FUNCTION(homegear);
static PHP_RSHUTDOWN_FUNCTION(homegear);
static PHP_MINFO_FUNCTION(homegear);

#define SEG(v) php_homegear_get_globals()->v

ZEND_FUNCTION(print_v);
ZEND_FUNCTION(hg_get_script_id);
ZEND_FUNCTION(hg_register_thread);
ZEND_FUNCTION(hg_list_modules);
ZEND_FUNCTION(hg_load_module);
ZEND_FUNCTION(hg_unload_module);
ZEND_FUNCTION(hg_reload_module);
ZEND_FUNCTION(hg_auth);
ZEND_FUNCTION(hg_create_user);
ZEND_FUNCTION(hg_delete_user);
ZEND_FUNCTION(hg_get_user_metadata);
ZEND_FUNCTION(hg_set_user_metadata);
ZEND_FUNCTION(hg_update_user);
ZEND_FUNCTION(hg_user_exists);
ZEND_FUNCTION(hg_users);
ZEND_FUNCTION(hg_log);
ZEND_FUNCTION(hg_set_script_log_level);
ZEND_FUNCTION(hg_get_http_contents);
ZEND_FUNCTION(hg_download);
ZEND_FUNCTION(hg_check_license);
ZEND_FUNCTION(hg_remove_license);
ZEND_FUNCTION(hg_get_license_states);
ZEND_FUNCTION(hg_poll_event);
ZEND_FUNCTION(hg_list_rpc_clients);
ZEND_FUNCTION(hg_peer_exists);
ZEND_FUNCTION(hg_subscribe_peer);
ZEND_FUNCTION(hg_unsubscribe_peer);
ZEND_FUNCTION(hg_shutting_down);
ZEND_FUNCTION(hg_gpio_export);
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
	ZEND_FE(print_v, NULL)
	ZEND_FE(hg_get_script_id, NULL)
	ZEND_FE(hg_register_thread, NULL)
	ZEND_FE(hg_list_modules, NULL)
	ZEND_FE(hg_load_module, NULL)
	ZEND_FE(hg_unload_module, NULL)
	ZEND_FE(hg_reload_module, NULL)
	ZEND_FE(hg_auth, NULL)
	ZEND_FE(hg_create_user, NULL)
	ZEND_FE(hg_delete_user, NULL)
    ZEND_FE(hg_get_user_metadata, NULL)
    ZEND_FE(hg_set_user_metadata, NULL)
	ZEND_FE(hg_update_user, NULL)
	ZEND_FE(hg_user_exists, NULL)
	ZEND_FE(hg_users, NULL)
	ZEND_FE(hg_log, NULL)
	ZEND_FE(hg_set_script_log_level, NULL)
	ZEND_FE(hg_get_http_contents, NULL)
	ZEND_FE(hg_download, NULL)
	ZEND_FE(hg_check_license, NULL)
	ZEND_FE(hg_remove_license, NULL)
	ZEND_FE(hg_get_license_states, NULL)
	ZEND_FE(hg_poll_event, NULL)
	ZEND_FE(hg_list_rpc_clients, NULL)
	ZEND_FE(hg_peer_exists, NULL)
	ZEND_FE(hg_subscribe_peer, NULL)
	ZEND_FE(hg_unsubscribe_peer, NULL)
	ZEND_FE(hg_shutting_down, NULL)
	ZEND_FE(hg_gpio_export, NULL)
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
	php_homegear_shutdown,             /* shutdown == MSHUTDOWN. Called for each new thread. */

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

int hg_stream_open(const char *filename, zend_file_handle *handle)
{
	std::string file(filename);
	if(file.size() > 3 && (file.compare(file.size() - 4, 4, ".hgs") == 0 || file.compare(file.size() - 4, 4, ".hgn") == 0))
	{
        std::lock_guard<std::mutex> scriptCacheGuard(_scriptCacheMutex);
		zend_homegear_globals* globals = php_homegear_get_globals();
        std::map<std::string, std::shared_ptr<ScriptEngine::CacheInfo>>::iterator scriptIterator = _scriptCache.find(file);
        if(scriptIterator != _scriptCache.end() && scriptIterator->second->lastModified == BaseLib::Io::getFileLastModifiedTime(file))
        {
			globals->additionalStrings.push_front(scriptIterator->second->script);
            handle->type = ZEND_HANDLE_MAPPED;
            handle->handle.fp = nullptr;
            handle->handle.stream.handle = nullptr;
            handle->handle.stream.closer = nullptr;
            memset(&handle->handle.stream.mmap, 0, sizeof(zend_mmap));
            handle->handle.stream.mmap.buf = (char*)globals->additionalStrings.front().c_str(); //String is not modified
            handle->handle.stream.mmap.len = scriptIterator->second->script.size();
            handle->filename = filename;
            handle->opened_path = nullptr;
            handle->free_filename = 0;
			return SUCCESS;
        }
        else
        {
            std::vector<char> data = BaseLib::Io::getBinaryFileContent(file);
            int32_t pos = -1;
            for(uint32_t i = 0; i < 11 && i < data.size(); i++)
            {
                if(data[i] == ' ')
                {
                    pos = (int32_t)i;
                    break;
                }
            }
            if(pos == -1)
            {
                GD::bl->out.printError("Error: License module id is missing in encrypted script file \"" + file + "\"");
                return FAILURE;
            }
            std::string moduleIdString(&data.at(0), static_cast<unsigned long>(pos));
            int32_t moduleId = BaseLib::Math::getNumber(moduleIdString);
            std::vector<char> input(&data.at(static_cast<unsigned long>(pos + 1)), &data.at(data.size() - 1) + 1);
            if(input.empty()) return FAILURE;
            std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
            if(i == GD::licensingModules.end() || !i->second)
            {
                GD::out.printError("Error: Could not decrypt script file. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
                return FAILURE;
            }
            std::shared_ptr<ScriptEngine::CacheInfo> cacheInfo = std::make_shared<ScriptEngine::CacheInfo>();
            i->second->decryptScript(input, cacheInfo->script);
            cacheInfo->lastModified = BaseLib::Io::getFileLastModifiedTime(file);
            if(!cacheInfo->script.empty())
            {
				globals->additionalStrings.push_front(cacheInfo->script);
                _scriptCache[file] = cacheInfo;
                handle->type = ZEND_HANDLE_MAPPED;
                handle->handle.fp = nullptr;
                handle->handle.stream.handle = nullptr;
                handle->handle.stream.closer = nullptr;
                memset(&handle->handle.stream.mmap, 0, sizeof(zend_mmap));
                handle->handle.stream.mmap.buf = (char*)globals->additionalStrings.front().c_str(); //String is not modified
                handle->handle.stream.mmap.len = cacheInfo->script.size();
                handle->filename = filename;
                handle->opened_path = nullptr;
                handle->free_filename = 0;
				return SUCCESS;
            }
			else return FAILURE;
        }
	}
	else
	{
		//{{{ 100% from zend_stream_open in zend_stream.c */
		handle->type = ZEND_HANDLE_FP;
		handle->opened_path = nullptr;
		handle->handle.fp = zend_fopen(filename, &handle->opened_path);
		handle->filename = filename;
		handle->free_filename = 0;
		memset(&handle->handle.stream.mmap, 0, sizeof(zend_mmap));
		//}}}
	}

    return (handle->handle.fp) ? SUCCESS : FAILURE;
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

static size_t php_homegear_ub_write_string(std::string& string)
{
	if(string.empty() || _disposed) return 0;
	zend_homegear_globals* globals = php_homegear_get_globals();
	if(string.size() > 2 && string.at(string.size() - 1) == '\n')
	{
		if(string.at(string.size() - 2) == '\r') string.resize(string.size() - 2);
		else string.resize(string.size() - 1);
	}
	if(globals->outputCallback) globals->outputCallback(string);
	else
	{
		if(SEG(peerId) != 0) GD::out.printMessage("Script output (peer id: " + std::to_string(SEG(peerId)) + "): " + string);
		else GD::out.printMessage("Script output: " + string);
	}
	return string.size();
}

static size_t php_homegear_ub_write(const char* str, size_t length)
{
	if(length == 0 || _disposed) return 0;
	if(length > 2 && *(str + length - 1) == '\n')
	{
		if(*(str + length - 2) == '\r') length -= 2;
		else length -= 1;
	}
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
	headers->structValue->insert(BaseLib::StructElement("RESPONSE_CODE", BaseLib::PVariable(new BaseLib::Variable(sapi_headers->http_response_code))));
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

#if PHP_VERSION_ID >= 70100
static void php_homegear_log_message(char* message, int syslog_type_int)
#else
static void php_homegear_log_message(char* message)
#endif
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
		BaseLib::ScriptEngine::PScriptInfo& scriptInfo = SEG(scriptInfo);
		if(!http || !scriptInfo) return;
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
			filename += (!scriptInfo->relativePath.empty() && scriptInfo->relativePath.front() == '/') ? scriptInfo->relativePath.substr(1) : scriptInfo->relativePath;
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
		php_register_variable_safe((char*)"SCRIPT_NAME", (char*)scriptInfo->relativePath.c_str(), scriptInfo->relativePath.size(), track_vars_array);
		std::string phpSelf = scriptInfo->relativePath + header.pathInfo;
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

void php_homegear_invoke_rpc(std::string& methodName, BaseLib::PVariable& parameters, zval* return_value, bool wait)
{
	if(SEG(id) == 0)
	{
		zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Please call \"registerThread\" before calling any Homegear specific method within threads.", -1);
		RETURN_FALSE
	}
	if(!SEG(rpcCallback)) RETURN_FALSE;
	if(!parameters) parameters.reset(new BaseLib::Variable(BaseLib::VariableType::tArray));
	BaseLib::PVariable result = SEG(rpcCallback)(methodName, parameters, wait);
	if(result->errorStruct)
	{
		zend_throw_exception(homegear_exception_class_entry, result->structValue->at("faultString")->stringValue.c_str(), result->structValue->at("faultCode")->integerValue);
		RETURN_NULL()
	}
	PhpVariableConverter::getPHPVariable(result, return_value);
}

/* RPC functions */
ZEND_FUNCTION(print_v)
{
	if(_disposed) RETURN_NULL();
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	if(argc != 1 && argc != 2)
	{
		php_error_docref(NULL, E_WARNING, "print_v() expects 1 or 2 arguments.");
		RETURN_NULL();
	}
	if(argc >= 2 && Z_TYPE(args[1]) != IS_TRUE && Z_TYPE(args[1]) != IS_FALSE)
	{
		php_error_docref(NULL, E_WARNING, "Parameter \"return\" is not of type boolean.");
		RETURN_NULL();
	}

	bool returnString = (Z_TYPE(args[1]) == IS_TRUE);

	BaseLib::PVariable parameter = PhpVariableConverter::getVariable(&args[0]);
	if(!parameter) RETURN_FALSE;
	std::string result = parameter->print();
	if(returnString) ZVAL_STRINGL(return_value, result.c_str(), result.size());
	else php_homegear_ub_write_string(result);
}

ZEND_FUNCTION(hg_get_script_id)
{
	std::string id = std::to_string(SEG(id)) + ',' + SEG(token);
	ZVAL_STRINGL(return_value, id.c_str(), id.size());
}

ZEND_FUNCTION(hg_register_thread)
{
	if(_disposed) RETURN_FALSE;
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string tokenPairString;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::registerThread().");
	else if(argc >= 1)
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "stringId is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) tokenPairString = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}
	}
	int32_t scriptId;
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
	SEG(logLevel) = phpEvents->getLogLevel();
	SEG(peerId) = phpEvents->getPeerId();
	RETURN_TRUE
}

// {{{ Module functions
	ZEND_FUNCTION(hg_list_modules)
	{
		if(_disposed) RETURN_NULL();

		std::string methodName("listModules");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_load_module)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

		std::string filename;
		if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::loadModule().");
		else if(argc == 1)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) filename = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}
		}
		if(filename.empty())
		{
			php_error_docref(NULL, E_WARNING, "filename must not be empty.");
			ZVAL_LONG(return_value, -1);
			return;
		}

		std::string methodName("loadModule");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(filename)));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_unload_module)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

		std::string filename;
		if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::unloadModule().");
		else if(argc == 1)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) filename = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}
		}
		if(filename.empty())
		{
			php_error_docref(NULL, E_WARNING, "filename must not be empty.");
			ZVAL_LONG(return_value, -1);
			return;
		}

		std::string methodName("unloadModule");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(filename)));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_reload_module)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

		std::string filename;
		if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::reloadModule().");
		else if(argc == 1)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) filename = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}
		}
		if(filename.empty())
		{
			php_error_docref(NULL, E_WARNING, "filename must not be empty.");
			ZVAL_LONG(return_value, -1);
			return;
		}

		std::string methodName("reloadModule");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(filename)));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

// }}}

// {{{ User functions

	ZEND_FUNCTION(hg_auth)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		std::string name;
		std::string password;
		if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::auth().");
		else if(argc == 2)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}

			if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
			else
			{
				if(Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
			}
		}
		if(name.empty() || password.empty()) RETURN_FALSE;

		std::string methodName("auth");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(name)));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(password)));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_create_user)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		std::string name;
		std::string password;
		BaseLib::PVariable groups;
        BaseLib::PVariable metadata;

		if(argc > 4) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::createUser().");
		else if(argc > 3)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}

			if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
			else
			{
				if(Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
			}

			if(Z_TYPE(args[2]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "groups is not of type array.");
			else
			{
				groups = PhpVariableConverter::getVariable(&args[2]);
			}
		}
        if(argc == 4)
        {
            if(Z_TYPE(args[3]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "metadata is not of type array.");
            else
            {
                metadata = PhpVariableConverter::getVariable(&args[3]);
            }
        }

		if(name.empty() || password.empty() || !groups || groups->arrayValue->empty()) RETURN_FALSE;

		std::string methodName("createUser");
		BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
		parameters->arrayValue->reserve(metadata ? 4 : 3);
		parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
		parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(password));
		parameters->arrayValue->push_back(groups);
        if(metadata) parameters->arrayValue->push_back(metadata);
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_delete_user)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		std::string name;
		if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::deleteUser().");
		else if(argc == 1)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}
		}
		if(name.empty()) RETURN_FALSE;

		std::string methodName("deleteUser");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(name)));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

    ZEND_FUNCTION(hg_get_user_metadata)
    {
        if(_disposed) RETURN_NULL();
        int argc = 0;
        zval* args = nullptr;
        if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
        std::string name;

        if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::getUserMetadata().");
        else if(argc == 1)
        {
            if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
            else
            {
                if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
            }
        }
        if(name.empty()) RETURN_FALSE;

        std::string methodName("getUserMetadata");
        BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
        php_homegear_invoke_rpc(methodName, parameters, return_value, true);
    }

    ZEND_FUNCTION(hg_set_user_metadata)
    {
        if(_disposed) RETURN_NULL();
        int argc = 0;
        zval* args = nullptr;
        if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
        std::string name;
        BaseLib::PVariable metadata;

        if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::setUserMetadata().");
        else if(argc == 2)
        {
            if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
            else
            {
                if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
            }

            if(Z_TYPE(args[1]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "metadata is not of type string.");
            else
            {
                metadata = PhpVariableConverter::getVariable(&args[1]);
            }
        }
        if(name.empty() || !metadata) RETURN_FALSE;

        std::string methodName("setUserMetadata");
        BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        parameters->arrayValue->reserve(2);
        parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
        parameters->arrayValue->push_back(metadata);
        php_homegear_invoke_rpc(methodName, parameters, return_value, true);
    }

	ZEND_FUNCTION(hg_update_user)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		std::string name;
		std::string password;
        BaseLib::PVariable groups;
		if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::updateUser().");
		else if(argc == 2)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}

			if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
			else
			{
				if(Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
			}
		}
		else if(argc == 3)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}

			if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
			else
			{
				if(Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
			}

            if(Z_TYPE(args[2]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "groups is not of type array.");
            else
            {
                groups = PhpVariableConverter::getVariable(&args[2]);
            }
		}
		if(name.empty() || password.empty()) RETURN_FALSE;

		std::string methodName("updateUser");
        BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        parameters->arrayValue->reserve(groups ? 3 : 2);
        parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
        parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(password));
        if(groups) parameters->arrayValue->push_back(groups);
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_user_exists)
	{
		if(_disposed) RETURN_NULL();
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		std::string name;
		if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::userExists().");
		else if(argc == 1)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}
		}
		if(name.empty()) RETURN_FALSE;

		std::string methodName("userExists");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(name)));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
	}

	ZEND_FUNCTION(hg_users)
	{
		if(_disposed) RETURN_NULL();
		std::string methodName("listUsers");
		BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
		php_homegear_invoke_rpc(methodName, parameters, return_value, true);
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
	else if(argc == 1)
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "timeout is not of type int.");
		else timeout = Z_LVAL(args[0]);
	}

	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::unique_lock<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end())
		{
			eventsMapGuard.unlock();
			zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
			RETURN_FALSE
		}
		if(!eventsIterator->second) eventsIterator->second = std::make_shared<PhpEvents>(SEG(token), SEG(outputCallback), SEG(rpcCallback));
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

		ZVAL_LONG(&element, eventData->id);
		add_assoc_zval_ex(return_value, "PEERID", sizeof("PEERID") - 1, &element);

		ZVAL_LONG(&element, eventData->channel);
		add_assoc_zval_ex(return_value, "CHANNEL", sizeof("CHANNEL") - 1, &element);

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
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_peer_exists)
{
	if(_disposed) RETURN_NULL();
	unsigned long peerId = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &peerId) != SUCCESS) RETURN_NULL();
	std::string methodName("peerExists");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)peerId)));
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_subscribe_peer)
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
	uint64_t peerId = 0;
	int32_t channel = -1;
	std::string variable;
	if(argc == 0) php_error_docref(NULL, E_WARNING, "Homegear::subscribePeer() expects 1 or 3 parameters.");
	else if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::subscribePeer().");
	else if(argc == 3)
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
		else
		{
			peerId = Z_LVAL(args[0]);
		}

		if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "channel is not of type integer.");
		else
		{
			channel = Z_LVAL(args[1]);
		}

		if(Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "variableName is not of type string.");
		else
		{
			if(Z_STRLEN(args[2]) > 0) variable = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
		}
	}
	else
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
		else
		{
			peerId = Z_LVAL(args[0]);
		}
	}

	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::unique_lock<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end())
		{
			eventsMapGuard.unlock();
			zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
			RETURN_FALSE
		}
		if(!eventsIterator->second) eventsIterator->second.reset(new PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
		phpEvents = eventsIterator->second;
	}
	phpEvents->addPeer(peerId, channel, variable);
}

ZEND_FUNCTION(hg_unsubscribe_peer)
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
	uint64_t peerId = 0;
	int32_t channel = -1;
	std::string variable;
	if(argc == 0) php_error_docref(NULL, E_WARNING, "Homegear::unsubscribePeer() expects 1 or 3 parameters.");
	else if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::unsubscribePeer().");
	else if(argc == 3)
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
		else
		{
			peerId = Z_LVAL(args[0]);
		}

		if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "channel is not of type integer.");
		else
		{
			channel = Z_LVAL(args[1]);
		}

		if(Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "variableName is not of type string.");
		else
		{
			if(Z_STRLEN(args[2]) > 0) variable = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
		}
	}
	else
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
		else
		{
			peerId = Z_LVAL(args[0]);
		}
	}
	std::shared_ptr<PhpEvents> phpEvents;
	{
		std::unique_lock<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end())
		{
			eventsMapGuard.unlock();
			zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
			RETURN_FALSE
		}
		if(!eventsIterator->second) eventsIterator->second.reset(new PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
		phpEvents = eventsIterator->second;
	}
	phpEvents->removePeer(peerId, channel, variable);
}

ZEND_FUNCTION(hg_log)
{
	if(_disposed) RETURN_NULL();
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	int32_t logLevel = 3;
	std::string message;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::log().");
	else if(argc == 2)
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

	bool errorLog = true;
	if(SEG(logLevel) > -1)
	{
		if(logLevel <= SEG(logLevel))
		{
			if(logLevel > 3) errorLog = false;
			logLevel = 0;
		}
		else RETURN_TRUE;
	}

	if(SEG(peerId) != 0) GD::out.printMessage("Script log (peer id: " + std::to_string(SEG(peerId)) + "): " + message, logLevel, errorLog);
	else GD::out.printMessage("Script log: " + message, logLevel, errorLog);
	RETURN_TRUE;
}

ZEND_FUNCTION(hg_set_script_log_level)
{
	if(_disposed) RETURN_NULL();
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	int32_t logLevel = -1;
	if(argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::setScriptLoglevel().");
	else if(argc == 1)
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "logLevel is not of type integer.");
		else
		{
			logLevel = Z_LVAL(args[0]);
		}
	}
	if(logLevel < 0 || logLevel > 10) RETURN_FALSE;
	SEG(logLevel) = logLevel;

	{
		std::lock_guard<std::mutex> eventsMapGuard(PhpEvents::eventsMapMutex);
		std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator eventsIterator = PhpEvents::eventsMap.find(SEG(id));
		if(eventsIterator == PhpEvents::eventsMap.end() || !eventsIterator->second || eventsIterator->second->getToken().empty() || eventsIterator->second->getToken() != SEG(token)) RETURN_FALSE
		eventsIterator->second->setLogLevel(logLevel);
	}

	RETURN_TRUE;
}

ZEND_FUNCTION(hg_get_http_contents)
{
	if(_disposed) RETURN_NULL();
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string hostname;
	int32_t port = 443;
	std::string path;
	std::string caFile;
	bool verifyCertificate = true;
	if(argc > 5) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::getHttpContents().");
	else if(argc == 5)
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "hostname is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) hostname = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "port is not of type integer.");
		else
		{
			port = Z_LVAL(args[1]);
		}

		if(Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "path is not of type string.");
		else
		{
			if(Z_STRLEN(args[2]) > 0) path = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
		}

		if(Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "caFile is not of type string.");
		else
		{
			if(Z_STRLEN(args[3]) > 0) caFile = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
		}

		if(Z_TYPE(args[4]) != IS_TRUE && Z_TYPE(args[4]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "verifyCertificate is not of type boolean.");
		else
		{
			verifyCertificate = (Z_TYPE(args[4]) == IS_TRUE);
		}
	}
	if(hostname.empty() || path.empty() || caFile.empty() || port < 1 || port > 65535) RETURN_FALSE;

	std::string data;
	try
	{
		BaseLib::HttpClient client(GD::bl.get(), hostname, port, false, true, caFile, verifyCertificate);
		client.get(path, data);
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printError("Error downloading file: " + ex.what());
		RETURN_FALSE;
	}

	ZVAL_STRINGL(return_value, data.c_str(), data.size());
}

ZEND_FUNCTION(hg_download)
{
	if(_disposed) RETURN_NULL();
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string hostname;
	int32_t port = 443;
	std::string path;
	std::string filename;
	std::string caFile;
	bool verifyCertificate = true;
	if(argc > 6) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::download().");
	else if(argc == 6)
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "hostname is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) hostname = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "port is not of type integer.");
		else
		{
			port = Z_LVAL(args[1]);
		}

		if(Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "path is not of type string.");
		else
		{
			if(Z_STRLEN(args[2]) > 0) path = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
		}

		if(Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
		else
		{
			if(Z_STRLEN(args[3]) > 0) filename = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
		}

		if(Z_TYPE(args[4]) != IS_STRING) php_error_docref(NULL, E_WARNING, "caFile is not of type string.");
		else
		{
			if(Z_STRLEN(args[4]) > 0) caFile = std::string(Z_STRVAL(args[4]), Z_STRLEN(args[4]));
		}

		if(Z_TYPE(args[5]) != IS_TRUE && Z_TYPE(args[5]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "verifyCertificate is not of type boolean.");
		else
		{
			verifyCertificate = (Z_TYPE(args[5]) == IS_TRUE);
		}
	}
	if(hostname.empty() || path.empty() || filename.empty() || caFile.empty() || port < 1 || port > 65535) RETURN_FALSE;

	BaseLib::Http http;
	try
	{
		BaseLib::HttpClient client(GD::bl.get(), hostname, port, false, true, caFile, verifyCertificate);
		client.get(path, http);

		if(http.getHeader().responseCode != 200) RETURN_FALSE;
		if(http.getContentSize() <= 1) RETURN_FALSE;
		BaseLib::Io::writeFile(filename, http.getContent(), http.getContentSize());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printError("Error downloading file: " + ex.what());
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

ZEND_FUNCTION(hg_check_license)
{
	if(_disposed) RETURN_NULL();
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	int32_t moduleId = -1;
	int32_t familyId = -1;
	int32_t deviceId = -1;
	std::string licenseKey;
	if(argc > 4) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::checkLicense().");
	else if(argc >= 3)
	{
		if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "licenseModuleId is not of type integer.");
		else moduleId = Z_LVAL(args[0]);

		if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "familyId is not of type integer.");
		else familyId = Z_LVAL(args[1]);

		if(Z_TYPE(args[2]) != IS_LONG) php_error_docref(NULL, E_WARNING, "deviceId is not of type integer.");
		else deviceId = Z_LVAL(args[2]);

		if(argc == 4)
		{
			if(Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "licenseKey is not of type string.");
			else
			{
				if(Z_STRLEN(args[3]) > 0) licenseKey = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
			}
		}
	}

	std::string methodName("checkLicense");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)moduleId)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)familyId)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)deviceId)));
	if(!licenseKey.empty()) parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(licenseKey)));
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
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
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_get_license_states)
{
	if(_disposed) RETURN_NULL();
	long moduleId = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &moduleId) != SUCCESS) RETURN_NULL();
	std::string methodName("getLicenseStates");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t)moduleId)));
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_shutting_down)
{
	if(_disposed || GD::bl->shuttingDown) RETURN_TRUE
	RETURN_FALSE
}

ZEND_FUNCTION(hg_gpio_export)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->exportGpio(gpio);
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
	_superglobals.gpio->setDirection(gpio, (BaseLib::LowLevel::Gpio::GpioDirection::Enum)direction);
}

ZEND_FUNCTION(hg_gpio_set_edge)
{
	if(_disposed) RETURN_NULL();
	long gpio = -1;
	long edge = -1;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &gpio, &edge) != SUCCESS) RETURN_NULL();
	_superglobals.gpio->setEdge(gpio, (BaseLib::LowLevel::Gpio::GpioEdge::Enum)edge);
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
		std::string device;
		long baudrate = 38400;
		bool evenParity = false;
		bool oddParity = false;
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		if(argc > 4) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearSerial::open().");
		else if(argc >= 1)
		{
			if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "device is not of type string.");
			else
			{
				if(Z_STRLEN(args[0]) > 0) device = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
			}

			if(argc >= 2)
			{
				if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "baudrate is not of type integer.");
				else baudrate = Z_LVAL(args[1]);

				if(argc >= 3)
				{
					if(Z_TYPE(args[2]) != IS_TRUE && Z_TYPE(args[2]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "evenParity is not of type boolean.");
					else
					{
						evenParity = Z_TYPE(args[1]) == IS_TRUE;
					}

					if(argc == 4)
					{
						if(Z_TYPE(args[3]) != IS_TRUE && Z_TYPE(args[3]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "oddParity is not of type boolean.");
						else
						{
							oddParity = Z_TYPE(args[3]) == IS_TRUE;
						}
					}
				}
			}
		}
		if(device.empty()) RETURN_FALSE;

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
		int argc = 0;
		zval* args = nullptr;
		if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
		long id = -1;
		std::string data;
		if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearSerial::write().");
		else if(argc == 2)
		{
			if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "handle is not of type integer.");
			else
			{
				id = Z_LVAL(args[0]);
			}

			if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "data is not of type string.");
			else
			{
				if(Z_STRLEN(args[1]) > 0) data = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
			}
		}
		if(data.empty()) RETURN_FALSE;

		std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;

		{
			std::lock_guard<std::mutex> serialDevicesGuard(_superglobals.serialDevicesMutex);
			std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = _superglobals.serialDevices.find(id);
			if(deviceIterator != _superglobals.serialDevices.end())
			{
				if(deviceIterator->second) serialReaderWriter = deviceIterator->second;
			}
		}
		if(!serialReaderWriter) RETURN_FALSE;
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
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
	std::string device;
	int address = 0;
	if(argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearI2c::open().");
	else if(argc == 2)
	{
		if(Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "device is not of type string.");
		else
		{
			if(Z_STRLEN(args[0]) > 0) device = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
		}

		if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "address is not of type integer.");
		else
		{
			address = Z_LVAL(args[1]);
		}
	}
	if(device.empty()) RETURN_FALSE;

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
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

	long descriptor = -1;
	long length = 1;
	bool binary = false;
	if(argc < 2) php_error_docref(NULL, E_WARNING, "HomegearI2c::read() expects at least 2 arguments.");
	else if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearI2c::read().");
	else if(argc == 3)
	{
		if(Z_TYPE(args[2]) != IS_TRUE && Z_TYPE(args[2]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "returnBinary is not of type bool.");
		else binary = (Z_TYPE(args[2]) == IS_TRUE);
	}

	if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "handle is not of type long.");
	else descriptor = Z_LVAL(args[0]);

	if(Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "length is not of type long.");
	else length = Z_LVAL(args[1]);

	if(length <= 0)
	{
		php_error_docref(NULL, E_WARNING, "length is invalid.");
		RETURN_FALSE;
	}

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
	int argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

	long descriptor = -1;
	bool binary = false;
	std::vector<uint8_t> data;

	if(argc < 2) php_error_docref(NULL, E_WARNING, "HomegearI2c::write() expects at least 2 arguments.");
	else if(argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearI2c::write().");
	else if(argc == 3)
	{
		if(Z_TYPE(args[2]) != IS_TRUE && Z_TYPE(args[2]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "dataIsBinary is not of type bool.");
		else binary = (Z_TYPE(args[2]) == IS_TRUE);
	}

	if(Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "handle is not of type long.");
	else descriptor = Z_LVAL(args[0]);

	if(Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "data is not of type string.");
	else
	{
		if(Z_STRLEN(args[1]) > 0)
		{
			if(binary) data = std::vector<uint8_t>(Z_STRVAL(args[1]), Z_STRVAL(args[1]) + Z_STRLEN(args[1]));
			else
			{
				std::string hexData(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
				data = GD::bl->hf.getUBinary(hexData);
			}
		}
	}
	if(data.empty()) RETURN_FALSE;

	if (write(descriptor, data.data(), data.size()) != (signed)data.size()) RETURN_FALSE;
	RETURN_TRUE;
}
#endif

ZEND_METHOD(Homegear, __call)
{
	if(_disposed) RETURN_NULL();
	zval* zMethodName = nullptr;
	zval* args = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &zMethodName, &args) != SUCCESS) RETURN_NULL();
	std::string methodName(std::string(Z_STRVAL_P(zMethodName), Z_STRLEN_P(zMethodName)));
	BaseLib::PVariable parameters = PhpVariableConverter::getVariable(args, methodName == "createGroup" || methodName == "updateGroup");
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_METHOD(Homegear, __callStatic)
{
	if(_disposed) RETURN_NULL();
	zval* zMethodName = nullptr;
	zval* args = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &zMethodName, &args) != SUCCESS) RETURN_NULL();
	std::string methodName(std::string(Z_STRVAL_P(zMethodName), Z_STRLEN_P(zMethodName)));
	BaseLib::PVariable parameters = PhpVariableConverter::getVariable(args, methodName == "createGroup" || methodName == "updateGroup");
	php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_BEGIN_ARG_INFO_EX(php_homegear_two_args, 0, 0, 2)
	ZEND_ARG_INFO(0, arg1)
	ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

static const zend_function_entry homegear_methods[] = {
	ZEND_ME(Homegear, __call, php_homegear_two_args, ZEND_ACC_PRIVATE)
	ZEND_ME(Homegear, __callStatic, php_homegear_two_args, ZEND_ACC_PRIVATE | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getScriptId, hg_get_script_id, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(registerThread, hg_register_thread, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(log, hg_log, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(setScriptLogLevel, hg_set_script_log_level, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(getHttpContents, hg_get_http_contents, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(download, hg_download, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(pollEvent, hg_poll_event, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(subscribePeer, hg_subscribe_peer, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(unsubscribePeer, hg_unsubscribe_peer, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(shuttingDown, hg_shutting_down, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

static const zend_function_entry homegear_gpio_methods[] = {
	ZEND_ME_MAPPING(export, hg_gpio_export, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
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
	_superglobals.gpio = new BaseLib::LowLevel::Gpio(GD::bl.get());
	_disposed = false;
	pthread_key_create(php_homegear_get_pthread_key(), pthread_data_destructor);
	int32_t maxScriptsPerProcess = GD::bl->settings.scriptEngineMaxScriptsPerProcess();
	if(maxScriptsPerProcess < 1) maxScriptsPerProcess = 20;
	int32_t maxThreadsPerScript = GD::bl->settings.scriptEngineMaxThreadsPerScript();
	if(maxThreadsPerScript < 1) maxThreadsPerScript = 4;
	int32_t threadCount = maxScriptsPerProcess * maxThreadsPerScript;
	if(tsrm_startup(threadCount, threadCount, 0, NULL) != 1) //Needs to be called once for the entire process (see TSRM.c)
	{
		GD::out.printCritical("Critical: Could not start script engine. tsrm_startup returned error.");
		return FAILURE;
	}
#ifdef ZEND_SIGNALS
	zend_signal_startup();
#endif
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

	tsrm_shutdown(); //Needs to be called once for the entire process (see TSRM.c)
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

static int php_homegear_startup(sapi_module_struct* sapi_globals)
{
	if(php_module_startup(sapi_globals, &homegear_module_entry, 1) == FAILURE) return FAILURE;

	// {{{ Fix for bug #71115 which causes process to crash when excessively using $_GLOBALS. Remove once bug is fixed.

			// Run the test code below a million times (at least 30 executions per second) to check if bug really is fixed. (This part is fixed)
			// Check if Homegear script engine processes crash on the Raspberry Pi (should crash pretty much immediately and often)
			/*
				<?php
				function setGlobals()
				{
						global $GLOBALS;
						$GLOBALS['ABC'.rand(0, 1000000).rand(1000, 10000000).rand(10000, 100000)] = rand(1000, 10000000)."BLA".rand(10000, 1000000);
						$GLOBALS['TEST'] = rand(0, 100000).'BLII'.rand(1000, 1000000);
						for($i = 0; $i < 1000; $i++)
						{
								$GLOBALS['TEST'.$i.'-'.rand(0, 10000000)] = $i.'-'.rand(100000, 100000)."TEST$i";
						}
				}
				$hg = new \Homegear\Homegear();
				$version = $hg->getVersion();
				$logLevel = $hg->logLevel();
				$logLevel += 4;
				setGlobals();
				$hg->log(4, print_r($GLOBALS, true));
				$hg->log(4, $version);
				$hg->writeLog($hg->getVersion(), 4);
				$hg->writeLog((string)$hg->logLevel(), 4);
				$hg->log(4, "$version $logLevel Bla");
				$hg->setValue(200, 1, "BALANCE", (int)rand(0, 1000));
				$hg->setValue(200, 1, "PRODUCTION", (int)rand(0, 1000));
				$hg->setValue(200, 1, "CONSUMPTION", (int)rand(0, 1000));
				$hg->setValue(200, 1, "ACTIVEPOWER", (int)rand(0, 1000));
				$hg->setValue(200, 1, "OWNCONSUMPTION", (int)rand(0, 1000).$GLOBALS['TEST']);
				$hg->setValue(200, 1, "PRODUCTION2", (int)rand(0, 1000));
				$hg->putParamset(200, 1, "MASTER", $hg->getParamset(200, 1, "MASTER"));
			 */
#if PHP_VERSION_ID < 70200
		void* global;
		void* function;
		void* classEntry;

		ZEND_HASH_FOREACH_PTR(CG(auto_globals), global) {
			GC_FLAGS(((zend_auto_global*)global)->name) |= IS_STR_INTERNED;
			zend_string_hash_val(((zend_auto_global*)global)->name);
		} ZEND_HASH_FOREACH_END();

		ZEND_HASH_FOREACH_PTR(CG(function_table), function) {
			GC_FLAGS(((zend_internal_function*)function)->function_name) |= IS_STR_INTERNED;
			zend_string_hash_val(((zend_internal_function*)function)->function_name);
		} ZEND_HASH_FOREACH_END();

		ZEND_HASH_FOREACH_PTR(CG(class_table), classEntry) {
			void* property;
			GC_FLAGS(((zend_class_entry*)classEntry)->name) |= IS_STR_INTERNED;
			zend_string_hash_val(((zend_class_entry*)classEntry)->name);
			ZEND_HASH_FOREACH_PTR(&((zend_class_entry*)classEntry)->properties_info, property) {
				GC_FLAGS(((zend_property_info*)property)->name) |= IS_STR_INTERNED;
				zend_string_hash_val(((zend_property_info*)property)->name);
				if (((zend_property_info*)property)->doc_comment) {
					GC_FLAGS(((zend_property_info*)property)->doc_comment) |= IS_STR_INTERNED;
				}
			} ZEND_HASH_FOREACH_END();

		} ZEND_HASH_FOREACH_END();
#endif
	// }}}

    zend_stream_open_function = hg_stream_open;

	return SUCCESS;
}

static int php_homegear_shutdown(sapi_module_struct* sapi_globals)
{
	// {{{ Fix for bug #71115 which causes process to crash when excessively using $_GLOBALS. Remove once bug is fixed.
#if PHP_VERSION_ID < 70200
		void* global;
		void* function;
		void* classEntry;

		ZEND_HASH_FOREACH_PTR(CG(auto_globals), global) {
			GC_FLAGS(((zend_auto_global*)global)->name) &= ~IS_STR_INTERNED;
		} ZEND_HASH_FOREACH_END();

		ZEND_HASH_FOREACH_PTR(CG(function_table), function) {
			GC_FLAGS(((zend_internal_function*)function)->function_name) &= ~IS_STR_INTERNED;
		} ZEND_HASH_FOREACH_END();

		ZEND_HASH_FOREACH_PTR(CG(class_table), classEntry) {
			void* property;
			GC_FLAGS(((zend_class_entry*)classEntry)->name) &= ~IS_STR_INTERNED;
			ZEND_HASH_FOREACH_PTR(&((zend_class_entry*)classEntry)->properties_info, property) {
				GC_FLAGS(((zend_property_info*)property)->name) &= ~IS_STR_INTERNED;
				if (((zend_property_info*)property)->doc_comment) {
					GC_FLAGS(((zend_property_info*)property)->doc_comment) &= ~IS_STR_INTERNED;
				}
			} ZEND_HASH_FOREACH_END();

		} ZEND_HASH_FOREACH_END();
#endif
	// }}}
	return php_module_shutdown_wrapper(sapi_globals);
}

static int php_homegear_activate()
{
	return SUCCESS;
}

static int php_homegear_deactivate()
{
	return SUCCESS;
}

static PHP_MINIT_FUNCTION(homegear)
{
	zend_class_entry homegearExceptionCe{};
	INIT_CLASS_ENTRY(homegearExceptionCe, "Homegear\\HomegearException", NULL);
	//Register child class inherited from Exception (fetched with "zend_exception_get_default")
	homegear_exception_class_entry = zend_register_internal_class_ex(&homegearExceptionCe, zend_exception_get_default());
	zend_declare_class_constant_long(homegear_exception_class_entry, "UNKNOWN_DEVICE", sizeof("UNKNOWN_DEVICE") - 1, -2);

	zend_class_entry homegearCe{};
	INIT_CLASS_ENTRY(homegearCe, "Homegear\\Homegear", homegear_methods);
	homegear_class_entry = zend_register_internal_class(&homegearCe);
	zend_declare_class_constant_stringl(homegear_class_entry, "TEMP_PATH", sizeof("TEMP_PATH") - 1, GD::bl->settings.tempPath().c_str(), GD::bl->settings.tempPath().size());
	zend_declare_class_constant_stringl(homegear_class_entry, "SCRIPT_PATH", sizeof("SCRIPT_PATH") - 1, GD::bl->settings.scriptPath().c_str(), GD::bl->settings.scriptPath().size());
	zend_declare_class_constant_stringl(homegear_class_entry, "MODULE_PATH", sizeof("MODULE_PATH") - 1, GD::bl->settings.modulePath().c_str(), GD::bl->settings.modulePath().size());
	zend_declare_class_constant_stringl(homegear_class_entry, "DATA_PATH", sizeof("DATA_PATH") - 1, GD::bl->settings.dataPath().c_str(), GD::bl->settings.dataPath().size());
	zend_declare_class_constant_stringl(homegear_class_entry, "SOCKET_PATH", sizeof("SOCKET_PATH") - 1, GD::bl->settings.socketPath().c_str(), GD::bl->settings.socketPath().size());
	zend_declare_class_constant_stringl(homegear_class_entry, "LOGFILE_PATH", sizeof("LOGFILE_PATH") - 1, GD::bl->settings.logfilePath().c_str(), GD::bl->settings.logfilePath().size());
	zend_declare_class_constant_stringl(homegear_class_entry, "WORKING_DIRECTORY", sizeof("WORKING_DIRECTORY") - 1, GD::bl->settings.workingDirectory().c_str(), GD::bl->settings.workingDirectory().size());

	zend_class_entry homegearGpioCe{};
	INIT_CLASS_ENTRY(homegearGpioCe, "Homegear\\HomegearGpio", homegear_gpio_methods);
	homegear_gpio_class_entry = zend_register_internal_class(&homegearGpioCe);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "DIRECTION_IN", 12, 0);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "DIRECTION_OUT", 13, 1);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "EDGE_RISING", 11, 0);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "EDGE_FALLING", 12, 1);
	zend_declare_class_constant_long(homegear_gpio_class_entry, "EDGE_BOTH", 9, 2);

	zend_class_entry homegearSerialCe{};
	INIT_CLASS_ENTRY(homegearSerialCe, "Homegear\\HomegearSerial", homegear_serial_methods);
	homegear_serial_class_entry = zend_register_internal_class(&homegearSerialCe);

#ifdef I2CSUPPORT
	zend_class_entry homegearI2cCe{};
	INIT_CLASS_ENTRY(homegearI2cCe, "Homegear\\HomegearI2c", homegear_i2c_methods);
	homegear_i2c_class_entry = zend_register_internal_class(&homegearI2cCe);
#endif

	php_node_startup();
	php_device_startup();

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

#endif
