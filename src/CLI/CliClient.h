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

#ifndef CLICLIENT_H_
#define CLICLIENT_H_

#include <homegear-base/BaseLib.h>
#include <homegear-ipc/IIpcClient.h>

#include <termios.h>

namespace Homegear {

class CliClient : public Ipc::IIpcClient {
 public:
  explicit CliClient(const std::string &socketPath);

  ~CliClient() override;

  void stop() override;

  int32_t terminal(const std::string &command);
 private:
  std::atomic_bool _printEvents{false};
  int32_t _currentFamily = -1;
  uint64_t _currentPeer = 0;

  std::mutex _outputMutex;

  std::mutex _onConnectWaitMutex;
  std::condition_variable _onConnectConditionVariable;

  std::string getBreadcrumb() const;

  void onConnect() override;

  void onDisconnect() override;

  static void loadHistory();
  static void safeHistory();

  Ipc::PVariable invokeGeneralCommand(const std::string &command);

  // {{{ RPC methods
  Ipc::PVariable broadcastEvent(Ipc::PArray &parameters) override;

  Ipc::PVariable output(Ipc::PArray &parameters);
  // }}}

  void standardOutput(const std::string &text);

  void errorOutput(const std::string &text);
};

}

#endif
