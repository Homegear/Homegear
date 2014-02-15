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

#ifndef HMWIREDPEER_H_
#define HMWIREDPEER_H_

#include "../General/Peer.h"
#include "../General/ServiceMessages.h"

namespace HMWired
{
class PendingHMWiredQueues;
class CallbackFunctionParameter;

class VariableToReset
{
public:
	uint32_t channel = 0;
	std::string key;
	std::vector<uint8_t> data;
	int64_t resetTime = 0;
	bool isDominoEvent = false;

	VariableToReset() {}
	virtual ~VariableToReset() {}
};

class HMWiredPeer : public Peer
{
public:
	HMWiredPeer(uint32_t parentID, bool centralFeatures);
	HMWiredPeer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures);
	virtual ~HMWiredPeer();

	std::mutex _variablesToResetMutex;
	std::vector<std::shared_ptr<VariableToReset>> _variablesToReset;

	std::shared_ptr<ServiceMessages> serviceMessages;

	std::shared_ptr<PendingHMWiredQueues> pendingHMWiredQueues;

	void addVariableToResetCallback(std::shared_ptr<CallbackFunctionParameter> parameters);
};

} /* namespace HMWired */

#endif /* HMWIREDPEER_H_ */
