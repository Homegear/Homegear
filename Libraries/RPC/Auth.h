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

#ifndef AUTH_H_
#define AUTH_H_

#include "HTTP.h"
#include "../../Modules/Base/BaseLib.h"
#include "Base64.h"
#include "../User/User.h"

#include <string>
#include <memory>
#include <vector>

namespace RPC
{

class AuthException : public BaseLib::Exception
{
public:
	AuthException(std::string message) : BaseLib::Exception(message) {}
};

class Auth
{
public:
	Auth();
	Auth(std::shared_ptr<BaseLib::SocketOperations>& socket, std::vector<std::string>& validUsers);
	Auth(std::shared_ptr<BaseLib::SocketOperations>& socket, std::string userName, std::string password);
	virtual ~Auth() {}

	bool initialized() { return _initialized; }
	std::pair<std::string, std::string> basicClient();
	bool basicServer(std::shared_ptr<BaseLib::RPC::RPCHeader>& binaryHeader);
	bool basicServer(HTTP& httpPacket);
protected:
	bool _initialized = false;
	std::string _hostname;
	std::shared_ptr<BaseLib::SocketOperations> _socket;
	std::string _basicAuthHTTPHeader;
	std::shared_ptr<std::vector<char>> _basicUnauthBinaryHeader;
	std::shared_ptr<std::vector<char>> _basicUnauthHTTPHeader;
	std::vector<std::string> _validUsers;
	std::string _userName;
	std::string _password;
	std::pair<std::string, std::string> _basicAuthString;
	HTTP _http;
	std::shared_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;

	void sendBasicUnauthorized(bool binary);
};

} /* namespace RPC */
#endif /* AUTH_H_ */
