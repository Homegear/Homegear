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

#ifndef HOMEMATICCENTRAL_H_
#define HOMEMATICCENTRAL_H_

#include "../../../LogicalDevices/Central.h"
#include "HM-CC-TC.h"
#include "HM-LC-SwX-FM.h"
#include "../HomeMaticDevice.h"
#include "../../../Types/RPCVariable.h"
#include "../../../Metadata/Metadata.h"

#include <memory>
#include <mutex>
#include <string>
#include <cmath>

namespace BidCoS
{

class BidCoSPacket;

class HomeMaticCentral : public HomeMaticDevice, public Central
{
public:
	HomeMaticCentral();
	HomeMaticCentral(uint32_t deviceType, std::string, int32_t);
	virtual ~HomeMaticCentral();
	void init();
	bool packetReceived(std::shared_ptr<Packet> packet);
	void enablePairingMode() { _pairing = true; }
	void disablePairingMode() { _pairing = false; }
	void unpair(int32_t address, bool defer);
	void reset(int32_t address, bool defer);
	void setUpBidCoSMessages();
	void setUpConfig() {}
	void deletePeer(int32_t address);
	void addPeerToTeam(std::shared_ptr<BidCoSPeer> peer, int32_t channel, uint32_t teamChannel, std::string teamSerialNumber);
	void addPeerToTeam(std::shared_ptr<BidCoSPeer> peer, int32_t channel, int32_t address, uint32_t teamChannel);
	void removePeerFromTeam(std::shared_ptr<BidCoSPeer> peer);
	void resetTeam(std::shared_ptr<BidCoSPeer> peer, uint32_t channel);
	std::string handleCLICommand(std::string command);
	virtual void enqueuePackets(int32_t deviceAddress, std::shared_ptr<BidCoSQueue> packets, bool pushPendingBidCoSQueues = false);
	virtual void enqueuePendingQueues(int32_t deviceAddress);
	int32_t getUniqueAddress(int32_t seed);
	std::string getUniqueSerialNumber(std::string seedPrefix, uint32_t seedNumber);
	void addPeersToVirtualDevices();
	void addHomegearFeatures(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues);
	void addHomegearFeaturesHMCCVD(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues);
	void addHomegearFeaturesRemote(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues);
	void addHomegearFeaturesMotionDetector(std::shared_ptr<BidCoSPeer> peer, int32_t channel, bool pushPendingBidCoSQueues);

	void handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket>);
	void handleReset(int32_t messageCounter, std::shared_ptr<BidCoSPacket>) {}
	void sendEnableAES(int32_t address, int32_t channel);
	void sendRequestConfig(int32_t address, uint8_t localChannel, uint8_t list = 0, int32_t remoteAddress = 0, uint8_t remoteChannel = 0);

	virtual bool knowsDevice(std::string serialNumber);
	virtual bool knowsDevice(uint64_t id);

	virtual std::shared_ptr<RPC::RPCVariable> addDevice(std::string serialNumber);
	virtual std::shared_ptr<RPC::RPCVariable> addLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<RPC::RPCVariable> removeLink(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<RPC::RPCVariable> deleteDevice(std::string serialNumber, int32_t flags);
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceDescriptionCentral();
	virtual std::shared_ptr<RPC::RPCVariable> getDeviceDescription(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<RPC::RPCVariable> getInstallMode();
	virtual std::shared_ptr<RPC::RPCVariable> getLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel);
	virtual std::shared_ptr<RPC::RPCVariable> setLinkInfo(std::string senderSerialNumber, int32_t senderChannel, std::string receiverSerialNumber, int32_t receiverChannel, std::string name, std::string description);
	virtual std::shared_ptr<RPC::RPCVariable> getLinkPeers(std::string serialNumber, int32_t channel);
	virtual std::shared_ptr<RPC::RPCVariable> getLinks(std::string serialNumber, int32_t channel, int32_t flags);
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetDescription(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getParamsetId(std::string serialNumber, uint32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel);
	virtual std::shared_ptr<RPC::RPCVariable> getServiceMessages();
	virtual std::shared_ptr<RPC::RPCVariable> getValue(std::string serialNumber, uint32_t channel, std::string valueKey);
	virtual std::shared_ptr<RPC::RPCVariable> listDevices();
	virtual std::shared_ptr<RPC::RPCVariable> listDevices(std::shared_ptr<std::map<std::string, int32_t>> knownDevices);
	virtual std::shared_ptr<RPC::RPCVariable> listTeams();
	virtual std::shared_ptr<RPC::RPCVariable> setTeam(std::string serialNumber, int32_t channel, std::string teamSerialNumber, int32_t teamChannel, bool force = false, bool burst = true);
	virtual std::shared_ptr<RPC::RPCVariable> putParamset(std::string serialNumber, int32_t channel, RPC::ParameterSet::Type::Enum type, std::string remoteSerialNumber, int32_t remoteChannel, std::shared_ptr<RPC::RPCVariable> paramset);
	virtual std::shared_ptr<RPC::RPCVariable> setInstallMode(bool on, int32_t duration = 60, bool debugOutput = true);
	virtual std::shared_ptr<RPC::RPCVariable> setValue(std::string serialNumber, uint32_t channel, std::string valueKey, std::shared_ptr<RPC::RPCVariable> value);
protected:
	std::shared_ptr<BidCoSPeer> createPeer(int32_t address, int32_t firmwareVersion, LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>(), bool save = true);
	virtual void worker();
private:
	uint32_t _timeLeftInPairingMode = 0;
	void pairingModeTimer(int32_t duration, bool debugOutput = true);
	bool _stopPairingModeThread = false;
	std::thread _pairingModeThread;
	std::map<std::string, std::shared_ptr<RPC::RPCVariable>> _metadata;
};

}
#endif /* HOMEMATICCENTRAL_H_ */
