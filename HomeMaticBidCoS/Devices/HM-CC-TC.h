/* Copyright 2013 Sathya Laufer
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

#ifndef HM_CC_TC_H
#define HM_CC_TC_H

#include <thread>
#include <memory>
#include <vector>

class BidCoSQueue;
enum class BidCoSQueueType;

#include "../HomeMaticDevice.h"

class HM_CC_TC : public HomeMaticDevice
{
    public:
        HM_CC_TC();
        HM_CC_TC(uint32_t deviceID, std::string, int32_t);
        virtual ~HM_CC_TC();
        void dispose();
        void stopThreads();

        void setValveState(int32_t valveState);
        int32_t getNewValueState() { return _newValveState; }
        std::string handleCLICommand(std::string command);

        void handlePairingRequest(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void handleConfigParamResponse(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void handleAck(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void handleSetPoint(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void handleSetValveState(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
        void handleConfigPeerAdd(int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet);
    protected:
        virtual void setUpBidCoSMessages();
        virtual void init();
        void loadVariables();
        void saveVariables();
    private:
        //In table variables
        int32_t _currentDutyCycleDeviceAddress = -1;
        int32_t _temperature = 213;
        int32_t _setPointTemperature = 42;
        int32_t _humidity = 56;
        int32_t _valveState = 0;
        int32_t _newValveState = 0;
        int64_t _lastDutyCycleEvent = 0;
        //End

        const int32_t _dutyCycleTimeOffset = 3000;
        bool _stopDutyCycleThread = false;
        std::shared_ptr<Peer> createPeer(int32_t address, int32_t firmwareVersion, LogicalDeviceType deviceType, std::string serialNumber, int32_t remoteChannel, int32_t messageCounter, std::shared_ptr<BidCoSPacket> packet = std::shared_ptr<BidCoSPacket>(), bool save = true);
        std::thread _dutyCycleThread;
        int32_t _dutyCycleCounter  = 0;
        bool _dutyCycleBroadcast = false;
        std::thread _sendDutyCyclePacketThread;
        void reset();

        int32_t getNextDutyCycleDeviceAddress();
        int64_t calculateLastDutyCycleEvent();
        int32_t getAdjustmentCommand(int32_t peerAddress);

        void sendRequestConfig(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet);
        void sendConfigParams(int32_t messageCounter, int32_t controlByte, std::shared_ptr<BidCoSPacket> packet);
        void sendDutyCycleBroadcast();
        void sendDutyCyclePacket(uint8_t messageCounter, int64_t sendingTime);
        void startDutyCycle(int64_t lastDutyCycleEvent);
        void dutyCycleThread(int64_t lastDutyCycleEvent);
        void setDecalcification();
        void setUpConfig();
};

#endif // HM_CC_TC_H
