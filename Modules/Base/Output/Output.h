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

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "../Exception.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <map>
#include <chrono>
#include <ctime>

namespace BaseLib
{
class Obj;

class Output
{
public:
	Output() {}
	virtual ~Output();

	void init(Obj* baseLib);

	void setPrefix(std::string prefix) { _prefix = prefix; }

	void printThreadPriority();

	std::string getTimeString(int64_t time = 0);

	void printBinary(std::vector<unsigned char>& data);
	void printBinary(std::shared_ptr<std::vector<char>> data);
	void printBinary(std::vector<char>& data);
	void printEx(std::string file, uint32_t line, std::string function, std::string what = "");
	void printCritical(std::string errorString);
	void printError(std::string errorString);
	void printWarning(std::string errorString);
	void printInfo(std::string message);
	void printDebug(std::string message, int32_t minDebugLevel = 5);
	void printMessage(std::string message, int32_t minDebugLevel = 0);
private:
	BaseLib::Obj* _bl = nullptr;
	const int32_t _defaultDebugLevel = 0;
	std::string _prefix;
};
}
#endif /* OUTPUT_H_ */
