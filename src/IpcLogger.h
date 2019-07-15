/* Copyright 2013-2019 Homegear GmbH
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

#ifndef HOMEGEAR_IPCLOGGER_H
#define HOMEGEAR_IPCLOGGER_H

#include <homegear-base/BaseLib.h>

namespace Homegear
{

enum class IpcModule
{
    scriptEngine = 1,
    nodeBlue = 2,
    ipc = 3
};

enum class IpcLoggerPacketDirection
{
    toServer,
    toClient
};

class IpcLogger
{
private:
    bool _enabled = false;
    std::mutex _outputMutex;
    std::ofstream _outputStream;
public:
    IpcLogger();
    ~IpcLogger();

    /**
     * Returns `true` when the logger is enabled. Don't check the settings as a change in them don't change the enabled state without Homegear restart.
     *
     * @return
     */
    bool enabled();

    /**
     * Writes an entry to the IPC log file.
     *
     * @param module The module the log entry is for.
     * @param packetId The packet ID as encoded in the IPC packet.
     * @param pid The process ID of the IPC client process.
     * @param direction Direction of the packet.
     * @param data The full binary RPC packet.
     */
    void log(IpcModule module, int32_t packetId, pid_t pid, IpcLoggerPacketDirection direction, const std::vector<char>& data);
};

}

#endif //HOMEGEAR_IPCLOGGER_H
