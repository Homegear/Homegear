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

#ifndef HOMEGEAR_SRC_NODE_BLUE_NODE_RED_NODEPINK_H_
#define HOMEGEAR_SRC_NODE_BLUE_NODE_RED_NODEPINK_H_

#include <homegear-base/BaseLib.h>

namespace Homegear {

namespace NodeBlue {

class Nodepink {
 private:
  BaseLib::Output _out;
  std::shared_ptr<BaseLib::RpcClientInfo> _nodeBlueClientInfo;
  std::atomic_bool _startUpError{false};
  std::mutex _processStartUpMutex;
  std::condition_variable _processStartUpConditionVariable;
  std::atomic_bool _processStartUpComplete{false};
  int32_t _callbackHandlerId = -1;
  std::atomic_bool _stopThread{false};
  std::thread _execThread;
  std::thread _errorThread;
  std::atomic_int _pid{-1};
  std::atomic_int _stdIn{-1};
  std::atomic_int _stdOut{-1};
  std::atomic_int _stdErr{-1};

  void sigchildHandler(pid_t pid, int exitCode, int signal, bool coreDumped);
  void execThread();
  void errorThread();
  void startProgram();
 public:
  Nodepink();

  bool isStarted();
  void start();
  void stop();
  BaseLib::PVariable invoke(const std::string &method, const BaseLib::PArray &parameters);
  void nodeInput(const std::string &nodeId, const BaseLib::PVariable &nodeInfo, uint32_t inputIndex, const BaseLib::PVariable &message, bool synchronous);
  void event(const BaseLib::PArray &parameters);
};

}

}

#endif //HOMEGEAR_SRC_NODE_BLUE_NODE_RED_NODEPINK_H_
