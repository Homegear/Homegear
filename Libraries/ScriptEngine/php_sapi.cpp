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

#include "php_sapi.h"

#define SEG(v) TSRMG(homegear_globals_id, zend_homegear_globals*, v)

static int homegear_globals_id;

const char HARDCODED_INI[] =
	"register_argc_argv=1\n"
	"implicit_flush=1\n"
	"output_buffering=0\n"
	"max_execution_time=0\n"
	"max_input_time=-1\n\0";

static int php_homegear_read_post(char *buf, uint count_bytes TSRMLS_DC)
{
	std::cout << "php_homegear_read_post" << std::endl;
	return count_bytes;
}

static char* php_homegear_read_cookies(TSRMLS_D)
{
	std::cout << "php_homegear_read_cookies" << std::endl;
	return NULL;
}

static inline size_t php_homegear_single_write(const char *str, uint str_length)
{
#ifdef PHP_WRITE_STDOUT
	long ret = write(STDOUT_FILENO, str, str_length);
	if (ret <= 0) return 0;
	return ret;
#else
	size_t ret;

	ret = fwrite(str, 1, MIN(str_length, 16384), stdout);
	return ret;
#endif
}


static int php_homegear_ub_write(const char *str, uint str_length TSRMLS_DC)
{
	const char *ptr = str;
	uint remaining = str_length;
	size_t ret;

	while (remaining > 0) {
		ret = php_homegear_single_write(ptr, remaining);
		if (!ret) {
			php_handle_aborted_connection();
		}
		ptr += ret;
		remaining -= ret;
	}

	return str_length;
}

static void php_homegear_flush(void *server_context)
{
	if (fflush(stdout)==EOF) {
		php_handle_aborted_connection();
	}
}

static void php_homegear_send_header(sapi_header_struct *sapi_header, void *server_context TSRMLS_DC)
{
	if(!sapi_header) return;
	std::string header(sapi_header->header, sapi_header->header_len);
	std::cout << "php_homegear_send_header: " << header << std::endl;
}

static void php_homegear_log_message(char *message TSRMLS_DC)
{
	fprintf (stderr, "%s\n", message);
}

static void php_homegear_register_variables(zval *track_vars_array TSRMLS_DC)
{
	std::cout << "php_homegear_register_variables" << std::endl;

	char buf[101];
	snprintf(buf, 100, "%s/%s", "TESTBLA", "TESTBLUPP");
	php_register_variable("TESTVAR", buf, track_vars_array TSRMLS_CC);
	php_register_variable("SAPI_TYPE", "Homegear", track_vars_array TSRMLS_CC);

	php_import_environment_variables(track_vars_array TSRMLS_CC);
}

static int php_homegear_startup(sapi_module_struct *sapi_module)
{
	if (php_module_startup(sapi_module, NULL, 0)==FAILURE) {
		return FAILURE;
	}

	return SUCCESS;
}

static int php_homegear_activate(TSRMLS_D)
{
	std::cout << "php_homegear_activate" << std::endl;
	return SUCCESS;
}

static int php_homegear_deactivate(TSRMLS_D)
{
	std::cout << "php_homegear_deactivate" << std::endl;
	return SUCCESS;
}

extern sapi_module_struct php_homegear_module = {
	"homegear",                       /* name */
	"PHP Homegear Library",        /* pretty name */

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
	NULL,                          /* send headers handler */
	php_homegear_send_header,          /* send header handler */

	php_homegear_read_post,            /* read POST data */
	php_homegear_read_cookies,         /* read Cookies */

	php_homegear_register_variables,   /* register server variables */
	php_homegear_log_message,          /* Log message */
	NULL,							/* Get request time */
	NULL,							/* Child terminate */

	STANDARD_SAPI_MODULE_PROPERTIES
};
/* }}} */

ZEND_FUNCTION(mein_blii) {
	unsigned long index = TSRMG(homegear_globals_id, zend_homegear_globals*, index);
	std::cout << "Index in mein_blii: " << index << std::endl;
	php_printf("Hi!");
}

ZEND_FUNCTION(mein_blupp) {
	RETURN_LONG(34);

	std::cout << "mein_blupp..." << std::endl;
	long number1;
	long number2;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &number1, &number2) != SUCCESS) {
		RETURN_NULL();
	}

	/* set return value */
	RETURN_LONG(number1 * number2);
}

ZEND_BEGIN_ARG_INFO(arginfo_foobar_void, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mein_blupp, 0, 0, 1)
	ZEND_ARG_INFO(0, number1)
	ZEND_ARG_INFO(0, number2)
ZEND_END_ARG_INFO()

static const zend_function_entry additional_functions[] = {
	ZEND_FE(mein_blii, arginfo_foobar_void)
	ZEND_FE(mein_blupp, arginfo_mein_blupp)
	{NULL, NULL, NULL}
};

static void php_homegear_globals_ctor(zend_homegear_globals* homegear_globals TSRMLS_DC)
{
	homegear_globals->index = std::rand();
	std::cout << "Setting globals index to: " << homegear_globals->index << std::endl;
}

static void php_homegear_globals_dtor(zend_homegear_globals* homegear_globals TSRMLS_DC)
{
	std::cout << "Cleaning up globals." << std::endl;
}

int php_homegear_init()
{
	tsrm_startup(1, 1, 0, NULL);
	sapi_startup(&php_homegear_module);
	php_homegear_module.ini_entries = (char*)malloc(sizeof(HARDCODED_INI));
	memcpy(php_homegear_module.ini_entries, HARDCODED_INI, sizeof(HARDCODED_INI));
	php_homegear_module.additional_functions = additional_functions;
	sapi_module.startup(&php_homegear_module);

	ts_allocate_id(&homegear_globals_id, sizeof(zend_homegear_globals), (void(*)(void*, void***))php_homegear_globals_ctor, (void(*)(void*, void***))php_homegear_globals_dtor);

	return SUCCESS;
}

void php_homegear_shutdown()
{
	php_homegear_module.shutdown(&php_homegear_module);
	sapi_shutdown();
	tsrm_shutdown();
}
