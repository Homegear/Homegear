#ifndef BINARYENCODER_H_
#define BINARYENCODER_H_

#include <iostream>
#include <memory>
#include <cstring>
#include <vector>
#include <string>

#include "HelperFunctions.h"
#include "Exception.h"

class BinaryEncoder
{
public:
	void encodeInteger(std::shared_ptr<std::vector<char>>& encodedData, int32_t integer);
	void encodeInteger(std::vector<uint8_t>& encodedData, int32_t integer);
	void encodeByte(std::shared_ptr<std::vector<char>>& encodedData, uint8_t byte);
	void encodeByte(std::vector<uint8_t>& encodedData, uint8_t byte);
	void encodeString(std::shared_ptr<std::vector<char>>& packet, std::string& string);
	void encodeString(std::vector<uint8_t>& encodedData, std::string& string);
	void encodeBoolean(std::shared_ptr<std::vector<char>>& encodedData, bool boolean);
	void encodeBoolean(std::vector<uint8_t>& encodedData, bool boolean);
	void encodeFloat(std::shared_ptr<std::vector<char>>& encodedData, double floatValue);
	void encodeFloat(std::vector<uint8_t>& encodedData, double floatValue);
private:
};

#endif /* BINARYENCODER_H_ */
