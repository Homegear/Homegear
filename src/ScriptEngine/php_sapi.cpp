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
#include "php_sapi.h"
#include "PhpVariableConverter.h"

#ifdef I2CSUPPORT
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#endif

#if PHP_VERSION_ID < 70100
#error "PHP 7.2 is required as ZTS in versions 7.0 and 7.1 is broken."
#endif
#if PHP_VERSION_ID >= 80500
#error "PHP 8.5 or greater is not officially supported yet. Please check the following points (only visible in source code) before removing this line."
/*
 * 1. Compare initialization with the initialization in one of the SAPI modules (e. g. "php_embed_init()" in "sapi/embed/php_embed.c").
 * 2. Check if content of marked part in hg_stream_open() equals zend_stream_open() in zend_stream.c
 * 3. Check if ext/opcache changed references to ZEND_HANDLE_STREAM, because the stream is interpreted as php_stream
 *    there, which causes PHP to crash when a script is started from Homegear. This is the only extension that needs to
 *    be checked, because we force Homegear to work with Opcache. The references everywhere else can't expect the file
 *    handle to be of type php_stream.
 */
#endif

static bool disposed_ = false;
static zend_homegear_superglobals super_globals_;
static std::mutex script_cache_mutex_;
static std::map<std::string, std::shared_ptr<Homegear::ScriptEngine::CacheInfo>> script_cache_;
static std::mutex env_mutex_;

static zend_class_entry *homegear_class_entry = nullptr;
static zend_class_entry *homegear_gpio_class_entry = nullptr;
static zend_class_entry *homegear_serial_class_entry = nullptr;
#ifdef I2CSUPPORT
static zend_class_entry* homegear_i2c_class_entry = nullptr;
#endif

static char *ini_path_override = nullptr;
static char *sapi_ini_entries = nullptr;
static const char HARDCODED_INI[] =
    "register_argc_argv=1\n"
    "max_execution_time=0\n"
    "max_input_time=-1\n\0";

static int php_homegear_startup(sapi_module_struct *sapi_globals);
static int php_homegear_shutdown(sapi_module_struct *sapi_globals);
static int php_homegear_activate();
static int php_homegear_deactivate();
static size_t php_homegear_ub_write_string(std::string &string);
static size_t php_homegear_ub_write(const char *str, size_t length);
static void php_homegear_flush(void *server_context);
static int php_homegear_send_headers(sapi_headers_struct *sapi_headers);
static size_t php_homegear_read_post(char *buf, size_t count_bytes);
static char *php_homegear_read_cookies();
static void php_homegear_register_variables(zval *track_vars_array);
#if PHP_VERSION_ID >= 80000
static void php_homegear_log_message(const char *message, int syslog_type_int);
#elif PHP_VERSION_ID >= 70100
static void php_homegear_log_message(char *message, int syslog_type_int);
#else
static void php_homegear_log_message(char* message);
#endif
static PHP_MINIT_FUNCTION(homegear);
static PHP_MSHUTDOWN_FUNCTION(homegear);
static PHP_RINIT_FUNCTION(homegear);
static PHP_RSHUTDOWN_FUNCTION(homegear);
static PHP_MINFO_FUNCTION(homegear);

#define SEG(v) php_homegear_get_globals()->v

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(print_v_arg_info, 0, 1, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(print_v);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_generate_webssh_token_arg_info, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_generate_webssh_token);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_get_script_id_arg_info, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_get_script_id);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_register_thread_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_register_thread);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_list_modules_arg_info, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_list_modules);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_load_module_arg_info, 0, 1, IS_LONG, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_load_module);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_unload_module_arg_info, 0, 1, IS_LONG, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_unload_module);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_reload_module_arg_info, 0, 1, IS_LONG, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_reload_module);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_auth_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_auth);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_create_user_arg_info, 0, 3, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_create_user);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_delete_user_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_delete_user);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_get_user_metadata_arg_info, 0, 1, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_get_user_metadata);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_set_user_metadata_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_set_user_metadata);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_set_user_privileges_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_set_user_privileges);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_update_user_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_update_user);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_user_exists_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_user_exists);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_users_arg_info, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_users);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_log_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_log);
ZEND_BEGIN_ARG_INFO_EX(hg_set_language_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_set_language);
ZEND_BEGIN_ARG_INFO_EX(hg_set_script_log_level_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_set_script_log_level);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_get_http_contents_arg_info, 0, 5, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_get_http_contents);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_download_arg_info, 0, 6, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_download);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_json_encode_arg_info, 0, 1, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_json_encode);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_json_decode_arg_info, 0, 1, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_json_decode);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_ssdp_search_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_ssdp_search);
ZEND_BEGIN_ARG_INFO_EX(hg_configure_gateway_arg_info, nullptr, 0, 6)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_configure_gateway);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_check_license_arg_info, 0, 3, IS_LONG, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_check_license);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_remove_license_arg_info, 0, 3, IS_LONG, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_remove_license);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_get_license_states_arg_info, 0, 1, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_get_license_states);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_poll_event_arg_info, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_poll_event);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_list_rpc_clients_arg_info, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_list_rpc_clients);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_peer_exists_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_peer_exists);
ZEND_BEGIN_ARG_INFO_EX(hg_subscribe_peer_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_subscribe_peer);
ZEND_BEGIN_ARG_INFO_EX(hg_unsubscribe_peer_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_unsubscribe_peer);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_shutting_down_arg_info, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_shutting_down);
ZEND_BEGIN_ARG_INFO_EX(hg_gpio_export_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_export);
ZEND_BEGIN_ARG_INFO_EX(hg_gpio_open_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_open);
ZEND_BEGIN_ARG_INFO_EX(hg_gpio_close_arg_info, nullptr, 0, 1)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_close);
ZEND_BEGIN_ARG_INFO_EX(hg_gpio_set_direction_arg_info, nullptr, 0, 2)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_set_direction);
ZEND_BEGIN_ARG_INFO_EX(hg_gpio_set_edge_arg_info, nullptr, 0, 2)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_set_edge);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_gpio_get_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_get);
ZEND_BEGIN_ARG_INFO_EX(hg_gpio_set_arg_info, nullptr, 0, 2)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_set);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_gpio_poll_arg_info, 0, 2, IS_LONG, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_gpio_poll);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_serial_open_arg_info, 0, 2, IS_RESOURCE, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_serial_open);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_serial_close_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_serial_close);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_serial_read_arg_info, 0, 2, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_serial_read);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_serial_readline_arg_info, 0, 2, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_serial_readline);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_serial_write_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_serial_write);
#ifdef I2CSUPPORT
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_i2c_open_arg_info, 0, 2, IS_RESOURCE, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_i2c_open);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_i2c_close_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_i2c_close);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_i2c_read_arg_info, 0, 3, IS_STRING, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_i2c_read);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_i2c_write_arg_info, 0, 2, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_i2c_write);
#endif

//{{{ Overwrite non thread safe function
//Todo: Remove once thread safe
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_getenv_arg_info, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_getenv);
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(hg_putenv_arg_info, 0, 1, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
ZEND_FUNCTION(hg_putenv);
// }}}


static const zend_function_entry homegear_functions[] = {
    ZEND_FE(print_v, print_v_arg_info)
    ZEND_FE(hg_generate_webssh_token, hg_generate_webssh_token_arg_info)
    ZEND_FE(hg_get_script_id, hg_get_script_id_arg_info)
    ZEND_FE(hg_register_thread, hg_register_thread_arg_info)
    ZEND_FE(hg_list_modules, hg_list_modules_arg_info)
    ZEND_FE(hg_load_module, hg_load_module_arg_info)
    ZEND_FE(hg_unload_module, hg_unload_module_arg_info)
    ZEND_FE(hg_reload_module, hg_reload_module_arg_info)
    ZEND_FE(hg_auth, hg_auth_arg_info)
    ZEND_FE(hg_create_user, hg_create_user_arg_info)
    ZEND_FE(hg_delete_user, hg_delete_user_arg_info)
    ZEND_FE(hg_get_user_metadata, hg_get_user_metadata_arg_info)
    ZEND_FE(hg_set_user_metadata, hg_set_user_metadata_arg_info)
    ZEND_FE(hg_set_user_privileges, hg_set_user_privileges_arg_info)
    ZEND_FE(hg_update_user, hg_update_user_arg_info)
    ZEND_FE(hg_user_exists, hg_user_exists_arg_info)
    ZEND_FE(hg_users, hg_users_arg_info)
    ZEND_FE(hg_log, hg_log_arg_info)
    ZEND_FE(hg_set_script_log_level, hg_set_script_log_level_arg_info)
    ZEND_FE(hg_get_http_contents, hg_get_http_contents_arg_info)
    ZEND_FE(hg_download, hg_download_arg_info)
    ZEND_FE(hg_json_encode, hg_json_encode_arg_info)
    ZEND_FE(hg_json_decode, hg_json_decode_arg_info)
    ZEND_FE(hg_ssdp_search, hg_ssdp_search_arg_info)
    ZEND_FE(hg_configure_gateway, hg_configure_gateway_arg_info)
    ZEND_FE(hg_check_license, hg_check_license_arg_info)
    ZEND_FE(hg_remove_license, hg_remove_license_arg_info)
    ZEND_FE(hg_get_license_states, hg_get_license_states_arg_info)
    ZEND_FE(hg_poll_event, hg_poll_event_arg_info)
    ZEND_FE(hg_list_rpc_clients, hg_list_rpc_clients_arg_info)
    ZEND_FE(hg_peer_exists, hg_peer_exists_arg_info)
    ZEND_FE(hg_subscribe_peer, hg_subscribe_peer_arg_info)
    ZEND_FE(hg_unsubscribe_peer, hg_unsubscribe_peer_arg_info)
    ZEND_FE(hg_shutting_down, hg_shutting_down_arg_info)
    ZEND_FE(hg_gpio_export, hg_gpio_export_arg_info)
    ZEND_FE(hg_gpio_open, hg_gpio_open_arg_info)
    ZEND_FE(hg_gpio_close, hg_gpio_close_arg_info)
    ZEND_FE(hg_gpio_set_direction, hg_gpio_set_direction_arg_info)
    ZEND_FE(hg_gpio_set_edge, hg_gpio_set_edge_arg_info)
    ZEND_FE(hg_gpio_get, hg_gpio_get_arg_info)
    ZEND_FE(hg_gpio_set, hg_gpio_set_arg_info)
    ZEND_FE(hg_gpio_poll, hg_gpio_poll_arg_info)
    ZEND_FE(hg_serial_open, hg_serial_open_arg_info)
    ZEND_FE(hg_serial_close, hg_serial_close_arg_info)
    ZEND_FE(hg_serial_read, hg_serial_read_arg_info)
    ZEND_FE(hg_serial_readline, hg_serial_readline_arg_info)
    ZEND_FE(hg_serial_write, hg_serial_write_arg_info)
    ZEND_FE(hg_getenv, hg_getenv_arg_info)
    ZEND_FE(hg_putenv, hg_putenv_arg_info)
#ifdef I2CSUPPORT
    ZEND_FE(hg_i2c_open, hg_i2c_open_arg_info)
    ZEND_FE(hg_i2c_close, hg_i2c_close_arg_info)
    ZEND_FE(hg_i2c_read, hg_i2c_read_arg_info)
    ZEND_FE(hg_i2c_write, hg_i2c_write_arg_info)
#endif
    {NULL, NULL, NULL}
};

#define INI_DEFAULT(name, value)\
    ZVAL_NEW_STR(&tmp, zend_string_init(value, sizeof(value)-1, 1));\
    zend_hash_str_update(configuration_hash, name, sizeof(name)-1, &tmp);\

static void homegear_ini_defaults(HashTable *configuration_hash) {
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
    Homegear::GD::baseLibVersion.c_str(),
    STANDARD_MODULE_PROPERTIES
};

static sapi_module_struct php_homegear_sapi_module = {
    (char *) "homegear",                       /* name */
    (char *) "PHP Homegear Library",        /* pretty name */

    php_homegear_startup,              /* startup == MINIT. Called for each new thread. */
    php_homegear_shutdown,             /* shutdown == MSHUTDOWN. Called for each new thread. */

    php_homegear_activate,                /* activate == RINIT. Called for each request. */
    php_homegear_deactivate,            /* deactivate == RSHUTDOWN. Called for each request. */

    php_homegear_ub_write,             /* unbuffered write */
    php_homegear_flush,                /* flush */
    nullptr,                          /* get uid */
    nullptr,                          /* getenv */

    php_error,                     /* error handler */

    nullptr,                          /* header handler */
    php_homegear_send_headers,                          /* send headers handler */
    nullptr,          /* send header handler */

    php_homegear_read_post,            /* read POST data */
    php_homegear_read_cookies,         /* read Cookies */

    php_homegear_register_variables,   /* register server variables */
    php_homegear_log_message,          /* Log message */
    nullptr,                            /* Get request time */
    nullptr,                            /* Child terminate */

    STANDARD_SAPI_MODULE_PROPERTIES
};

void pthread_data_destructor(void *data) {
  if (data) delete (zend_homegear_globals *) data;
}

void php_homegear_build_argv(std::vector<std::string> &arguments) {
  if (disposed_) return;
  zval argc;
  zval argv;
  array_init(&argv);

  for (std::vector<std::string>::const_iterator i = arguments.begin(); i != arguments.end(); ++i) {
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

#if PHP_VERSION_ID >= 70400
ssize_t hg_zend_stream_reader(void *handle, char *buf, size_t len) {
  if (!handle) return 0;

  auto hg_stream = (hg_stream_handle *) handle;

  size_t bytesToReturn = hg_stream->position + len > hg_stream->buffer.size() ? hg_stream->buffer.size() - hg_stream->position : len;
  std::copy(hg_stream->buffer.data() + hg_stream->position, hg_stream->buffer.data() + hg_stream->position + bytesToReturn, buf);

  hg_stream->position += bytesToReturn;

  return bytesToReturn;
}

size_t hg_zend_stream_fsizer(void *handle) {
  if (!handle) return 0;

  return ((hg_stream_handle *) handle)->buffer.size();
}

void hg_zend_stream_closer(void *handle) {
  if (handle) delete (hg_stream_handle *) handle;
}
#endif

#if PHP_VERSION_ID < 80000
int hg_stream_open(const char *filename, zend_file_handle *handle) {
#elif PHP_VERSION_ID < 80200
zend_result hg_stream_open(const char *filename, zend_file_handle *handle) {
#else
zend_result hg_stream_open(zend_file_handle *handle) {
#endif
#if PHP_VERSION_ID < 80200
  std::string file(filename);
#else
  std::string file(handle->filename->val, handle->filename->len);
#endif
  if (file.size() > 3 && (file.compare(file.size() - 4, 4, ".hgs") == 0 || file.compare(file.size() - 4, 4, ".hgn") == 0)) {
    std::lock_guard<std::mutex> scriptCacheGuard(script_cache_mutex_);
    auto scriptIterator = script_cache_.find(file);
    if (scriptIterator != script_cache_.end() && scriptIterator->second->lastModified == BaseLib::Io::getFileLastModifiedTime(file)) {
#if PHP_VERSION_ID < 70400
      zend_homegear_globals* globals = php_homegear_get_globals();
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
#else
      auto stream = new hg_stream_handle();
      stream->position = 0;
      stream->buffer = scriptIterator->second->script;

      handle->type = ZEND_HANDLE_STREAM;
      handle->opened_path = nullptr; //It might make sense to set this.
      handle->handle.stream.handle = stream;
      handle->handle.stream.reader = hg_zend_stream_reader;
      handle->handle.stream.fsizer = hg_zend_stream_fsizer;
      handle->handle.stream.isatty = 0;
      handle->handle.stream.closer = hg_zend_stream_closer;
#endif
      return SUCCESS;
    } else {
      std::vector<char> data = BaseLib::Io::getBinaryFileContent(file);
      int32_t pos = -1;
      for (uint32_t i = 0; i < 11 && i < data.size(); i++) {
        if (data[i] == ' ') {
          pos = (int32_t) i;
          break;
        }
      }
      if (pos == -1) {
        Homegear::GD::bl->out.printError("Error: License module id is missing in encrypted script file \"" + file + "\"");
        return FAILURE;
      }
      std::string moduleIdString(&data.at(0), static_cast<unsigned long>(pos));
      int32_t moduleId = BaseLib::Math::getNumber(moduleIdString);
      std::vector<char> input(data.data() + pos + 1, data.data() + data.size());
      if (input.empty()) return FAILURE;
      auto i = Homegear::GD::licensingModules.find(moduleId);
      if (i == Homegear::GD::licensingModules.end() || !i->second) {
        Homegear::GD::out.printError("Error: Could not decrypt script file. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
        return FAILURE;
      }
      std::shared_ptr<Homegear::ScriptEngine::CacheInfo> cacheInfo = std::make_shared<Homegear::ScriptEngine::CacheInfo>();
      i->second->decryptScript(input, cacheInfo->script);
      cacheInfo->lastModified = BaseLib::Io::getFileLastModifiedTime(file);
      if (!cacheInfo->script.empty()) {
#if PHP_VERSION_ID < 70400
        zend_homegear_globals* globals = php_homegear_get_globals();
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
#else
        auto stream = new hg_stream_handle();
        stream->position = 0;
        stream->buffer = cacheInfo->script;

        handle->type = ZEND_HANDLE_STREAM;
        handle->opened_path = nullptr; //It might make sense to set this.
        handle->handle.stream.handle = stream;
        handle->handle.stream.reader = hg_zend_stream_reader;
        handle->handle.stream.fsizer = hg_zend_stream_fsizer;
        handle->handle.stream.isatty = 0;
        handle->handle.stream.closer = hg_zend_stream_closer;
#endif
        return SUCCESS;
      } else return FAILURE;
    }
  } else {
    //{{{ From zend_stream_open in zend_stream.c
#if PHP_VERSION_ID < 70400
    handle->type = ZEND_HANDLE_FP;
    handle->opened_path = nullptr;
    handle->handle.fp = zend_fopen(filename, &handle->opened_path);
    handle->filename = filename;
    handle->free_filename = 0;
    memset(&handle->handle.stream.mmap, 0, sizeof(zend_mmap));
#else
    zend_string *opened_path = nullptr;
    auto fp = zend_fopen(handle->filename, &opened_path);
    if (!fp) {
      return FAILURE;
    }
#if PHP_VERSION_ID < 80200
    zend_stream_init_fp(handle, fp, filename);
    handle->opened_path = opened_path;
#else
    handle->type = ZEND_HANDLE_FP;
    handle->handle.fp = fp;
    handle->opened_path = opened_path;
#endif
#endif
    //}}}
  }

  return (handle->handle.fp) ? SUCCESS : FAILURE;
}

static size_t php_homegear_read_post(char *buf, size_t count_bytes) {
  if (disposed_ || SEG(commandLine)) return 0;
  BaseLib::Http *http = &SEG(http);
  if (!http || http->getContentSize() == 0) return 0;
  size_t bytesRead = http->readContentStream(buf, count_bytes);
  if (Homegear::GD::bl->debugLevel >= 5 && bytesRead > 0) Homegear::GD::out.printDebug("Debug: Raw post data: " + std::string(buf, bytesRead));
  return bytesRead;
}

static char *php_homegear_read_cookies() {
  if (disposed_ || SEG(commandLine)) return 0;
  BaseLib::Http *http = &SEG(http);
  if (!http) return 0;
  if (!SEG(cookiesParsed)) {
    SEG(cookiesParsed) = true;
    //Conversion from (const char*) to (char*) is ok, because PHP makes a copy before parsing and does not change the original data.
    if (!http->getHeader().cookie.empty()) return (char *) http->getHeader().cookie.c_str();
  }
  return nullptr;
}

static size_t php_homegear_ub_write_string(std::string &string) {
  if (string.empty() || disposed_) return 0;
  if (SEG(outputCallback)) SEG(outputCallback)(string, false);
  else {
    if (string.size() > 2 && string.at(string.size() - 1) == '\n') {
      if (string.at(string.size() - 2) == '\r') string.resize(string.size() - 2);
      else string.resize(string.size() - 1);
    }
    if (SEG(peerId) != 0) Homegear::GD::out.printMessage("Script output (peer id: " + std::to_string(SEG(peerId)) + "): " + string);
    else Homegear::GD::out.printMessage("Script output: " + string);
  }
  return string.size();
}

static size_t php_homegear_ub_write(const char *str, size_t length) {
  if (length == 0 || disposed_) return 0;
  if (SEG(outputCallback)) {
    std::string output(str, length);
    SEG(outputCallback)(output, false);
  } else {
    std::string output(str, length);
    if (length > 2 && *(str + length - 1) == '\n') {
      if (*(str + length - 2) == '\r') length -= 2;
      else length -= 1;
    }
    if (SEG(peerId) != 0) Homegear::GD::out.printMessage("Script output (peer id: " + std::to_string(SEG(peerId)) + "): " + output);
    else Homegear::GD::out.printMessage("Script output: " + output);
  }
  return length;
}

static void php_homegear_flush(void *server_context) {
  //We are storing or outputting all data immediately, so no flush is needed.
}

static int php_homegear_send_headers(sapi_headers_struct *sapi_headers) {
  if (disposed_ || SEG(commandLine)) return SAPI_HEADER_SENT_SUCCESSFULLY;
  if (!sapi_headers || !SEG(sendHeadersCallback)) return SAPI_HEADER_SEND_FAILED;
  BaseLib::PVariable headers(new BaseLib::Variable(BaseLib::VariableType::tStruct));
  if (!SEG(webRequest)) return SAPI_HEADER_SENT_SUCCESSFULLY;
  headers->structValue->emplace("RESPONSE_CODE", std::make_shared<BaseLib::Variable>(sapi_headers->http_response_code));
  zend_llist_element *element = sapi_headers->headers.head;
  while (element) {
    sapi_header_struct *header = (sapi_header_struct *) element->data;
    std::string temp(header->header, header->header_len);
    //PHP returns this sometimes
    if (temp.compare(0, 22, "Content-type: ext/html") == 0) {
      if (Homegear::GD::bl->settings.devLog()) {
        Homegear::GD::out.printError("PHP Content-Type error.");
        if (SG(default_mimetype)) Homegear::GD::out.printError("Default MIME type is: " + std::string(SG(default_mimetype)));
      }
      temp = "Content-Type: text/html; charset=UTF-8";
    }
    std::pair<std::string, std::string> dataPair = BaseLib::HelperFunctions::splitFirst(temp, ':');
    BaseLib::HelperFunctions::trim(dataPair.first);
    auto headerIterator = headers->structValue->find(dataPair.first);
    if (headerIterator == headers->structValue->end()) {
      auto headerElement = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      headerElement->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(dataPair.second));
      headers->structValue->emplace(dataPair.first, headerElement);
    } else {
      headerIterator->second->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(dataPair.second));
    }
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

#if PHP_VERSION_ID >= 80000
static void php_homegear_log_message(const char *message, int syslog_type_int)
#elif PHP_VERSION_ID >= 70100
static void php_homegear_log_message(char *message, int syslog_type_int)
#else
static void php_homegear_log_message(char* message)
#endif
{
  if (disposed_) return;
  std::string additionalInfo;
  if (SG(request_info).path_translated) additionalInfo += std::string(SG(request_info).path_translated);
  if (!SEG(nodeId).empty()) additionalInfo += (additionalInfo.empty() ? "" : ", ") + SEG(nodeId);
  std::string messageString(message);
  if (SEG(scriptInfo) && SEG(scriptInfo)->getType() == BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli && SEG(outputCallback) && PG(display_errors) == 0) {
    SEG(outputCallback)(messageString, true);
  }
  Homegear::GD::out.printError("Script engine" + (!additionalInfo.empty() ? std::string(" (") + additionalInfo + "): " : ": ") + messageString);
}

static void php_homegear_register_variables(zval *track_vars_array) {
  if (disposed_) return;
  if (SEG(commandLine)) {
    if (SG(request_info).path_translated) {
      php_register_variable((char *) "PHP_SELF", (char *) SG(request_info).path_translated, track_vars_array);
      php_register_variable((char *) "SCRIPT_NAME", (char *) SG(request_info).path_translated, track_vars_array);
      php_register_variable((char *) "SCRIPT_FILENAME", (char *) SG(request_info).path_translated, track_vars_array);
      php_register_variable((char *) "PATH_TRANSLATED", (char *) SG(request_info).path_translated, track_vars_array);
    }
    php_import_environment_variables(track_vars_array);
  } else {
    BaseLib::Http *http = &SEG(http);
    BaseLib::ScriptEngine::PScriptInfo &scriptInfo = SEG(scriptInfo);
    if (!http || !scriptInfo) return;
    const BaseLib::Http::Header &header = http->getHeader();
    BaseLib::Rpc::ServerInfo::Info *server = (BaseLib::Rpc::ServerInfo::Info *) SG(server_context);
    zval value;

    //PHP makes copies of all values, so conversion from const to non-const is ok.
    if (server) {
      if (server->ssl) php_register_variable_safe((char *) "HTTPS", (char *) "on", 2, track_vars_array);
      else php_register_variable_safe((char *) "HTTPS", (char *) "", 0, track_vars_array);
      std::string connection = (header.connection & BaseLib::Http::Connection::keepAlive) ? "keep-alive" : "close";
      php_register_variable_safe((char *) "HTTP_CONNECTION", (char *) connection.c_str(), connection.size(), track_vars_array);
      php_register_variable_safe((char *) "DOCUMENT_ROOT", (char *) scriptInfo->contentPath.c_str(), scriptInfo->contentPath.size(), track_vars_array);
      php_register_variable_safe((char *) "SCRIPT_FILENAME", (char *) scriptInfo->fullPath.c_str(), scriptInfo->fullPath.size(), track_vars_array);
      php_register_variable_safe((char *) "SERVER_NAME", (char *) server->name.c_str(), server->name.size(), track_vars_array);
      php_register_variable_safe((char *) "SERVER_ADDR", (char *) server->address.c_str(), server->address.size(), track_vars_array);
      ZVAL_LONG(&value, server->port);
      php_register_variable_ex((char *) "SERVER_PORT", &value, track_vars_array);
      if (server->webSocket) php_register_variable_safe((char *) "WEBSOCKET_ENABLED", (char *) "true", 4, track_vars_array);
      else php_register_variable_safe((char *) "WEBSOCKET_ENABLED", (char *) "false", 5, track_vars_array);
      if (server->restServer) php_register_variable_safe((char *) "REST_ENABLED", (char *) "true", 4, track_vars_array);
      else php_register_variable_safe((char *) "REST_ENABLED", (char *) "false", 5, track_vars_array);
      if (server->rpcServer) php_register_variable_safe((char *) "RPC_ENABLED", (char *) "true", 4, track_vars_array);
      else php_register_variable_safe((char *) "RPC_ENABLED", (char *) "false", 5, track_vars_array);
      std::string authType = (server->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::basic) ? "basic" : "none";
      php_register_variable_safe((char *) "AUTH_TYPE", (char *) authType.c_str(), authType.size(), track_vars_array);
      std::string webSocketAuthType =
          (server->websocketAuthType == BaseLib::Rpc::ServerInfo::Info::AuthType::basic) ? "basic" : ((server->websocketAuthType == BaseLib::Rpc::ServerInfo::Info::AuthType::session) ? "session"
                                                                                                                                                                                       : "none");
      php_register_variable_safe((char *) "WEBSOCKET_AUTH_TYPE", (char *) webSocketAuthType.c_str(), webSocketAuthType.size(), track_vars_array);
    }

    std::string version = std::string("Homegear ") + Homegear::GD::baseLibVersion;
    php_register_variable_safe((char *) "SERVER_SOFTWARE", (char *) version.c_str(), version.size(), track_vars_array);
    php_register_variable_safe((char *) "SCRIPT_NAME", (char *) scriptInfo->relativePath.c_str(), scriptInfo->relativePath.size(), track_vars_array);
    std::string phpSelf = scriptInfo->relativePath + header.pathInfo;
    php_register_variable_safe((char *) "PHP_SELF", (char *) phpSelf.c_str(), phpSelf.size(), track_vars_array);
    if (!header.pathInfo.empty()) {
      php_register_variable_safe((char *) "PATH_INFO", (char *) header.pathInfo.c_str(), header.pathInfo.size(), track_vars_array);
      if (SG(request_info).path_translated) php_register_variable((char *) "PATH_TRANSLATED", (char *) SG(request_info).path_translated, track_vars_array);
    }
    php_register_variable_safe((char *) "HTTP_HOST", (char *) header.host.c_str(), header.host.size(), track_vars_array);
    php_register_variable_safe((char *) "QUERY_STRING", (char *) header.args.c_str(), header.args.size(), track_vars_array);
    std::string redirectQueryString = http->getRedirectQueryString();
    if (!redirectQueryString.empty()) php_register_variable_safe((char *) "REDIRECT_QUERY_STRING", (char *) redirectQueryString.c_str(), redirectQueryString.size(), track_vars_array);
    std::string redirectUrl = http->getRedirectUrl();
    if (!redirectUrl.empty()) php_register_variable_safe((char *) "REDIRECT_URL", (char *) redirectUrl.c_str(), redirectUrl.size(), track_vars_array);
    if (http->getRedirectStatus() != -1) {
      ZVAL_LONG(&value, http->getRedirectStatus());
      php_register_variable_ex((char *) "REDIRECT_STATUS", &value, track_vars_array);
    }
    php_register_variable_safe((char *) "SERVER_PROTOCOL", (char *) "HTTP/1.1", 8, track_vars_array);
    php_register_variable_safe((char *) "REMOTE_ADDR", (char *) header.remoteAddress.c_str(), header.remoteAddress.size(), track_vars_array);
    ZVAL_LONG(&value, header.remotePort);
    php_register_variable_ex((char *) "REMOTE_PORT", &value, track_vars_array);
    if (SG(request_info).request_method) php_register_variable((char *) "REQUEST_METHOD", (char *) SG(request_info).request_method, track_vars_array);
    if (SG(request_info).request_uri) php_register_variable((char *) "REQUEST_URI", SG(request_info).request_uri, track_vars_array);
    if (header.contentLength > 0) {
      ZVAL_LONG(&value, header.contentLength);
      php_register_variable_ex((char *) "CONTENT_LENGTH", &value, track_vars_array);
    }
    if (!header.contentType.empty()) php_register_variable_safe((char *) "CONTENT_TYPE", (char *) header.contentType.c_str(), header.contentType.size(), track_vars_array);
    for (std::map<std::string, std::string>::const_iterator i = header.fields.begin(); i != header.fields.end(); ++i) {
      std::string name = "HTTP_" + i->first;
      BaseLib::HelperFunctions::stringReplace(name, "-", "_");
      BaseLib::HelperFunctions::toUpper(name);
      php_register_variable_safe((char *) name.c_str(), (char *) i->second.c_str(), i->second.size(), track_vars_array);
    }
    auto nodeBlueUriPathsExcludedFromLogin = Homegear::GD::bl->settings.nodeBlueUriPathsExcludedFromLogin();
    if (!nodeBlueUriPathsExcludedFromLogin.empty()) {
      php_register_variable_safe((char *) "ANONYMOUS_NODE_BLUE_PATHS", (char *) nodeBlueUriPathsExcludedFromLogin.c_str(), nodeBlueUriPathsExcludedFromLogin.size(), track_vars_array);
    }
    if (scriptInfo->clientInfo) {
      if (scriptInfo->clientInfo->authenticated) {
        php_register_variable_safe((char *) "CLIENT_AUTHENTICATED", (char *) "true", 4, track_vars_array);
        if (!scriptInfo->clientInfo->user.empty())
          php_register_variable_safe((char *) "CLIENT_VERIFIED_USERNAME",
                                     (char *) scriptInfo->clientInfo->user.c_str(),
                                     scriptInfo->clientInfo->user.size(),
                                     track_vars_array);
      } else php_register_variable_safe((char *) "CLIENT_AUTHENTICATED", (char *) "false", 5, track_vars_array);
      if (scriptInfo->clientInfo->hasClientCertificate) {
        std::string sslClientVerify = "SUCCESS";
        php_register_variable_safe((char *) "SSL_CLIENT_VERIFY", (char *) sslClientVerify.c_str(), sslClientVerify.size(), track_vars_array);
        php_register_variable_safe((char *) "SSL_CLIENT_S_DN", (char *) scriptInfo->clientInfo->distinguishedName.c_str(), scriptInfo->clientInfo->distinguishedName.size(), track_vars_array);
        auto dnParts = BaseLib::HelperFunctions::splitAll(scriptInfo->clientInfo->distinguishedName, ',');
        for (auto &attribute : dnParts) {
          auto attributePair = BaseLib::HelperFunctions::splitFirst(attribute, '=');
          BaseLib::HelperFunctions::trim(attributePair.first);
          BaseLib::HelperFunctions::toUpper(attributePair.first);
          BaseLib::HelperFunctions::trim(attributePair.second);
          BaseLib::HelperFunctions::toLower(attributePair.second);
          if (attributePair.first.empty() || attributePair.second.empty()) continue;
          std::string name = "SSL_CLIENT_S_DN_" + attributePair.first;
          php_register_variable_safe((char *) name.c_str(), (char *) attributePair.second.c_str(), attributePair.second.size(), track_vars_array);
        }
      }
    } else {
      std::string sslClientVerify = "NONE";
      php_register_variable_safe((char *) "SSL_CLIENT_VERIFY", (char *) sslClientVerify.c_str(), sslClientVerify.size(), track_vars_array);
    }

    zval_ptr_dtor(&value);
  }
}

void php_homegear_invoke_rpc(std::string &methodName, BaseLib::PVariable &parameters, zval *return_value, bool wait) {
  if (SEG(id) == 0) {
    zend_throw_exception(homegear_exception_class_entry, "Script ID is unset. Please call \"registerThread\" before calling any Homegear specific method within threads.", -1);
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

/* RPC functions */
ZEND_FUNCTION(print_v) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  if (argc != 1 && argc != 2) {
    php_error_docref(nullptr, E_WARNING, "print_v() expects 1 or 2 arguments.");
    RETURN_NULL();
  }
  if (argc >= 2 && Z_TYPE(args[1]) != IS_TRUE && Z_TYPE(args[1]) != IS_FALSE) {
    php_error_docref(nullptr, E_WARNING, "Parameter \"return\" is not of type boolean.");
    RETURN_NULL();
  }

  bool returnString = (Z_TYPE(args[1]) == IS_TRUE);

  BaseLib::PVariable parameter = Homegear::PhpVariableConverter::getVariable(args);
  if (!parameter) RETURN_FALSE;
  std::string result = parameter->print();
  if (returnString) ZVAL_STRINGL(return_value, result.c_str(), result.size());
  else php_homegear_ub_write_string(result);
}

ZEND_FUNCTION(hg_generate_webssh_token) {
  std::string methodName("generateWebSshToken");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_get_script_id) {
  std::string id = std::to_string(SEG(id)) + ',' + SEG(token);
  ZVAL_STRINGL(return_value, id.c_str(), id.size());
}

ZEND_FUNCTION(hg_register_thread) {
  if (disposed_) RETURN_FALSE;
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string tokenPairString;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::registerThread().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "stringId is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) tokenPairString = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (tokenPairString.empty()) {
    php_error_docref(NULL, E_WARNING, "stringId must not be empty.");
    RETURN_FALSE;
  }

  int32_t scriptId;
  std::pair<std::string, std::string> tokenPair = BaseLib::HelperFunctions::splitFirst(tokenPairString, ',');
  scriptId = BaseLib::Math::getNumber(tokenPair.first, false);
  std::string token = tokenPair.second;
  std::shared_ptr<Homegear::PhpEvents> phpEvents;
  {
    std::lock_guard<std::mutex> eventsMapGuard(Homegear::PhpEvents::eventsMapMutex);
    std::map<int32_t, std::shared_ptr<Homegear::PhpEvents>>::iterator eventsIterator = Homegear::PhpEvents::eventsMap.find(scriptId);
    if (eventsIterator == Homegear::PhpEvents::eventsMap.end() || !eventsIterator->second || eventsIterator->second->getToken().empty() || eventsIterator->second->getToken() != token) RETURN_FALSE;
    phpEvents = eventsIterator->second;
  }
  SEG(id) = scriptId;
  SEG(token) = token;
  SEG(outputCallback) = phpEvents->getOutputCallback();
  SEG(rpcCallback) = phpEvents->getRpcCallback();
  SEG(logLevel) = phpEvents->getLogLevel();
  SEG(peerId) = phpEvents->getPeerId();
  SEG(nodeId) = phpEvents->getNodeId();
  RETURN_TRUE;
}

// {{{ Module functions
ZEND_FUNCTION(hg_list_modules) {
  if (disposed_) RETURN_NULL();

  std::string methodName("listModules");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_load_module) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

  std::string filename;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::loadModule().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) filename = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (filename.empty()) {
    php_error_docref(NULL, E_WARNING, "filename must not be empty.");
    ZVAL_LONG(return_value, -1);
    return;
  }

  std::string methodName("loadModule");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(filename)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_unload_module) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

  std::string filename;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::unloadModule().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) filename = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (filename.empty()) {
    php_error_docref(NULL, E_WARNING, "filename must not be empty.");
    ZVAL_LONG(return_value, -1);
    return;
  }

  std::string methodName("unloadModule");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(filename));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_reload_module) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();

  std::string filename;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::reloadModule().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) filename = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (filename.empty()) {
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

ZEND_FUNCTION(hg_auth) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;
  std::string password;
  if (argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::auth().");
  else if (argc == 2) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
    else {
      if (Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
    }
  }
  if (name.empty() || password.empty()) RETURN_FALSE;

  std::string methodName("auth");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(name)));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(password)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_create_user) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;
  std::string password;
  BaseLib::PVariable groups;
  BaseLib::PVariable metadata;

  if (argc > 4) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::createUser().");
  else if (argc > 3) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
    else {
      if (Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
    }

    if (Z_TYPE(args[2]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "groups is not of type array.");
    else {
      groups = Homegear::PhpVariableConverter::getVariable(&args[2]);
    }
  }
  if (argc == 4) {
    if (Z_TYPE(args[3]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "metadata is not of type array.");
    else {
      metadata = Homegear::PhpVariableConverter::getVariable(&args[3]);
    }
  }

  if (name.empty() || password.empty() || !groups || groups->arrayValue->empty()) RETURN_FALSE;

  std::string methodName("createUser");
  BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  parameters->arrayValue->reserve(metadata ? 4 : 3);
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(password));
  parameters->arrayValue->push_back(groups);
  if (metadata) parameters->arrayValue->push_back(metadata);
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_delete_user) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::deleteUser().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (name.empty()) RETURN_FALSE;

  std::string methodName("deleteUser");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(name)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_get_user_metadata) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;

  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::getUserMetadata().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (name.empty()) RETURN_FALSE;

  std::string methodName("getUserMetadata");
  BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_set_user_metadata) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;
  BaseLib::PVariable metadata;

  if (argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::setUserMetadata().");
  else if (argc == 2) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "metadata is not of type string.");
    else {
      metadata = Homegear::PhpVariableConverter::getVariable(&args[1]);
    }
  }
  if (name.empty() || !metadata) RETURN_FALSE;

  std::string methodName("setUserMetadata");
  BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  parameters->arrayValue->reserve(2);
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
  parameters->arrayValue->push_back(metadata);
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_set_user_privileges) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string user;

  if (argc > 1) php_error_docref(NULL, E_ERROR, "Too many arguments passed to Homegear::setUserPrivileges().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_ERROR, "user is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) user = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (user.empty()) php_error_docref(NULL, E_ERROR, "user is empty.");

  SEG(user) = user;
}

ZEND_FUNCTION(hg_set_language) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string language;

  if (argc > 1) php_error_docref(NULL, E_ERROR, "Too many arguments passed to Homegear::setLanguage().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_ERROR, "language is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) language = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (language.empty()) php_error_docref(NULL, E_ERROR, "language is empty.");

  SEG(language) = language;
}

ZEND_FUNCTION(hg_update_user) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;
  std::string password;
  BaseLib::PVariable groups;
  if (argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::updateUser().");
  else if (argc == 2) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
    else {
      if (Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
    }
  } else if (argc == 3) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
    else {
      if (Z_STRLEN(args[1]) > 0) password = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
    }

    if (Z_TYPE(args[2]) != IS_ARRAY) php_error_docref(NULL, E_WARNING, "groups is not of type array.");
    else {
      groups = Homegear::PhpVariableConverter::getVariable(&args[2]);
    }
  }
  if (name.empty() || password.empty()) RETURN_FALSE;

  std::string methodName("updateUser");
  BaseLib::PVariable parameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  parameters->arrayValue->reserve(groups ? 3 : 2);
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(name));
  parameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(password));
  if (groups) parameters->arrayValue->push_back(groups);
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_user_exists) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string name;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::userExists().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "name is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) name = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (name.empty()) RETURN_FALSE;

  std::string methodName("userExists");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(name)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_users) {
  if (disposed_) RETURN_NULL();
  std::string methodName("listUsers");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}
// }}}

ZEND_FUNCTION(hg_poll_event) {
  if (disposed_) RETURN_NULL();
  if (SEG(id) == 0) {
    zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
    RETURN_FALSE;
  }
  if (!SEG(user).empty()) {
    zend_throw_exception(homegear_exception_class_entry, "Can't poll events when user privileges are set.", -1);
    RETURN_FALSE;
  }
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  int32_t timeout = -1;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::pollEvent().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "timeout is not of type int.");
    else timeout = Z_LVAL(args[0]);
  }

  std::shared_ptr<Homegear::PhpEvents> phpEvents;
  {
    std::unique_lock<std::mutex> eventsMapGuard(Homegear::PhpEvents::eventsMapMutex);
    std::map<int32_t, std::shared_ptr<Homegear::PhpEvents>>::iterator eventsIterator = Homegear::PhpEvents::eventsMap.find(SEG(id));
    if (eventsIterator == Homegear::PhpEvents::eventsMap.end()) {
      eventsMapGuard.unlock();
      zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
      RETURN_FALSE;
    }
    if (!eventsIterator->second) eventsIterator->second = std::make_shared<Homegear::PhpEvents>(SEG(token), SEG(outputCallback), SEG(rpcCallback));
    phpEvents = eventsIterator->second;
  }
  std::shared_ptr<Homegear::PhpEvents::EventData> eventData = phpEvents->poll(timeout);
  if (eventData && eventData->type != Homegear::PhpEvents::EventDataType::undefined) {
    array_init(return_value);
    zval element;

    if (eventData->type == Homegear::PhpEvents::EventDataType::event ||
        eventData->type == Homegear::PhpEvents::EventDataType::newDevices ||
        eventData->type == Homegear::PhpEvents::EventDataType::deleteDevices ||
        eventData->type == Homegear::PhpEvents::EventDataType::updateDevice) {
      if (eventData->type == Homegear::PhpEvents::EventDataType::event) ZVAL_STRINGL(&element, "event", sizeof("event") - 1);
      else if (eventData->type == Homegear::PhpEvents::EventDataType::newDevices) ZVAL_STRINGL(&element, "newDevices", sizeof("newDevices") - 1);
      else if (eventData->type == Homegear::PhpEvents::EventDataType::deleteDevices) ZVAL_STRINGL(&element, "deleteDevices", sizeof("deleteDevices") - 1);
      else if (eventData->type == Homegear::PhpEvents::EventDataType::updateDevice) ZVAL_STRINGL(&element, "updateDevice", sizeof("updateDevice") - 1);
      add_assoc_zval_ex(return_value, "TYPE", sizeof("TYPE") - 1, &element);

      if (!eventData->source.empty()) {
        ZVAL_STRINGL(&element, eventData->source.c_str(), eventData->source.size());
        add_assoc_zval_ex(return_value, "EVENTSOURCE", sizeof("EVENTSOURCE") - 1, &element);
      }

      ZVAL_LONG(&element, eventData->id);
      add_assoc_zval_ex(return_value, "PEERID", sizeof("PEERID") - 1, &element);

      ZVAL_LONG(&element, eventData->channel);
      add_assoc_zval_ex(return_value, "CHANNEL", sizeof("CHANNEL") - 1, &element);

      if (!eventData->variable.empty()) {
        ZVAL_STRINGL(&element, eventData->variable.c_str(), eventData->variable.size());
        add_assoc_zval_ex(return_value, "VARIABLE", sizeof("VARIABLE") - 1, &element);
      }

      if (eventData->hint != -1) {
        ZVAL_LONG(&element, eventData->hint);
        add_assoc_zval_ex(return_value, "HINT", sizeof("HINT") - 1, &element);
      }

      if (eventData->value) {
        Homegear::PhpVariableConverter::getPHPVariable(eventData->value, &element);
        add_assoc_zval_ex(return_value, "VALUE", sizeof("VALUE") - 1, &element);
      }
    } else if (eventData->type == Homegear::PhpEvents::EventDataType::serviceMessage) {
      Homegear::PhpVariableConverter::getPHPVariable(eventData->value, return_value);
    } else if (eventData->type == Homegear::PhpEvents::EventDataType::variableProfileStateChanged) {
      ZVAL_STRINGL(&element, "variableProfileStateChanged", sizeof("variableProfileStateChanged") - 1);
      add_assoc_zval_ex(return_value, "TYPE", sizeof("TYPE") - 1, &element);

      ZVAL_LONG(&element, eventData->id);
      add_assoc_zval_ex(return_value, "PROFILEID", sizeof("PROFILEID") - 1, &element);

      if (eventData->value) {
        Homegear::PhpVariableConverter::getPHPVariable(eventData->value, &element);
        add_assoc_zval_ex(return_value, "STATE", sizeof("STATE") - 1, &element);
      }
    } else if (eventData->type == Homegear::PhpEvents::EventDataType::uiNotificationCreated || eventData->type == Homegear::PhpEvents::EventDataType::uiNotificationRemoved) {
      if (eventData->type == Homegear::PhpEvents::EventDataType::uiNotificationCreated) ZVAL_STRINGL(&element, "uiNotificationCreated", sizeof("uiNotificationCreated") - 1);
      else if (eventData->type == Homegear::PhpEvents::EventDataType::uiNotificationRemoved) ZVAL_STRINGL(&element, "uiNotificationRemoved", sizeof("uiNotificationRemoved") - 1);
      add_assoc_zval_ex(return_value, "TYPE", sizeof("TYPE") - 1, &element);

      ZVAL_LONG(&element, eventData->id);
      add_assoc_zval_ex(return_value, "NOTIFICATIONID", sizeof("NOTIFICATIONID") - 1, &element);
    } else if (eventData->type == Homegear::PhpEvents::EventDataType::uiNotificationAction) {
      ZVAL_STRINGL(&element, "uiNotificationAction", sizeof("uiNotificationAction") - 1);
      add_assoc_zval_ex(return_value, "TYPE", sizeof("TYPE") - 1, &element);

      ZVAL_LONG(&element, eventData->id);
      add_assoc_zval_ex(return_value, "NOTIFICATIONID", sizeof("NOTIFICATIONID") - 1, &element);

      ZVAL_STRINGL(&element, eventData->source.c_str(), eventData->source.size());
      add_assoc_zval_ex(return_value, "NOTIFICATIONTYPE", sizeof("NOTIFICATIONTYPE") - 1, &element);

      ZVAL_LONG(&element, eventData->value->integerValue64);
      add_assoc_zval_ex(return_value, "BUTTONID", sizeof("BUTTONID") - 1, &element);
    }
  } else
    RETURN_FALSE;
}

ZEND_FUNCTION(hg_list_rpc_clients) {
  if (disposed_) RETURN_NULL();
  std::string methodName("listRpcClients");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_peer_exists) {
  if (disposed_) RETURN_NULL();
  unsigned long peerId = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &peerId) != SUCCESS) RETURN_NULL();
  std::string methodName("peerExists");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) peerId)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_subscribe_peer) {
  if (disposed_) RETURN_NULL();
  if (SEG(id) == 0) {
    zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
    RETURN_FALSE;
  }
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  uint64_t peerId = 0;
  int32_t channel = -1;
  std::string variable;
  if (argc == 0) php_error_docref(NULL, E_WARNING, "Homegear::subscribePeer() expects 1 or 3 parameters.");
  else if (argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::subscribePeer().");
  else if (argc == 3) {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
    else {
      peerId = Z_LVAL(args[0]);
    }

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "channel is not of type integer.");
    else {
      channel = Z_LVAL(args[1]);
    }

    if (Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "variableName is not of type string.");
    else {
      if (Z_STRLEN(args[2]) > 0) variable = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
    }
  } else {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
    else {
      peerId = Z_LVAL(args[0]);
    }
  }

  std::shared_ptr<Homegear::PhpEvents> phpEvents;
  {
    std::unique_lock<std::mutex> eventsMapGuard(Homegear::PhpEvents::eventsMapMutex);
    std::map<int32_t, std::shared_ptr<Homegear::PhpEvents>>::iterator eventsIterator = Homegear::PhpEvents::eventsMap.find(SEG(id));
    if (eventsIterator == Homegear::PhpEvents::eventsMap.end()) {
      eventsMapGuard.unlock();
      zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
      RETURN_FALSE;
    }
    if (!eventsIterator->second) eventsIterator->second.reset(new Homegear::PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
    phpEvents = eventsIterator->second;
  }
  phpEvents->addPeer(peerId, channel, variable);
}

ZEND_FUNCTION(hg_unsubscribe_peer) {
  if (disposed_) RETURN_NULL();
  if (SEG(id) == 0) {
    zend_throw_exception(homegear_exception_class_entry, "Script id is unset. Did you call \"registerThread\"?", -1);
    RETURN_FALSE;
  }
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  uint64_t peerId = 0;
  int32_t channel = -1;
  std::string variable;
  if (argc == 0) php_error_docref(NULL, E_WARNING, "Homegear::unsubscribePeer() expects 1 or 3 parameters.");
  else if (argc > 3) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::unsubscribePeer().");
  else if (argc == 3) {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
    else {
      peerId = Z_LVAL(args[0]);
    }

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "channel is not of type integer.");
    else {
      channel = Z_LVAL(args[1]);
    }

    if (Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "variableName is not of type string.");
    else {
      if (Z_STRLEN(args[2]) > 0) variable = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
    }
  } else {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "peerId is not of type integer.");
    else {
      peerId = Z_LVAL(args[0]);
    }
  }
  std::shared_ptr<Homegear::PhpEvents> phpEvents;
  {
    std::unique_lock<std::mutex> eventsMapGuard(Homegear::PhpEvents::eventsMapMutex);
    std::map<int32_t, std::shared_ptr<Homegear::PhpEvents>>::iterator eventsIterator = Homegear::PhpEvents::eventsMap.find(SEG(id));
    if (eventsIterator == Homegear::PhpEvents::eventsMap.end()) {
      eventsMapGuard.unlock();
      zend_throw_exception(homegear_exception_class_entry, "Script id is invalid.", -1);
      RETURN_FALSE;
    }
    if (!eventsIterator->second) eventsIterator->second.reset(new Homegear::PhpEvents(SEG(token), SEG(outputCallback), SEG(rpcCallback)));
    phpEvents = eventsIterator->second;
  }
  phpEvents->removePeer(peerId, channel, variable);
}

ZEND_FUNCTION(hg_log) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  int32_t logLevel = 3;
  std::string message;
  if (argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::log().");
  else if (argc == 2) {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "logLevel is not of type integer.");
    else {
      logLevel = Z_LVAL(args[0]);
    }

    if (Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "message is not of type string.");
    else {
      if (Z_STRLEN(args[1]) > 0) message = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
    }
  }
  if (message.empty()) RETURN_FALSE;

  bool errorLog = true;
  if (SEG(logLevel) > -1) {
    if (logLevel <= SEG(logLevel)) {
      if (logLevel > 3) errorLog = false;
      logLevel = 0;
    } else
      RETURN_TRUE;
  }

  if (SEG(peerId) != 0) Homegear::GD::out.printMessage("Script log (peer id: " + std::to_string(SEG(peerId)) + "): " + message, logLevel, errorLog);
  else if (!SEG(nodeId).empty()) Homegear::GD::out.printMessage("Script log (node id: " + SEG(nodeId) + "): " + message, logLevel, errorLog);
  else Homegear::GD::out.printMessage("Script log: " + message, logLevel, errorLog);
  RETURN_TRUE;
}

ZEND_FUNCTION(hg_set_script_log_level) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  int32_t logLevel = -1;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::setScriptLoglevel().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "logLevel is not of type integer.");
    else {
      logLevel = Z_LVAL(args[0]);
    }
  }
  if (logLevel < 0 || logLevel > 10) RETURN_FALSE;
  SEG(logLevel) = logLevel;

  {
    std::lock_guard<std::mutex> eventsMapGuard(Homegear::PhpEvents::eventsMapMutex);
    std::map<int32_t, std::shared_ptr<Homegear::PhpEvents>>::iterator eventsIterator = Homegear::PhpEvents::eventsMap.find(SEG(id));
    if (eventsIterator == Homegear::PhpEvents::eventsMap.end() || !eventsIterator->second || eventsIterator->second->getToken().empty() || eventsIterator->second->getToken() != SEG(token))
      RETURN_FALSE;
    eventsIterator->second->setLogLevel(logLevel);
  }

  RETURN_TRUE;
}

ZEND_FUNCTION(hg_get_http_contents) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string hostname;
  int32_t port = 443;
  std::string path;
  std::string caFile;
  bool verifyCertificate = true;
  if (argc > 5) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::getHttpContents().");
  else if (argc == 5) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "hostname is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) hostname = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "port is not of type integer.");
    else {
      port = Z_LVAL(args[1]);
    }

    if (Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "path is not of type string.");
    else {
      if (Z_STRLEN(args[2]) > 0) path = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
    }

    if (Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "caFile is not of type string.");
    else {
      if (Z_STRLEN(args[3]) > 0) caFile = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
    }

    if (Z_TYPE(args[4]) != IS_TRUE && Z_TYPE(args[4]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "verifyCertificate is not of type boolean.");
    else {
      verifyCertificate = (Z_TYPE(args[4]) == IS_TRUE);
    }
  }
  if (hostname.empty() || path.empty() || caFile.empty() || port < 1 || port > 65535) RETURN_FALSE;

  std::string data;
  try {
    BaseLib::HttpClient client(Homegear::GD::bl.get(), hostname, port, false, true, caFile, verifyCertificate);
    client.get(path, data);
  }
  catch (std::exception &ex) {
    Homegear::GD::out.printError("Error downloading file: " + std::string(ex.what()));
    RETURN_FALSE;
  }

  ZVAL_STRINGL(return_value, data.c_str(), data.size());
}

ZEND_FUNCTION(hg_download) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string hostname;
  int32_t port = 443;
  std::string path;
  std::string filename;
  std::string caFile;
  bool verifyCertificate = true;
  if (argc > 6) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::download().");
  else if (argc == 6) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "hostname is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) hostname = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "port is not of type integer.");
    else {
      port = Z_LVAL(args[1]);
    }

    if (Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "path is not of type string.");
    else {
      if (Z_STRLEN(args[2]) > 0) path = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
    }

    if (Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "filename is not of type string.");
    else {
      if (Z_STRLEN(args[3]) > 0) filename = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
    }

    if (Z_TYPE(args[4]) != IS_STRING) php_error_docref(NULL, E_WARNING, "caFile is not of type string.");
    else {
      if (Z_STRLEN(args[4]) > 0) caFile = std::string(Z_STRVAL(args[4]), Z_STRLEN(args[4]));
    }

    if (Z_TYPE(args[5]) != IS_TRUE && Z_TYPE(args[5]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "verifyCertificate is not of type boolean.");
    else {
      verifyCertificate = (Z_TYPE(args[5]) == IS_TRUE);
    }
  }
  if (hostname.empty() || path.empty() || filename.empty() || caFile.empty() || port < 1 || port > 65535) RETURN_FALSE;

  BaseLib::Http http;
  try {
    BaseLib::HttpClient client(Homegear::GD::bl.get(), hostname, port, false, true, caFile, verifyCertificate);
    client.get(path, http);

    if (http.getHeader().responseCode != 200) RETURN_FALSE;
    if (http.getContentSize() <= 1) RETURN_FALSE;
    BaseLib::Io::writeFile(filename, http.getContent(), http.getContentSize());
  }
  catch (std::exception &ex) {
    Homegear::GD::out.printError("Error downloading file: " + std::string(ex.what()));
    RETURN_FALSE;
  }

  RETURN_TRUE;
}

ZEND_FUNCTION(hg_json_encode) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  BaseLib::PVariable value;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::jsonEncode().");
  else if (argc == 1) {
    value = Homegear::PhpVariableConverter::getVariable(&args[0]);
  }
  if (!value) RETURN_FALSE;

  BaseLib::Rpc::JsonEncoder jsonEncoder(Homegear::GD::bl.get());
  std::string json;
  jsonEncoder.encode(value, json);

  ZVAL_STRINGL(return_value, json.c_str(), json.size());
}

ZEND_FUNCTION(hg_json_decode) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string json;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::jsonDecode().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "value is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) json = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (json.empty()) RETURN_FALSE;

  BaseLib::Rpc::JsonDecoder jsonDecoder(Homegear::GD::bl.get());
  auto value = jsonDecoder.decode(json);

  Homegear::PhpVariableConverter::getPHPVariable(value, return_value);
}

ZEND_FUNCTION(hg_ssdp_search) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string stHeader;
  int32_t searchTime = -1;
  if (argc != 2) php_error_docref(NULL, E_WARNING, "Wrong number of arguments passed to Homegear::ssdpSearch().");
  else if (argc == 2) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "stHeader is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) stHeader = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "searchTime is not of type integer.");
    else {
      searchTime = Z_LVAL(args[1]);
    }
  }
  if (stHeader.empty() || searchTime < 1 || searchTime > 120000) RETURN_FALSE;

  auto result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  try {
    BaseLib::Ssdp ssdp(Homegear::GD::bl.get());
    std::vector<BaseLib::SsdpInfo> searchResult;
    ssdp.searchDevices(stHeader, searchTime, searchResult);
    result->arrayValue->reserve(searchResult.size());

    for (auto &resultElement : searchResult) {
      auto info = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      info->structValue->emplace("ip", std::make_shared<BaseLib::Variable>(resultElement.ip()));
      info->structValue->emplace("location", std::make_shared<BaseLib::Variable>(resultElement.location()));
      info->structValue->emplace("port", std::make_shared<BaseLib::Variable>(resultElement.port()));

      auto fieldInfo = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      auto fields = resultElement.getFields();
      for (auto &field : fields) {
        fieldInfo->structValue->emplace(field.first, std::make_shared<BaseLib::Variable>(field.second));
      }
      info->structValue->emplace("additionalFields", fieldInfo);
      if (resultElement.info()) info->structValue->emplace("additionalInfo", resultElement.info());

      result->arrayValue->push_back(info);
    }

    Homegear::PhpVariableConverter::getPHPVariable(result, return_value);
  }
  catch (std::exception &ex) {
    Homegear::GD::out.printError("Error searching devices: " + std::string(ex.what()));
    RETURN_FALSE;
  }
}

ZEND_FUNCTION(hg_configure_gateway) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string host;
  int32_t port = -1;
  std::string caCert;
  std::string gatewayCert;
  std::string gatewayKey;
  std::string password;
  if (argc != 6) php_error_docref(NULL, E_WARNING, "Wrong number of arguments passed to Homegear::configureGateway().");
  else if (argc == 6) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "host is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) host = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "port is not of type integer.");
    else {
      port = Z_LVAL(args[1]);
    }

    if (Z_TYPE(args[2]) != IS_STRING) php_error_docref(NULL, E_WARNING, "caCert is not of type string.");
    else {
      if (Z_STRLEN(args[2]) > 0) caCert = std::string(Z_STRVAL(args[2]), Z_STRLEN(args[2]));
    }

    if (Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "gatewayCert is not of type string.");
    else {
      if (Z_STRLEN(args[3]) > 0) gatewayCert = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
    }

    if (Z_TYPE(args[4]) != IS_STRING) php_error_docref(NULL, E_WARNING, "gatewayKey is not of type string.");
    else {
      if (Z_STRLEN(args[4]) > 0) gatewayKey = std::string(Z_STRVAL(args[4]), Z_STRLEN(args[4]));
    }

    if (Z_TYPE(args[5]) != IS_STRING) php_error_docref(NULL, E_WARNING, "password is not of type string.");
    else {
      if (Z_STRLEN(args[5]) > 0) password = std::string(Z_STRVAL(args[5]), Z_STRLEN(args[5]));
    }
  }
  if (host.empty() || port < 1 || port > 65535 || caCert.empty() || gatewayCert.empty() || gatewayKey.empty() || password.empty()) RETURN_FALSE;

  try {
    BaseLib::Rpc::RpcEncoder rpcEncoder(Homegear::GD::bl.get(), true, true);
    BaseLib::Rpc::RpcDecoder rpcDecoder(Homegear::GD::bl.get(), false, false);

    auto parameters = std::make_shared<BaseLib::Array>();

    auto dataStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    dataStruct->structValue->emplace("caCert", std::make_shared<BaseLib::Variable>(caCert));
    dataStruct->structValue->emplace("gatewayCert", std::make_shared<BaseLib::Variable>(gatewayCert));
    dataStruct->structValue->emplace("gatewayKey", std::make_shared<BaseLib::Variable>(gatewayKey));

    std::vector<uint8_t> encodedData;
    rpcEncoder.encodeResponse(dataStruct, encodedData);
    dataStruct.reset();

    BaseLib::Security::Gcrypt aes(GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);

    std::vector<uint8_t> key;
    if (!BaseLib::Security::Hash::sha256(Homegear::GD::bl->hf.getUBinary(password), key) || key.empty()) {
      zend_throw_exception(homegear_exception_class_entry, "Could not encrypt data.", -1);
      RETURN_NULL();
    }
    aes.setKey(key);

    std::vector<uint8_t> iv = BaseLib::HelperFunctions::getRandomBytes(12);
    aes.setIv(iv);

    password.clear();
    key.clear();

    std::vector<uint8_t> counter(16);
    aes.setCounter(counter);

    std::vector<uint8_t> encryptedData;
    aes.encrypt(encryptedData, encodedData);
    encodedData.clear();

    parameters->emplace_back(std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::getHexString(iv) + BaseLib::HelperFunctions::getHexString(encryptedData)));

    std::vector<char> encodedRequest;
    rpcEncoder.encodeRequest("configure", parameters, encodedRequest);

    C1Net::TcpSocketInfo tcp_socket_info;
    C1Net::TcpSocketHostInfo tcp_socket_host_info;
    tcp_socket_host_info.host = host;
    tcp_socket_host_info.port = port;
    C1Net::TcpSocket socket(tcp_socket_info, tcp_socket_host_info);
    socket.Open();
    if (!socket.Connected()) {
      zend_throw_exception(homegear_exception_class_entry, "Could not connect to gateway.", -1);
      RETURN_NULL();
    }
    socket.Send((uint8_t *) encodedRequest.data(), encodedRequest.size());

    //{{{ Receive response
    ssize_t receivedBytes = 0;
    const int32_t bufferSize = 1024;
    std::array<char, bufferSize + 1> buffer;
    bool more_data = false;

    BaseLib::Rpc::BinaryRpc binaryRpc(Homegear::GD::bl.get());

    while (!binaryRpc.isFinished()) {
      try {
        receivedBytes = socket.Read((uint8_t *) buffer.data(), bufferSize, more_data);
        //Some clients send only one byte in the first packet
        if (receivedBytes == 1 && !binaryRpc.processingStarted()) receivedBytes += socket.Read((uint8_t *) buffer.data() + 1, bufferSize - 1, more_data);
      }
      catch (const C1Net::TimeoutException &ex) {
        zend_throw_exception(homegear_exception_class_entry, "Reading from gateway timed out", -2);
        RETURN_NULL();
      }
      catch (const C1Net::ClosedException &ex) {
        zend_throw_exception(homegear_exception_class_entry, ex.what(), -1);
        RETURN_NULL();
      }
      catch (const C1Net::Exception &ex) {
        zend_throw_exception(homegear_exception_class_entry, ex.what(), -1);
        RETURN_NULL();
      }

      //We are using string functions to process the buffer. So just to make sure,
      //they don't do something in the memory after buffer, we add '\0'
      buffer[receivedBytes] = '\0';

      try {
        int32_t processedBytes = binaryRpc.process(buffer.data(), receivedBytes);
        if (processedBytes < receivedBytes) {
          zend_throw_exception(homegear_exception_class_entry,
                               ("Received more bytes (" + std::to_string(receivedBytes) + ") than binary packet size (" + std::to_string(processedBytes) + ")").c_str(),
                               -1);
          RETURN_NULL();
        }
      }
      catch (BaseLib::Rpc::BinaryRpcException &ex) {
        zend_throw_exception(homegear_exception_class_entry, ex.what(), -1);
        RETURN_NULL();
      }
    }

    auto result = rpcDecoder.decodeResponse(binaryRpc.getData());
    if (result->errorStruct) {
      zend_throw_exception(homegear_exception_class_entry, result->structValue->at("faultString")->stringValue.c_str(), result->structValue->at("faultCode")->integerValue);
      RETURN_NULL();
    }
    //}}}

    Homegear::PhpVariableConverter::getPHPVariable(result, return_value);
  }
  catch (std::exception &ex) {
    zend_throw_exception(homegear_exception_class_entry, ex.what(), -1);
    RETURN_NULL();
  }
}

ZEND_FUNCTION(hg_check_license) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  int32_t moduleId = -1;
  int32_t familyId = -1;
  int32_t deviceId = -1;
  std::string licenseKey;
  if (argc > 4) php_error_docref(NULL, E_WARNING, "Too many arguments passed to Homegear::checkLicense().");
  else if (argc >= 3) {
    if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "licenseModuleId is not of type integer.");
    else moduleId = Z_LVAL(args[0]);

    if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "familyId is not of type integer.");
    else familyId = Z_LVAL(args[1]);

    if (Z_TYPE(args[2]) != IS_LONG) php_error_docref(NULL, E_WARNING, "deviceId is not of type integer.");
    else deviceId = Z_LVAL(args[2]);

    if (argc == 4) {
      if (Z_TYPE(args[3]) != IS_STRING) php_error_docref(NULL, E_WARNING, "licenseKey is not of type string.");
      else {
        if (Z_STRLEN(args[3]) > 0) licenseKey = std::string(Z_STRVAL(args[3]), Z_STRLEN(args[3]));
      }
    }
  }

  std::string methodName("checkLicense");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) moduleId)));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) familyId)));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) deviceId)));
  if (!licenseKey.empty()) parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(licenseKey)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_remove_license) {
  if (disposed_) RETURN_NULL();
  long moduleId = -1;
  long familyId = -1;
  long deviceId = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "lll", &moduleId, &familyId, &deviceId) != SUCCESS) RETURN_NULL();
  std::string methodName("removeLicense");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) moduleId)));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) familyId)));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) deviceId)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_get_license_states) {
  if (disposed_) RETURN_NULL();
  long moduleId = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &moduleId) != SUCCESS) RETURN_NULL();
  std::string methodName("getLicenseStates");
  BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
  parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((int32_t) moduleId)));
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_FUNCTION(hg_shutting_down) {
  if (disposed_ || Homegear::GD::bl->shuttingDown) RETURN_TRUE;
  RETURN_FALSE;
}

ZEND_FUNCTION(hg_gpio_export) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
  super_globals_.gpio->exportGpio(gpio);
}

ZEND_FUNCTION(hg_gpio_open) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
  super_globals_.gpio->openDevice(gpio, false);
}

ZEND_FUNCTION(hg_gpio_close) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
  super_globals_.gpio->closeDevice(gpio);
}

ZEND_FUNCTION(hg_gpio_set_direction) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  long direction = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &gpio, &direction) != SUCCESS) RETURN_NULL();
  super_globals_.gpio->setDirection(gpio, (BaseLib::LowLevel::Gpio::GpioDirection::Enum) direction);
}

ZEND_FUNCTION(hg_gpio_set_edge) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  long edge = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &gpio, &edge) != SUCCESS) RETURN_NULL();
  super_globals_.gpio->setEdge(gpio, (BaseLib::LowLevel::Gpio::GpioEdge::Enum) edge);
}

ZEND_FUNCTION(hg_gpio_get) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &gpio) != SUCCESS) RETURN_NULL();
  if (super_globals_.gpio->get(gpio)) {
    RETURN_TRUE;
  } else {
    RETURN_FALSE;
  }
}

ZEND_FUNCTION(hg_gpio_set) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  zend_bool value = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "lb", &gpio, &value) != SUCCESS) RETURN_NULL();
  super_globals_.gpio->set(gpio, (bool) value);
}

ZEND_FUNCTION(hg_gpio_poll) {
  if (disposed_) RETURN_NULL();
  long gpio = -1;
  long timeout = -1;
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll*", &gpio, &timeout, &args, &argc) != SUCCESS) RETURN_NULL();

  bool debounce = false;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearGpio::poll().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_TRUE && Z_TYPE(args[0]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "debounce is not of type bool.");
    else debounce = Z_TYPE(args[0]) == IS_TRUE;
  }

  if (gpio < 0 || timeout < 0) {
    ZVAL_LONG(return_value, -1);
    return;
  }

  ZVAL_LONG(return_value, super_globals_.gpio->poll(gpio, timeout, debounce));
}

ZEND_FUNCTION(hg_serial_open) {
  try {
    if (disposed_) RETURN_NULL();
    std::string device;
    long baudrate = 38400;
    bool evenParity = false;
    bool oddParity = false;
    BaseLib::SerialReaderWriter::CharacterSize characterSize = BaseLib::SerialReaderWriter::CharacterSize::Eight;
    bool twoStopBits = false;
    int argc = 0;
    zval *args = nullptr;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
    if (argc > 6) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearSerial::open().");
    else if (argc >= 1) {
      if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "device is not of type string.");
      else {
        if (Z_STRLEN(args[0]) > 0) device = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
      }

      if (argc >= 2) {
        if (Z_TYPE(args[1]) != IS_LONG) php_error_docref(NULL, E_WARNING, "baudrate is not of type integer.");
        else baudrate = Z_LVAL(args[1]);

        if (argc >= 3) {
          if (Z_TYPE(args[2]) != IS_TRUE && Z_TYPE(args[2]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "evenParity is not of type boolean.");
          else {
            evenParity = Z_TYPE(args[1]) == IS_TRUE;
          }

          if (argc >= 4) {
            if (Z_TYPE(args[3]) != IS_TRUE && Z_TYPE(args[3]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "oddParity is not of type boolean.");
            else {
              oddParity = Z_TYPE(args[3]) == IS_TRUE;
            }

            if (argc >= 5) {
              if (Z_TYPE(args[4]) != IS_LONG) php_error_docref(NULL, E_WARNING, "characterSize is not of type integer.");
              else if (Z_LVAL(args[4]) < 5 || Z_LVAL(args[4]) > 8) php_error_docref(NULL, E_WARNING, "Value for characterSize is invalid.");
              else {
                characterSize = (BaseLib::SerialReaderWriter::CharacterSize) Z_LVAL(args[4]);
              }

              if (argc == 6) {
                if (Z_TYPE(args[5]) != IS_TRUE && Z_TYPE(args[5]) != IS_FALSE) php_error_docref(NULL, E_WARNING, "twoStopBits is not of type boolean.");
                else {
                  twoStopBits = Z_TYPE(args[5]) == IS_TRUE;
                }
              }
            }
          }
        }
      }
    }
    if (device.empty()) RETURN_FALSE;

    std::shared_ptr<BaseLib::SerialReaderWriter> serialDevice(new BaseLib::SerialReaderWriter(Homegear::GD::bl.get(), device, baudrate, 0, true, -1));
    serialDevice->openDevice(evenParity, oddParity, false, characterSize, twoStopBits);
    if (serialDevice->isOpen()) {
      int32_t descriptor = serialDevice->fileDescriptor()->descriptor;
      super_globals_.serialDevicesMutex.lock();
      super_globals_.serialDevices.insert(std::pair<int, std::shared_ptr<BaseLib::SerialReaderWriter>>(descriptor, serialDevice));
      super_globals_.serialDevicesMutex.unlock();
      ZVAL_LONG(return_value, descriptor);
    } else {
      RETURN_FALSE;
    }
  }
  catch (BaseLib::SerialReaderWriterException &ex) {
    Homegear::GD::out.printError("Script engine: " + std::string(ex.what()));
    ZVAL_LONG(return_value, -1);
  }
}

ZEND_FUNCTION(hg_serial_close) {
  try {
    if (disposed_) RETURN_NULL();
    long id = -1;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &id) != SUCCESS) RETURN_NULL();
    super_globals_.serialDevicesMutex.lock();
    std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = super_globals_.serialDevices.find(id);
    if (deviceIterator != super_globals_.serialDevices.end()) {
      if (deviceIterator->second) deviceIterator->second->closeDevice();
      super_globals_.serialDevices.erase(id);
    }
    super_globals_.serialDevicesMutex.unlock();
    RETURN_TRUE;
  }
  catch (BaseLib::SerialReaderWriterException &ex) {
    Homegear::GD::out.printError("Script engine: " + std::string(ex.what()));
    RETURN_FALSE;
  }
}

ZEND_FUNCTION(hg_serial_read) {
  try {
    if (disposed_) RETURN_NULL();
    long id = -1;
    long timeout = -1;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &id, &timeout) != SUCCESS) RETURN_NULL();
    if (timeout < 0) {
      ZVAL_LONG(return_value, -1);
      return;
    }
    if (timeout > 5000) timeout = 5000;
    std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;
    super_globals_.serialDevicesMutex.lock();
    std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = super_globals_.serialDevices.find(id);
    if (deviceIterator != super_globals_.serialDevices.end()) {
      if (deviceIterator->second) serialReaderWriter = deviceIterator->second;
    }
    super_globals_.serialDevicesMutex.unlock();
    if (!serialReaderWriter) {
      ZVAL_LONG(return_value, -1);
      return;
    }
    char data;
    int32_t result = serialReaderWriter->readChar(data, timeout * 1000);
    if (result == -1) {
      ZVAL_LONG(return_value, -1);
      return;
    } else if (result == 1) {
      ZVAL_LONG(return_value, -2);
      return;
    }
    ZVAL_STRINGL(return_value, &data, 1);
  }
  catch (BaseLib::SerialReaderWriterException &ex) {
    Homegear::GD::out.printError("Script engine: " + std::string(ex.what()));
    ZVAL_LONG(return_value, -1);
  }
}

ZEND_FUNCTION(hg_serial_readline) {
  try {
    if (disposed_) RETURN_NULL();
    long id = -1;
    long timeout = -1;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &id, &timeout) != SUCCESS) RETURN_NULL();
    if (timeout < 0) {
      ZVAL_LONG(return_value, -1);
      return;
    }
    if (timeout > 5000) timeout = 5000;
    std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;
    super_globals_.serialDevicesMutex.lock();
    std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = super_globals_.serialDevices.find(id);
    if (deviceIterator != super_globals_.serialDevices.end()) {
      if (deviceIterator->second) serialReaderWriter = deviceIterator->second;
    }
    super_globals_.serialDevicesMutex.unlock();
    if (!serialReaderWriter) {
      ZVAL_LONG(return_value, -1);
      return;
    }
    std::string data;
    int32_t result = serialReaderWriter->readLine(data, timeout * 1000);
    if (result == -1) {
      ZVAL_LONG(return_value, -1);
      return;
    } else if (result == 1) {
      ZVAL_LONG(return_value, -2);
      return;
    }
    if (data.empty()) ZVAL_STRINGL(return_value, "", 0); //At least once, input->stringValue.c_str() on an empty string was a nullptr causing a segementation fault, so check for empty string
    else
      ZVAL_STRINGL(return_value, data.c_str(), data.size());
  }
  catch (BaseLib::SerialReaderWriterException &ex) {
    Homegear::GD::out.printError("Script engine: " + std::string(ex.what()));
    ZVAL_LONG(return_value, -1);
  }
}

ZEND_FUNCTION(hg_serial_write) {
  try {
    if (disposed_) RETURN_NULL();
    int argc = 0;
    zval *args = nullptr;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
    long id = -1;
    std::string data;
    if (argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to HomegearSerial::write().");
    else if (argc == 2) {
      if (Z_TYPE(args[0]) != IS_LONG) php_error_docref(NULL, E_WARNING, "handle is not of type integer.");
      else {
        id = Z_LVAL(args[0]);
      }

      if (Z_TYPE(args[1]) != IS_STRING) php_error_docref(NULL, E_WARNING, "data is not of type string.");
      else {
        if (Z_STRLEN(args[1]) > 0) data = std::string(Z_STRVAL(args[1]), Z_STRLEN(args[1]));
      }
    }
    if (data.empty()) RETURN_FALSE;

    std::shared_ptr<BaseLib::SerialReaderWriter> serialReaderWriter;

    {
      std::lock_guard<std::mutex> serialDevicesGuard(super_globals_.serialDevicesMutex);
      std::map<int, std::shared_ptr<BaseLib::SerialReaderWriter>>::iterator deviceIterator = super_globals_.serialDevices.find(id);
      if (deviceIterator != super_globals_.serialDevices.end()) {
        if (deviceIterator->second) serialReaderWriter = deviceIterator->second;
      }
    }
    if (!serialReaderWriter) RETURN_FALSE;
    if (data.size() > 0) serialReaderWriter->writeLine(data);
    RETURN_TRUE;
  }
  catch (BaseLib::SerialReaderWriterException &ex) {
    Homegear::GD::out.printError("Script engine: " + std::string(ex.what()));
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

    int32_t descriptor = open(device.c_str(), O_RDWR | O_CLOEXEC);
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
                data = Homegear::GD::bl->hf.getUBinary(hexData);
            }
        }
    }
    if(data.empty()) RETURN_FALSE;

    if (write(descriptor, data.data(), data.size()) != (signed)data.size()) RETURN_FALSE;
    RETURN_TRUE;
}
#endif

ZEND_FUNCTION(hg_getenv) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string varname;
  if (argc > 2) php_error_docref(NULL, E_WARNING, "Too many arguments passed to getenv().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "varname is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) varname = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (varname.empty()) {
    array_init(return_value);
    std::lock_guard<std::mutex> envGuard(env_mutex_);
    for (int32_t i = 0; environ[i] != nullptr; i++) {
      std::string entry(environ[i]);
      auto pair = BaseLib::HelperFunctions::splitFirst(entry, '=');
      zval value;
      if (pair.second.empty()) ZVAL_STRINGL(&value, "", 0); //At least once, *.c_str() on an empty string was a nullptr causing a segementation fault, so check for empty string
      else
        ZVAL_STRINGL(&value, pair.second.c_str(), pair.second.size());
      add_assoc_zval_ex(return_value, pair.first.c_str(), pair.first.size(), &value);
    }
  } else {
    std::string value;

    {
      std::lock_guard<std::mutex> envGuard(env_mutex_);
      auto variable = getenv((char *) varname.c_str());
      if (!variable) RETURN_FALSE;
      value = std::string(variable);
    }

    RETURN_STRINGL(value.c_str(), value.length());
  }
}

ZEND_FUNCTION(hg_putenv) {
  if (disposed_) RETURN_NULL();
  int argc = 0;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "*", &args, &argc) != SUCCESS) RETURN_NULL();
  std::string setting;
  if (argc > 1) php_error_docref(NULL, E_WARNING, "Too many arguments passed to putenv().");
  else if (argc == 1) {
    if (Z_TYPE(args[0]) != IS_STRING) php_error_docref(NULL, E_WARNING, "setting is not of type string.");
    else {
      if (Z_STRLEN(args[0]) > 0) setting = std::string(Z_STRVAL(args[0]), Z_STRLEN(args[0]));
    }
  }
  if (setting.empty()) RETURN_FALSE;

  int returnCode = -1;

  auto envPair = BaseLib::HelperFunctions::splitFirst(setting, '=');

  //Don't use putenv, as it requires the string to be persistent throughout the lifetime of the program.
  {
    if (envPair.second.empty()) {
      std::lock_guard<std::mutex> envGuard(env_mutex_);
      returnCode = unsetenv(envPair.first.c_str());
    } else {
      std::lock_guard<std::mutex> envGuard(env_mutex_);
      returnCode = setenv(envPair.first.c_str(), envPair.second.c_str(), 1);
    }
  }

  if (returnCode == 0) {
    RETURN_TRUE;
  } else {
    RETURN_FALSE;
  }
}

ZEND_METHOD(Homegear, __call) {
  if (disposed_) RETURN_NULL();
  zval *zMethodName = nullptr;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &zMethodName, &args) != SUCCESS) RETURN_NULL();
  std::string methodName(Z_STRVAL_P(zMethodName), Z_STRLEN_P(zMethodName));
  BaseLib::PVariable parameters = Homegear::PhpVariableConverter::getVariable(args, false, methodName == "createGroup" || methodName == "updateGroup");
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_METHOD(Homegear, __callStatic) {
  if (disposed_) RETURN_NULL();
  zval *zMethodName = nullptr;
  zval *args = nullptr;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "zz", &zMethodName, &args) != SUCCESS) RETURN_NULL();
  std::string methodName(Z_STRVAL_P(zMethodName), Z_STRLEN_P(zMethodName));
  BaseLib::PVariable parameters = Homegear::PhpVariableConverter::getVariable(args, false, methodName == "createGroup" || methodName == "updateGroup");
  php_homegear_invoke_rpc(methodName, parameters, return_value, true);
}

ZEND_BEGIN_ARG_INFO_EX(php_homegear_two_args, 0, 0, 2)
        ZEND_ARG_INFO(0, arg1)
        ZEND_ARG_INFO(0, arg2)
ZEND_END_ARG_INFO()

static const zend_function_entry homegear_methods[] = {
    ZEND_ME(Homegear, __call, php_homegear_two_args, ZEND_ACC_PUBLIC)
    ZEND_ME(Homegear, __callStatic, php_homegear_two_args, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(generateWebSshToken, hg_generate_webssh_token, hg_generate_webssh_token_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(getScriptId, hg_get_script_id, hg_get_script_id_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(registerThread, hg_register_thread, hg_register_thread_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(log, hg_log, hg_log_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(setScriptLogLevel, hg_set_script_log_level, hg_set_script_log_level_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(getHttpContents, hg_get_http_contents, hg_get_http_contents_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(download, hg_download, hg_download_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(jsonEncode, hg_json_encode, hg_json_encode_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(jsonDecode, hg_json_decode, hg_json_decode_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(ssdpSearch, hg_ssdp_search, hg_ssdp_search_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(configureGateway, hg_configure_gateway, hg_configure_gateway_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(pollEvent, hg_poll_event, hg_poll_event_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(setLanguage, hg_set_language, hg_set_language_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(setUserPrivileges, hg_set_user_privileges, hg_set_user_privileges_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(subscribePeer, hg_subscribe_peer, hg_subscribe_peer_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(unsubscribePeer, hg_unsubscribe_peer, hg_unsubscribe_peer_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(shuttingDown, hg_shutting_down, hg_shutting_down_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

static const zend_function_entry homegear_gpio_methods[] = {
    ZEND_ME_MAPPING(export, hg_gpio_export, hg_gpio_export_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(open, hg_gpio_open, hg_gpio_open_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(close, hg_gpio_close, hg_gpio_close_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(setDirection, hg_gpio_set_direction, hg_gpio_set_direction_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(setEdge, hg_gpio_set_edge, hg_gpio_set_edge_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(get, hg_gpio_get, hg_gpio_get_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(set, hg_gpio_set, hg_gpio_set_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(poll, hg_gpio_poll, hg_gpio_poll_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

static const zend_function_entry homegear_serial_methods[] = {
    ZEND_ME_MAPPING(open, hg_serial_open, hg_serial_open_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(close, hg_serial_close, hg_serial_close_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(read, hg_serial_read, hg_serial_read_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(readline, hg_serial_readline, hg_serial_readline_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    ZEND_ME_MAPPING(write, hg_serial_write, hg_serial_write_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    {NULL, NULL, NULL}
};

#ifdef I2CSUPPORT
static const zend_function_entry homegear_i2c_methods[] = {
        ZEND_ME_MAPPING(open, hg_i2c_open, hg_i2c_open_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        ZEND_ME_MAPPING(close, hg_i2c_close, hg_i2c_close_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        ZEND_ME_MAPPING(read, hg_i2c_read, hg_i2c_read_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        ZEND_ME_MAPPING(write, hg_i2c_write, hg_i2c_write_arg_info, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
        {NULL, NULL, NULL}
};
#endif

int php_homegear_init() {
  super_globals_.http = new BaseLib::Http();
  super_globals_.gpio = new BaseLib::LowLevel::Gpio(Homegear::GD::bl.get(), Homegear::GD::bl->settings.gpioPath());
  disposed_ = false;
  pthread_key_create(php_homegear_get_pthread_key(), pthread_data_destructor);

#if PHP_VERSION_ID < 70400
  int32_t maxScriptsPerProcess = Homegear::GD::bl->settings.scriptEngineMaxScriptsPerProcess();
  if(maxScriptsPerProcess < 1) maxScriptsPerProcess = 20;
  int32_t maxThreadsPerScript = Homegear::GD::bl->settings.scriptEngineMaxThreadsPerScript();
  if(maxThreadsPerScript < 1) maxThreadsPerScript = 4;
  int32_t threadCount = maxScriptsPerProcess * maxThreadsPerScript;
  if(tsrm_startup(threadCount, threadCount, 0, NULL) != 1) //Needs to be called once for the entire process (see TSRM.c)
  {
      Homegear::GD::out.printCritical("Critical: Could not start script engine. tsrm_startup returned error.");
      return FAILURE;
  }
#ifdef ZEND_SIGNALS
  zend_signal_startup();
#endif
#else
  if (php_tsrm_startup() != 1) //Needs to be called once for the entire process (see TSRM.c)
  {
    Homegear::GD::out.printCritical("Critical: Could not start script engine. tsrm_startup returned error.");
    return FAILURE;
  }
  zend_signal_startup();
#endif

  php_homegear_sapi_module.ini_defaults = homegear_ini_defaults;
  ini_path_override = strndup(Homegear::GD::bl->settings.phpIniPath().c_str(), Homegear::GD::bl->settings.phpIniPath().size());
  php_homegear_sapi_module.php_ini_path_override = ini_path_override;
  php_homegear_sapi_module.phpinfo_as_text = 0;
  php_homegear_sapi_module.php_ini_ignore_cwd = 1;

  /* From sapi/embed/php_embed.c of the PHP source code:
   * SAPI initialization (SINIT)
   *
   * Initialize the SAPI globals (memset to 0). After this point we can set
   * SAPI globals via the SG() macro.
   *
   * Reentrancy startup.
   *
   * This also sets 'php_embed_module.ini_entries = NULL' so we cannot
   * allocate the INI entries until after this call.
   */
  sapi_startup(&php_homegear_sapi_module);

  sapi_ini_entries = (char *) malloc(sizeof(HARDCODED_INI));
  memcpy(sapi_ini_entries, HARDCODED_INI, sizeof(HARDCODED_INI));
  php_homegear_sapi_module.ini_entries = sapi_ini_entries;

  sapi_module.startup(&php_homegear_sapi_module);

  return SUCCESS;
}

void php_homegear_deinit() {
  if (disposed_) return;
  disposed_ = true;
  php_homegear_sapi_module.shutdown(&php_homegear_sapi_module);
  sapi_shutdown();

  tsrm_shutdown(); //Needs to be called once for the entire process (see TSRM.c)
  if (ini_path_override) free(ini_path_override);
  if (sapi_ini_entries) free(sapi_ini_entries);
  if (super_globals_.http) {
    delete (super_globals_.http);
    super_globals_.http = nullptr;
  }
  if (super_globals_.gpio) {
    delete (super_globals_.gpio);
    super_globals_.gpio = nullptr;
  }
  super_globals_.serialDevicesMutex.lock();
  super_globals_.serialDevices.clear();
  super_globals_.serialDevicesMutex.unlock();
}

static int php_homegear_startup(sapi_module_struct *sapi_globals) {
#if PHP_VERSION_ID < 80200
  if (php_module_startup(sapi_globals, &homegear_module_entry, 1) == FAILURE) return FAILURE;
#else
  if (php_module_startup(sapi_globals, &homegear_module_entry) == FAILURE) return FAILURE;
#endif

  // {{{ Fix for bug #71115 which causes process to crash when excessively using $_GLOBALS. Remove once bug is fixed. => is fixed

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

static int php_homegear_shutdown(sapi_module_struct *sapi_globals) {
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

static int php_homegear_activate() {
  return SUCCESS;
}

static int php_homegear_deactivate() {
  return SUCCESS;
}

static PHP_MINIT_FUNCTION(homegear) {
  zend_class_entry homegearExceptionCe{};
  INIT_CLASS_ENTRY(homegearExceptionCe, "Homegear\\HomegearException", NULL);
  //Register child class inherited from Exception (fetched with "zend_exception_get_default")
  homegear_exception_class_entry = zend_register_internal_class_ex(&homegearExceptionCe, zend_exception_get_default());
  zend_declare_class_constant_long(homegear_exception_class_entry, "UNKNOWN_DEVICE", sizeof("UNKNOWN_DEVICE") - 1, -2);

  zend_class_entry homegearCe{};
  INIT_CLASS_ENTRY(homegearCe, "Homegear\\Homegear", homegear_methods);
  homegear_class_entry = zend_register_internal_class(&homegearCe);
  zend_declare_class_constant_stringl(homegear_class_entry, "TEMP_PATH", sizeof("TEMP_PATH") - 1, Homegear::GD::bl->settings.tempPath().c_str(), Homegear::GD::bl->settings.tempPath().size());
  zend_declare_class_constant_stringl(homegear_class_entry, "SCRIPT_PATH", sizeof("SCRIPT_PATH") - 1, Homegear::GD::bl->settings.scriptPath().c_str(), Homegear::GD::bl->settings.scriptPath().size());
  zend_declare_class_constant_stringl(homegear_class_entry, "MODULE_PATH", sizeof("MODULE_PATH") - 1, Homegear::GD::bl->settings.modulePath().c_str(), Homegear::GD::bl->settings.modulePath().size());
  zend_declare_class_constant_stringl(homegear_class_entry, "DATA_PATH", sizeof("DATA_PATH") - 1, Homegear::GD::bl->settings.dataPath().c_str(), Homegear::GD::bl->settings.dataPath().size());
  zend_declare_class_constant_stringl(homegear_class_entry,
                                      "WRITEABLE_DATA_PATH",
                                      sizeof("WRITEABLE_DATA_PATH") - 1,
                                      Homegear::GD::bl->settings.writeableDataPath().c_str(),
                                      Homegear::GD::bl->settings.writeableDataPath().size());
  zend_declare_class_constant_stringl(homegear_class_entry, "SOCKET_PATH", sizeof("SOCKET_PATH") - 1, Homegear::GD::bl->settings.socketPath().c_str(), Homegear::GD::bl->settings.socketPath().size());
  zend_declare_class_constant_stringl(homegear_class_entry,
                                      "LOGFILE_PATH",
                                      sizeof("LOGFILE_PATH") - 1,
                                      Homegear::GD::bl->settings.logfilePath().c_str(),
                                      Homegear::GD::bl->settings.logfilePath().size());
  zend_declare_class_constant_stringl(homegear_class_entry,
                                      "WORKING_DIRECTORY",
                                      sizeof("WORKING_DIRECTORY") - 1,
                                      Homegear::GD::bl->settings.workingDirectory().c_str(),
                                      Homegear::GD::bl->settings.workingDirectory().size());
  zend_declare_class_constant_stringl(homegear_class_entry,
                                      "NODE_BLUE_PATH",
                                      sizeof("NODE_BLUE_PATH") - 1,
                                      Homegear::GD::bl->settings.nodeBluePath().c_str(),
                                      Homegear::GD::bl->settings.nodeBluePath().size());
  zend_declare_class_constant_stringl(homegear_class_entry,
                                      "NODE_BLUE_DATA_PATH",
                                      sizeof("NODE_BLUE_DATA_PATH") - 1,
                                      Homegear::GD::bl->settings.nodeBlueDataPath().c_str(),
                                      Homegear::GD::bl->settings.nodeBlueDataPath().size());

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

static PHP_MSHUTDOWN_FUNCTION(homegear) {
  return SUCCESS;
}

static PHP_RINIT_FUNCTION(homegear) {
  return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(homegear) {
  return SUCCESS;
}

static PHP_MINFO_FUNCTION(homegear) {
  php_info_print_table_start();
  php_info_print_table_row(2, "Homegear support", "enabled");
  php_info_print_table_row(2, "Homegear version", Homegear::GD::baseLibVersion);
  php_info_print_table_end();
}

#endif
