/* Copyright 2013-2020 Homegear GmbH
 *
 * libhomegear-base is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * libhomegear-base is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libhomegear-base.  If not, see
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

#include "Nodered.h"
#include "../../GD/GD.h"
#include <homegear-base/Managers/ProcessManager.h>

namespace Homegear {

namespace NodeBlue {

Nodered::Nodered() {
  _out.init(GD::bl.get());
  _out.setPrefix("Node-RED: ");

  _nodeBlueClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
  _nodeBlueClientInfo->flowsServer = true;
  _nodeBlueClientInfo->initInterfaceId = "nodeBlue";
  _nodeBlueClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  std::vector<uint64_t> groups{4};
  _nodeBlueClientInfo->acls->fromGroups(groups);
  _nodeBlueClientInfo->user = "SYSTEM (4)";
}

void Nodered::sigchildHandler(pid_t pid, int exitCode, int signal, bool coreDumped) {
  try {
    if (pid == _pid) {
      close(_stdIn);
      close(_stdOut);
      close(_stdErr);
      _stdIn = -1;
      _stdOut = -1;
      _stdErr = -1;
      _pid = -1;
      if (!_processStartUpComplete) _startUpError = true;
      _out.printInfo("Info: Node-RED process " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + " (Core dumped: " + std::to_string(coreDumped) + +", signal: " + std::to_string(signal) + ").");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

bool Nodered::isStarted() {
  return _processStartUpComplete;
}

void Nodered::start() {
  try {
    _out.printInfo("Starting Node-RED...");
    _callbackHandlerId = BaseLib::ProcessManager::registerCallbackHandler(std::function<void(pid_t pid, int exitCode, int signal, bool coreDumped)>(std::bind(&Nodered::sigchildHandler,
                                                                                                                                                              this,
                                                                                                                                                              std::placeholders::_1,
                                                                                                                                                              std::placeholders::_2,
                                                                                                                                                              std::placeholders::_3,
                                                                                                                                                              std::placeholders::_4)));

    startProgram();

    std::unique_lock<std::mutex> waitLock(_processStartUpMutex);
    while (!_processStartUpConditionVariable.wait_for(waitLock, std::chrono::milliseconds(10000), [&] {
      return _stopThread || _startUpError || _processStartUpComplete;
    })) {
      _out.printInfo("Waiting for Node-RED to get ready.");
    }

    if (_startUpError || _pid == -1) {
      _out.printError("Error: Node-RED could not be started. ");
    } else if (_processStartUpComplete) {
      _out.printInfo("Node-RED started and is ready (PID: " + std::to_string(_pid) + ").");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::stop() {
  try {
    if (!_processStartUpComplete && _pid == -1) return;
    _out.printInfo("Stopping Node-RED...");
    _processStartUpComplete = false;
    _stopThread = true;
    if (_pid != -1) {
      kill(_pid, 15);
      for (int32_t i = 0; i < 600; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (_pid == -1) break;
      }
    }
    if (_pid != -1) {
      _out.printError("Error: Process " + std::to_string(_pid) + " did not finish within 60 seconds. Killing it.");
      kill(_pid, 9);
      close(_stdIn);
      close(_stdOut);
      close(_stdErr);
      _stdIn = -1;
      _stdOut = -1;
      _stdErr = -1;
    }
    if (_execThread.joinable()) _execThread.join();
    if (_errorThread.joinable()) _errorThread.join();
    BaseLib::ProcessManager::unregisterCallbackHandler(_callbackHandlerId);
    _callbackHandlerId = -1;
    _out.printInfo("Node-RED stopped.");
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::startProgram() {
  try {
    if (_execThread.joinable()) _execThread.join();
    if (_errorThread.joinable()) _errorThread.join();
    _stopThread = false;
    _execThread = std::thread(&Nodered::execThread, this);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::execThread() {
  try {
    do {
      if (_stdErr != -1) {
        close(_stdErr);
        _stdErr = -1;
      }
      if (_errorThread.joinable()) _errorThread.join();

      _processStartUpComplete = false;
      _startUpError = false;

      auto redJsPath = GD::bl->settings.nodeRedJsPath();
      if (redJsPath.empty()) redJsPath = GD::bl->settings.dataPath() + "node-blue/node-red/node_modules/node-red/red.js";

      std::vector<std::string> arguments{"-n", redJsPath, "-s", GD::bl->settings.nodeBlueDataPath() + "node-red/settings.js", "--title", "node-pink"};
      int stdIn = -1;
      int stdOut = -1;
      int stdErr = -1;
      _pid = BaseLib::ProcessManager::systemp(GD::executablePath + "/" + GD::executableFile, arguments, GD::bl->fileDescriptorManager.getMax(), stdIn, stdOut, stdErr);
      _stdIn = stdIn;
      _stdOut = stdOut;
      _stdErr = stdErr;

      _errorThread = std::thread(&Nodered::errorThread, this);

      std::array<uint8_t, 4096> buffer{};
      ssize_t bytesRead = 0;
      while (_stdOut != -1) {
        bytesRead = read(_stdOut, buffer.data(), buffer.size());
        if (bytesRead > 0) {
          auto output = std::string(buffer.begin(), buffer.begin() + bytesRead);
          _out.printInfo(BaseLib::HelperFunctions::trim(output));
        }
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (!_stopThread && !GD::bl->shuttingDown);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::errorThread() {
  try {
    std::array<uint8_t, 4096> buffer{};
    ssize_t bytesRead = 0;
    while (_stdErr != -1) {
      bytesRead = read(_stdErr, buffer.data(), buffer.size());
      if (bytesRead > 0) {
        auto output = std::string(buffer.begin(), buffer.begin() + bytesRead);
        _out.printError(BaseLib::HelperFunctions::trim(output));
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable Nodered::invoke(const std::string &method, const BaseLib::PArray &parameters) {
  try {
    if (_pid == -1) return BaseLib::Variable::createError(-1, "No process found.");
    return GD::ipcServer->callProcessRpcMethod(_pid, _nodeBlueClientInfo, method, parameters);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

void Nodered::nodeInput(const std::string &nodeId, const BaseLib::PVariable &nodeInfo, uint32_t inputIndex, const BaseLib::PVariable &message, bool synchronous) {
  try {
    if (!_processStartUpComplete) return;
    auto parameters = std::make_shared<BaseLib::Array>();
    parameters->reserve(5);
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(nodeId));
    parameters->emplace_back(nodeInfo);
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(inputIndex));
    parameters->emplace_back(message);
    parameters->emplace_back(std::make_shared<BaseLib::Variable>(synchronous));
    GD::ipcServer->callProcessRpcMethod(_pid, _nodeBlueClientInfo, "nodeInput", parameters);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::event(const BaseLib::PArray &parameters) {
  try {
    if (parameters->size() >= 2 && parameters->at(0)->stringValue == "global") {
      if (parameters->at(1)->stringValue == "isReady") {
        {
          std::lock_guard<std::mutex> waitLock(_processStartUpMutex);
          _processStartUpComplete = true;
        }
        _processStartUpConditionVariable.notify_all();
      } else if (!_processStartUpComplete) {
        return;
      } else if (parameters->size() >= 3 && parameters->at(1)->stringValue == "node-output") {
        auto &payload = parameters->at(2);
        auto nodeIdIterator = payload->structValue->find("nodeId");
        if (nodeIdIterator == payload->structValue->end()) return;
        auto &nodeId = nodeIdIterator->second->stringValue;
        bool synchronous = false;
        auto synchronousIterator = payload->structValue->find("synchronous");
        if (synchronousIterator != payload->structValue->end()) synchronous = synchronousIterator->second->booleanValue;
        auto messageIterator = payload->structValue->find("message");
        if (messageIterator != payload->structValue->end()) {
          auto &message = messageIterator->second;
          if (message->type == BaseLib::VariableType::tArray) {
            for (uint32_t i = 0; i < message->arrayValue->size(); i++) {
              GD::nodeBlueServer->nodeOutput(nodeId, i, message->arrayValue->at(i), synchronous);
            }
          } else GD::nodeBlueServer->nodeOutput(nodeId, 0, message, synchronous);
        }
      } else if (parameters->size() >= 3 && parameters->at(1)->stringValue == "frontend-event") {
        GD::rpcClient->broadcastNodeEvent("", parameters->at(2)->structValue->at("topic")->stringValue, parameters->at(2)->structValue->at("data"), parameters->at(2)->structValue->at("retain")->booleanValue);
      }
    } else if (!_processStartUpComplete) {
      return;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}

}