/* Copyright 2013-2015 Sathya Laufer
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

#ifndef INSTEONPACKET_H_
#define INSTEONPACKET_H_

#include "../Base/BaseLib.h"
#include "PhysicalInterfaces/IInsteonInterface.h"

#include <map>

namespace Insteon
{

enum class InsteonPacketFlags : uint32_t
{
	Direct = 				0,
	DirectAck = 			1,
	GroupCleanupDirect = 	2,
	GroupCleanupDirectAck = 3,
	Broadcast =				4,
	DirectNak =				5,
	GroupBroadcast =		6,
	GroupCleanupDirectNak = 7
};

class InsteonPacket : public BaseLib::Systems::Packet
{
    public:
        //Properties
        InsteonPacket();
        InsteonPacket(std::string packet, std::string interfaceID = "", int64_t timeReceived = 0);
        InsteonPacket(std::vector<char>& packet, std::string interfaceID = "", int64_t timeReceived = 0);
        InsteonPacket(std::vector<uint8_t>& packet, std::string interfaceID = "", int64_t timeReceived = 0);
        InsteonPacket(uint8_t messageType, uint8_t messageSubtype, int32_t destinationAddress, uint8_t hopsLeft, uint8_t hopsMax, InsteonPacketFlags flags, std::vector<uint8_t> payload);
        virtual ~InsteonPacket();

        void import(std::vector<char>& packet);
        void import(std::vector<uint8_t>& packet);
        void import(std::string packetHex);

        std::string interfaceID() { return _interfaceID; }
        void setInterfaceID(std::string value) { _interfaceID = value; }
        void setDestinationAddress(int32_t value) { _destinationAddress = value; }
        bool extended() { return _extended; }
        InsteonPacketFlags flags() { return _flags; }
        void setFlags(InsteonPacketFlags value) { _flags = value; }
        uint8_t hopsLeft() { return _hopsLeft; }
        void setHopsLeft(uint8_t value) { _hopsLeft = value; }
        uint8_t hopsMax() { return _hopsMax; }
        void setHopsMax(uint8_t value) { _hopsMax = value; }
        uint8_t messageType() { return _messageType; }
        void setMessageType(uint8_t type) { _messageType = type; }
        uint8_t messageSubtype() { return _messageSubtype; }
        virtual std::string hexString();
        virtual std::vector<char> byteArray();

        virtual std::vector<uint8_t> getPosition(double index, double size, int32_t mask);
        virtual void setPosition(double index, double size, std::vector<uint8_t>& value);

        bool equals(std::shared_ptr<InsteonPacket>& rhs);
    protected:
        std::string _interfaceID;
        bool _extended = false;
        InsteonPacketFlags _flags = InsteonPacketFlags::Direct;
        uint8_t _hopsLeft = 0;
        uint8_t _hopsMax = 0;
        uint8_t _messageType = 0;
        uint8_t _messageSubtype = 0;

        virtual uint8_t getByte(std::string);
        int32_t getInt(std::string);
        void calculateChecksum();
};

}
#endif
