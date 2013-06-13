#ifndef XMLRPCENCODER_H_
#define XMLRPCENCODER_H_

#include <iostream>
#include <memory>

#include "RPCVariable.h"

namespace RPC {

class XMLRPCEncoder
{
public:
	XMLRPCEncoder() {}
	virtual ~XMLRPCEncoder() {}

	uint32_t encodeResponse(std::shared_ptr<char>& packet, std::shared_ptr<RPCVariable> variable) { return 0; }
};

} /* namespace RPC */
#endif /* XMLRPCENCODER_H_ */
