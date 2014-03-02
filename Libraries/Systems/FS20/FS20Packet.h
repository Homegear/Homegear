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

#ifndef FS20PACKET_H_
#define FS20PACKET_H_

#include "../../Types/Packet.h"
#include "../../HelperFunctions/HelperFunctions.h"

#include <map>

namespace FS20
{
class FS20Packet : public Packet
{
    public:
        //Properties
        FS20Packet();
        FS20Packet(std::string packet, int64_t timeReceived = 0);
        FS20Packet(std::vector<uint8_t>& packet, int64_t timeReceived = 0);
        virtual ~FS20Packet();

        virtual std::string hexString();
        virtual std::vector<uint8_t> byteArray();

        void import(std::vector<uint8_t>& packet);
        void import(std::string packetHex, bool removeFirstCharacter = true);
    private:
        std::vector<uint8_t> _packet;

        int32_t _houseCode = 0;
        uint8_t _address = 0;
        uint8_t _command = 0;
        uint8_t _rssiDevice = 0;
};

} /* namespace FS20 */
#endif /* FS20PACKET_H_ */
