/*
   base64.cpp and base64.h

   Copyright (C) 2004-2008 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#ifndef BASE64_H_
#define BASE64_H_

#include <string>
#include <vector>

namespace BaseLib
{

class Base64 {
public:
	virtual ~Base64() {}

	/**
	 * Encodes a string to Base64.
	 *
	 * @param[in] in The string to encode.
	 * @param[out] out The string the result is stored in.
	 */
	static void encode(const std::string& in, std::string& out);

	/**
	 * Encodes a char vector to Base64.
	 *
	 * @param[in] in The binary data to encode.
	 * @param[out] out The string the result is stored in.
	 */
	static void encode(const std::vector<char>& in, std::string& out);

	/**
	 * Decodes a Base64 encoded string.
	 *
	 * @param[in] in The data to decode.
	 * @param[out] out The string the result is stored in.
	 */
	static void decode(const std::string& in, std::string& out);

	/**
	 * Decodes Base64 encoded binary data.
	 *
	 * @param[in] in The data to decode.
	 * @param[out] out The char vector the result is stored in.
	 */
	static void decode(const std::string& in, std::vector<char>& out);
private:
	Base64() {}

	/**
	 * Checks whether a character is Base64 decodeable.
	 *
	 * @param c The character to check.
	 * @return Returns "true" when the character is decodeable, otherwise "false".
	 */
	static inline bool isBase64(unsigned char c) {
		return (isalnum(c) || (c == '+') || (c == '/'));
	}
};

}
#endif
