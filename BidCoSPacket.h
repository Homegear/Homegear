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

#ifndef BIDCOSPACKET_H
#define BIDCOSPACKET_H

#include "Exception.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

class BidCoSPacket
{
    public:
        //Properties
        uint8_t length();
        uint8_t messageCounter();
        void setMessageCounter(uint8_t counter) { _messageCounter = counter; }
        uint8_t controlByte();
        uint8_t messageType();
        int32_t senderAddress();
        int32_t destinationAddress();
        uint8_t rssi() { return _rssi; }
        std::vector<uint8_t>* payload();
        std::string hexString();
        std::vector<uint8_t> byteArray();
        int64_t timeReceived() { return _timeReceived; }
        int64_t timeSending() { return _timeSending; }
        void setTimeSending(int64_t time) { _timeSending = time; }

        /** Default constructor */
        BidCoSPacket();
        BidCoSPacket(std::string&, int64_t timeReceived = 0);
        BidCoSPacket(std::vector<uint8_t>&, bool rssiByte, int64_t timeReceived = 0);
        BidCoSPacket(uint8_t, uint8_t, uint8_t, int32_t, int32_t, std::vector<uint8_t>);
        /** Default destructor */
        virtual ~BidCoSPacket();
        void import(std::string&, bool removeFirstCharacter = true);
        void import(std::vector<uint8_t>&, bool rssiByte);
        std::vector<uint8_t> getPosition(double index, double size, int32_t mask);
        void setPosition(double index, double size, std::vector<uint8_t>& value);
    protected:
    private:
        uint8_t _length = 0;
        uint8_t _messageCounter = 0;
        uint8_t _controlByte = 0;
        uint8_t _messageType = 0;
        uint32_t _senderAddress = 0;
        uint32_t _destinationAddress = 0;
        std::vector<uint8_t> _payload;
        uint32_t _bitmask[8] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F};
        uint8_t _rssi = 0;
        int64_t _timeReceived = 0;
        int64_t _timeSending = 0;

        uint8_t getByte(std::string);
        int32_t getInt(std::string);
};

#endif // BIDCOSPACKET_H
