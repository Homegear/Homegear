/* Copyright 2013-2020 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#include "IpcLogger.h"
#include "GD/GD.h"

namespace Homegear
{

IpcLogger::IpcLogger()
{
    _enabled = GD::bl->settings.ipcLog();
    if(!_enabled) return;
    _outputStream.open(GD::bl->settings.logfilePath() + std::to_string(BaseLib::HelperFunctions::getTime()) + " homegear-socket.pcap", std::ios::app | std::ios::binary);
    std::vector<uint8_t> buffer{
        0xa1, // Magic number
        0xb2, // Magic number
        0xc3, // Magic number
        0xd4, // Magic number
        0, // Major version number (Version 2.4)
        2, // Major version number
        0, // Minor version number
        4, // Minor version number
        0, // Time zone correction
        0, // Time zone correction
        0, // Time zone correction
        0, // Time zone correction
        0, // Accuracy of timestamps
        0, // Accuracy of timestamps
        0, // Accuracy of timestamps
        0, // Accuracy of timestamps
        0x7F, // Max length of captured packets in octects
        0xFF, // Max length of captured packets in octects
        0xFF, // Max length of captured packets in octects
        0xFF, // Max length of captured packets in octects
        0, // Data link type
        0, // Data link type
        0, // Data link type
        0xe4 // Data link type (IPv4)
    };
    _outputStream.write((char*)buffer.data(), buffer.size());
}

IpcLogger::~IpcLogger()
{
    _outputStream.close();
}

bool IpcLogger::enabled()
{
    return _enabled;
}

void IpcLogger::log(IpcModule module, int32_t packetId, pid_t pid, IpcLoggerPacketDirection direction, const std::vector<char>& data)
{
    try
    {
        if(!_enabled) return;

        uint8_t ipcModule = (uint8_t)module;

        int64_t time = BaseLib::HelperFunctions::getTimeMicroseconds();
        int32_t timeSeconds = time / 1000000;
        int32_t timeMicroseconds = time % 1000000;

        uint32_t length = 20 + 8 + data.size();

        std::vector<uint8_t> buffer;
        buffer.reserve(length);
        buffer.push_back((uint8_t)(timeSeconds >> 24));
        buffer.push_back((uint8_t)(timeSeconds >> 16));
        buffer.push_back((uint8_t)(timeSeconds >> 8));
        buffer.push_back((uint8_t)timeSeconds);

        buffer.push_back((uint8_t)(timeMicroseconds >> 24));
        buffer.push_back((uint8_t)(timeMicroseconds >> 16));
        buffer.push_back((uint8_t)(timeMicroseconds >> 8));
        buffer.push_back((uint8_t)timeMicroseconds);

        buffer.push_back((uint8_t)(length >> 24)); //incl_len
        buffer.push_back((uint8_t)(length >> 16));
        buffer.push_back((uint8_t)(length >> 8));
        buffer.push_back((uint8_t)length);

        buffer.push_back((uint8_t)(length >> 24)); //orig_len
        buffer.push_back((uint8_t)(length >> 16));
        buffer.push_back((uint8_t)(length >> 8));
        buffer.push_back((uint8_t)length);

        //{{{ IPv4 header
        buffer.push_back(0x45); //Version 4 (0100....); Header length 20 (....0101)
        buffer.push_back(0); //Differentiated Services Field

        buffer.push_back((uint8_t)(length >> 8)); //Length
        buffer.push_back((uint8_t)length);

        buffer.push_back((uint8_t)((packetId % 65536) >> 8)); //Identification
        buffer.push_back((uint8_t)(packetId % 65536));

        buffer.push_back(0); //Flags: 0 (000.....); Fragment offset 0 (...00000 00000000)
        buffer.push_back(0);

        buffer.push_back(0x80); //TTL

        buffer.push_back(17); //Protocol UDP

        buffer.push_back(0); //Header checksum
        buffer.push_back(0);

        if(direction == IpcLoggerPacketDirection::toServer)
        {
            buffer.push_back(0x80 | ipcModule); //Source
            buffer.push_back(0);
            buffer.push_back((uint8_t)(pid >> 8));
            buffer.push_back((uint8_t)pid);

            buffer.push_back(ipcModule); //Destination
            buffer.push_back(ipcModule);
            buffer.push_back(ipcModule);
            buffer.push_back(ipcModule);
        }
        else
        {
            buffer.push_back(ipcModule); //Source
            buffer.push_back(ipcModule);
            buffer.push_back(ipcModule);
            buffer.push_back(ipcModule);

            buffer.push_back(0x80 | ipcModule); //Destination
            buffer.push_back(0);
            buffer.push_back((uint8_t)(pid >> 8));
            buffer.push_back((uint8_t)pid);
        }
        // }}}
        // {{{ UDP header
        buffer.push_back(0); //Source port
        buffer.push_back(ipcModule);

        buffer.push_back((uint8_t)(pid >> 8)); //Destination port
        buffer.push_back((uint8_t)pid);

        length -= 20;
        buffer.push_back((uint8_t)(length >> 8)); //Length
        buffer.push_back((uint8_t)length);

        buffer.push_back(0); //Checksum
        buffer.push_back(0);
        // }}}

        buffer.insert(buffer.end(), data.begin(), data.end());

        std::lock_guard<std::mutex> socketOutputGuard(_outputMutex);
        _outputStream.write((char*)buffer.data(), buffer.size());
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

}