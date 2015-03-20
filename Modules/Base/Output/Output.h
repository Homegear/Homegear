/* Copyright 2013-2015 Sathya Laufer
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
#include <functional>

namespace BaseLib
{
class Obj;

/**
 * Class to print output of different kinds to the standard and error output.
 * The output is automatically prefixed with the date and filtered according to the current debug level.
 */
class Output
{
public:
	/**
	 * The main constructor.
	 * The constructor does nothing. You need to call "init" after creating the object.
	 */
	Output();

	/**
	 * The destructor.
	 * It does nothing.
	 */
	virtual ~Output();

	/**
	 * Initializes the object.
	 * Not calling this method might cause segmentation faults as the base library pointer is unset.
	 * @param baseLib A pointer to the common base library object.
	 * @param errorCallback
	 */
	void init(Obj* baseLib);

	/**
	 * Returns the prefix previously defined with setPrefix.
	 * @see setPrefix()
	 * @return Returns the prefix previously defined with setPrefix.
	 */
	std::string getPrefix() { return _prefix; }

	/**
	 * Sets a string, which will be used to prefix all output.
	 * @see getPrefix()
	 * @param prefix The new prefix.
	 */
	void setPrefix(std::string prefix) { _prefix = prefix; }

	/**
	 * Returns the error callback function provided with init.
	 * @see setErrorCallback()
	 * @return Returns the error callback function.
	 */
	std::function<void(int32_t, std::string)>* getErrorCallback();

	/**
	 * Sets a callback function which will be called for all error messages. First parameter of the function is the error level (1 = critical, 2 = error, 3 = warning), second parameter is the error string.
	 * @see getErrorCallback()
	 * @return Returns the error callback function.
	 */
	void setErrorCallback(std::function<void(int32_t, std::string)>* errorCallback);

	/**
	 * Prints the policy and priority of the thread executing this method.
	 */
	void printThreadPriority();

	/**
	 * Returns a time string like "08/27/14 14:13:53.471".
	 * @return Returns a time string like "08/27/14 14:13:53.471".
	 */
	std::string getTimeString(int64_t time = 0);

	/**
	 * Prints the provided binary data as a hexadecimal string.
	 *
	 * @param data The binary data to print.
	 */
	void printBinary(std::vector<unsigned char>& data);

	/**
	 * Prints the provided binary data as a hexadecimal string.
	 *
	 * @param data The binary data to print.
	 */
	void printBinary(std::shared_ptr<std::vector<char>> data);

	/**
	 * Prints the provided binary data as a hexadecimal string.
	 *
	 * @param data The binary data to print.
	 */
	void printBinary(std::vector<char>& data);

	/**
	 * Prints an error message with filename, line number and function name.
	 *
	 * @param file The name of the file where the error occured.
	 * @param line The line number where the error occured.
	 * @param function The function name where the error occured.
	 * @param what The error message.
	 */
	void printEx(std::string file, uint32_t line, std::string function, std::string what = "");

	/**
	 * Prints a critical error message (debug level < 1).
	 *
	 * @see printError()
	 * @see printWarning()
	 * @see printInfo()
	 * @see printDebug()
	 * @see printMessage()
	 * @param errorString The error message.
	 * @param errorCallback If set to false, the error will not be send to RPC event servers (default true)
	 */
	void printCritical(std::string errorString, bool errorCallback = true);

	/**
	 * Prints an error message (debug level < 2).
	 *
	 * @see printCritical()
	 * @see printWarning()
	 * @see printInfo()
	 * @see printDebug()
	 * @see printMessage()
	 * @param errorString The error message.
	 */
	void printError(std::string errorString);

	/**
	 * Prints a warning message (debug level < 3).
	 *
	 * @see printCritical()
	 * @see printError()
	 * @see printInfo()
	 * @see printDebug()
	 * @see printMessage()
	 * @param errorString The warning message.
	 */
	void printWarning(std::string errorString);

	/**
	 * Prints a info message (debug level < 4).
	 *
	 * @see printCritical()
	 * @see printError()
	 * @see printWarning()
	 * @see printDebug()
	 * @see printMessage()
	 * @param message The message.
	 */
	void printInfo(std::string message);

	/**
	 * Prints a debug message (debug level < 5).
	 *
	 * @see printCritical()
	 * @see printError()
	 * @see printWarning()
	 * @see printInfo()
	 * @see printMessage()
	 * @param message The message.
	 * @param minDebugLevel The minimal debug level (default 5).
	 */
	void printDebug(std::string message, int32_t minDebugLevel = 5);

	/**
	 * Prints a message regardless of the current debug level.
	 *
	 * @see printCritical()
	 * @see printError()
	 * @see printWarning()
	 * @see printInfo()
	 * @see printDebug()
	 * @param message The message.
	 * @param minDebugLevel The minimal debug level (default 0).
	 * @param errorLog If set to true and minDebugLevel is at least "warning", the message is written to the error log, too (default false).
	 */
	void printMessage(std::string message, int32_t minDebugLevel = 0, bool errorLog = false);

	/**
	 * Calls the error callback function registered with the constructor.
	 */
private:

	/**
	 * Pointer to the common base library object.
	 */
	BaseLib::Obj* _bl = nullptr;

	/**
	 * A prefix put before all messages.
	 */
	std::string _prefix;

	/**
	 * Pointer to an optional callback function, which will be called whenever printEx, printWarning, printCritical or printError are called.
	 */
	std::function<void(int32_t, std::string)>* _errorCallback = nullptr;

	Output(const Output&);
	Output& operator=(const Output&);
};
}
#endif
