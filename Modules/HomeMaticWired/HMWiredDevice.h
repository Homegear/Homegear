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

#include "../Base/BaseLib.h"
#include "HMWiredPeer.h"
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
class HMWiredDevice : public BaseLib::Systems::LogicalDevice
{
    public:
		//In table variables
		int32_t getFirmwareVersion() { return _firmwareVersion; }
		void setFirmwareVersion(int32_t value) { _firmwareVersion = value; saveVariable(0, value); }
		int32_t getCentralAddress() { return _centralAddress; }
		void setCentralAddress(int32_t value) { _centralAddress = value; saveVariable(1, value); }
		//End

		virtual bool isCentral();

        HMWiredDevice(IDeviceEventSink* eventHandler);
        HMWiredDevice(uint32_t deviceID, std::string serialNumber, int32_t address, IDeviceEventSink* eventHandler);
        virtual ~HMWiredDevice();
        virtual void dispose(bool wait = true);
        virtual bool onPacketReceived(std::string& senderID, std::shared_ptr<BaseLib::Systems::Packet> packet);

        virtual void addPeer(std::shared_ptr<HMWiredPeer> peer);
        virtual bool peerSelected() { return (bool)_currentPeer; }
		bool peerExists(int32_t address);
		bool peerExists(uint64_t id);
		virtual std::shared_ptr<BaseLib::Systems::Central> getCentral();
		std::shared_ptr<HMWiredPeer> getPeer(int32_t address);
		std::shared_ptr<HMWiredPeer> getPeer(uint64_t id);
		std::shared_ptr<HMWiredPeer> getPeer(std::string serialNumber);
		virtual void loadPeers();
		virtual void savePeers(bool full);
        virtual void loadVariables();
        virtual void saveVariables();
        virtual void saveMessageCounters();
        virtual void serializeMessageCounters(std::vector<uint8_t>& encodedData);
        virtual void unserializeMessageCounters(std::shared_ptr<std::vector<char>> serializedData);
        virtual uint8_t getMessageCounter(int32_t destinationAddress);

        virtual bool isInPairingMode() { return _pairing; }
        virtual std::shared_ptr<HMWiredPacket> sendPacket(std::shared_ptr<HMWiredPacket> packet, bool resend, bool systemResponse = false);
        std::shared_ptr<HMWiredPacket> getSentPacket(int32_t address) { return _sentPackets.get(address); }

        virtual std::shared_ptr<HMWiredPacket> getResponse(uint8_t command, int32_t destinationAddress, bool synchronizationBit = false);
        virtual std::shared_ptr<HMWiredPacket> getResponse(std::vector<uint8_t>& payload, int32_t destinationAddress, bool synchronizationBit = false);
        virtual std::shared_ptr<HMWiredPacket> getResponse(std::shared_ptr<HMWiredPacket> packet, bool systemResponse = false);
        virtual std::vector<uint8_t> readEEPROM(int32_t deviceAddress, int32_t eepromAddress);
        virtual bool writeEEPROM(int32_t deviceAddress, int32_t eepromAddress, std::vector<uint8_t>& data);
        virtual void sendOK(int32_t messageCounter, int32_t destinationAddress);
    protected:
        //In table variables
        int32_t _firmwareVersion = 0;
        int32_t _centralAddress = 0;
        std::unordered_map<int32_t, uint8_t> _messageCounter;
        //End

        HMWiredPacketManager _receivedPackets;
        HMWiredPacketManager _sentPackets;
        bool _pairing = false;

        virtual void init();
        void lockBus();
        void unlockBus();
    private:
};
}
#endif // HMWIREDDEVICE_H
