#ifndef BINARYDECODER_H_
#define BINARYDECODER_H_

#include <iostream>
#include <memory>
#include <cstring>
#include <vector>
#include <string>

#include "HelperFunctions.h"
#include "Exception.h"

class BinaryDecoder
{
public:
	int32_t decodeInteger(std::shared_ptr<std::vector<char>>& encodedData, uint32_t& position);
	uint8_t decodeByte(std::shared_ptr<std::vector<char>>& encodedData, uint32_t& position);
	std::string decodeString(std::shared_ptr<std::vector<char>>& encodedData, uint32_t& position);
	bool decodeBoolean(std::shared_ptr<std::vector<char>>& encodedData, uint32_t& position);
	double decodeFloat(std::shared_ptr<std::vector<char>>& encodedData, uint32_t& position);
};

#endif /* BINARYDECODER_H_ */
