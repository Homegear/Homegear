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

#ifndef RPCENCODER_H_
#define RPCENCODER_H_

#include "RPCHeader.h"
#include "../Variable.h"
#include "BinaryEncoder.h"

#include <memory>
#include <cstring>
#include <list>

namespace BaseLib
{

class Obj;

namespace RPC
{

class RPCEncoder
{
public:
	RPCEncoder(BaseLib::Obj* baseLib);
	virtual ~RPCEncoder() {}

	virtual void insertHeader(std::vector<char>& packet, const RPCHeader& header);
	virtual void insertHeader(std::vector<uint8_t>& packet, const RPCHeader& header);
	virtual void encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<Variable>>> parameters, std::vector<char>& encodedData, std::shared_ptr<RPCHeader> header = nullptr);
	virtual void encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<Variable>>> parameters, std::vector<uint8_t>& encodedData, std::shared_ptr<RPCHeader> header = nullptr);
	virtual void encodeResponse(std::shared_ptr<Variable> variable, std::vector<char>& encodedData);
	virtual void encodeResponse(std::shared_ptr<Variable> variable, std::vector<uint8_t>& encodedData);
private:
	BaseLib::Obj* _bl = nullptr;
	std::unique_ptr<BinaryEncoder> _encoder;
	char _packetStartRequest[4];
	char _packetStartResponse[5];
	char _packetStartError[5];

	uint32_t encodeHeader(std::vector<char>& packet, const RPCHeader& header);
	uint32_t encodeHeader(std::vector<uint8_t>& packet, const RPCHeader& header);
	void encodeVariable(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeVariable(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeInteger(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeInteger(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeFloat(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeFloat(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeBoolean(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeBoolean(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeType(std::vector<char>& packet, VariableType type);
	void encodeType(std::vector<uint8_t>& packet, VariableType type);
	void encodeString(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeString(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeBase64(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeBase64(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeVoid(std::vector<char>& packet);
	void encodeVoid(std::vector<uint8_t>& packet);
	void encodeStruct(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeStruct(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
	void encodeArray(std::vector<char>& packet, std::shared_ptr<Variable>& variable);
	void encodeArray(std::vector<uint8_t>& packet, std::shared_ptr<Variable>& variable);
};
}
}
#endif
