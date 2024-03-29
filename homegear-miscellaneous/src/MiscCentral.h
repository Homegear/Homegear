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

#ifndef MISCCENTRAL_H_
#define MISCCENTRAL_H_

#include <homegear-base/BaseLib.h>
#include "MiscPeer.h"

#include <memory>
#include <mutex>
#include <string>

namespace Misc {

class MiscCentral : public BaseLib::Systems::ICentral {
 public:
  explicit MiscCentral(ICentralEventSink *eventHandler);
  MiscCentral(uint32_t deviceType, std::string serialNumber, ICentralEventSink *eventHandler);
  ~MiscCentral() override;
  void dispose(bool wait) override;

  std::string handleCliCommand(std::string command) override;
  uint64_t getPeerIdFromSerial(std::string &serialNumber) override {
    std::shared_ptr<MiscPeer> peer = getPeer(serialNumber);
    if (peer) return peer->getID(); else return 0;
  }

  bool onPacketReceived(std::string &senderID, std::shared_ptr<BaseLib::Systems::Packet> packet) override { return true; }

  void addPeer(std::shared_ptr<MiscPeer> peer);
  std::shared_ptr<MiscPeer> getPeer(uint64_t id);
  std::shared_ptr<MiscPeer> getPeer(std::string serialNumber);

  BaseLib::PVariable createDevice(BaseLib::PRpcClientInfo clientInfo, int32_t deviceType, std::string serialNumber, int32_t address, int32_t firmwareVersion, std::string interfaceId) override;
  BaseLib::PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags) override;
  BaseLib::PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerID, int32_t flags) override;
 protected:
  std::atomic_bool _stopWorkerThread{false};
  std::thread _workerThread;

  std::shared_ptr<MiscPeer> createPeer(uint32_t deviceType, std::string serialNumber, bool save = true);
  void deletePeer(uint64_t id);
  void init();
  void worker();

  void loadPeers() override;
  void savePeers(bool full) override;
  void loadVariables() override {}
  void saveVariables() override {}
};

}

#endif
