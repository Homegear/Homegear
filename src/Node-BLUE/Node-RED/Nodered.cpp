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
      _out.printInfo("Info: Node-RED process " + std::to_string(pid) + " exited with code " + std::to_string(exitCode) + " (Core dumped: " + std::to_string(coreDumped) + +", signal: " + std::to_string(signal) + ").");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::start() {
  try {
    _callbackHandlerId = BaseLib::ProcessManager::registerCallbackHandler(std::function<void(pid_t pid, int exitCode, int signal, bool coreDumped)>(std::bind(&Nodered::sigchildHandler,
                                                                                                                                                              this,
                                                                                                                                                              std::placeholders::_1,
                                                                                                                                                              std::placeholders::_2,
                                                                                                                                                              std::placeholders::_3,
                                                                                                                                                              std::placeholders::_4)));

    startProgram();

    {
      while (!_startUpError) {
        if (_processStartUpComplete) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

    if (_startUpError) {
      GD::out.printError("Error: Node-RED could not be started.");
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::stop() {
  try {
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
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::startProgram() {
  try {
    if (_execThread.joinable()) _execThread.join();
    if (_errorThread.joinable()) _errorThread.join();
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

      auto redJsPath = GD::bl->settings.nodeRedJsPath();
      if (redJsPath.empty()) redJsPath = GD::bl->settings.dataPath() + "node-blue/node-red/node_modules/node-red/red.js";

      std::vector<std::string> arguments{"-n", redJsPath, "-s", GD::bl->settings.dataPath() + "node-blue/node-red/settings.js"};
      int stdIn = -1;
      int stdOut = -1;
      int stdErr = -1;
      _pid = BaseLib::ProcessManager::systemp(GD::executablePath + "/" + GD::executableFile, arguments, GD::bl->fileDescriptorManager.getMax(), stdIn, stdOut, stdErr);
      _stdIn = stdIn;
      _stdOut = stdOut;
      _stdErr = stdErr;

      _errorThread = std::thread(&Nodered::errorThread, this);

      while (_pid != -1) //While process is running
      {
        auto result = invoke("ping", std::make_shared<BaseLib::Array>());
        if (!result->errorStruct) break; //Process started and responding

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
      if (_pid == -1) _startUpError = true;
      else _processStartUpComplete = true;

      std::array<uint8_t, 4096> buffer{};
      std::string bufferOut;
      while (_stdOut != -1) {
        auto bytesRead = 0;
        bufferOut.clear();
        do {
          bytesRead = read(_stdOut, buffer.data(), buffer.size());
          if (bytesRead > 0) {
            bufferOut.insert(bufferOut.end(), buffer.begin(), buffer.begin() + bytesRead);
          }
        } while (bytesRead > 0);

        if (!bufferOut.empty()) _out.printInfo(bufferOut);
      }

      std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (!_stopThread);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void Nodered::errorThread() {
  try {
    std::array<uint8_t, 4096> buffer{};
    std::string bufferErr;
    while (_stdErr != -1) {
      int32_t bytesRead = 0;
      bufferErr.clear();
      do {
        bytesRead = read(_stdErr, buffer.data(), buffer.size());
        if (bytesRead > 0) {
          bufferErr.insert(bufferErr.end(), buffer.begin(), buffer.begin() + bytesRead);
        }
      } while (bytesRead > 0);

      if (!bufferErr.empty()) _out.printError(bufferErr);
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

}

}