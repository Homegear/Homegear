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

#ifndef PACKETMANAGER_H_
#define PACKETMANAGER_H_

#include "../Base/BaseLib.h"
#include "InsteonPacket.h"

#include <iostream>
#include <string>
#include <chrono>
#include <memory>
#include <unordered_map>
#include <thread>
#include <mutex>

namespace Insteon
{
class InsteonPacketInfo
{
public:
	InsteonPacketInfo();
	virtual ~InsteonPacketInfo() {}

	uint32_t id = 0;
	int64_t time;
	std::shared_ptr<InsteonPacket> packet;
};

class PacketManager
{
public:
	PacketManager();
	virtual ~PacketManager();

	std::shared_ptr<InsteonPacket> get(int32_t address);
	std::shared_ptr<InsteonPacketInfo> getInfo(int32_t address);
	bool set(int32_t address, std::shared_ptr<InsteonPacket>& packet, int64_t time = 0);
	void deletePacket(int32_t address, uint32_t id);
	void keepAlive(int32_t address);
	void dispose(bool wait = true);
protected:
	bool _disposing = false;
	bool _stopWorkerThread = false;
    std::thread _workerThread;
	uint32_t _id = 0;
	std::unordered_map<int32_t, std::shared_ptr<InsteonPacketInfo>> _packets;
	std::mutex _packetMutex;

	void worker();
};

}
#endif
