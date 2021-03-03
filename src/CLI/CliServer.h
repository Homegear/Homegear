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

#ifndef CLISERVER_H_
#define CLISERVER_H_

#include <homegear-base/BaseLib.h>
#include "../User/User.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

namespace Homegear {

class CliServer {
 public:
  CliServer(int32_t clientId);

  virtual ~CliServer();

  BaseLib::PVariable generalCommand(std::string &command);

  std::string familyCommand(int32_t familyId, std::string &command);

  std::string peerCommand(uint64_t peerId, std::string &command);

 private:
  int32_t _clientId = 0;
  std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
  bool _scriptFinished = false;
  std::mutex _waitMutex;
  std::condition_variable _waitConditionVariable;

#ifndef NO_SCRIPTENGINE
  void scriptFinished(const BaseLib::ScriptEngine::PScriptInfo &scriptInfo, int32_t exitCode);

  void scriptOutput(const BaseLib::ScriptEngine::PScriptInfo &scriptInfo, const std::string &output, bool error);
#endif

  std::string userCommand(std::string &command);

  std::string moduleCommand(std::string &command);
};

}

#endif
