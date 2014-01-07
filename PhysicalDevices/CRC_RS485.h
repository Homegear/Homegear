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

#ifndef CRC_RS485_H
#define CRC_RS485_H

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>

#include "PhysicalDevice.h"

namespace PhysicalDevices
{

class CRCRS485  : public PhysicalDevice
{
    public:
        CRCRS485(std::shared_ptr<PhysicalDeviceSettings> settings);
        virtual ~CRCRS485();
        void startListening();
        void stopListening();
        void sendPacket(std::shared_ptr<Packet> packet);
        bool isOpen() { return _fileDescriptor > -1; }
    protected:
        int32_t _fileDescriptor = -1;

        void openDevice();
        void closeDevice();
        void setupDevice();
        void writeToDevice(std::string, bool);
        std::vector<uint8_t> readFromDevice();
        void listen();
    private:
};

}
#endif // CRC_RS485_H
