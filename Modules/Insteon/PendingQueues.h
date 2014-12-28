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

#ifndef PENDINGQUEUESINSTEON_H_
#define PENDINGQUEUESINSTEON_H_

#include "PacketQueue.h"
#include "InsteonPeer.h"

#include <string>
#include <iostream>
#include <memory>
#include <queue>
#include <mutex>

namespace Insteon
{
class InsteonDevice;

class PendingQueues {
public:
	PendingQueues();
	virtual ~PendingQueues() {}
	void serialize(std::vector<uint8_t>& encodedData);
	void unserialize(std::shared_ptr<std::vector<char>> serializedData, InsteonPeer* peer, InsteonDevice* device);

	void push(std::shared_ptr<PacketQueue> queue);
	void pop();
	void pop(uint32_t id);
	bool empty();
	uint32_t size();
	std::shared_ptr<PacketQueue> front();
	void clear();
	void remove(std::string value, int32_t channel);
	bool exists(std::string value, int32_t channel);
	bool find(PacketQueueType queueType);

	void getInfoString(std::ostringstream& stringStream);
private:
	uint32_t _currentID = 0;
	std::mutex _queuesMutex;
    std::deque<std::shared_ptr<PacketQueue>> _queues;
};
}
#endif
