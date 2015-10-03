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

#include "../GD/GD.h"
#include "php_sapi.h"
#include "PHPVariableConverter.h"

static std::shared_ptr<BaseLib::HTTP> _http;
static pthread_key_t pthread_key;
static zend_class_entry* homegear_class_entry = nullptr;
static zend_class_entry* homegear_exception_class_entry = nullptr;

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

ZEND_FUNCTION(hg_invoke);
ZEND_FUNCTION(hg_get_meta);
ZEND_FUNCTION(hg_get_system);
ZEND_FUNCTION(hg_get_value);
ZEND_FUNCTION(hg_set_meta);
ZEND_FUNCTION(hg_set_system);
ZEND_FUNCTION(hg_set_value);
ZEND_FUNCTION(hg_auth);
ZEND_FUNCTION(hg_create_user);
ZEND_FUNCTION(hg_delete_user);
ZEND_FUNCTION(hg_update_user);
ZEND_FUNCTION(hg_user_exists);
ZEND_FUNCTION(hg_users);

static const zend_function_entry homegear_functions[] = {
	ZEND_FE(hg_invoke, NULL)
	ZEND_FE(hg_get_meta, NULL)
	ZEND_FE(hg_get_system, NULL)
	ZEND_FE(hg_get_value, NULL)
	ZEND_FE(hg_set_meta, NULL)
	ZEND_FE(hg_set_system, NULL)
	ZEND_FE(hg_set_value, NULL)
	ZEND_FE(hg_auth, NULL)
	ZEND_FE(hg_create_user, NULL)
	ZEND_FE(hg_delete_user, NULL)
	ZEND_FE(hg_update_user, NULL)
	ZEND_FE(hg_user_exists, NULL)
	ZEND_FE(hg_users, NULL)
	{NULL, NULL, NULL}
};

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

bool php_homegear_write_socket(BaseLib::SocketOperations* socket, std::vector<char>& data)
{
	try
	{
		if(!socket) return false;
		if(data.empty()) return true;
		try
		{
			socket->proofwrite(data);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			GD::out.printWarning("Warning: " + ex.what());
			socket->close();
			return false;
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			GD::out.printError("Error: " + ex.what());
			socket->close();
			return false;
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

bool php_homegear_write_socket(BaseLib::SocketOperations* socket, const char* buffer, uint32_t length)
{
	try
	{
		if(!socket || !buffer) return false;
		if(length == 0) return true;
		try
		{
			socket->proofwrite(buffer, length);
		}
		catch(BaseLib::SocketDataLimitException& ex)
		{
			GD::out.printWarning("Warning: " + ex.what());
			socket->close();
			return false;
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			GD::out.printError("Error: " + ex.what());
			socket->close();
			return false;
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
		}
	}
	return data;
}

void php_homegear_build_argv(std::vector<std::string>& arguments)
{
	zval argc;
	zval argv;
	array_init(&argv);

	for(std::vector<std::string>::const_iterator i = arguments.begin(); i != arguments.end(); ++i)
	{
		zval arg;
		ZVAL_STR(&arg, zend_string_init(i->c_str(), i->size() - 1, 0));
		if (zend_hash_next_index_insert(Z_ARRVAL_P(&argv), &arg) == NULL) {
			if (Z_TYPE(arg) == IS_STRING) {
				zend_string_release(Z_STR(arg));
			}
		}
	}


	ZVAL_LONG(&argc, arguments.size());
	zend_hash_update(&EG(symbol_table), zend_string_init("argv", sizeof("argv") - 1, 0), &argv);
	zend_hash_update(&EG(symbol_table), zend_string_init("argc", sizeof("argc") - 1, 0), &argc);

	//zval_ptr_dtor(&argc);
	//zval_ptr_dtor(&argv);
}

static size_t php_homegear_read_post(char *buf, size_t count_bytes)
{
	if(SEG(commandLine)) return 0;
	BaseLib::HTTP* http = SEG(http);
	if(!http || http->getContentSize() == 0) return 0;
	size_t bytesRead = http->readContentStream(buf, count_bytes);
	if(GD::bl->debugLevel >= 5 && bytesRead > 0) GD::out.printDebug("Debug: Raw post data: " + std::string(buf, bytesRead));
	return bytesRead;
}

static char* php_homegear_read_cookies()
{
	if(SEG(commandLine)) return 0;
	BaseLib::HTTP* http = SEG(http);
	if(!http || !http->getHeader()) return 0;
	if(!SEG(cookiesParsed))
	{
		SEG(cookiesParsed) = true;
		//Conversion from (const char*) to (char*) is ok, because PHP makes a copy before parsing and does not change the original data.
		if(!http->getHeader()->cookie.empty()) return (char*)http->getHeader()->cookie.c_str();
	}
	return NULL;
}

static size_t php_homegear_ub_write(const char* str, size_t length)
{
	if(length == 0) return 0;
	BaseLib::SocketOperations* socket = SEG(socket);
	if(socket) php_homegear_write_socket(socket, str, length);
	else
	{
		std::vector<char>* out = SEG(output);
		if(out)
		{
			if(out->size() + length > out->capacity()) out->reserve(out->capacity() + 1024);
			out->insert(out->end(), str, str + length);
		}
		else GD::out.printMessage("Script output: " + std::string(str, length));
	}
	return length;
}

static void php_homegear_flush(void *server_context)
{
	//We are storing all data, so no flush is needed.
}

static int php_homegear_send_headers(sapi_headers_struct* sapi_headers)
{
	if(SEG(commandLine)) return SAPI_HEADER_SENT_SUCCESSFULLY;
	if(!sapi_headers) return SAPI_HEADER_SEND_FAILED;
	std::vector<char> out;
	BaseLib::SocketOperations* socket = SEG(socket);
	if(!socket) return SAPI_HEADER_SENT_SUCCESSFULLY;
	if(out.size() + 100 > out.capacity()) out.reserve(out.capacity() + 1024);
	if(sapi_headers->http_status_line)
	{
		out.insert(out.end(), sapi_headers->http_status_line, sapi_headers->http_status_line + strlen(sapi_headers->http_status_line));
		out.push_back('\r');
		out.push_back('\n');
	}
	else
	{
		std::string status = "HTTP/1.1 " + std::to_string(sapi_headers->http_response_code) + " " + _http->getStatusText(sapi_headers->http_response_code) + "\r\n";
		out.insert(out.end(), &status[0], &status[0] + status.size());
	}
	zend_llist_element* element = sapi_headers->headers.head;
	while(element)
	{
		sapi_header_struct* header = (sapi_header_struct*)element->data;
		if(out.size() + header->header_len + 4 > out.capacity()) out.reserve(out.capacity() + 1024);
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
		out.insert(out.end(), temp.begin(), temp.end());
		out.push_back('\r');
		out.push_back('\n');
		element = element->next;
	}
	out.push_back('\r');
	out.push_back('\n');
	if(!php_homegear_write_socket(socket, out)) return SAPI_HEADER_SEND_FAILED;
	return SAPI_HEADER_SENT_SUCCESSFULLY;
}

/*static int php_homegear_header_handler(sapi_header_struct *sapi_header, sapi_header_op_enum op, sapi_headers_struct *sapi_headers)
{
	std::cerr << "php_homegear_header_handler: " << std::string(sapi_header->header, sapi_header->header_len) << std::endl;
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
			if (!memchr(sapi_header->header, ':', sapi_header->header_len))
			{
				sapi_free_header(sapi_header);
				return SUCCESS;
			}
			if(out->size() + sapi_header->header_len + 4 > out->capacity()) out->reserve(out->capacity() + 1024);
			out->insert(out->end(), sapi_header->header, sapi_header->header + sapi_header->header_len);
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
		if(out->size() + sapi_header->header_len + 4 > out->capacity()) out->reserve(out->capacity() + 1024);
		out->insert(out->end(), sapi_header->header, sapi_header->header + sapi_header->header_len);
		out->push_back('\r');
		out->push_back('\n');
		std::cerr << "php_homegear_send_header: " << std::string(sapi_header->header, sapi_header->header_len) << std::endl;
	}
}*/

static void php_homegear_log_message(char* message)
{
	std::string pathTranslated;
	if(SG(request_info).path_translated) pathTranslated = SG(request_info).path_translated;
	GD::out.printError("Scriptengine" + (SG(request_info).path_translated ? std::string(" (") + std::string(SG(request_info).path_translated) + "): " : ": ") + std::string(message));
}

static void php_homegear_register_variables(zval* track_vars_array)
{
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
		BaseLib::HTTP* http = SEG(http);
		if(!http) return;
		BaseLib::HTTP::Header* header = http->getHeader();
		if(!header) return;
		BaseLib::Rpc::ServerInfo::Info* server = (BaseLib::Rpc::ServerInfo::Info*)SG(server_context);
		zval value;

		//PHP makes copies of all values, so conversion from const to non-const is ok.
		if(server)
		{
			if(server->ssl) php_register_variable_safe((char*)"HTTPS", (char*)"on", 2, track_vars_array);
			else php_register_variable_safe((char*)"HTTPS", (char*)"", 0, track_vars_array);
			std::string connection = (header->connection & BaseLib::HTTP::Connection::keepAlive) ? "keep-alive" : "close";
			php_register_variable_safe((char*)"HTTP_CONNECTION", (char*)connection.c_str(), connection.size(), track_vars_array);
			php_register_variable_safe((char*)"DOCUMENT_ROOT", (char*)server->contentPath.c_str(), server->contentPath.size(), track_vars_array);
			std::string filename = server->contentPath;
			filename += (!header->path.empty() && header->path.front() == '/') ? header->path.substr(1) : header->path;
			php_register_variable_safe((char*)"SCRIPT_FILENAME", (char*)filename.c_str(), filename.size(), track_vars_array);
			php_register_variable_safe((char*)"SERVER_NAME", (char*)server->name.c_str(), server->name.size(), track_vars_array);
			php_register_variable_safe((char*)"SERVER_ADDR", (char*)server->address.c_str(), server->address.size(), track_vars_array);
			ZVAL_LONG(&value, server->port);
			php_register_variable_ex((char*)"SERVER_PORT", &value, track_vars_array);
		}

		std::string version = std::string("Homegear ") + VERSION;
		php_register_variable_safe((char*)"SERVER_SOFTWARE", (char*)version.c_str(), version.size(), track_vars_array);
		php_register_variable_safe((char*)"SCRIPT_NAME", (char*)header->path.c_str(), header->path.size(), track_vars_array);
		std::string phpSelf = header->path + header->pathInfo;
		php_register_variable_safe((char*)"PHP_SELF", (char*)phpSelf.c_str(), phpSelf.size(), track_vars_array);
		if(!header->pathInfo.empty())
		{
			php_register_variable_safe((char*)"PATH_INFO", (char*)header->pathInfo.c_str(), header->pathInfo.size(), track_vars_array);
			if(SG(request_info).path_translated) php_register_variable((char*)"PATH_TRANSLATED", (char*)SG(request_info).path_translated, track_vars_array);
		}
		php_register_variable_safe((char*)"HTTP_HOST", (char*)header->host.c_str(), header->host.size(), track_vars_array);
		php_register_variable_safe((char*)"QUERY_STRING", (char*)header->args.c_str(), header->args.size(), track_vars_array);
		php_register_variable_safe((char*)"SERVER_PROTOCOL", (char*)"HTTP/1.1", 8, track_vars_array);
		php_register_variable_safe((char*)"REMOTE_ADDRESS", (char*)header->remoteAddress.c_str(), header->remoteAddress.size(), track_vars_array);
		ZVAL_LONG(&value, header->remotePort);
		php_register_variable_ex((char*)"REMOTE_PORT", &value, track_vars_array);
		if(SG(request_info).request_method) php_register_variable((char*)"REQUEST_METHOD", (char*)SG(request_info).request_method, track_vars_array);
		if(SG(request_info).request_uri) php_register_variable((char*)"REQUEST_URI", SG(request_info).request_uri, track_vars_array);
		if(!header->pathInfo.empty()) php_register_variable_safe((char*)"REMOTE_ADDRESS", (char*)header->pathInfo.c_str(), header->pathInfo.size(), track_vars_array);
		if(header->contentLength > 0)
		{
			ZVAL_LONG(&value, header->contentLength);
			php_register_variable_ex((char*)"CONTENT_LENGTH", &value, track_vars_array);
		}
		if(!header->contentType.empty()) php_register_variable_safe((char*)"CONTENT_TYPE", (char*)header->contentType.c_str(), header->contentType.size(), track_vars_array);
		for(std::map<std::string, std::string>::const_iterator i = header->fields.begin(); i != header->fields.end(); ++i)
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
	BaseLib::PVariable result = GD::rpcServers.begin()->second.callMethod(methodName, parameters);
	if(result->errorStruct)
	{
		zend_throw_exception(homegear_exception_class_entry, result->structValue->at("faultString")->stringValue.c_str(), result->structValue->at("faultCode")->integerValue);
		RETURN_NULL()
	}
	PHPVariableConverter::getPHPVariable(result, return_value);
}

/* RPC functions */

ZEND_FUNCTION(hg_invoke)
{
	char* pMethodName = nullptr;
	int32_t methodNameLength = 0;
	int32_t argc = 0;
	zval* args = nullptr;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s*", &pMethodName, &methodNameLength, &args, &argc) != SUCCESS) RETURN_NULL();
	if(methodNameLength == 0) RETURN_NULL();
	std::string methodName(std::string(pMethodName, methodNameLength));
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	for(int32_t i = 0; i < argc; i++)
	{
		BaseLib::PVariable parameter = PHPVariableConverter::getVariable(&args[i]);
		if(parameter) parameters->arrayValue->push_back(parameter);
	}
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_meta)
{
	uint32_t id = 0;
	char* pName = nullptr;
	int32_t nameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "ls", &id, &pName, &nameLength) != SUCCESS) RETURN_NULL();
	std::string methodName("getMetadata");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_system)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "s",&pName, &nameLength) != SUCCESS) RETURN_NULL();
	std::string methodName("getSystemVariable");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_get_value)
{
	uint32_t id = 0;
	int32_t channel = -1;
	char* pParameterName = nullptr;
	int32_t parameterNameLength = 0;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lls", &id, &channel, &pParameterName, &parameterNameLength) != SUCCESS) RETURN_NULL();
	std::string methodName("getValue");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pParameterName, parameterNameLength))));
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_set_meta)
{
	uint32_t id = 0;
	char* pName = nullptr;
	int32_t nameLength = 0;
	zval* newValue = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "lsz", &id, &pName, &nameLength, &newValue) != SUCCESS) RETURN_NULL();
	std::string methodName("setMetadata");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	BaseLib::PVariable parameter = PHPVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_set_system)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	zval* newValue = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &pName, &nameLength, &newValue) != SUCCESS) RETURN_NULL();
	std::string methodName("setSystemVariable");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pName, nameLength))));
	BaseLib::PVariable parameter = PHPVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_FUNCTION(hg_set_value)
{
	uint32_t id = 0;
	int32_t channel = -1;
	char* pParameterName = nullptr;
	int32_t parameterNameLength = 0;
	zval* newValue = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "llsz", &id, &channel, &pParameterName, &parameterNameLength, &newValue) != SUCCESS) RETURN_NULL();
	std::string methodName("setValue");
	BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(id)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
	parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(std::string(pParameterName, parameterNameLength))));
	BaseLib::PVariable parameter = PHPVariableConverter::getVariable(newValue);
	if(parameter) parameters->arrayValue->push_back(parameter);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

/* User functions/methods */

ZEND_FUNCTION(hg_auth)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	char* pPassword = nullptr;
	int32_t passwordLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
	if(User::verify(std::string(pName, nameLength), std::string(pPassword, passwordLength))) RETURN_TRUE;
	RETURN_FALSE
}

ZEND_FUNCTION(hg_create_user)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	char* pPassword = nullptr;
	int32_t passwordLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0 || passwordLength < 8) RETURN_FALSE;
	std::string userName(pName, nameLength);
	if(!BaseLib::HelperFunctions::isAlphaNumeric(userName)) RETURN_FALSE;
	if(User::create(userName, std::string(pPassword, passwordLength))) RETURN_TRUE;
	RETURN_FALSE
}

ZEND_FUNCTION(hg_delete_user)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pName, &nameLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_FALSE;
	if(User::remove(std::string(pName, nameLength))) RETURN_TRUE;
	RETURN_FALSE
}

ZEND_FUNCTION(hg_update_user)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	char* pPassword = nullptr;
	int32_t passwordLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &pName, &nameLength, &pPassword, &passwordLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0 || passwordLength == 0) RETURN_FALSE;
	if(User::update(std::string(pName, nameLength), std::string(pPassword, passwordLength))) RETURN_TRUE;
	RETURN_FALSE
}

ZEND_FUNCTION(hg_user_exists)
{
	char* pName = nullptr;
	int32_t nameLength = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pName, &nameLength) != SUCCESS) RETURN_NULL();
	if(nameLength == 0) RETURN_FALSE;
	if(User::exists(std::string(pName, nameLength))) RETURN_TRUE;
	RETURN_FALSE
}

ZEND_FUNCTION(hg_users)
{
	std::map<uint64_t, std::string> users;
	User::getAll(users);
	array_init(return_value);
	for(std::map<uint64_t, std::string>::iterator i = users.begin(); i != users.end(); ++i)
	{
		add_next_index_stringl(return_value, i->second.c_str(), i->second.size());
	}
}

ZEND_METHOD(Homegear, __call)
{
	char* pMethodName = nullptr;
	int32_t methodNameLength = 0;
	zval* args = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &pMethodName, &methodNameLength, &args) != SUCCESS) RETURN_NULL();
	std::string methodName(std::string(pMethodName, methodNameLength));
	BaseLib::PVariable parameters = PHPVariableConverter::getVariable(args);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_METHOD(Homegear, __callStatic)
{
	char* pMethodName = nullptr;
	int32_t methodNameLength = 0;
	zval* args = nullptr;
	if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &pMethodName, &methodNameLength, &args) != SUCCESS) RETURN_NULL();
	std::string methodName(std::string(pMethodName, methodNameLength));
	BaseLib::PVariable parameters = PHPVariableConverter::getVariable(args);
	php_homegear_invoke_rpc(methodName, parameters, return_value);
}

ZEND_BEGIN_ARG_INFO_EX(php_homegear_two_args, 0, 0, 2)
	ZEND_ARG_INFO(0, arg1)
	ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

static const zend_function_entry homegear_methods[] = {
	ZEND_ME(Homegear, __call, php_homegear_two_args, ZEND_ACC_PUBLIC)
	ZEND_ME(Homegear, __callStatic, php_homegear_two_args, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(auth, hg_auth, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(createUser, hg_create_user, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(deleteUser, hg_delete_user, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(updateUser, hg_update_user, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(userExists, hg_user_exists, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	ZEND_ME_MAPPING(listUsers, hg_users, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
	{NULL, NULL, NULL}
};

int php_homegear_init()
{
	_http.reset(new BaseLib::HTTP());
	pthread_key_create(&pthread_key, pthread_data_destructor);
	tsrm_startup(20, 1, 0, NULL);
	sapi_startup(&php_homegear_sapi_module);
	sapi_module.startup(&php_homegear_sapi_module);

	return SUCCESS;
}

void php_homegear_shutdown()
{
	php_homegear_sapi_module.shutdown(&php_homegear_sapi_module);
	sapi_shutdown();

	ts_free_worker_threads();

	tsrm_shutdown();
	_http.reset();
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
