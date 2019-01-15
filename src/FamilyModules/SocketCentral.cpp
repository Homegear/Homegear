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

#include "SocketCentral.h"
#include "../GD/GD.h"

namespace Homegear
{

SocketCentral::SocketCentral(int32_t familyId, ICentralEventSink* eventHandler) : BaseLib::Systems::ICentral(familyId, GD::bl.get(), eventHandler)
{
    init();
}

SocketCentral::SocketCentral(int32_t familyId, uint32_t deviceId, std::string serialNumber, ICentralEventSink* eventHandler) : BaseLib::Systems::ICentral(familyId, GD::bl.get(), deviceId, serialNumber, -1, eventHandler)
{
    init();
}

SocketCentral::~SocketCentral()
{
    dispose();
}

void SocketCentral::dispose(bool wait)
{
    try
    {
        if(_disposing) return;
        _disposing = true;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void SocketCentral::init()
{
    try
    {
        _shuttingDown = false;
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void SocketCentral::homegearShuttingDown()
{
    _shuttingDown = true;
}

void SocketCentral::loadPeers()
{
    //Todo: Implement
}

PSocketPeer SocketCentral::getPeer(uint64_t id)
{
    //Todo: Implement
    return PSocketPeer();
}

PSocketPeer SocketCentral::getPeer(std::string serialNumber)
{
    //Todo: Implement
    return PSocketPeer;
}

bool SocketCentral::onPacketReceived(std::string& senderId, std::shared_ptr<BaseLib::Systems::Packet> packet)
{
    return false;
}

void SocketCentral::savePeers(bool full)
{
    //Todo: Implement
}

void SocketCentral::deletePeer(uint64_t id)
{
    //Todo: Implement
}

std::string SocketCentral::handleCliCommand(std::string command)
{
    //Todo: Implement
    return "";
}

PSocketPeer SocketCentral::createPeer(uint32_t deviceType, int32_t firmwareVersion, std::string serialNumber, bool save)
{
    //Todo: Implement
    return PSocketPeer();
}

BaseLib::PVariable SocketCentral::deleteDevice(BaseLib::PRpcClientInfo clientInfo, std::string serialNumber, int32_t flags)
{
    try
    {
        //Todo: Implement
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable SocketCentral::deleteDevice(BaseLib::PRpcClientInfo clientInfo, uint64_t peerID, int32_t flags)
{
    try
    {
        //Todo: Implement
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}
