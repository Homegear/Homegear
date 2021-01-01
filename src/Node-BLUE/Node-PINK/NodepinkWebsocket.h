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

#ifndef HOMEGEAR_SRC_NODE_BLUE_NODE_RED_NODEPINKWEBSOCKET_H_
#define HOMEGEAR_SRC_NODE_BLUE_NODE_RED_NODEPINKWEBSOCKET_H_

#include <homegear-base/BaseLib.h>

namespace Homegear {

namespace NodeBlue {

/**
 * This class proxies WebSocket connections to Node-RED. Only third party connections are handled here (like from Dashboard). Editor connections are handled by Node-BLUE.
 */
class NodepinkWebsocket {
 private:
  struct ClientData {
    int32_t id = 0;
    BaseLib::PTcpSocket socket;
    BaseLib::PFileDescriptor fileDescriptor;
    std::vector<uint8_t> buffer;
    BaseLib::PTcpSocket clientSocket;

    ClientData() {
      buffer.resize(1024);
    }
  };
  typedef std::shared_ptr<ClientData> PClientData;

  BaseLib::Output _out;
  std::atomic_bool _started{false};
  std::atomic_bool _stopThreads{false};
  std::thread _mainThread;

  int64_t _lastGarbageCollection = 0;
  static const uint32_t _maxConnections = 50;
  int32_t _currentClientId = 0;
  std::mutex _clientsMutex;
  std::map<int32_t, PClientData> _clients;

  void readClient(const PClientData &clientData);
  void readNoderedClient(const PClientData &clientData);
  void mainThread();
  void collectGarbage();
 public:
  NodepinkWebsocket();
  ~NodepinkWebsocket();

  void start();
  void stop();
  void handoverClient(const BaseLib::PTcpSocket &socket, const BaseLib::PFileDescriptor &fileDescriptor, const BaseLib::Http &initialPacket);
};

}

}

#endif //HOMEGEAR_SRC_NODE_BLUE_NODE_RED_NODEPINKWEBSOCKET_H_
