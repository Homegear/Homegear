cmake_minimum_required(VERSION 3.8)
project(Homegear)

set(CMAKE_CXX_STANDARD 17)

include_directories(/usr/include/nodejs-homegear /usr/include/php8-homegear /usr/include/php8-homegear/main /usr/include/php8-homegear/sapi /usr/include/php8-homegear/TSRM /usr/include/php8-homegear/Zend /usr/include/php8-homegear/php /usr/include/php8-homegear/php/main /usr/include/php8-homegear/php/sapi /usr/include/php8-homegear/php/TSRM /usr/include/php8-homegear/php/Zend)

set(SOURCE_FILES
        homegear-miscellaneous/src/Factory.cpp
        homegear-miscellaneous/src/Factory.h
        homegear-miscellaneous/src/Gd.cpp
        homegear-miscellaneous/src/Gd.h
        homegear-miscellaneous/src/MiscCentral.cpp
        homegear-miscellaneous/src/MiscCentral.h
        homegear-miscellaneous/src/Miscellaneous.cpp
        homegear-miscellaneous/src/Miscellaneous.h
        homegear-miscellaneous/src/MiscPeer.cpp
        homegear-miscellaneous/src/MiscPeer.h
        src/CLI/CliClient.cpp
        src/CLI/CliClient.h
        src/CLI/CliServer.cpp
        src/CLI/CliServer.h
        src/Database/DatabaseController.cpp
        src/Database/DatabaseController.h
        src/Database/SQLite3.cpp
        src/Database/SQLite3.h
        src/FamilyModules/FamilyModuleInfo.h
        src/FamilyModules/FamilyController.cpp
        src/FamilyModules/FamilyController.h
        src/FamilyModules/FamilyServer.cpp
        src/FamilyModules/FamilyServer.h
        src/FamilyModules/SocketCentral.cpp
        src/FamilyModules/SocketCentral.h
        src/FamilyModules/SocketDeviceFamily.cpp
        src/FamilyModules/SocketDeviceFamily.h
        src/FamilyModules/SocketPeer.cpp
        src/FamilyModules/SocketPeer.h
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
        src/RPC/Roles.cpp
        src/RPC/Roles.h
        src/RPC/RpcClient.cpp
        src/RPC/RpcClient.h
        src/RPC/RpcMethods/RPCMethods.cpp
        src/RPC/RpcMethods/RPCMethods.h
        src/RPC/RpcServer.cpp
        src/RPC/RpcServer.h
        src/ScriptEngine/CacheInfo.h
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
        src/UI/UiController.cpp
        src/UI/UiController.h
        src/UPnP/UPnP.cpp
        src/UPnP/UPnP.h
        src/User/User.cpp
        src/User/User.h
        src/WebServer/WebServer.cpp
        src/WebServer/WebServer.h
        src/main.cpp
        src/IpcLogger.cpp
        src/IpcLogger.h
        src/Database/SystemVariableController.cpp
        src/Database/SystemVariableController.h
        src/VariableProfiles/VariableProfileManager.cpp
        src/VariableProfiles/VariableProfileManager.h
        src/RPC/RpcMethods/VariableProfileRpcMethods.cpp
        src/RPC/RpcMethods/VariableProfileRpcMethods.h
        src/RPC/RpcMethods/UiRpcMethods.cpp
        src/RPC/RpcMethods/UiRpcMethods.h
        src/RPC/RpcMethods/BuildingRpcMethods.cpp
        src/RPC/RpcMethods/BuildingRpcMethods.h
        src/RPC/RpcMethods/BuildingPartRpcMethods.cpp
        src/RPC/RpcMethods/BuildingPartRpcMethods.h
        src/Node-BLUE/FlowParser.cpp
        src/Node-BLUE/FlowParser.h
        src/RPC/RpcMethods/NodeBlueRpcMethods.cpp
        src/RPC/RpcMethods/NodeBlueRpcMethods.h
        src/RPC/RpcMethods/UiNotificationsRpcMethods.cpp
        src/RPC/RpcMethods/UiNotificationsRpcMethods.h
        src/Node-BLUE/NodeBlueCredentials.cpp
        src/Node-BLUE/NodeBlueCredentials.h
        src/Node-BLUE/Node-PINK/Nodepink.cpp
        src/Node-BLUE/Node-PINK/Nodepink.h
        src/Node-BLUE/NodeRedNode.cpp
        src/Node-BLUE/NodeRedNode.h
        src/Node-BLUE/Node-PINK/NodepinkWebsocket.cpp
        src/Node-BLUE/Node-PINK/NodepinkWebsocket.h
        src/RPC/RpcMethods/MaintenanceRpcMethods.cpp
        src/RPC/RpcMethods/MaintenanceRpcMethods.h)

set(SOURCE_FILES_NODE
        src/Nodejs/Nodejs.cpp
        src/Nodejs/Nodejs.h
        src/Nodejs/main.cpp)

add_custom_target(homegear COMMAND ../../devscripts/makeAll.sh SOURCES ${SOURCE_FILES})
add_custom_target(homegear-node COMMAND ../../devscripts/makeAll.sh SOURCES ${SOURCE_FILES_NODE})

add_library(homegear-dummy ${SOURCE_FILES})
add_library(homegear-dummy2 ${SOURCE_FILES_NODE})