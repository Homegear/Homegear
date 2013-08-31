#ifndef RPCENCODER_H_
#define RPCENCODER_H_

#include <memory>
#include <cstring>

#include "RPCVariable.h"
#include "../BinaryEncoder.h"

namespace RPC
{

class RPCEncoder
{
public:
	RPCEncoder() { _packetStart[0] = 'B'; _packetStart[1] = 'i'; _packetStart[2] = 'n'; _packetStart[3] = 1; _packetStartError[0] = 'B'; _packetStartError[1] = 'i'; _packetStartError[2] = 'n'; _packetStartError[3] = 0xFF; }
	virtual ~RPCEncoder() {}

	std::shared_ptr<std::vector<char>> encodeResponse(std::shared_ptr<RPCVariable> variable);
private:
	BinaryEncoder _encoder;
	char _packetStart[4];
	char _packetStartError[4];

	void encodeVariable(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeInteger(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeFloat(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeBoolean(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeType(std::shared_ptr<std::vector<char>>& packet, RPCVariableType type);
	void encodeString(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeVoid(std::shared_ptr<std::vector<char>>& packet);
	void encodeStruct(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
	void encodeArray(std::shared_ptr<std::vector<char>>& packet, std::shared_ptr<RPCVariable>& variable);
};

} /* namespace RPC */
#endif /* RPCENCODER_H_ */
