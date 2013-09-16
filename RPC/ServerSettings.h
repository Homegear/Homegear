/* Copyright 2013 Sathya Laufer
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

#ifndef SERVERSETTINGS_H_
#define SERVERSETTINGS_H_

#include "../Exception.h"

#include <memory>
#include <iostream>
#include <string>
#include <map>
#include <cstring>
#include <vector>

namespace RPC
{
class ServerSettings
{
public:
	class Settings
	{
	public:
		enum AuthType { none, basic };

		Settings()
		{
			interface = "0.0.0.0";
		}
		virtual ~Settings() {}
		int32_t index = -1;
		std::string name;
		std::string interface;
		int32_t port = -1;
		bool ssl = true;
		AuthType authType = AuthType::basic;
		std::vector<std::string> validUsers;
		int32_t diffieHellmanKeySize = 1024;
	};

	ServerSettings();
	virtual ~ServerSettings() {}
	void load(std::string filename);

	int32_t count() { return _servers.size(); }
	std::shared_ptr<Settings> get(int32_t index) { if(_servers.find(index) != _servers.end()) return _servers[index]; else return std::shared_ptr<Settings>(); }
private:
	std::map<int32_t, std::shared_ptr<Settings>> _servers;

	void reset();
};
}
#endif /* SERVERSETTINGS_H_ */
