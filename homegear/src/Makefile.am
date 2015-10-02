AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
AM_LDFLAGS = -Wl,-rpath=/lib/homegear -Wl,-rpath=/usr/lib/homegear
# {{{ PHP libraries
LIBS += -Wl,-Bstatic -lphp7-homegear -Wl,-Bdynamic -lz -lexslt -lresolv -ltermcap -ledit -lrt -lmcrypt -lltdl -lgmp -lenchant -lqdbm -lrt -lm -lnsl -lxml2 -lxslt -lcrypt -lcrypto -lssl
# }}}

noinst_LIBRARIES = libpaho-mqtt.a
libpaho_mqtt_a_SOURCES = MQTT/paho.mqtt.c/src/Clients.c MQTT/paho.mqtt.c/src/Heap.c MQTT/paho.mqtt.c/src/LinkedList.c MQTT/paho.mqtt.c/src/Log.c MQTT/paho.mqtt.c/src/Messages.c MQTT/paho.mqtt.c/src/MQTTAsync.c MQTT/paho.mqtt.c/src/MQTTClient.c MQTT/paho.mqtt.c/src/MQTTClientPersistence.h MQTT/paho.mqtt.c/src/MQTTPacket.h MQTT/paho.mqtt.c/src/MQTTPacketOut.h MQTT/paho.mqtt.c/src/MQTTPersistenceDefault.c MQTT/paho.mqtt.c/src/MQTTPersistence.h MQTT/paho.mqtt.c/src/MQTTProtocolClient.h MQTT/paho.mqtt.c/src/MQTTProtocolOut.c MQTT/paho.mqtt.c/src/MQTTVersion.c MQTT/paho.mqtt.c/src/SocketBuffer.c MQTT/paho.mqtt.c/src/Socket.c MQTT/paho.mqtt.c/src/SSLSocket.c MQTT/paho.mqtt.c/src/StackTrace.c MQTT/paho.mqtt.c/src/Thread.c MQTT/paho.mqtt.c/src/Tree.c MQTT/paho.mqtt.c/src/utf-8.c MQTT/paho.mqtt.c/src/Clients.h MQTT/paho.mqtt.c/src/Heap.h MQTT/paho.mqtt.c/src/LinkedList.h MQTT/paho.mqtt.c/src/Log.h MQTT/paho.mqtt.c/src/Messages.h MQTT/paho.mqtt.c/src/MQTTAsync.h MQTT/paho.mqtt.c/src/MQTTClient.h MQTT/paho.mqtt.c/src/MQTTPacket.c MQTT/paho.mqtt.c/src/MQTTPacketOut.c MQTT/paho.mqtt.c/src/MQTTPersistence.c MQTT/paho.mqtt.c/src/MQTTPersistenceDefault.h MQTT/paho.mqtt.c/src/MQTTProtocolClient.c MQTT/paho.mqtt.c/src/MQTTProtocol.h MQTT/paho.mqtt.c/src/MQTTProtocolOut.h MQTT/paho.mqtt.c/src/SocketBuffer.h MQTT/paho.mqtt.c/src/Socket.h MQTT/paho.mqtt.c/src/SSLSocket.h MQTT/paho.mqtt.c/src/StackTrace.h MQTT/paho.mqtt.c/src/Thread.h MQTT/paho.mqtt.c/src/Tree.h MQTT/paho.mqtt.c/src/utf-8.h
libpaho_mqtt_a_CPPFLAGS = -Wall -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
bin_PROGRAMS = homegear
homegear_SOURCES = main.cpp CLI/CLIClient.cpp CLI/CLIClient.h CLI/CLIServer.cpp CLI/CLIServer.h Database/SQLite3.cpp Database/SQLite3.h Events/EventHandler.cpp Events/EventHandler.h GD/GD.cpp GD/GD.h MQTT/MQTT.cpp MQTT/MQTT.h MQTT/MQTTSettings.cpp MQTT/MQTTSettings.h RPC/Auth.cpp RPC/Auth.h RPC/Client.cpp RPC/Client.h RPC/ClientSettings.cpp RPC/ClientSettings.h RPC/RemoteRpcServer.cpp RPC/RemoteRpcServer.h RPC/RpcClient.cpp RPC/RpcClient.h RPC/RPCMethod.cpp RPC/RPCMethod.h RPC/RPCMethods.cpp RPC/RPCMethods.h RPC/RPCServer.cpp RPC/RPCServer.h RPC/Server.cpp RPC/Server.h RPC/Webserver.cpp RPC/Webserver.h Systems/DatabaseController.cpp Systems/DatabaseController.h Systems/FamilyController.cpp Systems/FamilyController.h Systems/PhysicalInterfaces.cpp Systems/PhysicalInterfaces.h UPnP/UPnP.cpp UPnP/UPnP.h User/User.cpp User/User.h
homegear_LDADD = -lpthread -lreadline -lgcrypt -lgnutls -lhomegear-base -lgpg-error -lsqlite3 libpaho-mqtt.a

if BSDSYSTEM
else
homegear_LDADD += -ldl
endif

if WITH_SCRIPTENGINE
noinst_LIBRARIES += libscriptengine.a
libscriptengine_a_SOURCES = ScriptEngine/php_sapi.cpp ScriptEngine/php_sapi.h ScriptEngine/PHPVariableConverter.cpp ScriptEngine/PHPVariableConverter.h ScriptEngine/ScriptEngine.cpp ScriptEngine/ScriptEngine.h
homegear_LDADD += libscriptengine.a
libscriptengine_a_CPPFLAGS = -Wall -std=c++11 -DFORTIFY_SOURCE=2 -DGCRYPT_NO_DEPRECATED
if BSDSYSTEM
libscriptengine_a_CPPFLAGS += -I/usr/include/php7-homegear -I/usr/include/php7-homegear/main -I/usr/include/php7-homegear/sapi -I/usr/include/php7-homegear/TSRM -I/usr/include/php7-homegear/Zend
else
libscriptengine_a_CPPFLAGS += -I/usr/include/php7-homegear -I/usr/include/php7-homegear/main -I/usr/include/php7-homegear/sapi -I/usr/include/php7-homegear/TSRM -I/usr/include/php7-homegear/Zend
endif
endif

if WITH_OPENSSL
homegear_LDADD += -lcrypto -lssl
endif