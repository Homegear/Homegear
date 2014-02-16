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

#ifndef HMWIREDDEVICE_H
#define HMWIREDDEVICE_H

#include "../../LogicalDevices/LogicalDevice.h"
#include "../../HelperFunctions/HelperFunctions.h"
#include "HMWiredQueue.h"
#include "HMWiredPeer.h"
#include "HMWiredQueueManager.h"
#include "HMWiredPacketManager.h"
#include "HMWiredDeviceTypes.h"

#include <string>
#include <unordered_map>
#include <map>
#include <mutex>
#include <vector>
#include <queue>
#include <thread>
#include <chrono>
#include "pthread.h"

namespace HMWired
{
class HMWiredMessage;
class HMWiredMessages;

class HMWiredDevice : public LogicalDevice
{
    public:
		//In table variables
		int32_t getFirmwareVersion() { return _firmwareVersion; }
		void setFirmwareVersion(int32_t value) { _firmwareVersion = value; saveVariable(0, value); }
		int32_t getCentralAddress() { return _centralAddress; }
		void setCentralAddress(int32_t value) { _centralAddress = value; saveVariable(1, value); }
		//End

		virtual bool isCentral();

        HMWiredDevice();
        HMWiredDevice(uint32_t deviceID, std::string serialNumber, int32_t address);
        virtual ~HMWiredDevice();
        virtual DeviceFamilies deviceFamily() { return DeviceFamilies::HomeMaticWired; }
        bool packetReceived(std::shared_ptr<Packet> packet);

        virtual void addPeer(std::shared_ptr<HMWiredPeer> peer);
        virtual bool peerSelected() { return (bool)_currentPeer; }
		bool peerExists(int32_t address);
		std::shared_ptr<HMWiredPeer> getPeer(int32_t address);
		std::shared_ptr<HMWiredPeer> getPeer(uint64_t id);
		std::shared_ptr<HMWiredPeer> getPeer(std::string serialNumber);
		virtual void deletePeersFromDatabase();
		virtual void loadPeers(bool version_0_0_7);
		virtual void savePeers(bool full);
        virtual void loadVariables();
        virtual void saveVariables();
        virtual void saveVariable(uint32_t index, int64_t intValue);
        virtual void saveVariable(uint32_t index, std::string& stringValue);
        virtual void saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue);
        virtual void saveMessageCounters();
        virtual void load();
        virtual void save(bool saveDevice);
        virtual void serializeMessageCounters(std::vector<uint8_t>& encodedData);
        virtual void unserializeMessageCounters(std::shared_ptr<std::vector<char>> serializedData);

        virtual bool isInPairingMode() { return _pairing; }
        virtual std::shared_ptr<HMWiredMessages> getMessages() { return _messages; }
        virtual std::shared_ptr<HMWiredPacket> sendPacket(std::shared_ptr<HMWiredPacket> packet, bool resend, bool stealthy = false);

        virtual void handleAck(std::shared_ptr<HMWiredPacket> packet) {}

        virtual std::shared_ptr<HMWiredPacket> getResponse(uint8_t command, int32_t destinationAddress);
        virtual void sendOK(int32_t messageCounter, int32_t destinationAddress);
    protected:
        //In table variables
        int32_t _firmwareVersion = 0;
        int32_t _centralAddress = 0;
        std::unordered_map<int32_t, uint8_t> _messageCounter;
        //End

        std::map<uint32_t, uint32_t> _variableDatabaseIDs;
        bool _disposing = false;

        std::shared_ptr<HMWiredPeer> _currentPeer;
        std::unordered_map<int32_t, std::shared_ptr<HMWiredPeer>> _peers;
        std::unordered_map<std::string, std::shared_ptr<HMWiredPeer>> _peersBySerial;
        std::unordered_map<uint64_t, std::shared_ptr<HMWiredPeer>> _peersByID;
        std::timed_mutex _peersMutex;
        std::mutex _databaseMutex;
        HMWiredQueueManager _hmWiredQueueManager;
        HMWiredPacketManager _receivedPackets;
        HMWiredPacketManager _sentPackets;
        std::mutex _sendMutex;
        bool _pairing = false;
        std::shared_ptr<HMWiredMessages> _messages;
        bool _initialized = false;

        virtual void init();
        virtual void setUpHMWiredMessages() {}
        void lockBus();
        void unlockBus();
    private:
};
}
#endif // HMWIREDDEVICE_H
