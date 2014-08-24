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

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include "../Systems/DeviceFamilies.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <fstream>
#include <sstream>
#include <mutex>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <gcrypt.h>

namespace BaseLib
{
class Obj;

class HelperFunctions
{
public:
	HelperFunctions();
	virtual ~HelperFunctions();

	void init(Obj* baseLib);

	static bool fileExists(std::string filename);
	static std::string getFileContent(std::string filename);
	static std::vector<char> getBinaryFileContent(std::string filename);
	static std::vector<uint8_t> getUBinaryFileContent(std::string filename);
	std::vector<std::string> getFiles(std::string path);

	static int64_t getTime();
	static int32_t getTimeSeconds();
	static int32_t getFileLastModifiedTime(const std::string& filename);
	static std::string getTimeString(int64_t time = 0);

	static inline std::string &ltrim(std::string &s)
	{
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
			return s;
	}

	static inline std::string &rtrim(std::string &s)
	{
			s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
			return s;
	}

	static inline std::string& trim(std::string& s)
	{
			return ltrim(rtrim(s));
	}

	static inline std::string& toLower (std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}

	static inline std::string& toUpper (std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		return s;
	}

	static inline std::string& stringReplace(std::string& haystack, std::string search, std::string replace)
	{
		int32_t pos = 0;
		while(true)
		{
			 pos = haystack.find(search, pos);
			 if (pos == std::string::npos) break;
			 haystack.replace(pos, search.size(), replace);
			 pos += search.size();
		}
		return haystack;
	}

	static std::pair<std::string, std::string> split(std::string string, char delimiter);
	static std::vector<std::string> splitAll(std::string, char delimiter);

	static inline double getDouble(std::string &s)
	{
		double number = 0;
		try { number = std::stod(s); } catch(...) {}
		return number;
	}

	static inline bool isNotAlphaNumeric(char c)
	{
		return !(isalpha(c) || isdigit(c) || (c == '_') || (c == '-'));
	}

	static bool isAlphaNumeric(std::string& s)
	{
		return find_if
		(
			s.begin(),
			s.end(),
			[](const char c){ return !(isalpha(c) || isdigit(c) || (c == '_') || (c == '-')); }
		) == s.end();
	}

	static bool isNumber(std::string& s)
	{
		int32_t xpos = s.find('x');
		if(xpos == -1) try { std::stoll(s, 0, 10); } catch(...) { return false; }
		else try { std::stoll(s, 0, 16); } catch(...) { return false; }
		return true;
	}

	static int32_t getNumber(std::string& s, bool isHex = false)
	{
		int32_t xpos = s.find('x');
		int32_t number = 0;
		if(xpos == -1 && !isHex) try { number = std::stoll(s, 0, 10); } catch(...) {}
		else try { number = std::stoll(s, 0, 16); } catch(...) {}
		return number;
	}

	int32_t getNumber(char hexChar)
	{
		if(_hexMap.find(hexChar) == _hexMap.end()) return 0;
		return _hexMap.at(hexChar);
	}

	static uint32_t getUnsignedNumber(std::string &s, bool isHex = false)
	{
		int32_t xpos = s.find('x');
		uint32_t number = 0;
		if(xpos == -1 && !isHex) try { number = std::stoull(s, 0, 10); } catch(...) {}
		else try { number = std::stoull(s, 0, 16); } catch(...) {}
		return number;
	}

	bool getBigEndian() { return _isBigEndian; }

	void copyFile(std::string source, std::string dest);
	static int32_t getRandomNumber(int32_t min, int32_t max);

	void memcpyBigEndian(char* to, char* from, const uint32_t& length);
	void memcpyBigEndian(uint8_t* to, uint8_t* from, const uint32_t& length);
	void memcpyBigEndian(int32_t& to, std::vector<uint8_t>& from);
	void memcpyBigEndian(std::vector<uint8_t>& to, int32_t& from);
	static std::string getHexString(const std::vector<char>& data);
	static std::string getHexString(const std::vector<uint8_t>& data);
	static std::string getHexString(int32_t number, int32_t width = -1);
	char getHexChar(int32_t nibble);
	std::vector<char> getBinary(std::string hexString);
	std::vector<uint8_t> getUBinary(std::string hexString);
	std::vector<uint8_t>& getUBinary(std::string hexString, uint32_t size, std::vector<uint8_t>& binary);
	std::vector<uint8_t> getUBinary(std::vector<uint8_t>& hexData);
	int32_t userID(std::string username);
	int32_t groupID(std::string groupname);
	std::string getGCRYPTError(int32_t errorCode);
	static std::string getGNUTLSCertVerificationError(uint32_t errorCode);
private:
	BaseLib::Obj* _bl = nullptr;
	bool _isBigEndian;
	std::map<char, int32_t> _hexMap;
	int32_t _asciiToBinaryTable[23];
	char _binaryToASCIITable[16];
	char _gcryptBuffer[1024];
	std::mutex _gcryptBufferMutex;

	void checkEndianness();
};
}
#endif /* HELPERFUNCTIONS_H_ */
