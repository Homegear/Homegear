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

#ifndef SOCKET_DEVICE_FAMILY_H_
#define SOCKET_DEVICE_FAMILY_H_

#include "SocketCentral.h"

#include <homegear-base/BaseLib.h>

namespace Homegear
{

class SocketDeviceFamily : public BaseLib::Systems::DeviceFamily
{
public:
    SocketDeviceFamily(BaseLib::Systems::IFamilyEventSink* eventHandler, int32_t id, const std::string& name);
    ~SocketDeviceFamily() final = default;
    bool init() final;
    void dispose() final;

    bool hasPhysicalInterface() final { return false; }
    BaseLib::PVariable getPairingInfo() final;
    void reloadRpcDevices();
protected:
    std::shared_ptr<BaseLib::Systems::ICentral> initializeCentral(uint32_t deviceId, int32_t address, std::string serialNumber) final;
    void createCentral() final;
};

}

#endif
