AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
if HAVE_PHP_HEADERS
# {{{ PHP libraries
LIBS += -Wl,-Bstatic -lphp7-homegear -Wl,-Bdynamic -lz -lexslt -lresolv -ledit -lrt -lmcrypt -lltdl -lgmp -lenchant -lrt -lm -lnsl -lxml2 -lxslt -lcrypt -lcrypto -lssl -lgnutls
# }}}
endif

if HAVE_CURL
if HAVE_CURL_HEADERS
LIBS += -lcurl-gnutls
endif
endif

if HAVE_MYSQLCLIENT
LIBS += -lmysqlclient
endif


bin_PROGRAMS = homegear
homegear_SOURCES = main.cpp Monitor.cpp CLI/CLIClient.cpp CLI/CLIServer.cpp Database/SQLite3.cpp Events/EventHandler.cpp Flows/FlowsClient.cpp Flows/FlowsClientData.cpp Flows/FlowsProcess.cpp Flows/FlowsServer.cpp Flows/NodeManager.cpp Flows/SimplePhpNode.cpp Flows/StatefulPhpNode.cpp IPC/IpcClientData.cpp IPC/IpcServer.cpp GD/GD.cpp Licensing/LicensingController.cpp MQTT/Mqtt.cpp MQTT/MqttSettings.cpp RPC/Auth.cpp RPC/Client.cpp RPC/ClientSettings.cpp RPC/RemoteRpcServer.cpp RPC/RestServer.cpp RPC/RpcClient.cpp RPC/RPCMethods.cpp RPC/RPCServer.cpp RPC/Server.cpp WebServer/WebServer.cpp Systems/DatabaseController.cpp Systems/FamilyController.cpp UPnP/UPnP.cpp User/User.cpp
homegear_LDADD = -lpthread -lreadline -lgcrypt -lgnutls -lhomegear-base -lhomegear-node -lgpg-error -lsqlite3

if BSDSYSTEM
else
homegear_LDADD += -ldl
endif

#if WITH_SCRIPTENGINE
noinst_LIBRARIES = libscriptengine.a
libscriptengine_a_SOURCES = ScriptEngine/php_homegear_globals.cpp ScriptEngine/php_node.cpp ScriptEngine/php_sapi.cpp ScriptEngine/PhpVariableConverter.cpp ScriptEngine/PhpEvents.cpp ScriptEngine/PhpEvents.h ScriptEngine/ScriptEngineServer.cpp ScriptEngine/ScriptEngineServer.h ScriptEngine/ScriptEngineClient.cpp ScriptEngine/ScriptEngineClient.h ScriptEngine/ScriptEngineClientData.cpp ScriptEngine/ScriptEngineClientData.h ScriptEngine/ScriptEngineProcess.cpp ScriptEngine/ScriptEngineProcess.h
homegear_LDADD += libscriptengine.a
libscriptengine_a_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
if BSDSYSTEM
libscriptengine_a_CPPFLAGS += -I/usr/include/php7-homegear -I/usr/include/php7-homegear/main -I/usr/include/php7-homegear/sapi -I/usr/include/php7-homegear/TSRM -I/usr/include/php7-homegear/Zend -I/usr/include/php7-homegear/php -I/usr/include/php7-homegear/php/main -I/usr/include/php7-homegear/php/sapi -I/usr/include/php7-homegear/php/TSRM -I/usr/include/php7-homegear/php/Zend
else
libscriptengine_a_CPPFLAGS += -I/usr/include/php7-homegear -I/usr/include/php7-homegear/main -I/usr/include/php7-homegear/sapi -I/usr/include/php7-homegear/TSRM -I/usr/include/php7-homegear/Zend -I/usr/include/php7-homegear/php -I/usr/include/php7-homegear/php/main -I/usr/include/php7-homegear/php/sapi -I/usr/include/php7-homegear/php/TSRM -I/usr/include/php7-homegear/php/Zend
endif
#endif
