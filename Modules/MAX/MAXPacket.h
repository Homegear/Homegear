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

#ifndef MAXPACKET_H_
#define MAXPACKET_H_

#include "../Base/BaseLib.h"

#include <map>

namespace MAX
{
class MAXPacket : public BaseLib::Systems::Packet
{
    public:
        //Properties
        MAXPacket();
        MAXPacket(std::vector<uint8_t>&, bool rssiByte, int64_t timeReceived = 0);
        MAXPacket(std::string packet, int64_t timeReceived = 0);
        virtual ~MAXPacket();

        uint8_t messageCounter() { return _messageCounter; }
        void setMessageCounter(uint8_t counter) { _messageCounter = counter; }
        uint8_t messageType() { return _messageType; }
        uint8_t messageSubtype() { return _messageSubtype; }
        uint8_t rssiDevice() { return _rssiDevice; }
        virtual std::string hexString();
        virtual std::vector<uint8_t> byteArray();

        bool equals(std::shared_ptr<MAXPacket>& rhs);
    protected:
        uint8_t _messageCounter = 0;
        uint8_t _messageType = 0;
        uint8_t _messageSubtype = 0;
        uint8_t _rssiDevice = 0;

        virtual void import(std::string& packet, bool removeFirstCharacter = true);
        virtual void import(std::vector<uint8_t>& packet, bool rssiByte);
        virtual uint8_t getByte(std::string);
        int32_t getInt(std::string);
};

}
#endif
