/* Copyright 2013-2015 Sathya Laufer
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

#include "homegear-base/BaseLib.h"
#include "MiscPeer.h"

#include <memory>
#include <mutex>
#include <string>

namespace Misc
{

class MiscCentral : public BaseLib::Systems::ICentral
{
public:
	MiscCentral(ICentralEventSink* eventHandler);
	MiscCentral(uint32_t deviceType, std::string serialNumber, ICentralEventSink* eventHandler);
	virtual ~MiscCentral();
	virtual void dispose(bool wait = true);

	std::string handleCliCommand(std::string command);
	uint64_t getPeerIdFromSerial(std::string serialNumber) { std::shared_ptr<MiscPeer> peer = getPeer(serialNumber); if(peer) return peer->getID(); else return 0; }

	virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet) { return true; }

	virtual void addPeer(std::shared_ptr<MiscPeer> peer);
	std::shared_ptr<MiscPeer> getPeer(uint64_t id);
	std::shared_ptr<MiscPeer> getPeer(std::string serialNumber);

	virtual BaseLib::PVariable createDevice(int32_t clientID, int32_t deviceType, std::string serialNumber, int32_t address, int32_t firmwareVersion);
	virtual BaseLib::PVariable deleteDevice(int32_t clientID, std::string serialNumber, int32_t flags);
	virtual BaseLib::PVariable deleteDevice(int32_t clientID, uint64_t peerID, int32_t flags);
	virtual BaseLib::PVariable getDeviceInfo(int32_t clientID, uint64_t id, std::map<std::string, bool> fields);
	virtual BaseLib::PVariable putParamset(int32_t clientID, std::string serialNumber, int32_t channel, ParameterGroup::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, BaseLib::PVariable paramset);
	virtual BaseLib::PVariable putParamset(int32_t clientID, uint64_t peerID, int32_t channel, ParameterGroup::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, BaseLib::PVariable paramset);
protected:
	std::shared_ptr<MiscPeer> createPeer(BaseLib::Systems::LogicalDeviceType deviceType, std::string serialNumber, bool save = true);
	void deletePeer(uint64_t id);
	virtual void init();

	virtual void loadPeers();
	virtual void savePeers(bool full);
	virtual void loadVariables() {}
	virtual void saveVariables() {}
};

}

#endif
