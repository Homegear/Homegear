cmake_minimum_required(VERSION 3.8)
project(Homegear)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include/php7-homegear/ext /usr/include/php7-homegear/main /usr/include/php7-homegear/sapi /usr/include/php7-homegear/TSRM /usr/include/php7-homegear/Zend)

set(SOURCE_FILES
        homegear-miscellaneous/src/Factory.cpp
        homegear-miscellaneous/src/Factory.h
        homegear-miscellaneous/src/GD.cpp
        homegear-miscellaneous/src/GD.h
        homegear-miscellaneous/src/MiscCentral.cpp
        homegear-miscellaneous/src/MiscCentral.h
        homegear-miscellaneous/src/Miscellaneous.cpp
        homegear-miscellaneous/src/Miscellaneous.h
        homegear-miscellaneous/src/MiscPeer.cpp
        homegear-miscellaneous/src/MiscPeer.h
        src/CLI/CLIClient.cpp
        src/CLI/CLIClient.h
        src/CLI/CLIServer.cpp
        src/CLI/CLIServer.h
        src/Database/SQLite3.cpp
        src/Database/SQLite3.h
        src/Events/EventHandler.cpp
        src/Events/EventHandler.h
        src/Node-BLUE/FlowInfoClient.h
        src/Node-BLUE/FlowInfoServer.h
        src/Node-BLUE/NodeBlueClient.cpp
        src/Node-BLUE/NodeBlueClient.h
        src/Node-BLUE/NodeBlueClientData.cpp
        src/Node-BLUE/NodeBlueClientData.h
        src/Node-BLUE/NodeBlueProcess.cpp
        src/Node-BLUE/NodeBlueProcess.h
        src/Node-BLUE/NodeBlueResponseClient.h
        src/Node-BLUE/NodeBlueResponseServer.h
        src/Node-BLUE/NodeBlueServer.cpp
        src/Node-BLUE/NodeBlueServer.h
        src/Node-BLUE/NodeManager.cpp
        src/Node-BLUE/NodeManager.h
        src/Node-BLUE/SimplePhpNode.cpp
        src/Node-BLUE/SimplePhpNode.h
        src/Node-BLUE/StatefulPhpNode.cpp
        src/Node-BLUE/StatefulPhpNode.h
        src/GD/GD.cpp
        src/GD/GD.h
        src/IPC/IpcClientData.cpp
        src/IPC/IpcClientData.h
        src/IPC/IpcResponse.h
        src/IPC/IpcServer.cpp
        src/IPC/IpcServer.h
        src/Licensing/LicensingController.cpp
        src/Licensing/LicensingController.h
        src/MQTT/Mqtt.cpp
        src/MQTT/Mqtt.h
        src/MQTT/MqttSettings.cpp
        src/MQTT/MqttSettings.h
        src/RPC/Auth.cpp
        src/RPC/Auth.h
        src/RPC/Client.cpp
        src/RPC/Client.h
        src/RPC/ClientSettings.cpp
        src/RPC/ClientSettings.h
        src/RPC/RemoteRpcServer.cpp
        src/RPC/RemoteRpcServer.h
        src/RPC/RestServer.cpp
        src/RPC/RestServer.h
        src/RPC/RpcClient.cpp
        src/RPC/RpcClient.h
        src/RPC/RPCMethods.cpp
        src/RPC/RPCMethods.h
        src/RPC/RPCServer.cpp
        src/RPC/RPCServer.h
        src/RPC/Server.cpp
        src/RPC/Server.h
        src/ScriptEngine/php_config_fixes.h
        src/ScriptEngine/php_homegear_globals.cpp
        src/ScriptEngine/php_homegear_globals.h
        src/ScriptEngine/php_device.cpp
        src/ScriptEngine/php_device.h
        src/ScriptEngine/php_node.cpp
        src/ScriptEngine/php_node.h
        src/ScriptEngine/php_sapi.cpp
        src/ScriptEngine/php_sapi.h
        src/ScriptEngine/PhpEvents.cpp
        src/ScriptEngine/PhpEvents.h
        src/ScriptEngine/PhpVariableConverter.cpp
        src/ScriptEngine/PhpVariableConverter.h
        src/ScriptEngine/ScriptEngineClient.cpp
        src/ScriptEngine/ScriptEngineClient.h
        src/ScriptEngine/ScriptEngineClientData.cpp
        src/ScriptEngine/ScriptEngineClientData.h
        src/ScriptEngine/ScriptEngineProcess.cpp
        src/ScriptEngine/ScriptEngineProcess.h
        src/ScriptEngine/ScriptEngineResponse.h
        src/ScriptEngine/ScriptEngineServer.cpp
        src/ScriptEngine/ScriptEngineServer.h
        src/Systems/DatabaseController.cpp
        src/Systems/DatabaseController.h
        src/Systems/FamilyController.cpp
        src/Systems/FamilyController.h
        src/Systems/UiController.cpp
        src/Systems/UiController.h
        src/UPnP/UPnP.cpp
        src/UPnP/UPnP.h
        src/User/User.cpp
        src/User/User.h
        src/WebServer/WebServer.cpp
        src/WebServer/WebServer.h
        src/main.cpp
        src/Monitor.cpp
        src/Monitor.h
        config.h src/ScriptEngine/CacheInfo.h)

add_custom_target(homegear COMMAND ../../makeAll.sh SOURCES ${SOURCE_FILES})

add_library(homegear-dummy ${SOURCE_FILES})