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

#ifndef RS485_H
#define RS485_H

#include "IHMWiredInterface.h"

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

namespace HMWired
{

class RS485 : public IHMWiredInterface
{
    public:
        RS485(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings);
        virtual ~RS485();
        void startListening();
        void stopListening();
        void sendPacket(std::vector<uint8_t>& rawPacket);
        void sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet);
        int64_t lastAction() { return _lastAction; }
        virtual void setup(int32_t userID, int32_t groupID);
        virtual void search(std::vector<int32_t>& foundDevices);
    protected:
        bool _searchMode = false;
        int64_t _searchResponse = 0;
        uint8_t _firstByte = 0;
        int64_t _lastAction = 0;
        bool _sending = false;
        bool _receivingSending = false;
        std::vector<uint8_t> _receivedSentPacket;
        std::timed_mutex _sendingMutex;

        void openDevice();
        void closeDevice();
        void setupDevice();
        void writeToDevice(std::vector<uint8_t>& packet, bool printPacket);
        std::vector<uint8_t> readFromDevice();
        void listen();
    private:
        struct termios _termios;
};

}
#endif
