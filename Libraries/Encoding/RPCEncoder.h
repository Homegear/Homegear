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

#ifndef RPCENCODER_H_
#define RPCENCODER_H_

#include "RPCHeader.h"
#include "../Types/RPCVariable.h"
#include "BinaryEncoder.h"

#include <memory>
#include <cstring>
#include <list>

namespace RPC
{

class RPCEncoder
{
public:
	RPCEncoder();
	virtual ~RPCEncoder() {}

	void insertHeader(std::shared_ptr<std::vector<char>> packet, std::shared_ptr<RPCHeader> header);
	std::shared_ptr<std::vector<char>> encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters, std::shared_ptr<RPCHeader> header = std::shared_ptr<RPCHeader>());
	std::shared_ptr<std::vector<char>> encodeResponse(std::shared_ptr<RPCVariable> variable);
private:
	BinaryEncoder _encoder;
	char _packetStartRequest[4];
	char _packetStartResponse[4];
	char _packetStartError[4];

	uint32_t encodeHeader(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCHeader>& header);
	void encodeVariable(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeInteger(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeFloat(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeBoolean(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeType(std::shared_ptr<std::vector<char>>& packet, RPCVariableType type);
	void encodeString(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeBase64(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeVoid(std::shared_ptr<std::vector<char>>& packet);
	void encodeStruct(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeArray(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
};

} /* namespace RPC */
#endif /* RPCENCODER_H_ */
