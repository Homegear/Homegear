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

#include "SocketDeviceFamily.h"
#include "../GD/GD.h"

namespace Homegear
{

SocketDeviceFamily::SocketDeviceFamily(BaseLib::Systems::IFamilyEventSink* eventHandler, int32_t id, const std::string& name) : BaseLib::Systems::DeviceFamily(GD::bl.get(), eventHandler, id, name)
{
}

bool SocketDeviceFamily::init()
{
    //Todo: Retrieve device list
    return true;
}

void SocketDeviceFamily::dispose()
{
    if(_disposed) return;
    DeviceFamily::dispose();

    _central.reset();
}

void SocketDeviceFamily::reloadRpcDevices()
{
    //Todo: Implement
}

void SocketDeviceFamily::createCentral()
{
    //Todo: Implement
}

std::shared_ptr<BaseLib::Systems::ICentral> SocketDeviceFamily::initializeCentral(uint32_t deviceId, int32_t address, std::string serialNumber)
{
    return std::make_shared<SocketCentral>(getFamily(), deviceId, serialNumber, this);
}

BaseLib::PVariable SocketDeviceFamily::getPairingInfo()
{
    try
    {
        //Todo: Implement
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
}
