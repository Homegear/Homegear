/* Copyright 2013-2019 Homegear GmbH
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

#ifndef SOCKET_CENTRAL_H_
#define SOCKET_CENTRAL_H_

#include "SocketPeer.h"

#include <homegear-base/BaseLib.h>

namespace Homegear
{

class SocketCentral : public BaseLib::Systems::ICentral
{
public:
    SocketCentral(int32_t familyId, ICentralEventSink* eventHandler);
    SocketCentral(int32_t familyId, uint32_t deviceType, std::string serialNumber, ICentralEventSink* eventHandler);
    ~SocketCentral() final;
    void dispose(bool wait = true) final;

    void homegearShuttingDown() final;

    std::string handleCliCommand(std::string command);
    bool onPacketReceived(std::string& senderId, std::shared_ptr<BaseLib::Systems::Packet> packet) final;

    uint64_t getPeerIdFromSerial(std::string& serialNumber) { PSocketPeer peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }
    PSocketPeer getPeer(uint64_t id);
    PSocketPeer getPeer(std::string serialNumber);

    BaseLib::PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags) final;
    BaseLib::PVariable deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerId, int32_t flags) final;
protected:
    std::atomic_bool _shuttingDown;

    void init();
    void loadPeers() final;
    void savePeers(bool full) final;
    void loadVariables() final {}
    void saveVariables() final {}
    PSocketPeer createPeer(uint32_t deviceType, int32_t firmwareVersion, std::string serialNumber, bool save = true);
    void deletePeer(uint64_t id);
};

}

#endif
