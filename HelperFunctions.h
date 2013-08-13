/*
 * HelperFunctions.h
 *
 *  Created on: May 23, 2013
 *      Author: sathya
 */

#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <iostream>
#include <string>
#include <memory>
#include <random>
#include <chrono>

class HelperFunctions {
public:
	virtual ~HelperFunctions();

	static inline int64_t getTime()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	}

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

	static inline std::string &trim(std::string &s)
	{
			return ltrim(rtrim(s));
	}

	static inline std::string &toLower (std::string &s)
	{
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}

	static std::pair<std::string, std::string> split(std::string string, char delimiter);

	static inline double getDouble(std::string &s)
	{
		double number = 0;
		try { number = std::stod(s); } catch(...) {}
		return number;
	}

	static bool isNumber(std::string &s)
	{
		int32_t xpos = s.find('x');
		if(xpos == -1) try { std::stoll(s, 0, 10); } catch(...) { return false; }
		else try { std::stoll(s, 0, 16); } catch(...) { return false; }
		return true;
	}

	static int32_t getNumber(std::string &s)
	{
		int32_t xpos = s.find('x');
		int32_t number = 0;
		if(xpos == -1) try { number = std::stoll(s, 0, 10); } catch(...) {}
		else try { number = std::stoll(s, 0, 16); } catch(...) {}
		return number;
	}

	static bool isBigEndian()
	{
		union {
			uint32_t i;
			char c[4];
		} bint = {0x01020304};

		return bint.c[0] == 1;
	}

	static int32_t getRandomNumber(int32_t min, int32_t max);

	static void memcpyBigEndian(char* to, char* from, const uint32_t& length);
	static void memcpyBigEndian(uint8_t* to, uint8_t* from, const uint32_t& length);
	static void memcpyBigEndian(int32_t& to, std::vector<uint8_t>& from);
	static void memcpyBigEndian(std::vector<uint8_t>& to, int32_t& from);
	static void printBinary(std::shared_ptr<std::vector<char>> data);
	static std::string getHexString(const std::vector<uint8_t>& data);
private:
	//Non public constructor
	HelperFunctions();
};

#endif /* HELPERFUNCTIONS_H_ */
