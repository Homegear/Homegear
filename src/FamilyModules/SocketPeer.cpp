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

#include "SocketPeer.h"
#include "../GD/GD.h"

namespace Homegear
{
std::shared_ptr<BaseLib::Systems::ICentral> SocketPeer::getCentral()
{
    //Todo: Implement
    return std::shared_ptr<BaseLib::Systems::ICentral>();
}

bool SocketPeer::wireless()
{
    //Todo: Implement
    return false;
}

SocketPeer::SocketPeer(uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl.get(), parentID, eventHandler)
{
    init();
}

SocketPeer::SocketPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl.get(), id, address, serialNumber, parentID, eventHandler)
{
    init();
}

SocketPeer::~SocketPeer()
{
    try
    {
        dispose();
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

void SocketPeer::init()
{
    try
    {

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

void SocketPeer::dispose()
{
    if(_disposing) return;
    Peer::dispose();
}

void SocketPeer::homegearStarted()
{
    try
    {
        Peer::homegearStarted();
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

void SocketPeer::homegearShuttingDown()
{
    try
    {
        _shuttingDown = true;
        Peer::homegearShuttingDown();
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

std::string SocketPeer::handleCliCommand(std::string command)
{
    //Todo: Implement
    return "";
}

std::string SocketPeer::getPhysicalInterfaceId()
{
    return _physicalInterfaceId;
}

bool SocketPeer::load(BaseLib::Systems::ICentral* central)
{
    //Todo: Implement
    return false;
}

PParameterGroup SocketPeer::getParameterSet(int32_t channel, ParameterGroup::Type::Enum type)
{
    try
    {
        PFunction rpcChannel = _rpcDevice->functions.at(channel);
        if(type == ParameterGroup::Type::Enum::variables) return rpcChannel->variables;
        else if(type == ParameterGroup::Type::Enum::config) return rpcChannel->configParameters;
        else if(type == ParameterGroup::Type::Enum::link) return rpcChannel->linkParameters;
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
    return PParameterGroup();
}

}
