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

#ifndef PACKET_H_
#define PACKET_H_

#include <string>
#include <vector>

namespace BaseLib
{
namespace Systems
{

class Packet
{
public:
	Packet();
	virtual ~Packet();

	virtual uint8_t length() { return _length; }
	virtual uint8_t controlByte() { return _controlByte; }
	virtual int32_t senderAddress() { return _senderAddress; }
	virtual int32_t destinationAddress() { return _destinationAddress; }
	virtual std::vector<uint8_t>* payload() { return &_payload; }
	virtual std::string hexString() { return ""; }
	//virtual std::vector<uint8_t> byteArray() { return std::vector<uint8_t>(); }
	virtual int64_t timeReceived() { return _timeReceived; }
	virtual int64_t timeSending() { return _timeSending; }
	virtual void setTimeReceived(int64_t time) { _timeReceived = time; }
	virtual void setTimeSending(int64_t time) { _timeSending = time; }
	virtual std::vector<uint8_t> getPosition(double index, double size, int32_t mask) { return std::vector<uint8_t>(); }
	virtual void setPosition(double index, double size, std::vector<uint8_t>& value) {}
protected:
	uint8_t _length = 0;
    uint8_t _controlByte = 0;
    int32_t _senderAddress = 0;
    int32_t _destinationAddress = 0;
    std::vector<uint8_t> _payload;
    uint32_t _bitmask[9] = {0xFF, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
	int64_t _timeReceived = 0;
    int64_t _timeSending = 0;

};

}
}

#endif /* PACKET_H_ */
