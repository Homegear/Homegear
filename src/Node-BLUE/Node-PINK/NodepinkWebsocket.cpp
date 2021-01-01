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

#include "NodepinkWebsocket.h"
#include "../../GD/GD.h"

namespace Homegear {

namespace NodeBlue {

NodepinkWebsocket::NodepinkWebsocket() {
  _out.init(GD::bl.get());
  _out.setPrefix("Node-PINK WebSocket: ");
}

NodepinkWebsocket::~NodepinkWebsocket() {
  if (_started == false) return;
  stop();
}

void NodepinkWebsocket::start() {
  try {
    if (_started) return;
    GD::out.printInfo("Info: Starting Node-PINK WebSockets.");
    _started = true;
    _stopThreads = false;
    GD::bl->threadManager.start(_mainThread, false, &NodepinkWebsocket::mainThread, this);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodepinkWebsocket::stop() {
  try {
    GD::out.printInfo("Info: Stopping Node-PINK WebSockets.");
    _stopThreads = true;
    GD::bl->threadManager.join(_mainThread);
    std::lock_guard<std::mutex> clientsGuard(_clientsMutex);
    _clients.clear();
    _started = false;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodepinkWebsocket::collectGarbage() {
  try {
    _lastGarbageCollection = BaseLib::HelperFunctions::getTime();
    std::vector<int32_t> clientsToRemove;
    std::lock_guard<std::mutex> clientsGuard(_clientsMutex);
    for (auto &client : _clients) {
      if (!client.second->fileDescriptor ||
          client.second->fileDescriptor->descriptor == -1 ||
          client.second->clientSocket->getFileDescriptor()->descriptor == -1) {
        clientsToRemove.push_back(client.first);
      }
    }
    for (auto &client : clientsToRemove) {
      _clients.erase(client);
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodepinkWebsocket::mainThread() {
  try {
    int32_t result = 0;
    std::map<int32_t, PClientData> clients;
    while (!_stopThreads) {
      try {
        {
          std::lock_guard<std::mutex> clientsGuard(_clientsMutex);
          clients = _clients;
        }

        if (clients.empty()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        fd_set readFileDescriptor;
        int32_t maxfd = 0;
        FD_ZERO(&readFileDescriptor);
        {
          auto fileDescriptorGuard = GD::bl->fileDescriptorManager.getLock();
          fileDescriptorGuard.lock();
          for (auto &client : clients) {
            auto clientSocketFileDescriptor = client.second->clientSocket->getFileDescriptor();
            if (!client.second->fileDescriptor->descriptor ||
                client.second->fileDescriptor->descriptor == -1 ||
                clientSocketFileDescriptor->descriptor == -1) {
              continue;
            }
            FD_SET(clientSocketFileDescriptor->descriptor, &readFileDescriptor);
            if (clientSocketFileDescriptor->descriptor > maxfd) maxfd = clientSocketFileDescriptor->descriptor;
            FD_SET(client.second->fileDescriptor->descriptor, &readFileDescriptor);
            if (client.second->fileDescriptor->descriptor > maxfd) maxfd = client.second->fileDescriptor->descriptor;
          }
        }

        result = select(maxfd + 1, &readFileDescriptor, nullptr, nullptr, &timeout);
        if (result == 0) {
          if (BaseLib::HelperFunctions::getTime() - _lastGarbageCollection > 60000 || _clients.size() >= _maxConnections) {
            collectGarbage();
          }
          continue;
        } else if (result == -1) {
          if (errno == EINTR) continue;
          _out.printError("Error: select returned -1: " + std::string(strerror(errno)));
          continue;
        }

        for (auto &client : clients) {
          if (client.second->fileDescriptor->descriptor == -1 || client.second->clientSocket->getFileDescriptor()->descriptor == -1) continue;
          if (FD_ISSET(client.second->clientSocket->getFileDescriptor()->descriptor, &readFileDescriptor)) {
            readNoderedClient(client.second);
          }
          if (FD_ISSET(client.second->fileDescriptor->descriptor, &readFileDescriptor)) {
            readClient(client.second);
          }
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void NodepinkWebsocket::readClient(const PClientData &clientData) {
  try {
    int32_t bytesRead = 0;
    bool moreData = true;

    while (moreData) {
      bytesRead = clientData->socket->proofread((char *)clientData->buffer.data(), clientData->buffer.size(), moreData);

      if (bytesRead > (signed)clientData->buffer.size()) bytesRead = clientData->buffer.size();

      if (!clientData->clientSocket->connected()) {
        GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
        _out.printInfo("Info: Connection to client " + std::to_string(clientData->id) + " closed (1).");
        return;
      }
      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Writing to client socket: " + BaseLib::HelperFunctions::getHexString(clientData->buffer.data(), bytesRead));
      clientData->clientSocket->proofwrite((char *)clientData->buffer.data(), bytesRead);
    }
  }
  catch (const BaseLib::SocketClosedException &ex) {
    _out.printInfo("Info: Connection to client " + std::to_string(clientData->id) + " closed (2).");
  }
  catch (const std::exception &ex) {
    GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
    _out.printError("Error reading from client " + std::to_string(clientData->id) + ": " + ex.what());
  }
}

void NodepinkWebsocket::readNoderedClient(const PClientData &clientData) {
  try {
    int32_t bytesRead = 0;
    bool moreData = true;

    while (moreData) {
      bytesRead = clientData->clientSocket->proofread((char *)clientData->buffer.data(), clientData->buffer.size(), moreData);

      if (bytesRead > (signed)clientData->buffer.size()) bytesRead = clientData->buffer.size();

      if (!clientData->socket->connected()) {
        clientData->clientSocket->close();
        _out.printInfo("Info: Connection to client " + std::to_string(clientData->id) + " closed (3).");
        return;
      }
      if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Writing to browser socket: " + BaseLib::HelperFunctions::getHexString(clientData->buffer.data(), bytesRead));
      clientData->socket->proofwrite((char *)clientData->buffer.data(), bytesRead);
    }
  }
  catch (const BaseLib::SocketClosedException &ex) {
    _out.printInfo("Info: Connection to client " + std::to_string(clientData->id) + " closed (4).");
  }
  catch (const std::exception &ex) {
    GD::bl->fileDescriptorManager.close(clientData->fileDescriptor);
    _out.printError("Error reading from client " + std::to_string(clientData->id) + ": " + ex.what());
  }
}

void NodepinkWebsocket::handoverClient(const BaseLib::PTcpSocket &socket, const BaseLib::PFileDescriptor &fileDescriptor, const BaseLib::Http &initialPacket) {
  try {
    _out.printInfo("Info: New Node-PINK WebSocket connection.");
    auto clientData = std::make_shared<ClientData>();
    clientData->socket = socket;
    clientData->fileDescriptor = fileDescriptor;
    clientData->clientSocket = std::make_shared<BaseLib::TcpSocket>(GD::bl.get(), "127.0.0.1", std::to_string(GD::bl->settings.nodeRedPort()));
    clientData->clientSocket->setReadTimeout(100000);
    clientData->clientSocket->setWriteTimeout(15000000);
    clientData->clientSocket->setAutoConnect(false);
    clientData->clientSocket->open();

    auto rawHeader = std::string(initialPacket.getRawHeader().data(), initialPacket.getRawHeader().size());
    BaseLib::HelperFunctions::stringReplace(rawHeader, "GET /node-blue/", "GET /");

    std::vector<char> packet;
    packet.reserve(rawHeader.size() + initialPacket.getContentSize());
    packet.insert(packet.end(), rawHeader.begin(), rawHeader.end());
    packet.insert(packet.end(), initialPacket.getContent().data(), initialPacket.getContent().data() + initialPacket.getContentSize());
    if (GD::bl->debugLevel >= 5) _out.printDebug("Debug: Writing to client socket: " + BaseLib::HelperFunctions::getHexString(packet.data(), packet.size()));
    clientData->clientSocket->proofwrite(packet);

    std::lock_guard<std::mutex> clientsGuard(_clientsMutex);
    clientData->id = _currentClientId++;
    _clients.emplace(clientData->id, clientData);
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}

}