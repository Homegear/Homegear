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

#ifndef SERVICEMESSAGES_H_
#define SERVICEMESSAGES_H_

class Peer;

#include <string>
#include <iomanip>
#include <memory>
#include <chrono>
#include <map>
#include <mutex>

#include "Libraries/Types/RPCVariable.h"

class ServiceMessages {
public:
	void setPeer(Peer* peer) { _peer = peer; }

	ServiceMessages(Peer* peer) { _peer = peer; }
	virtual ~ServiceMessages();

	void serialize(std::vector<uint8_t>& encodedData);
	void unserialize(std::shared_ptr<std::vector<char>> serializedData);
	void unserialize_0_0_6(std::string serializedObject);
	bool set(std::string id, bool value);
	void set(std::string id, uint8_t value, uint32_t channel);
	std::shared_ptr<RPC::RPCVariable> get();
	void dispose();

	void setConfigPending(bool value);
	bool getConfigPending() { return _configPending; }

	void setUnreach(bool value);
	bool getUnreach() { return _unreach; }
	void checkUnreach();
    void endUnreach();
private:
    bool _disposing = false;
    bool _configPending = false;
    int32_t _unreachResendCounter = 0;
    bool _unreach = false;
	bool _stickyUnreach = false;
	bool _lowbat = false;

	std::mutex _errorMutex;
	std::map<uint32_t, std::map<std::string, uint8_t>> _errors;

	std::mutex _peerMutex;
	Peer* _peer = nullptr;
};

#endif /* SERVICEMESSAGES_H_ */
