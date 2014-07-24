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

#ifndef MAXCENTRAL_H_
#define MAXCENTRAL_H_

class MAXPacket;

#include "../MAXDevice.h"

#include <memory>
#include <mutex>
#include <string>

namespace MAX
{

class MAXCentral : public MAXDevice, public BaseLib::Systems::Central
{
public:
	MAXCentral(IDeviceEventSink* eventHandler);
	MAXCentral(uint32_t deviceType, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
	virtual ~MAXCentral();
	void init();
	virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet);
	std::string handleCLICommand(std::string command);
	uint64_t getPeerIDFromSerial(std::string serialNumber) { std::shared_ptr<MAXPeer> peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }

	virtual bool knowsDevice(std::string serialNumber);
	virtual bool knowsDevice(uint64_t id);
protected:
	std::shared_ptr<MAXPeer> createPeer(int32_t address, int32_t firmwareVersion, BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);
private:
	std::mutex _peerInitMutex;
};

}

#endif
