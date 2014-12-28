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

#ifndef BIDCOSPACKET_H
#define BIDCOSPACKET_H

#include "../Base/BaseLib.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace BidCoS
{

class BidCoSPacket : public BaseLib::Systems::Packet
{
    public:
        //Properties
        uint8_t messageCounter() { return _messageCounter; }
        void setMessageCounter(uint8_t counter) { _messageCounter = counter; }
        uint8_t messageType() { return _messageType; }
        uint8_t rssiDevice() { return _rssiDevice; }
        bool isUpdatePacket() { return _updatePacket; }
        virtual void setControlByte(uint8_t value) { _controlByte = value; }
        virtual std::string hexString();
        virtual std::vector<uint8_t> byteArray();
        virtual std::vector<char> byteArraySigned();

        BidCoSPacket();
        BidCoSPacket(std::string& packet, int64_t timeReceived = 0);
        BidCoSPacket(std::vector<uint8_t>& packet, bool rssiByte, int64_t timeReceived = 0);
        BidCoSPacket(uint8_t messageCounter, uint8_t controlByte, uint8_t messageType, int32_t senderAddress, int32_t destinationAddress, std::vector<uint8_t> payload, bool updatePacket = false);
        virtual ~BidCoSPacket();
        void import(std::string& packet, bool removeFirstCharacter = true);
        void import(std::vector<uint8_t>& packet, bool rssiByte);
        virtual std::vector<uint8_t> getPosition(double index, double size, int32_t mask);
        virtual void setPosition(double index, double size, std::vector<uint8_t>& value);

        bool equals(std::shared_ptr<BidCoSPacket>& rhs);
    protected:
    private:
        uint8_t _messageCounter = 0;
        uint8_t _messageType = 0;
        uint8_t _rssiDevice = 0;
        bool _updatePacket = false;

        uint8_t getByte(std::string);
        int32_t getInt(std::string);
};

}
#endif // BIDCOSPACKET_H
