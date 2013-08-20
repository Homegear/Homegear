#ifndef RPCDECODER_H_
#define RPCDECODER_H_

#include <memory>
#include <vector>
#include <cstring>
#include <cmath>

#include "../Exception.h"
#include "RPCVariable.h"

namespace RPC
{

class RPCDecoder
{
public:
	RPCDecoder() {}
	virtual ~RPCDecoder() {}

	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeRequest(std::shared_ptr<std::vector<char>> packet, std::string& methodName);
	std::shared_ptr<RPCVariable> decodeResponse(std::shared_ptr<std::vector<char>> packet, uint32_t offset = 0);
private:
	int32_t decodeInteger(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	double decodeFloat(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	bool decodeBoolean(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	std::string decodeString(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	std::shared_ptr<RPCVariable> decodeParameter(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	RPCVariableType decodeType(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeArray(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
	std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> decodeStruct(std::shared_ptr<std::vector<char>>& packet, uint32_t& position);
};

} /* namespace RPC */
#endif /* RPCDECODER_H_ */
