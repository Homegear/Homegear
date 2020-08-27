/* Copyright 2013-2020 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#ifndef SOCKET_PEER_H_
#define SOCKET_PEER_H_

#include <homegear-base/BaseLib.h>

namespace Homegear {

class SocketPeer : public BaseLib::Systems::Peer, public BaseLib::Rpc::IWebserverEventSink {
 public:
  SocketPeer(uint32_t parentID, IPeerEventSink *eventHandler);
  SocketPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, IPeerEventSink *eventHandler);
  ~SocketPeer() final;
  void init();
  void dispose();

  //Features
  bool wireless() final;
  //End features

  //{{{ In table variables
  std::string getPhysicalInterfaceId();
  void setPhysicalInterfaceId(std::string);
  //}}}

  std::string handleCliCommand(std::string command) final;

  bool load(BaseLib::Systems::ICentral *central) final;
  void savePeers() final {}

  int32_t getChannelGroupedWith(int32_t channel) final { return -1; }
  int32_t getNewFirmwareVersion() final { return 0; }
  std::string getFirmwareVersionString(int32_t firmwareVersion) final { return "1.0"; }
  bool firmwareUpdateAvailable() final { return false; }

  /**
   * {@inheritDoc}
   */
  void homegearStarted() final;

  /**
   * {@inheritDoc}
   */
  void homegearShuttingDown() final;
 protected:
  //In table variables:
  std::string _physicalInterfaceId;
  //End

  std::atomic_bool _shuttingDown{false};

  std::shared_ptr<BaseLib::Systems::ICentral> getCentral() final;

  PParameterGroup getParameterSet(int32_t channel, ParameterGroup::Type::Enum type) final;
};

typedef std::shared_ptr<SocketPeer> PSocketPeer;

}

#endif
