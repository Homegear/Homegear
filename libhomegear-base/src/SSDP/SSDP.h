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

#ifndef SSDP_H_
#define SSDP_H_

#include "../Encoding/HTTP.h"
#include "../Variable.h"

#include <string>
#include <vector>
#include <memory>
#include <set>

namespace BaseLib
{

class Obj;
class FileDescriptor;

class SSDPInfo
{
public:
	SSDPInfo(std::string ip, PVariable info);
	virtual ~SSDPInfo();
	std::string ip();
	const PVariable info();
private:
	std::string _ip;
	PVariable _info;
};

class SSDP
{
public:
	SSDP(BaseLib::Obj* baseLib);
	virtual ~SSDP();

	/**
	 * Searches for SSDP devices and returns the IPv4 addresses.
	 *
	 * @param[in] stHeader The ST header with the URN to search for (e. g. urn:schemas-upnp-org:device:basic:1)
	 * @param[in] timeout The time to wait for responses
	 * @param[out] devices The found devices with device information parsed from XML to a Homegear variable struct.
	 */
	void searchDevices(const std::string& stHeader, uint32_t timeout, std::vector<SSDPInfo>& devices);
private:
	BaseLib::Obj* _bl = nullptr;
	std::string _address;
	int32_t _port = 1900;

	void getAddress();
	void sendSearchBroadcast(std::shared_ptr<FileDescriptor>& serverSocketDescriptor, const std::string& stHeader, uint32_t timeout);
	void processPacket(HTTP& http, const std::string& stHeader, std::set<std::string>& locations);
	void getDeviceInfo(std::set<std::string>& locations, std::vector<SSDPInfo>& devices);
	std::shared_ptr<FileDescriptor> getSocketDescriptor();
};

}
#endif
