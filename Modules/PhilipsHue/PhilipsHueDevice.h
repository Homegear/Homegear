/* Copyright 2013-2014 Sathya Laufer
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

#ifndef PHILIPSHUEDEVICE_H
#define PHILIPSHUEDEVICE_H

#include "../Base/BaseLib.h"
#include "PhilipsHuePeer.h"
#include "PhilipsHueDeviceTypes.h"
#include "PhilipsHuePacket.h"

#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include "pthread.h"

using namespace BaseLib::Systems;
using namespace BaseLib::RPC;

namespace PhilipsHue
{
class PhilipsHueDevice : public BaseLib::Systems::LogicalDevice
{
    public:
		//In table variables
		int32_t getFirmwareVersion() { return _firmwareVersion; }
		void setFirmwareVersion(int32_t value) { _firmwareVersion = value; saveVariable(0, value); }
		//End

		virtual bool isCentral();

        PhilipsHueDevice(IDeviceEventSink* eventHandler);
        PhilipsHueDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
        virtual ~PhilipsHueDevice();
        virtual void dispose(bool wait = true);

        virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet);
        virtual bool peerSelected() { return (bool)_currentPeer; }
        bool peerExists(int32_t address);
		bool peerExists(uint64_t id);
        virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
        std::shared_ptr<PhilipsHuePeer> getPeer(int32_t address);
		std::shared_ptr<PhilipsHuePeer> getPeer(uint64_t id);
		std::shared_ptr<PhilipsHuePeer> getPeer(std::string serialNumber);

        virtual void loadVariables();
        virtual void saveVariables();
        virtual void loadPeers();
        virtual void savePeers(bool full);

        virtual void sendPacket(std::shared_ptr<PhilipsHuePacket> packet);
    protected:
        //In table variables
        int32_t _firmwareVersion = 0;
        //End
    private:
};
}
#endif
