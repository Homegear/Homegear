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

#ifndef HOMEGEARNET_H_
#define HOMEGEARNET_H_

#include "../Exception.h"

#include <string>
#include <vector>
#include <memory>

namespace BaseLib
{
/**
 * Exception class for the HTTP client.
 *
 * @see HTTPClient
 */
class NetException : public Exception
{
public:
	NetException(std::string message) : Exception(message) {}
};

/**
 * Class with network related helper functions.
 */
class Net
{
public:
	struct RouteInfo
	{
        uint32_t destinationAddress;
        uint32_t sourceAddress;
        uint32_t gateway;
        std::string interfaceName;
	};

	typedef std::vector<std::shared_ptr<RouteInfo>> RouteInfoList;

	Net();

	/**
	 * Destructor.
	 * Does nothing.
	 */
	virtual ~Net();

	/**
	 * Tries to automatically determine the computers IPv4 address.
	 *
	 * @return Returns the computers IPv4 address.
	 */
	static std::string getMyIpAddress();
	static void getRoutes(RouteInfoList& routeInfo);
private:
	static int32_t readNlSocket(int32_t sockFd, char* buffer, int32_t bufferLength, uint32_t messageIndex, uint32_t pid);
};

}

#endif
