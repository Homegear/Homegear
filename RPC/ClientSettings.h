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

#ifndef CLIENTSETTINGS_H_
#define CLIENTSETTINGS_H_

#include "../Exception.h"

#include <memory>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

namespace RPC
{
class ClientSettings
{
public:
	struct Settings
	{
		enum AuthType { none, basic };

		std::string name;
		std::string hostname;
		bool forceSSL = true;
		AuthType authType = AuthType::none;
		bool verifyCertificate = true;
	};

	ClientSettings();
	virtual ~ClientSettings() {}
	void load(std::string filename);

	std::shared_ptr<Settings> get(std::string& hostname) { if(_clients.find(hostname) != _clients.end()) return _clients[hostname]; else return std::shared_ptr<Settings>(); }
private:
	std::map<std::string, std::shared_ptr<Settings>> _clients;

	void reset();
};
}
#endif /* CLIENTSETTINGS_H_ */
