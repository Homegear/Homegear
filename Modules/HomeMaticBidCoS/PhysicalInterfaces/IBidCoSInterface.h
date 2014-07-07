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

#ifndef IBIDCOSINTERFACE_H_
#define IBIDCOSINTERFACE_H_

#include "../../Base/BaseLib.h"

namespace BidCoS {

class IBidCoSInterface : public BaseLib::Systems::IPhysicalInterface
{
public:
	class PeerInfo
	{
	public:
		PeerInfo() {}
		virtual ~PeerInfo() {}
		std::vector<char> getAESChannelMap();

		bool wakeUp = 0;
		int32_t address = 0;
		int32_t keyIndex = 0;
		std::map<int32_t, bool> aesChannels;
	};

	IBidCoSInterface(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
	virtual ~IBidCoSInterface();

	virtual void addPeer(PeerInfo peerInfo) {}
	virtual void addPeers(std::vector<PeerInfo>& peerInfos) {}
	virtual void setWakeUp(PeerInfo peerInfo) {}
	virtual void removePeer(int32_t address) {}
	virtual void setAES(int32_t address, int32_t channel, bool enabled) {}

	virtual bool aesSupported() { return false; }
	virtual bool autoResend() { return false; }
	virtual bool needsPeers() { return false; }
	virtual bool firmwareUpdatesSupported() { return true; }

	virtual uint32_t getCurrentRFKeyIndex() { return _settings->currentRFKeyIndex; }
};

}

#endif
