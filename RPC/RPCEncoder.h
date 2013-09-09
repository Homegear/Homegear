#ifndef RPCENCODER_H_
#define RPCENCODER_H_

#include <memory>
#include <cstring>
#include <list>

#include "RPCVariable.h"
#include "../BinaryEncoder.h"

namespace RPC
{

class RPCEncoder
{
public:
	RPCEncoder();
	virtual ~RPCEncoder() {}

	std::shared_ptr<std::vector<char>> encodeRequest(std::string methodName, std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters);
	std::shared_ptr<std::vector<char>> encodeResponse(std::shared_ptr<RPCVariable> variable);
private:
	BinaryEncoder _encoder;
	char _packetStartRequest[4];
	char _packetStartResponse[4];
	char _packetStartError[4];

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
