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

class HelperFunctions {
public:
	virtual ~HelperFunctions();

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

	static int32_t getNumber(std::string &s)
	{
		int32_t xpos = s.find('x');
		int32_t number = 0;
		if(xpos == -1) try { number = std::stoll(s, 0, 10); } catch(...) {}
		else if(xpos == 1) try { number = std::stoll(s, 0, 16); } catch(...) {}
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

	static void memcpyBigEndian(char* to, char* from, uint32_t length);
private:
	//Non public constructor
	HelperFunctions();
};

#endif /* HELPERFUNCTIONS_H_ */
