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

#ifndef BIDCOSMESSAGE_H
#define BIDCOSMESSAGE_H

#include <iostream>
#include <vector>
#include <functional>
#include <map>
#include <memory>

#include "BidCoSPacket.h"
#include "HomeMaticDevice.h"

namespace BidCoS
{
enum MessageAccess { NOACCESS = 0x00, ACCESSPAIREDTOSENDER = 0x01, ACCESSDESTISME = 0x02, ACCESSCENTRAL = 0x04, ACCESSUNPAIRING = 0x08, FULLACCESS = 0x80 };
enum MessageDirection { DIRECTIONIN, DIRECTIONOUT };

class BidCoSMessage
{
    public:
        BidCoSMessage();
        /** Default constructor */
        BidCoSMessage(int32_t messageType, HomeMaticDevice* device, int32_t access, void (HomeMaticDevice::*messageHandlerIncoming)(int32_t, std::shared_ptr<BidCoSPacket>));
        BidCoSMessage(int32_t messageType, HomeMaticDevice* device, int32_t access, int32_t accessPairing, void (HomeMaticDevice::*messageHandlerIncoming)(int32_t, std::shared_ptr<BidCoSPacket>));
        BidCoSMessage(int32_t messageType, int32_t controlByte, HomeMaticDevice* device, void (HomeMaticDevice::*messageHandlerOutgoing)(int32_t, int32_t, std::shared_ptr<BidCoSPacket>));
        /** Default destructor */
        virtual ~BidCoSMessage();

        MessageDirection getDirection() { return _direction; }
        void setDirection(MessageDirection direction) { _direction = direction; }
        int32_t getControlByte() { return _controlByte; }
        void setControlByte(int32_t controlByte) { _controlByte = controlByte; }
        int32_t getMessageType() { return _messageType; }
        void setMessageType(int32_t messageType) { _messageType = messageType; }
        int32_t getMessageAccess() { return _access; }
        void setMessageAccess(int32_t access) { _access = access; }
        int32_t getMessageAccessPairing() { return _accessPairing; }
        void setMessageAccessPairing(int32_t accessPairing) { _accessPairing = accessPairing; }
        HomeMaticDevice* getDevice() { return _device; }
        void invokeMessageHandlerIncoming(std::shared_ptr<BidCoSPacket> packet);
        void invokeMessageHandlerOutgoing(std::shared_ptr<BidCoSPacket> packet);
        bool checkAccess(std::shared_ptr<BidCoSPacket> packet, std::shared_ptr<BidCoSQueue> queue);
        std::vector<std::pair<uint32_t, int32_t>>* getSubtypes() { return &_subtypes; }
        void addSubtype(int32_t subtypePosition, int32_t subtype) { _subtypes.push_back(std::pair<uint32_t, int32_t>(subtypePosition, subtype)); };
        uint32_t subtypeCount() { return _subtypes.size(); }
        void setMessageCounter(std::shared_ptr<BidCoSPacket> packet);
        bool typeIsEqual(std::shared_ptr<BidCoSPacket> packet);
        bool typeIsEqual(std::shared_ptr<BidCoSMessage> message, std::shared_ptr<BidCoSPacket> packet);
        bool typeIsEqual(std::shared_ptr<BidCoSMessage> message);
        bool typeIsEqual(int32_t messageType, std::vector<std::pair<uint32_t, int32_t>>* subtypes);
    protected:
        MessageDirection _direction = DIRECTIONIN;
        int32_t _messageType = -1;
        int32_t _controlByte = 0;
        HomeMaticDevice* _device = nullptr;
        int32_t _access = 0;
        int32_t _accessPairing = 0;
        std::vector<std::pair<uint32_t, int32_t>> _subtypes;
        void (HomeMaticDevice::*_messageHandlerIncoming)(int32_t, std::shared_ptr<BidCoSPacket>) = nullptr;
        void (HomeMaticDevice::*_messageHandlerOutgoing)(int32_t, int32_t, std::shared_ptr<BidCoSPacket>) = nullptr;
    private:
};
}
#endif // BIDCOSMESSAGE_H
