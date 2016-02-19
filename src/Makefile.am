AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
# {{{ PHP libraries
LIBS += -Wl,-Bstatic -lphp7-homegear -Wl,-Bdynamic -lz -lexslt -lresolv -ltermcap -ledit -lrt -lmcrypt -lltdl -lgmp -lenchant -lrt -lm -lnsl -lxml2 -lxslt -lcrypt -lcrypto -lssl
# }}}

bin_PROGRAMS = homegear
homegear_SOURCES = main.cpp Monitor.cpp Monitor.h CLI/CLIClient.cpp CLI/CLIClient.h CLI/CLIServer.cpp CLI/CLIServer.h Database/SQLite3.cpp Database/SQLite3.h Events/EventHandler.cpp Events/EventHandler.h GD/GD.cpp GD/GD.h Licensing/LicensingController.cpp Licensing/LicensingController.h MQTT/Mqtt.cpp MQTT/Mqtt.h MQTT/MqttSettings.cpp MQTT/MqttSettings.h RPC/Auth.cpp RPC/Auth.h RPC/Client.cpp RPC/Client.h RPC/ClientSettings.cpp RPC/ClientSettings.h RPC/RemoteRpcServer.cpp RPC/RemoteRpcServer.h RPC/RpcClient.cpp RPC/RpcClient.h RPC/RPCMethod.cpp RPC/RPCMethod.h RPC/RPCMethods.cpp RPC/RPCMethods.h RPC/RPCServer.cpp RPC/RPCServer.h RPC/Server.cpp RPC/Server.h RPC/Webserver.cpp RPC/Webserver.h Systems/DatabaseController.cpp Systems/DatabaseController.h Systems/FamilyController.cpp Systems/FamilyController.h UPnP/UPnP.cpp UPnP/UPnP.h User/User.cpp User/User.h
homegear_LDADD = -lpthread -lreadline -lgcrypt -lgnutls -lhomegear-base -lgpg-error -lsqlite3

if BSDSYSTEM
else
homegear_LDADD += -ldl
endif

#if WITH_SCRIPTENGINE
noinst_LIBRARIES = libscriptengine.a
libscriptengine_a_SOURCES = ScriptEngine/php_sapi.cpp ScriptEngine/php_sapi.h ScriptEngine/PhpVariableConverter.cpp ScriptEngine/PhpVariableConverter.h ScriptEngine/PhpEvents.cpp ScriptEngine/PhpEvents.h ScriptEngine/ScriptEngineServer.cpp ScriptEngine/ScriptEngineServer.h ScriptEngine/ScriptEngineClient.cpp ScriptEngine/ScriptEngineClient.h ScriptEngine/ScriptEngineClientData.cpp ScriptEngine/ScriptEngineClientData.h ScriptEngine/ScriptEngineProcess.cpp ScriptEngine/ScriptEngineProcess.h
homegear_LDADD += libscriptengine.a
libscriptengine_a_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
if BSDSYSTEM
libscriptengine_a_CPPFLAGS += -I/usr/include/php7-homegear -I/usr/include/php7-homegear/main -I/usr/include/php7-homegear/sapi -I/usr/include/php7-homegear/TSRM -I/usr/include/php7-homegear/Zend -I/usr/include/php7-homegear/php -I/usr/include/php7-homegear/php/main -I/usr/include/php7-homegear/php/sapi -I/usr/include/php7-homegear/php/TSRM -I/usr/include/php7-homegear/php/Zend
else
libscriptengine_a_CPPFLAGS += -I/usr/include/php7-homegear -I/usr/include/php7-homegear/main -I/usr/include/php7-homegear/sapi -I/usr/include/php7-homegear/TSRM -I/usr/include/php7-homegear/Zend -I/usr/include/php7-homegear/php -I/usr/include/php7-homegear/php/main -I/usr/include/php7-homegear/php/sapi -I/usr/include/php7-homegear/php/TSRM -I/usr/include/php7-homegear/php/Zend
endif
#endif

#if WITH_OPENSSL
#homegear_LDADD += -lcrypto -lssl
#endif
