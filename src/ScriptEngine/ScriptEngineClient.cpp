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

#include "ScriptEngineClient.h"
#include "PhpVariableConverter.h"
#include "php_config_fixes.h"
#include "../GD/GD.h"
#include "php_sapi.h"
#include "php_node.h"
#include "php_device.h"
#include <wordexp.h>

#include <memory>
#include <utility>
#include <zend_stream.h>

namespace Homegear {

namespace ScriptEngine {

std::mutex ScriptEngineClient::_resourceMutex;
std::mutex ScriptEngineClient::_nodeInfoMutex;
std::unordered_map<std::string, ScriptEngineClient::PNodeInfo> ScriptEngineClient::_nodeInfo;
std::mutex ScriptEngineClient::_deviceInfoMutex;
std::unordered_map<uint64_t, ScriptEngineClient::PDeviceInfo> ScriptEngineClient::_deviceInfo;

ScriptEngineClient::ScriptEngineClient() : IQueue(GD::bl.get(), 2, 100000) {
  signal(SIGPIPE, SIG_IGN);

  _stopped = false;
  _nodesStopped = false;

  _fileDescriptor = std::make_shared<BaseLib::FileDescriptor>();
  _out.init(GD::bl.get());
  _out.setPrefix("Script Engine (" + std::to_string(getpid()) + "): ");

  _dummyClientInfo.reset(new BaseLib::RpcClientInfo());

  _binaryRpc = std::unique_ptr<BaseLib::Rpc::BinaryRpc>(new BaseLib::Rpc::BinaryRpc(GD::bl.get()));
  _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get(), false, false));
  _rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get(), true, true));

  _localRpcMethods.emplace("devTest", std::bind(&ScriptEngineClient::devTest, this, std::placeholders::_1));
  _localRpcMethods.emplace("reload", std::bind(&ScriptEngineClient::reload, this, std::placeholders::_1));
  _localRpcMethods.emplace("shutdown", std::bind(&ScriptEngineClient::shutdown, this, std::placeholders::_1));
  _localRpcMethods.emplace("lifetick", std::bind(&ScriptEngineClient::lifetick, this, std::placeholders::_1));
  _localRpcMethods.emplace("getLoad", std::bind(&ScriptEngineClient::getLoad, this, std::placeholders::_1));
  _localRpcMethods.emplace("stopDevices", std::bind(&ScriptEngineClient::stopDevices, this, std::placeholders::_1));
  _localRpcMethods.emplace("executeScript", std::bind(&ScriptEngineClient::executeScript, this, std::placeholders::_1));
  _localRpcMethods.emplace("scriptCount", std::bind(&ScriptEngineClient::scriptCount, this, std::placeholders::_1));
  _localRpcMethods.emplace("getRunningScripts", std::bind(&ScriptEngineClient::getRunningScripts, this, std::placeholders::_1));
  _localRpcMethods.emplace("checkSessionId", std::bind(&ScriptEngineClient::checkSessionId, this, std::placeholders::_1));
  _localRpcMethods.emplace("executePhpNodeMethod", std::bind(&ScriptEngineClient::executePhpNodeMethod, this, std::placeholders::_1));
  _localRpcMethods.emplace("executeDeviceMethod", std::bind(&ScriptEngineClient::executeDeviceMethod, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastEvent", std::bind(&ScriptEngineClient::broadcastEvent, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastServiceMessage", std::bind(&ScriptEngineClient::broadcastServiceMessage, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastNewDevices", std::bind(&ScriptEngineClient::broadcastNewDevices, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastDeleteDevices", std::bind(&ScriptEngineClient::broadcastDeleteDevices, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUpdateDevice", std::bind(&ScriptEngineClient::broadcastUpdateDevice, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastVariableProfileStateChanged", std::bind(&ScriptEngineClient::broadcastVariableProfileStateChanged, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUiNotificationCreated", std::bind(&ScriptEngineClient::broadcastUiNotificationCreated, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUiNotificationRemoved", std::bind(&ScriptEngineClient::broadcastUiNotificationRemoved, this, std::placeholders::_1));
  _localRpcMethods.emplace("broadcastUiNotificationAction", std::bind(&ScriptEngineClient::broadcastUiNotificationAction, this, std::placeholders::_1));

  php_homegear_init();
}

ScriptEngineClient::~ScriptEngineClient() {
  dispose(true);
  php_homegear_deinit();
  if (_maintenanceThread.joinable()) _maintenanceThread.join();
  if (_watchdogThread.joinable()) _watchdogThread.join();
}

void ScriptEngineClient::stopEventThreads() {
  std::lock_guard<std::mutex> eventMapGuard(PhpEvents::eventsMapMutex);
  for (std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i) {
    if (i->second) i->second->stop();
  }
}

void ScriptEngineClient::dispose(bool broadcastShutdown) {
  try {
    BaseLib::PArray eventData
        (new BaseLib::Array{std::make_shared<BaseLib::Variable>(0), std::make_shared<BaseLib::Variable>(-1), std::make_shared<BaseLib::Variable>(BaseLib::PArray(new BaseLib::Array{std::make_shared<BaseLib::Variable>(std::string("DISPOSING"))})),
                            std::make_shared<BaseLib::Variable>(BaseLib::PArray(new BaseLib::Array{std::make_shared<BaseLib::Variable>(true)}))});

    GD::bl->shuttingDown = true;
    if (broadcastShutdown) broadcastEvent(eventData);

    int32_t i = 0;
    while (!_scriptThreads.empty() && i < 310) {
      if (i > 0 && i % 10 == 0) {
        GD::out.printInfo("Info: Waiting for script threads to finish (1). Scripts still running: " + std::to_string(_scriptThreads.size()));
        std::string ids = "IDs of running scripts: ";
        std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
        for (auto j = _scriptThreads.begin(); j != _scriptThreads.end(); ++j) {
          ids.append(std::to_string(j->first) + " ");
        }
        GD::out.printInfo(ids);
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      collectGarbage();
      i++;
    }
    if (!_scriptThreads.empty()) {
      GD::out.printError("Error: At least one script did not finish within 30 seconds during shutdown. Killing myself.");
      kill(getpid(), 9);
      return;
    }

    _stopped = true;
    stopEventThreads();
    stopQueue(0);
    stopQueue(1);
    _scriptCache.clear();
    _rpcResponses.clear();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::start() {
  try {
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.log").c_str(), "a", stdout)) {
      _out.printError("Error: Could not redirect output to log file.");
    }
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.err").c_str(), "a", stderr)) {
      _out.printError("Error: Could not redirect errors to log file.");
    }

    _threadCount = 10;
    _processingThreadCount1 = 0;
    _processingThreadCount2 = 0;

    startQueue(0, false, _threadCount, 0, SCHED_OTHER);
    startQueue(1, false, _threadCount, 0, SCHED_OTHER);

    if (GD::bl->settings.scriptEngineWatchdogTimeout() >= 1000) _watchdogThread = std::thread(&ScriptEngineClient::watchdog, this);

    _socketPath = GD::bl->settings.socketPath() + "homegearSE.sock";
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Socket path is " + _socketPath);
    for (int32_t i = 0; i < 2; i++) {
      _fileDescriptor = GD::bl->fileDescriptorManager.add(socket(AF_LOCAL, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0));
      if (!_fileDescriptor || _fileDescriptor->descriptor == -1) {
        _out.printError("Could not create socket.");
        return;
      }

      if (GD::bl->debugLevel >= 4 && i == 0) std::cout << "Info: Trying to connect..." << std::endl;
      sockaddr_un remoteAddress{};
      remoteAddress.sun_family = AF_LOCAL;
      //104 is the size on BSD systems - slightly smaller than in Linux
      if (_socketPath.length() > 104) {
        //Check for buffer overflow
        _out.printCritical("Critical: Socket path is too long.");
        return;
      }
      strncpy(remoteAddress.sun_path, _socketPath.c_str(), 104);
      remoteAddress.sun_path[103] = 0; //Just to make sure it is null terminated.
      if (connect(_fileDescriptor->descriptor, (struct sockaddr *)&remoteAddress, static_cast<socklen_t>(strlen(remoteAddress.sun_path) + 1 + sizeof(remoteAddress.sun_family))) == -1) {
        if (i == 0) {
          _out.printDebug("Debug: Socket closed. Trying again...");
          //When socket was not properly closed, we sometimes need to reconnect
          std::this_thread::sleep_for(std::chrono::milliseconds(2000));
          continue;
        } else {
          _out.printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
          return;
        }
      } else break;
    }
    if (GD::bl->debugLevel >= 4) _out.printMessage("Connected.");

    {
      std::lock_guard<std::mutex> maintenanceThreadGuard(_maintenanceThreadMutex);
      if (_maintenanceThread.joinable()) _maintenanceThread.join();
      _maintenanceThread = std::thread(&ScriptEngineClient::registerClient, this);
    }

    std::vector<char> buffer(1024);
    int32_t result = 0;
    int32_t bytesRead = 0;
    int32_t processedBytes = 0;
    while (!_stopped) {
      try {
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        fd_set readFileDescriptor{};
        FD_ZERO(&readFileDescriptor);
        {
          auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
          fileDescriptorGuard.lock();
          FD_SET(_fileDescriptor->descriptor, &readFileDescriptor);
        }

        result = select(_fileDescriptor->descriptor + 1, &readFileDescriptor, nullptr, nullptr, &timeout);
        if (result == 0) {
          if (GD::bl->hf.getTime() - _lastGargabeCollection > 60000) collectGarbage();
          continue;
        } else if (result == -1) {
          if (errno == EINTR) continue;
          GD::bl->fileDescriptorManager.close(_fileDescriptor);
          _out.printMessage("Connection to script server closed (1). Exiting.");
          if (_maintenanceThread.joinable()) _maintenanceThread.join();
          return;
        }

        bytesRead = static_cast<int32_t>(read(_fileDescriptor->descriptor, buffer.data(), buffer.size()));
        if (bytesRead <= 0) //read returns 0, when connection is disrupted.
        {
          GD::bl->fileDescriptorManager.close(_fileDescriptor);
          _out.printMessage("Connection to script server closed (2). Exiting.");
          if (_maintenanceThread.joinable()) _maintenanceThread.join();
          return;
        }

        if (bytesRead > (signed)buffer.size()) bytesRead = static_cast<int32_t>(buffer.size());

        try {
          processedBytes = 0;
          while (processedBytes < bytesRead) {
            processedBytes += _binaryRpc->process(buffer.data() + processedBytes, bytesRead - processedBytes);
            if (_binaryRpc->isFinished()) {
              if (_binaryRpc->getType() == BaseLib::Rpc::BinaryRpc::Type::request) {
                std::string methodName;
                BaseLib::PArray parameters = _rpcDecoder->decodeRequest(_binaryRpc->getData(), methodName);
                std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(methodName, parameters);
                if (methodName == "shutdown") shutdown(parameters->at(2)->arrayValue);
                else if (!enqueue(0, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC request because buffer is full. Dropping it.");
              } else {
                std::shared_ptr<BaseLib::IQueueEntry> queueEntry = std::make_shared<QueueEntry>(_binaryRpc->getData());
                if (!enqueue(1, queueEntry)) printQueueFullError(_out, "Error: Could not queue RPC response because buffer is full. Dropping it.");
              }
              _binaryRpc->reset();
            }
          }
        }
        catch (BaseLib::Rpc::BinaryRpcException &ex) {
          _out.printError("Error processing packet: " + std::string(ex.what()));
          _binaryRpc->reset();
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
    }
    buffer.clear();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

std::vector<std::string> ScriptEngineClient::getArgs(const std::string &path, const std::string &args) {
  std::vector<std::string> argv;
  if (!path.empty() && path.back() != '/') argv.push_back(path.substr(path.find_last_of('/') + 1));
  else argv.push_back(path);
  wordexp_t p{};
  if (wordexp(args.c_str(), &p, 0) == 0) {
    for (size_t i = 0; i < p.we_wordc; i++) {
      argv.push_back(std::string(p.we_wordv[i]));
    }
    wordfree(&p);
  }
  return argv;
}

void ScriptEngineClient::sendScriptFinished(int32_t exitCode) {
  try {
    zend_homegear_globals *globals = php_homegear_get_globals();
    std::string methodName("scriptFinished");
    BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(exitCode))});
    sendRequest(globals->id, globals->peerId, globals->user, globals->language, methodName, parameters, false);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::registerClient() {
  try {
    std::string methodName("registerScriptEngineClient");
    BaseLib::PArray parameters(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable((int32_t)getpid()))});
    BaseLib::PVariable result = sendGlobalRequest(methodName, parameters);
    if (result->errorStruct) {
      _out.printCritical("Critical: Could not register client.");
      dispose(false);
    }
    _out.printInfo("Info: Client registered to server.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry> &entry) {
  try {
    std::shared_ptr<QueueEntry> queueEntry;
    queueEntry = std::dynamic_pointer_cast<QueueEntry>(entry);
    if (!queueEntry) return;

    if (BaseLib::HelperFunctions::getTime() - queueEntry->time > 2000) {
      _lastQueueSlowErrorCounter++;
      if (BaseLib::HelperFunctions::getTime() - _lastQueueSlowError > 10000) {
        _lastQueueSlowError = BaseLib::HelperFunctions::getTime();
        _lastQueueSlowErrorCounter = 0;
        _out.printWarning(
            "Warning: Queue entry was queued for " + std::to_string(BaseLib::HelperFunctions::getTime() - queueEntry->time) + "ms. Either something is hanging or you need to increase your number of processing threads. Messages since last log entry: "
                + std::to_string(_lastQueueSlowErrorCounter));
      }
    }

    if (index == 0) //Request
    {
      _processingThreadCount1++;
      try {
        if (_processingThreadCount1 == _threadCount) _processingThreadCountMaxReached1 = BaseLib::HelperFunctions::getTime();
        if (queueEntry->parameters->size() < 3) {
          _out.printError("Error: Wrong parameter count while calling method " + queueEntry->methodName);
          return;
        }
        auto localMethodIterator = _localRpcMethods.find(queueEntry->methodName);
        if (localMethodIterator == _localRpcMethods.end()) {
          _out.printError("Warning: RPC method not found: " + queueEntry->methodName);
          BaseLib::PVariable error = BaseLib::Variable::createError(-32601, "Requested method not found in script engine client.");
          if (queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), error);
          return;
        }

        if (GD::bl->debugLevel >= 5) {
          _out.printInfo("Debug: Server is calling RPC method: " + queueEntry->methodName);
          for (const auto &parameter : *queueEntry->parameters) {
            parameter->print(true, false);
          }
        }

        BaseLib::PVariable result = localMethodIterator->second(queueEntry->parameters->at(2)->arrayValue);
        if (GD::bl->debugLevel >= 5) {
          _out.printDebug("Response: ");
          result->print(true, false);
        }
        if (queueEntry->parameters->at(1)->booleanValue) sendResponse(queueEntry->parameters->at(0), result);
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
      _processingThreadCountMaxReached1 = 0;
      _processingThreadCount1--;
    } else //Response
    {
      _processingThreadCount2++;
      try {
        if (_processingThreadCount2 == _threadCount) _processingThreadCountMaxReached2 = BaseLib::HelperFunctions::getTime();
        BaseLib::PVariable response = _rpcDecoder->decodeResponse(queueEntry->packet);
        if (response->arrayValue->size() < 3) {
          _out.printError("Error: Response has wrong array size.");
          return;
        }
        int32_t scriptId = response->arrayValue->at(0)->integerValue;
        int32_t packetId = response->arrayValue->at(1)->integerValue;

        {
          std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
          auto responseIterator = _rpcResponses[scriptId].find(packetId);
          if (responseIterator != _rpcResponses[scriptId].end()) {
            PScriptEngineResponse element = responseIterator->second;
            if (element) {
              element->response = response;
              element->packetId = packetId;
              element->finished = true;
            }
          }
        }
        if (scriptId != 0) {
          std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
          std::map<int32_t, PRequestInfo>::iterator requestIterator = _requestInfo.find(scriptId);
          if (requestIterator != _requestInfo.end()) {
            std::unique_lock<std::mutex> waitLock(requestIterator->second->waitMutex);

            {
              std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
              auto responseIterator = _rpcResponses[scriptId].find(packetId);
              if (responseIterator != _rpcResponses[scriptId].end()) {
                PScriptEngineResponse element = responseIterator->second;
                if (element) {
                  element->response = response;
                  element->packetId = packetId;
                  element->finished = true;
                }
              }
            }

            waitLock.unlock();
            requestIterator->second->conditionVariable.notify_all();
          }
        } else {
          std::unique_lock<std::mutex> waitLock(_waitMutex);

          {
            std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
            auto responseIterator = _rpcResponses[scriptId].find(packetId);
            if (responseIterator != _rpcResponses[scriptId].end()) {
              PScriptEngineResponse element = responseIterator->second;
              if (element) {
                element->response = response;
                element->packetId = packetId;
                element->finished = true;
              }
            }
          }

          waitLock.unlock();
          _requestConditionVariable.notify_all();
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
      _processingThreadCountMaxReached2 = 0;
      _processingThreadCount2--;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::sendOutput(std::string output, bool error) {
  try {
    zend_homegear_globals *globals = php_homegear_get_globals();
    std::string methodName("scriptOutput");
    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(2);
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(output));
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(error));
    sendRequest(globals->id, globals->peerId, globals->user, globals->language, methodName, parameters, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::sendHeaders(BaseLib::PVariable headers) {
  try {
    zend_homegear_globals *globals = php_homegear_get_globals();
    std::string methodName("scriptHeaders");
    auto parameters = std::make_shared<BaseLib::Array>();
    parameters->emplace_back(std::move(headers));
    sendRequest(globals->id, globals->peerId, globals->user, globals->language, methodName, parameters, true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

BaseLib::PVariable ScriptEngineClient::callMethod(std::string methodName, BaseLib::PVariable parameters, bool wait) {
  try {
    if (_nodesStopped) return BaseLib::Variable::createError(-32500, "RPC calls are forbidden after \"stop\" is executed.");
    zend_homegear_globals *globals = php_homegear_get_globals();
    return sendRequest(globals->id, globals->peerId, globals->user, globals->language, methodName, parameters->arrayValue, wait);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::send(std::vector<char> &data) {
  try {
    int32_t totallySentBytes = 0;
    std::lock_guard<std::mutex> sendGuard(_sendMutex);
    while (totallySentBytes < (signed)data.size()) {
      int32_t sentBytes = static_cast<int32_t>(::send(_fileDescriptor->descriptor, &data.at(0) + totallySentBytes, data.size() - totallySentBytes, MSG_NOSIGNAL));
      if (sentBytes <= 0) {
        if (errno == EAGAIN) continue;
        if (_fileDescriptor->descriptor != -1)
          _out.printError("Could not send data to client " + std::to_string(_fileDescriptor->descriptor) + ". Sent bytes: " + std::to_string(totallySentBytes) + " of " + std::to_string(data.size())
                              + (sentBytes == -1 ? ". Error message: " + std::string(strerror(errno)) : ""));
        return BaseLib::Variable::createError(-32500, "Unknown application error.");
      }
      totallySentBytes += sentBytes;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::PVariable(new BaseLib::Variable());
}

BaseLib::PVariable ScriptEngineClient::sendRequest(int32_t scriptId, uint64_t peerId, std::string &user, std::string &language, std::string methodName, BaseLib::PArray &parameters, bool wait) {
  try {
    std::unique_lock<std::mutex> requestInfoGuard(_requestInfoMutex);
    PRequestInfo requestInfo = _requestInfo[scriptId];
    if (!requestInfo) {
      _out.printError("Error: Request info does not exist.");
      return BaseLib::Variable::createError(-32500, "Unknown application error.");
    }
    requestInfoGuard.unlock();

    std::lock_guard<std::mutex> requestGuard(requestInfo->requestMutex);

    int32_t packetId;
    {
      std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
      packetId = _currentPacketId++;
    }
    BaseLib::PArray array = std::make_shared<BaseLib::Array>();
    array->reserve(4);
    auto scriptInfo = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    scriptInfo->structValue->emplace("scriptId", std::make_shared<BaseLib::Variable>(scriptId));
    scriptInfo->structValue->emplace("peerId", std::make_shared<BaseLib::Variable>(peerId));
    scriptInfo->structValue->emplace("user", std::make_shared<BaseLib::Variable>(user));
    scriptInfo->structValue->emplace("language", std::make_shared<BaseLib::Variable>(language));
    array->push_back(scriptInfo);
    array->push_back(std::make_shared<BaseLib::Variable>(packetId));
    array->push_back(std::make_shared<BaseLib::Variable>(wait));
    array->push_back(std::make_shared<BaseLib::Variable>(parameters));
    std::vector<char> data;
    _rpcEncoder->encodeRequest(methodName, array, data);

    PScriptEngineResponse response;
    if (wait) {
      {
        std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
        auto result = _rpcResponses[scriptId].emplace(packetId, std::make_shared<ScriptEngineResponse>());
        if (result.second) response = result.first->second;
      }
      if (!response) {
        _out.printCritical("Critical: Could not insert response struct into map.");
        return BaseLib::Variable::createError(-32500, "Unknown application error.");
      }
    }

    BaseLib::PVariable result = send(data);
    if (result->errorStruct || !wait) {
      if (!wait) return std::make_shared<BaseLib::Variable>();
      else {
        std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
        _rpcResponses[scriptId].erase(packetId);
        return result;
      }
    }

    std::unique_lock<std::mutex> waitLock(requestInfo->waitMutex);
    while (!requestInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&] {
      return GD::bl->shuttingDown || response->finished || _stopped;
    }));

    if (!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId) {
      if (GD::bl->shuttingDown) {
        _out.printInfo("Info: Aborting RPC request after 10 seconds, because I'm shutting down. Method: " + methodName);
        result = BaseLib::Variable::createError(-32501, "Shutting down.");
      } else {
        _out.printError("Error: No response received to RPC request. Method: " + methodName);
        result = BaseLib::Variable::createError(-1, "No response received.");
      }
    } else result = response->response->arrayValue->at(2);

    {
      std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
      _rpcResponses[scriptId].erase(packetId);
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::sendGlobalRequest(std::string methodName, BaseLib::PArray &parameters) {
  try {
    std::lock_guard<std::mutex> requestGuard(_requestMutex);
    int32_t packetId;
    {
      std::lock_guard<std::mutex> packetIdGuard(_packetIdMutex);
      packetId = _currentPacketId++;
    }
    BaseLib::PArray array = std::make_shared<BaseLib::Array>();
    array->reserve(4);
    auto scriptInfo = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    scriptInfo->structValue->emplace("scriptId", std::make_shared<BaseLib::Variable>(0));
    scriptInfo->structValue->emplace("user", std::make_shared<BaseLib::Variable>(std::string()));
    array->push_back(scriptInfo);
    array->push_back(std::make_shared<BaseLib::Variable>(packetId));
    array->push_back(std::make_shared<BaseLib::Variable>(true));
    array->push_back(std::make_shared<BaseLib::Variable>(parameters));
    std::vector<char> data;
    _rpcEncoder->encodeRequest(methodName, array, data);

    PScriptEngineResponse response;
    {
      std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
      auto result = _rpcResponses[0].emplace(packetId, std::make_shared<ScriptEngineResponse>());
      if (result.second) response = result.first->second;
    }
    if (!response) {
      _out.printError("Critical: Could not insert response struct into map.");
      return BaseLib::Variable::createError(-32500, "Unknown application error.");
    }

    BaseLib::PVariable result = send(data);
    if (result->errorStruct) {
      std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
      _rpcResponses[0].erase(packetId);
      return result;
    }

    std::unique_lock<std::mutex> waitLock(_waitMutex);
    while (!_requestConditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&] {
      return GD::bl->shuttingDown || response->finished || _stopped;
    }));

    if (!response->finished || response->response->arrayValue->size() != 3 || response->packetId != packetId) {
      if (GD::bl->shuttingDown) {
        _out.printInfo("Info: Aborting RPC request after 10 seconds, because I'm shutting down. Method: " + methodName);
        result = BaseLib::Variable::createError(-32501, "Shutting down.");
      } else {
        _out.printError("Error: No response received to RPC request. Method: " + methodName);
        result = BaseLib::Variable::createError(-1, "No response received.");
      }
    } else result = response->response->arrayValue->at(2);

    {
      std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
      _rpcResponses[0].erase(packetId);
    }

    return result;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void ScriptEngineClient::sendResponse(BaseLib::PVariable &packetId, BaseLib::PVariable &variable) {
  try {
    BaseLib::PVariable array(new BaseLib::Variable(BaseLib::PArray(new BaseLib::Array{packetId, variable})));
    std::vector<char> data;
    _rpcEncoder->encodeResponse(array, data);
    send(data);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::collectGarbage() {
  try {
    _lastGargabeCollection = GD::bl->hf.getTime();

    std::vector<int32_t> threadsToRemove;
    std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
    try {
      for (std::map<int32_t, PThreadInfo>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i) {
        if (!i->second->running) {
          if (i->second->thread.joinable()) i->second->thread.join();
          threadsToRemove.push_back(i->first);
        }
      }
    }
    catch (const std::exception &ex) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    for (std::vector<int32_t>::iterator i = threadsToRemove.begin(); i != threadsToRemove.end(); ++i) {
      try {
        {
          std::lock_guard<std::mutex> responseGuard(_rpcResponsesMutex);
          _rpcResponses.erase(*i);
        }
        {
          std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
          _requestInfo.erase(*i);
        }
        _scriptThreads.erase(*i);
      }
      catch (const std::exception &ex) {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::setThreadNotRunning(int32_t threadId) {
  if (threadId != -1) {
    std::lock_guard<std::mutex> scriptThreadsGuard(_scriptThreadMutex);
    _scriptThreads.at(threadId)->running = false;
  }
}

ScriptEngineClient::ScriptGuard::~ScriptGuard() {
  if (!_client) return;

  {
    std::lock_guard<std::mutex> eventMapGuard(PhpEvents::eventsMapMutex);
    PhpEvents::eventsMap.erase(_scriptId);
  }

  {
    if (_scriptInfo && _scriptInfo->nodeInfo) {
      auto infoIterator = _scriptInfo->nodeInfo->structValue->find("id");
      if (infoIterator != _scriptInfo->nodeInfo->structValue->end()) {

        std::lock_guard<std::mutex> nodeInfoGuard(ScriptEngineClient::_nodeInfoMutex);
        ScriptEngineClient::_nodeInfo.erase(infoIterator->second->stringValue);
      }
    }
  }

  {
    std::lock_guard<std::mutex> resourceGuard(_resourceMutex);
    if (tsrm_get_ls_cache()) {
      //SG(request_info).path_translated, SG(request_info).query_string and SG(request_info).request_uri are cleaned up by PHP

      if (SG(request_info).argv) {
        free(SG(request_info).argv);
        SG(request_info).argv = nullptr;
      }
      SG(request_info).argc = 0;

      zend_homegear_globals *globals = php_homegear_get_globals();
      if (globals && globals->executionStarted) {
        php_request_shutdown(nullptr);

        ts_free_thread();
      }
    }
  }
  if (!GD::bl->shuttingDown) _client->sendScriptFinished(_scriptInfo->exitCode);
  if (_scriptInfo->peerId > 0) GD::out.printInfo("Info: PHP script of peer " + std::to_string(_scriptInfo->peerId) + " exited with code " + std::to_string(_scriptInfo->exitCode) + ".");
  else if (_scriptInfo->getType() != BaseLib::ScriptEngine::ScriptInfo::ScriptType::simpleNode || _scriptInfo->exitCode != 0)
    GD::out.printInfo("Info: Script " + std::to_string(_scriptInfo->id) + " exited with code " + std::to_string(_scriptInfo->exitCode) + ".");
  _client->setThreadNotRunning(_scriptId);
}

void ScriptEngineClient::runScript(int32_t id, PScriptInfo scriptInfo) {
  try {
    BaseLib::Rpc::PServerInfo serverInfo(new BaseLib::Rpc::ServerInfo::Info());
    zend_file_handle zendHandle{};

    ScriptInfo::ScriptType type = scriptInfo->getType();

    {
      std::lock_guard<std::mutex> resourceGuard(_resourceMutex);
      ts_resource(0); //Replaces TSRMLS_FETCH()

      if (!scriptInfo->script.empty()) {
#if PHP_VERSION_ID < 70400
        zendHandle.type = ZEND_HANDLE_MAPPED;
        zendHandle.handle.fp = nullptr;
        zendHandle.handle.stream.handle = nullptr;
        zendHandle.handle.stream.closer = nullptr;
        memset(&zendHandle.handle.stream.mmap, 0, sizeof(zend_mmap));
        zendHandle.handle.stream.mmap.buf = (char *)scriptInfo->script.c_str(); //String is not modified
        zendHandle.handle.stream.mmap.len = scriptInfo->script.size();
        zendHandle.filename = scriptInfo->fullPath.c_str();
        zendHandle.opened_path = nullptr;
        zendHandle.free_filename = 0;
#elif PHP_VERSION_ID < 80200
        auto stream = new hg_stream_handle();
        stream->position = 0;
        stream->buffer = scriptInfo->script;

        zendHandle.type = ZEND_HANDLE_STREAM;
        zendHandle.filename = scriptInfo->fullPath.c_str();
        zendHandle.opened_path = nullptr; //It might make sense to set this.
        zendHandle.handle.stream.handle = stream;
        zendHandle.handle.stream.reader = hg_zend_stream_reader;
        zendHandle.handle.stream.fsizer = hg_zend_stream_fsizer;
        zendHandle.handle.stream.isatty = 0;
        zendHandle.handle.stream.closer = hg_zend_stream_closer;
#else
        auto stream = new hg_stream_handle();
        stream->position = 0;
        stream->buffer = scriptInfo->script;

        zendHandle.type = ZEND_HANDLE_STREAM;
        zendHandle.filename = zend_string_init(scriptInfo->fullPath.c_str(), scriptInfo->fullPath.length(), false);
        zendHandle.opened_path = nullptr; //It might make sense to set this.
        zendHandle.handle.stream.handle = stream;
        zendHandle.handle.stream.reader = hg_zend_stream_reader;
        zendHandle.handle.stream.fsizer = hg_zend_stream_fsizer;
        zendHandle.handle.stream.isatty = 0;
        zendHandle.handle.stream.closer = hg_zend_stream_closer;
#endif
      } else {
        zendHandle.type = ZEND_HANDLE_FILENAME;
#if PHP_VERSION_ID < 80200
        zendHandle.filename = scriptInfo->fullPath.c_str();
        zendHandle.free_filename = 0;
#else
        zendHandle.filename = zend_string_init(scriptInfo->fullPath.c_str(), scriptInfo->fullPath.length(), false);
#endif
        zendHandle.opened_path = nullptr;
      }

      zend_homegear_globals *globals = php_homegear_get_globals();
      if (!globals) return;

      if (type == ScriptInfo::ScriptType::web) {
        globals->webRequest = true;
        globals->commandLine = false;
        globals->cookiesParsed = false;
        globals->http = scriptInfo->http;
        serverInfo = scriptInfo->serverInfo;
      } else {
        globals->commandLine = true;
        globals->cookiesParsed = true;
        globals->peerId = scriptInfo->peerId;
      }

      if (type == ScriptInfo::ScriptType::simpleNode || type == ScriptInfo::ScriptType::statefulNode) {
        globals->nodeId = scriptInfo->nodeInfo->structValue->at("id")->stringValue;
        auto flowIdIterator = scriptInfo->nodeInfo->structValue->at("info")->structValue->find("z");
        if (flowIdIterator != scriptInfo->nodeInfo->structValue->at("info")->structValue->end()) {
          globals->flowId = flowIdIterator->second->stringValue;
        } else globals->flowId = "g";
      }

      ZEND_TSRMLS_CACHE_UPDATE();

      if (type == ScriptInfo::ScriptType::cli || type == ScriptInfo::ScriptType::device || type == ScriptInfo::ScriptType::device2 || type == ScriptInfo::ScriptType::statefulNode) {
        //BaseLib::Base64::encode(BaseLib::HelperFunctions::getRandomBytes(16), globals->token);
        BaseLib::Base64::encode(std::vector<uint8_t>{0, 1, 2, 3, 4, 5}, globals->token);
        std::shared_ptr<PhpEvents> phpEvents = std::make_shared<PhpEvents>(globals->token, globals->outputCallback, globals->rpcCallback);
        phpEvents->setPeerId(static_cast<uint64_t>(scriptInfo->peerId));
        if (type == ScriptInfo::ScriptType::statefulNode) phpEvents->setNodeId(scriptInfo->nodeInfo->structValue->at("id")->stringValue);
        std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
        PhpEvents::eventsMap.emplace(id, phpEvents);
      }

      SG(server_context) = (void *)serverInfo.get(); //Must be defined! Otherwise php_homegear_activate is not called.
      std::string charset = "UTF-8";
      SG(default_charset) = estrndup(charset.data(), charset.size());
      SG(default_mimetype) = nullptr;
      if (type == ScriptInfo::ScriptType::cli || type == ScriptInfo::ScriptType::device || type == ScriptInfo::ScriptType::device2 || type == ScriptInfo::ScriptType::simpleNode || type == ScriptInfo::ScriptType::statefulNode) {
        PG(register_argc_argv) = 1;
        PG(implicit_flush) = 1;
        PG(html_errors) = 0;
        SG(options) |= SAPI_OPTION_NO_CHDIR;
        SG(headers_sent) = 1;
        SG(request_info).no_headers = 1;
        SG(request_info).path_translated = estrndup(scriptInfo->fullPath.c_str(), scriptInfo->fullPath.size());
      } else if (type == ScriptInfo::ScriptType::web) {
        SG(sapi_headers).http_response_code = 200;
        SG(request_info).content_length = globals->http.getHeader().contentLength;
        if (!globals->http.getHeader().contentTypeFull.empty()) SG(request_info).content_type = globals->http.getHeader().contentTypeFull.c_str();
        SG(request_info).request_method = globals->http.getHeader().method.c_str();
        SG(request_info).proto_num = globals->http.getHeader().protocol == BaseLib::Http::Protocol::http10 ? 1000 : 1001;
        std::string redirectUrl = globals->http.getRedirectUrl();
        std::string uri = (redirectUrl.empty() ? globals->http.getHeader().path : redirectUrl) + globals->http.getHeader().pathInfo;
        if (!globals->http.getHeader().args.empty()) uri.append('?' + globals->http.getHeader().args);
        if (!globals->http.getHeader().args.empty()) SG(request_info).query_string = estrndup(&globals->http.getHeader().args.at(0), globals->http.getHeader().args.size());
        if (!uri.empty()) SG(request_info).request_uri = estrndup(uri.data(), uri.size());
        std::string pathTranslated = serverInfo->contentPath.substr(0, serverInfo->contentPath.size() - 1) + scriptInfo->relativePath;
        SG(request_info).path_translated = estrndup(pathTranslated.data(), pathTranslated.size());
      }

      if (php_request_startup() == FAILURE) {
        GD::bl->out.printError("Error calling php_request_startup...");
        SG(server_context) = nullptr;
        return;
      }
      globals->executionStarted = true;

      if (type == ScriptInfo::ScriptType::cli || type == ScriptInfo::ScriptType::device) {
        std::vector<std::string> argv = getArgs(scriptInfo->fullPath, scriptInfo->arguments);
        if (type == ScriptInfo::ScriptType::device) argv[0] = std::to_string(scriptInfo->peerId);
        php_homegear_build_argv(argv);
        SG(request_info).argc = static_cast<int>(argv.size());
        SG(request_info).argv = (char **)malloc((argv.size() + 1) * sizeof(char *));
        for (uint32_t i = 0; i < argv.size(); ++i) {
          SG(request_info).argv[i] = (char *)argv[i].c_str(); //Value is not modified.
        }
        SG(request_info).argv[argv.size()] = nullptr;
      }

      { //Overwrite getenv and putenv
        zend_string *getenvString = zend_string_init("getenv", sizeof("getenv") - 1, false);
        zend_string *hgGetenvString = zend_string_init("hg_getenv", sizeof("hg_getenv") - 1, false);
        zval *getenvFunction = zend_hash_find(EG(function_table), getenvString);
        zval *hgGetenvFunction = zend_hash_find(EG(function_table), hgGetenvString);
        if (getenvFunction && hgGetenvFunction) {
          //Don't call zend_hash_del => double free
          zend_hash_add_new(EG(function_table), getenvString, hgGetenvFunction);
        }
        //No need to call zend_string_free on getenvString and hgGetenvString

        zend_string *putenvString = zend_string_init("putenv", sizeof("putenv") - 1, false);
        zend_string *hgPutenvString = zend_string_init("hg_putenv", sizeof("hg_putenv") - 1, false);
        zval *putenvFunction = zend_hash_find(EG(function_table), putenvString);
        zval *hgPutenvFunction = zend_hash_find(EG(function_table), hgPutenvString);
        if (putenvFunction && hgPutenvFunction) {
          //Don't call zend_hash_del => double free
          zend_hash_add_new(EG(function_table), putenvString, hgPutenvFunction);
        }
        //No need to call zend_string_free on putenvString and hgPutenvString
      }

      if (scriptInfo->peerId > 0) GD::out.printInfo("Info: Starting PHP script of peer " + std::to_string(scriptInfo->peerId) + ".");
    }

    php_execute_script(&zendHandle);
    if (type == ScriptInfo::ScriptType::simpleNode || type == ScriptInfo::ScriptType::statefulNode) {
      runNode(id, scriptInfo);
    } else if (type == ScriptInfo::ScriptType::device2) {
      runDevice(id, scriptInfo);
    }

    scriptInfo->exitCode = EG(exit_status);

    SG(server_context) = nullptr;
    return;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  SG(server_context) = nullptr;
  std::string error("Error executing script. Check Homegear log for more details.");
  sendOutput(error, true);
}

void ScriptEngineClient::runNode(int32_t id, PScriptInfo scriptInfo) {
  zval homegearNodeObject{};
  try {
    std::string nodeId = scriptInfo->nodeInfo->structValue->at("id")->stringValue;
    PNodeInfo nodeInfo;

    if (scriptInfo->getType() == ScriptInfo::ScriptType::statefulNode) {
      std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMutex);
      nodeInfo = _nodeInfo.at(nodeId);
    }

    zend_class_entry *homegearNodeClassEntry = nullptr;
    bool result = php_init_node(scriptInfo, homegearNodeClassEntry, &homegearNodeObject);

    if (result) {
      if (scriptInfo->getType() == ScriptInfo::ScriptType::statefulNode) {
        bool stop = false;
        while (!GD::bl->shuttingDown && !stop) {
          std::unique_lock<std::mutex> waitLock(nodeInfo->waitMutex);
          while (!nodeInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
            return nodeInfo->ready || GD::bl->shuttingDown;
          }));
          if (!nodeInfo->ready || GD::bl->shuttingDown) continue;
          nodeInfo->ready = false;

          nodeInfo->response = php_node_object_invoke_local(scriptInfo, &homegearNodeObject, nodeInfo->methodName, nodeInfo->parameters);
          if (nodeInfo->methodName == "init") {
            _nodesStopped = false;
            if (!nodeInfo->response->booleanValue) stop = true;
          } else if (nodeInfo->methodName == "start" && !nodeInfo->response->booleanValue) stop = true;
          else if (nodeInfo->methodName == "stop") {
            //Cause `pollEvent()` to return
            std::lock_guard<std::mutex> eventMapGuard(PhpEvents::eventsMapMutex);
            auto event = PhpEvents::eventsMap.find(id);
            if (event != PhpEvents::eventsMap.end()) event->second->stop();
          } else if (nodeInfo->methodName == "waitForStop") {
            _nodesStopped = true;
            stop = true;
          }

          nodeInfo->parameters.reset();
          waitLock.unlock();
          nodeInfo->conditionVariable.notify_all();
        }
      } else {
        zval returnValue{};
        zval function{};
        zval parameters[3];

        ZVAL_STRINGL(&function, "input", sizeof("input") - 1);
        PhpVariableConverter::getPHPVariable(scriptInfo->nodeInfo, &parameters[0]);
        ZVAL_LONG(&parameters[1], scriptInfo->inputPort);
        PhpVariableConverter::getPHPVariable(scriptInfo->message, &parameters[2]);

        int callResult = 0;

        zend_try
            {
              callResult = call_user_function(&(Z_OBJ(homegearNodeObject)->ce->function_table), &homegearNodeObject, &function, &returnValue, 3, parameters);
            }
        zend_end_try();

        if (callResult != 0) _out.printError("Error calling function \"input\" in file: " + scriptInfo->fullPath);
        zval_ptr_dtor(&function);
        zval_ptr_dtor(&parameters[0]);
        zval_ptr_dtor(&parameters[1]);
        zval_ptr_dtor(&parameters[2]);
        zval_ptr_dtor(&returnValue); //Not really necessary as returnValue is of primitive type
      }
      std::string methodName = "__destruct";
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      php_node_object_invoke_local(scriptInfo, &homegearNodeObject, methodName, parameters);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::runDevice(int32_t id, PScriptInfo scriptInfo) {
  zval homegearDeviceObject{};
  try {
    uint64_t peerId = scriptInfo->peerId;
    PDeviceInfo deviceInfo;

    {
      std::lock_guard<std::mutex> deviceInfoGuard(_deviceInfoMutex);
      deviceInfo = _deviceInfo.at(peerId);
    }

    zend_class_entry *homegearDeviceClassEntry = nullptr;
    bool result = php_init_device(scriptInfo, homegearDeviceClassEntry, &homegearDeviceObject);

    if (result) {
      bool stop = false;
      while (!GD::bl->shuttingDown && !stop) {
        std::unique_lock<std::mutex> waitLock(deviceInfo->waitMutex);
        while (!deviceInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
          return deviceInfo->ready || GD::bl->shuttingDown;
        }));
        if (!deviceInfo->ready || GD::bl->shuttingDown) continue;
        deviceInfo->ready = false;

        deviceInfo->response = php_device_object_invoke_local(scriptInfo, &homegearDeviceObject, deviceInfo->methodName, deviceInfo->parameters);
        if (deviceInfo->methodName == "init") {
          if (!deviceInfo->response->booleanValue) stop = true;
        } else if (deviceInfo->methodName == "start" && !deviceInfo->response->booleanValue) stop = true;
        else if (deviceInfo->methodName == "stop") {
          //Cause `pollEvent()` to return
          std::lock_guard<std::mutex> eventMapGuard(PhpEvents::eventsMapMutex);
          auto event = PhpEvents::eventsMap.find(id);
          if (event != PhpEvents::eventsMap.end()) event->second->stop();
        } else if (deviceInfo->methodName == "waitForStop") {
          stop = true;
        }

        deviceInfo->parameters.reset();
        waitLock.unlock();
        deviceInfo->conditionVariable.notify_all();
      }
      std::string methodName = "__destruct";
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      php_device_object_invoke_local(scriptInfo, &homegearDeviceObject, methodName, parameters);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::scriptThread(int32_t id, PScriptInfo scriptInfo, bool sendOutput) {
  try {
    zend_homegear_globals *globals = php_homegear_get_globals();
    if (!globals) exit(1);
    globals->id = id;
    globals->scriptInfo = scriptInfo;
    if (sendOutput) {
      globals->outputCallback = std::bind(&ScriptEngineClient::sendOutput, this, std::placeholders::_1, std::placeholders::_2);
      globals->sendHeadersCallback = std::bind(&ScriptEngineClient::sendHeaders, this, std::placeholders::_1);
    }
    globals->rpcCallback = std::bind(&ScriptEngineClient::callMethod, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    {
      std::lock_guard<std::mutex> requestInfoGuard(_requestInfoMutex);
      _requestInfo.emplace(std::piecewise_construct, std::make_tuple(id), std::make_tuple(std::make_shared<RequestInfo>()));
    }
    ScriptGuard scriptGuard(this, globals, id, scriptInfo);

    if (scriptInfo->script.empty() && scriptInfo->fullPath.size() > 3 && (scriptInfo->fullPath.compare(scriptInfo->fullPath.size() - 4, 4, ".hgs") == 0 || scriptInfo->fullPath.compare(scriptInfo->fullPath.size() - 4, 4, ".hgn") == 0)) {
      std::lock_guard<std::mutex> scriptCacheGuard(_scriptCacheMutex);
      std::map<std::string, std::shared_ptr<CacheInfo>>::iterator scriptIterator = _scriptCache.find(scriptInfo->fullPath);
      if (scriptIterator != _scriptCache.end() && scriptIterator->second->lastModified == BaseLib::Io::getFileLastModifiedTime(scriptInfo->fullPath)) scriptInfo->script = scriptIterator->second->script;
      else {
        std::vector<char> data = BaseLib::Io::getBinaryFileContent(scriptInfo->fullPath);
        int32_t pos = -1;
        for (uint32_t i = 0; i < 11 && i < data.size(); i++) {
          if (data[i] == ' ') {
            pos = (int32_t)i;
            break;
          }
        }
        if (pos == -1) {
          GD::bl->out.printError("Error: License module id is missing in encrypted script file \"" + scriptInfo->fullPath + "\"");
          return;
        }
        std::string moduleIdString(&data.at(0), static_cast<unsigned long>(pos));
        int32_t moduleId = BaseLib::Math::getNumber(moduleIdString);
        std::vector<char> input(&data.at(static_cast<unsigned long>(pos + 1)), &data.at(data.size() - 1) + 1);
        if (input.empty()) return;
        std::map<int32_t, std::unique_ptr<BaseLib::Licensing::Licensing>>::iterator i = GD::licensingModules.find(moduleId);
        if (i == GD::licensingModules.end() || !i->second) {
          GD::out.printError("Error: Could not decrypt script file. Licensing module with id 0x" + BaseLib::HelperFunctions::getHexString(moduleId) + " not found");
          return;
        }
        std::shared_ptr<CacheInfo> cacheInfo = std::make_shared<ScriptEngine::CacheInfo>();
        i->second->decryptScript(input, cacheInfo->script);
        cacheInfo->lastModified = BaseLib::Io::getFileLastModifiedTime(scriptInfo->fullPath);
        if (!cacheInfo->script.empty()) {
          _scriptCache[scriptInfo->fullPath] = cacheInfo;
          scriptInfo->script = cacheInfo->script;
        }
      }
    }

    runScript(id, scriptInfo);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void ScriptEngineClient::checkSessionIdThread(std::string sessionId, std::string *result) {
  *result = "";
  std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> serverInfo(new BaseLib::Rpc::ServerInfo::Info());

  {
    std::lock_guard<std::mutex> resourceGuard(_resourceMutex);
    ts_resource(0); //Replaces TSRMLS_FETCH()
    zend_homegear_globals *globals = php_homegear_get_globals();
    if (!globals) return;
    try {
      ZEND_TSRMLS_CACHE_UPDATE();
      SG(server_context) = (void *)serverInfo.get(); //Must be defined! Otherwise POST data is not processed.
      SG(sapi_headers).http_response_code = 200;
      SG(default_mimetype) = nullptr;
      SG(default_charset) = nullptr;
      SG(request_info).content_length = 0;
      SG(request_info).request_method = "GET";
      SG(request_info).proto_num = 1001;
      globals->http.getHeader().cookie = "PHPSESSID=" + sessionId;

      if (php_request_startup() == FAILURE) {
        GD::bl->out.printError("Error calling php_request_startup...");
        ts_free_thread();
        return;
      }
    }
    catch (const std::exception &ex) {
      GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      ts_free_thread();
      return;
    }
    catch (...) {
      GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      ts_free_thread();
      return;
    }
  }

  try {
    zval returnValue{};
    zval function{};

    ZVAL_STRINGL(&function, "session_start", sizeof("session_start") - 1);

    zend_try
        {
          call_user_function(EG(function_table), nullptr, &function, &returnValue, 0, nullptr);
        }
    zend_end_try();

    zval_ptr_dtor(&function);
    zval_ptr_dtor(&returnValue); //Not really necessary as returnValue is of primitive type

    zval *reference = zend_hash_str_find(&EG(symbol_table), "_SESSION", sizeof("_SESSION") - 1);
    if (reference != nullptr) {
      if (Z_ISREF_P(reference) && Z_TYPE_P(Z_REFVAL_P(reference)) == IS_ARRAY) {
        zval *token = zend_hash_str_find(Z_ARRVAL_P(Z_REFVAL_P(reference)), "authorized", sizeof("authorized") - 1);
        if (token != nullptr) {
          if (Z_TYPE_P(token) == IS_TRUE) {
            zval *token2 = zend_hash_str_find(Z_ARRVAL_P(Z_REFVAL_P(reference)), "user", sizeof("user") - 1);
            if (token2 != nullptr && Z_STRLEN_P(token2) > 0) {
              *result = std::string(Z_STRVAL_P(token2), Z_STRLEN_P(token2));
              BaseLib::HelperFunctions::toLower(*result);
            }
          }
        }
      }
    }
  }
  catch (const std::exception &ex) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  php_request_shutdown(nullptr);
  ts_free_thread();
}

void ScriptEngineClient::watchdog() {
  while (!_stopped) {
    try {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));

      if (_processingThreadCount1 == _threadCount) {
        int64_t time = _processingThreadCountMaxReached1;
        if (time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.scriptEngineWatchdogTimeout()) {
          std::cout << "Watchdog 1 timed out. Killing process." << std::endl;
          std::cerr << "Watchdog 1 timed out. Killing process." << std::endl;
          kill(getpid(), 9);
        }
      }

      if (_processingThreadCount2 == _threadCount) {
        int64_t time = _processingThreadCountMaxReached2;
        if (time != 0 && BaseLib::HelperFunctions::getTime() - time > GD::bl->settings.scriptEngineWatchdogTimeout()) {
          std::cout << "Watchdog 2 timed out. Killing process." << std::endl;
          std::cerr << "Watchdog 2 timed out. Killing process." << std::endl;
          kill(getpid(), 9);
        }
      }
    }
    catch (const std::exception &ex) {
      _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...) {
      _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
  }
}

// {{{ RPC methods
BaseLib::PVariable ScriptEngineClient::reload(BaseLib::PArray &parameters) {
  try {
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.log").c_str(), "a", stdout)) {
      GD::out.printError("Error: Could not redirect output to new log file.");
    }
    if (!std::freopen((GD::bl->settings.logfilePath() + "homegear-scriptengine.err").c_str(), "a", stderr)) {
      GD::out.printError("Error: Could not redirect errors to new log file.");
    }

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::shutdown(BaseLib::PArray &parameters) {
  try {
    std::lock_guard<std::mutex> maintenanceThreadGuard(_maintenanceThreadMutex);
    if (_maintenanceThread.joinable()) _maintenanceThread.join();
    _maintenanceThread = std::thread(&ScriptEngineClient::dispose, this, true);

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::lifetick(BaseLib::PArray &parameters) {
  try {
    for (int32_t i = 0; i < _queueCount; i++) {
      if (queueSize(i) > 1000) {
        _out.printError("Error in lifetick: More than 1000 items are queued in queue number " + std::to_string(i));
        return std::make_shared<BaseLib::Variable>(false);
      }
    }

    return std::make_shared<BaseLib::Variable>(true);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::getLoad(BaseLib::PArray &parameters) {
  try {
    auto loadStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    for (int32_t i = 0; i < _queueCount; i++) {
      auto queueStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

      queueStruct->structValue->emplace("Max load", std::make_shared<BaseLib::Variable>(maxThreadLoad(i)));
      queueStruct->structValue->emplace("Max load 1m", std::make_shared<BaseLib::Variable>(maxThreadLoad1m(i)));
      queueStruct->structValue->emplace("Max load 10m", std::make_shared<BaseLib::Variable>(maxThreadLoad10m(i)));
      queueStruct->structValue->emplace("Max load 1h", std::make_shared<BaseLib::Variable>(maxThreadLoad1h(i)));
      queueStruct->structValue->emplace("Max latency", std::make_shared<BaseLib::Variable>(maxWait(i)));
      queueStruct->structValue->emplace("Max latency 1m", std::make_shared<BaseLib::Variable>(maxWait1m(i)));
      queueStruct->structValue->emplace("Max latency 10m", std::make_shared<BaseLib::Variable>(maxWait10m(i)));
      queueStruct->structValue->emplace("Max latency 1h", std::make_shared<BaseLib::Variable>(maxWait1h(i)));

      loadStruct->structValue->emplace("Queue " + std::to_string(i + 1), queueStruct);
    }

    return loadStruct;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::stopDevices(BaseLib::PArray &parameters) {
  try {
    _out.printMessage("Calling stop() on devices...");

    std::unordered_map<uint64_t, PDeviceInfo> deviceInfo;

    {
      std::lock_guard<std::mutex> deviceInfoGuard(_deviceInfoMutex);
      deviceInfo = _deviceInfo;
    }

    for (auto &device : deviceInfo) {
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(3);
      parameters->push_back(std::make_shared<BaseLib::Variable>(device.first));
      parameters->push_back(std::make_shared<BaseLib::Variable>("stop"));
      BaseLib::PVariable innerParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      parameters->push_back(innerParameters);

      BaseLib::PVariable result = executeDeviceMethod(parameters);
      if (result->errorStruct) _out.printError("Error calling stop on peer " + std::to_string(device.first) + ": " + result->structValue->at("faultString")->stringValue);
    }

    _out.printMessage("Call to stop() on devices completed.");

    _out.printMessage("Calling waitForStop() on devices...");

    for (auto &device : deviceInfo) {
      BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
      parameters->reserve(3);
      parameters->push_back(std::make_shared<BaseLib::Variable>(device.first));
      parameters->push_back(std::make_shared<BaseLib::Variable>("waitForStop"));
      BaseLib::PVariable innerParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      parameters->push_back(innerParameters);

      BaseLib::PVariable result = executeDeviceMethod(parameters);
      if (result->errorStruct) _out.printError("Error calling waitForStop on peer " + std::to_string(device.first) + ": " + result->structValue->at("faultString")->stringValue);
    }

    _out.printMessage("Call to waitForStop() on devices completed.");
    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::executeScript(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() < 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
    PScriptInfo scriptInfo;
    bool sendOutput = false;
    auto type = (ScriptInfo::ScriptType)parameters->at(1)->integerValue;
    if (type == ScriptInfo::ScriptType::cli) {
      if (parameters->at(2)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Path is empty.");
      scriptInfo.reset(new ScriptInfo(type, parameters->at(2)->stringValue, parameters->at(3)->stringValue, parameters->at(4)->stringValue, parameters->at(5)->stringValue));

      if (scriptInfo->script.empty() && !GD::bl->io.fileExists(scriptInfo->fullPath)) {
        _out.printError("Error: PHP script \"" + parameters->at(2)->stringValue + "\" does not exist.");
        return BaseLib::Variable::createError(-1, "Script file does not exist: " + scriptInfo->fullPath);
      }
      sendOutput = parameters->at(6)->booleanValue;
    } else if (type == ScriptInfo::ScriptType::web) {
      if (parameters->at(2)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Path is empty.");
      scriptInfo.reset(new ScriptInfo(GD::bl.get(), type, parameters->at(2)->stringValue, parameters->at(3)->stringValue, parameters->at(4)->stringValue, parameters->at(5), parameters->at(6), parameters->at(7)));

      if (scriptInfo->script.empty() && !GD::bl->io.fileExists(scriptInfo->fullPath)) {
        _out.printError("Error: PHP script \"" + parameters->at(2)->stringValue + "\" does not exist.");
        return BaseLib::Variable::createError(-1, "Script file does not exist: " + scriptInfo->fullPath);
      }
      sendOutput = true;
    } else if (type == ScriptInfo::ScriptType::device || type == ScriptInfo::ScriptType::device2) {
      scriptInfo.reset(new ScriptInfo(type, parameters->at(2)->stringValue, parameters->at(3)->stringValue, parameters->at(4)->stringValue, parameters->at(5)->stringValue, parameters->at(6)->integerValue64));
      if (type == ScriptInfo::ScriptType::device2) {
        std::lock_guard<std::mutex> deviceInfoGuard(_deviceInfoMutex);
        _deviceInfo.erase(scriptInfo->peerId);
        _deviceInfo.emplace(scriptInfo->peerId, std::make_shared<DeviceInfo>());
      }
    } else if (type == ScriptInfo::ScriptType::simpleNode) {
      scriptInfo.reset(new ScriptInfo(type, parameters->at(2), parameters->at(3)->stringValue, parameters->at(4)->stringValue, static_cast<uint32_t>(parameters->at(5)->integerValue), parameters->at(6)));
      if (!GD::bl->io.fileExists(scriptInfo->fullPath)) {
        _out.printError("Error: PHP node script \"" + scriptInfo->fullPath + "\" does not exist.");
        return BaseLib::Variable::createError(-1, "Script file does not exist: " + scriptInfo->fullPath);
      }
    } else if (type == ScriptInfo::ScriptType::statefulNode) {
      scriptInfo.reset(new ScriptInfo(type, parameters->at(2), parameters->at(3)->stringValue, parameters->at(4)->stringValue, static_cast<uint32_t>(parameters->at(5)->integerValue)));
      if (!GD::bl->io.fileExists(scriptInfo->fullPath)) {
        _out.printError("Error: PHP node script \"" + scriptInfo->fullPath + "\" does not exist.");
        return BaseLib::Variable::createError(-1, "Script file does not exist: " + scriptInfo->fullPath);
      }

      std::string nodeId = scriptInfo->nodeInfo->structValue->at("id")->stringValue;
      std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMutex);
      _nodeInfo.erase(nodeId);
      _nodeInfo.emplace(nodeId, std::make_shared<NodeInfo>());
    }

    scriptInfo->id = parameters->at(0)->integerValue;

    {
      std::lock_guard<std::mutex> scriptGuard(_scriptThreadMutex);
      if (_scriptThreads.find(scriptInfo->id) == _scriptThreads.end()) {
        PThreadInfo threadInfo = std::make_shared<ThreadInfo>();
        threadInfo->filename = scriptInfo->fullPath;
        threadInfo->peerId = static_cast<uint64_t>(scriptInfo->peerId);
        if (scriptInfo->nodeInfo) {
          auto idIterator = scriptInfo->nodeInfo->structValue->find("id");
          if (idIterator != scriptInfo->nodeInfo->structValue->end()) threadInfo->nodeId = idIterator->second->stringValue;
        }
        threadInfo->thread = std::thread(&ScriptEngineClient::scriptThread, this, scriptInfo->id, scriptInfo, sendOutput);
        _scriptThreads.emplace(scriptInfo->id, threadInfo);

        if (type == ScriptInfo::ScriptType::device2) {
          BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
          parameters->reserve(3);
          parameters->push_back(std::make_shared<BaseLib::Variable>(scriptInfo->peerId));
          parameters->push_back(std::make_shared<BaseLib::Variable>("init"));
          BaseLib::PVariable innerParameters = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
          innerParameters->arrayValue->push_back(std::make_shared<BaseLib::Variable>(scriptInfo->peerId));
          parameters->push_back(innerParameters);

          BaseLib::PVariable result = executeDeviceMethod(parameters);
          if (result->errorStruct) GD::out.printError("Error calling init on peer " + std::to_string(scriptInfo->peerId) + ": " + result->structValue->at("faultString")->stringValue);

          if (!result->booleanValue) {
            GD::out.printError("Error calling init on peer " + std::to_string(scriptInfo->peerId) + ".");
          } else {
            parameters->at(1)->stringValue = "start";
            result = executeDeviceMethod(parameters);
            if (result->errorStruct) GD::out.printError("Error calling start on peer " + std::to_string(scriptInfo->peerId) + ": " + result->structValue->at("faultString")->stringValue);

            if (!result->booleanValue) {
              GD::out.printError("Error calling start on peer " + std::to_string(scriptInfo->peerId) + ".");
            }
          }
        }
      } else _out.printError("Error: Tried to execute script with ID of already running script.");
    }
    collectGarbage();

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::devTest(BaseLib::PArray &parameters) {
  try {
    /*std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
    for(std::map<int32_t, PThreadInfo>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i)
    {
        if(i->second->running && (i->first % 5 == 0))
        {
            pthread_kill((pthread_t)i->second->thread.native_handle(), SIGUSR1);
        }
    }*/

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::scriptCount(BaseLib::PArray &parameters) {
  try {
    collectGarbage();

    return std::make_shared<BaseLib::Variable>((int32_t)_scriptThreads.size());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::getRunningScripts(BaseLib::PArray &parameters) {
  try {
    BaseLib::PVariable scripts = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    std::lock_guard<std::mutex> threadGuard(_scriptThreadMutex);
    for (std::map<int32_t, PThreadInfo>::iterator i = _scriptThreads.begin(); i != _scriptThreads.end(); ++i) {
      if (i->second->running) {
        BaseLib::Array data{std::make_shared<BaseLib::Variable>(i->second->peerId), std::make_shared<BaseLib::Variable>(i->second->nodeId), std::make_shared<BaseLib::Variable>(i->first), std::make_shared<BaseLib::Variable>(i->second->filename)};
        scripts->arrayValue->push_back(std::make_shared<BaseLib::Variable>(std::make_shared<BaseLib::Array>(std::move(data))));
      }
    }
    return scripts;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::checkSessionId(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");
    if (parameters->at(0)->type != BaseLib::VariableType::tString) return BaseLib::Variable::createError(-1, "Parameter 1 is not of type string.");
    if (parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-1, "Session ID is empty.");

    std::string result = "";
    std::lock_guard<std::mutex> maintenanceThreadGuard(_maintenanceThreadMutex);
    if (_maintenanceThread.joinable()) _maintenanceThread.join();
    _maintenanceThread = std::thread(&ScriptEngineClient::checkSessionIdThread, this, parameters->at(0)->stringValue, &result);
    if (_maintenanceThread.joinable()) _maintenanceThread.join();

    return std::make_shared<BaseLib::Variable>(result);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::executePhpNodeMethod(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::string nodeId = parameters->at(0)->stringValue;
    PNodeInfo nodeInfo;

    {
      std::lock_guard<std::mutex> nodeInfoGuard(_nodeInfoMutex);
      auto nodeIterator = _nodeInfo.find(nodeId);
      if (nodeIterator == _nodeInfo.end() || !nodeIterator->second) return BaseLib::Variable::createError(-1, "Node is unknown in script engine client.");
      nodeInfo = nodeIterator->second;
    }

    std::lock_guard<std::mutex> requestLock(nodeInfo->requestMutex);

    {
      std::lock_guard<std::mutex> nodeInfoGuard(nodeInfo->waitMutex);
      nodeInfo->methodName = parameters->at(1)->stringValue;
      nodeInfo->parameters = parameters->at(2)->arrayValue;
      nodeInfo->ready = true;
      nodeInfo->response.reset();
    }
    nodeInfo->conditionVariable.notify_all();

    //Wait for response
    {
      std::unique_lock<std::mutex> waitLock(nodeInfo->waitMutex);
      int32_t i = 0;
      while (!nodeInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
        return nodeInfo->response || GD::bl->shuttingDown;
      })) {
        i++;
        if (i == 60) {
          _out.printError("Error: Node with ID " + nodeId + " is not responding...");
          break;
        }
      }
    }

    auto response = nodeInfo->response;

    if (parameters->at(1)->stringValue == "waitForStop") {
      std::lock_guard<std::mutex> deviceInfoGuard(_deviceInfoMutex);
      _nodeInfo.erase(nodeId);
    }

    return response ? response : std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::executeDeviceMethod(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    uint64_t peerId = parameters->at(0)->integerValue64;
    PDeviceInfo deviceInfo;

    {
      std::lock_guard<std::mutex> deviceInfoGuard(_deviceInfoMutex);
      auto deviceIterator = _deviceInfo.find(peerId);
      if (deviceIterator == _deviceInfo.end() || !deviceIterator->second) return BaseLib::Variable::createError(-1, "Unknown peer.");
      deviceInfo = deviceIterator->second;
    }

    std::lock_guard<std::mutex> requestLock(deviceInfo->requestMutex);

    {
      std::lock_guard<std::mutex> deviceInfoGuard(deviceInfo->waitMutex);
      deviceInfo->methodName = parameters->at(1)->stringValue;
      deviceInfo->parameters = parameters->at(2)->arrayValue;
      deviceInfo->ready = true;
      deviceInfo->response.reset();
    }
    deviceInfo->conditionVariable.notify_all();

    //Wait for response
    {
      std::unique_lock<std::mutex> waitLock(deviceInfo->waitMutex);
      int32_t i = 0;
      while (!deviceInfo->conditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
        return deviceInfo->response || GD::bl->shuttingDown;
      })) {
        i++;
        if (i == 10) {
          _out.printError("Error: Peer with ID " + std::to_string(peerId) + " is not responding...");
          break;
        }
      }
    }

    auto response = deviceInfo->response;
    if (parameters->at(1)->stringValue == "waitForStop") {
      std::lock_guard<std::mutex> deviceInfoGuard(_deviceInfoMutex);
      _deviceInfo.erase(peerId);
    }

    return response ? response : std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastEvent(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 5) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i) {
      int32_t channel = parameters->at(2)->integerValue;
      std::string variableName;
      if (i->second && (parameters->at(1)->integerValue64 == 0 || i->second->peerSubscribed(parameters->at(1)->integerValue64, -1, variableName))) {
        for (uint32_t j = 0; j < parameters->at(3)->arrayValue->size(); j++) {
          variableName = parameters->at(3)->arrayValue->at(j)->stringValue;
          if (!i->second->peerSubscribed(static_cast<uint64_t>(parameters->at(1)->integerValue64), channel, variableName)) continue;
          auto eventData = std::make_shared<PhpEvents::EventData>();
          eventData->source = parameters->at(0)->stringValue;
          eventData->type = PhpEvents::EventDataType::event;
          eventData->id = static_cast<uint64_t>(parameters->at(1)->integerValue64);
          eventData->channel = channel;
          eventData->variable = variableName;
          eventData->value = parameters->at(4)->arrayValue->at(j);
          if (!i->second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
        }
      }
    }

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastServiceMessage(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (auto &event : PhpEvents::eventsMap) {
      if (!event.second) continue;
      auto eventData = std::make_shared<PhpEvents::EventData>();
      eventData->type = PhpEvents::EventDataType::serviceMessage;
      eventData->value = parameters->at(0);
      if (!event.second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastNewDevices(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i) {
      if (i->second) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::newDevices;
        eventData->value = parameters->at(0);
        if (!i->second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastDeleteDevices(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i) {
      if (i->second) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::deleteDevices;
        eventData->value = parameters->at(0);
        if (!i->second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return BaseLib::PVariable(new BaseLib::Variable());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastUpdateDevice(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (std::map<int32_t, std::shared_ptr<PhpEvents>>::iterator i = PhpEvents::eventsMap.begin(); i != PhpEvents::eventsMap.end(); ++i) {
      std::string variableName;
      if (i->second && i->second->peerSubscribed(static_cast<uint64_t>(parameters->at(0)->integerValue64), -1, variableName)) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::updateDevice;
        eventData->id = static_cast<uint64_t>(parameters->at(0)->integerValue64);
        eventData->channel = parameters->at(1)->integerValue;
        eventData->hint = parameters->at(2)->integerValue;
        if (!i->second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastVariableProfileStateChanged(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 2) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (auto &i : PhpEvents::eventsMap) {
      if (i.second) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::variableProfileStateChanged;
        eventData->id = static_cast<uint64_t>(parameters->at(0)->integerValue64);
        eventData->value = parameters->at(1);
        if (!i.second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastUiNotificationCreated(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (auto &i : PhpEvents::eventsMap) {
      if (i.second) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::uiNotificationCreated;
        eventData->id = static_cast<uint64_t>(parameters->at(0)->integerValue64);
        if (!i.second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastUiNotificationRemoved(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 1) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (auto &i : PhpEvents::eventsMap) {
      if (i.second) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::uiNotificationRemoved;
        eventData->id = static_cast<uint64_t>(parameters->at(0)->integerValue64);
        if (!i.second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable ScriptEngineClient::broadcastUiNotificationAction(BaseLib::PArray &parameters) {
  try {
    if (parameters->size() != 3) return BaseLib::Variable::createError(-1, "Wrong parameter count.");

    std::lock_guard<std::mutex> eventsGuard(PhpEvents::eventsMapMutex);
    for (auto &i : PhpEvents::eventsMap) {
      if (i.second) {
        std::shared_ptr<PhpEvents::EventData> eventData(new PhpEvents::EventData());
        eventData->type = PhpEvents::EventDataType::uiNotificationAction;
        eventData->id = static_cast<uint64_t>(parameters->at(0)->integerValue64);
        eventData->source = parameters->at(1)->stringValue;
        eventData->value = parameters->at(2);
        if (!i.second->enqueue(eventData)) printQueueFullError(_out, "Error: Could not queue event because event buffer is full. Dropping it.");
      }
    }

    return std::make_shared<BaseLib::Variable>();
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}

}

#endif
