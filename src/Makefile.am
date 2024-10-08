AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++17 -D_FORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,--export-dynamic -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear -Wl,-rpath=/usr/local/lib/homegear
if HAVE_PHP_HEADERS
# {{{ PHP libraries
LIBS += -Wl,-Bstatic -lphp8-homegear -Wl,-Bdynamic -lz -lexslt -lresolv -ledit -lrt -lltdl -lgmp -lrt -lm -lnsl -lxml2 -lxslt -licuuc -licui18n -licuio -lzip -lcrypto -lssl -lgnutls -lonig -lsodium -lutil
# }}}
if HAVE_ENCHANT2_HEADERS
LIBS += -lenchant-2
else
LIBS += -lenchant
endif
endif

if HAVE_CURL
if HAVE_CURL_HEADERS
LIBS += -lcurl-gnutls
endif
endif

if HAVE_MYSQLCLIENT
LIBS += -lmysqlclient
endif

LIBS += -latomic

bin_PROGRAMS = homegear homegear-node
homegear_SOURCES = main.cpp IpcLogger.cpp CLI/CliClient.cpp CLI/CliServer.cpp Database/DatabaseController.cpp Database/SQLite3.cpp Database/SystemVariableController.cpp FamilyModules/FamilyController.cpp FamilyModules/FamilyServer.cpp FamilyModules/SocketCentral.cpp FamilyModules/SocketDeviceFamily.cpp FamilyModules/SocketPeer.cpp Node-BLUE/Node-PINK/Nodepink.cpp Node-BLUE/Node-PINK/NodepinkWebsocket.cpp Node-BLUE/NodeBlueClient.cpp Node-BLUE/NodeBlueClientData.cpp Node-BLUE/NodeBlueCredentials.cpp Node-BLUE/FlowParser.cpp Node-BLUE/NodeBlueProcess.cpp Node-BLUE/NodeBlueServer.cpp Node-BLUE/NodeManager.cpp Node-BLUE/NodeRedNode.cpp Node-BLUE/SimplePhpNode.cpp Node-BLUE/StatefulPhpNode.cpp IPC/IpcClientData.cpp IPC/IpcServer.cpp GD/GD.cpp Licensing/LicensingController.cpp MQTT/Mqtt.cpp MQTT/MqttSettings.cpp  RPC/RpcMethods/BuildingPartRpcMethods.cpp RPC/RpcMethods/BuildingRpcMethods.cpp RPC/RpcMethods/MaintenanceRpcMethods.cpp RPC/RpcMethods/NodeBlueRpcMethods.cpp RPC/RpcMethods/RPCMethods.cpp RPC/RpcMethods/UiNotificationsRpcMethods.cpp RPC/RpcMethods/UiRpcMethods.cpp RPC/RpcMethods/VariableProfileRpcMethods.cpp RPC/Auth.cpp RPC/Client.cpp RPC/ClientSettings.cpp RPC/RemoteRpcServer.cpp RPC/RestServer.cpp RPC/Roles.cpp RPC/RpcClient.cpp RPC/RpcServer.cpp UI/UiController.cpp WebServer/WebServer.cpp UPnP/UPnP.cpp User/User.cpp VariableProfiles/VariableProfileManager.cpp
homegear_LDADD = -lpthread -lreadline -lgcrypt -lgnutls -lhomegear-base -lhomegear-node -lhomegear-ipc -lgpg-error -lsqlite3 -lc1-net -lz

if WITH_NODEJS
homegear_node_SOURCES = Nodejs/main.cpp Nodejs/Nodejs.cpp
homegear_node_LDADD = -lpthread -lnodejs-homegear
homegear_node_CPPFLAGS = -Wall -std=c++17 -D_FORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED -I/usr/include/nodejs-homegear
endif

if BSDSYSTEM
else
homegear_LDADD += -ldl
endif

if WITH_SCRIPTENGINE
noinst_LIBRARIES = libscriptengine.a
libscriptengine_a_SOURCES = ScriptEngine/php_homegear_globals.cpp ScriptEngine/php_device.cpp ScriptEngine/php_node.cpp ScriptEngine/php_sapi.cpp ScriptEngine/PhpVariableConverter.cpp ScriptEngine/PhpEvents.cpp ScriptEngine/PhpEvents.h ScriptEngine/ScriptEngineServer.cpp ScriptEngine/ScriptEngineServer.h ScriptEngine/ScriptEngineClient.cpp ScriptEngine/ScriptEngineClient.h ScriptEngine/ScriptEngineClientData.cpp ScriptEngine/ScriptEngineClientData.h ScriptEngine/ScriptEngineProcess.cpp ScriptEngine/ScriptEngineProcess.h
homegear_LDADD += libscriptengine.a
libscriptengine_a_CPPFLAGS = -Wall -std=c++17 -D_FORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
libscriptengine_a_CPPFLAGS += -I/usr/include/php8-homegear -I/usr/include/php8-homegear/main -I/usr/include/php8-homegear/sapi -I/usr/include/php8-homegear/TSRM -I/usr/include/php8-homegear/Zend -I/usr/include/php8-homegear/php -I/usr/include/php8-homegear/php/main -I/usr/include/php8-homegear/php/sapi -I/usr/include/php8-homegear/php/TSRM -I/usr/include/php8-homegear/php/Zend
else
AM_CPPFLAGS += -DNO_SCRIPTENGINE
endif

if WITH_I2C
AM_CPPFLAGS += -DI2CSUPPORT
endif
