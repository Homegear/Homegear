/* Copyright 2013-2020 Homegear GmbH
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef AUTH_H_
#define AUTH_H_

#include <homegear-base/BaseLib.h>
#include "../User/User.h"

#include <string>
#include <memory>
#include <vector>
#include <homegear-base/Security/Acls.h>

namespace Homegear {

namespace Rpc {

class AuthException : public BaseLib::Exception {
 public:
  explicit AuthException(const std::string &message) : BaseLib::Exception(message) {}
};

class Auth {
 public:
  explicit Auth(std::unordered_set<uint64_t> &validGroups);

  virtual ~Auth() = default;

  bool authenticated() { return _authenticated; }

  void setAuthenticated(bool value) { _authenticated = value; }

  bool validUser(std::string &userName);

  bool basicServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                   std::shared_ptr<BaseLib::Rpc::RpcHeader> &binaryHeader,
                   std::string &userName,
                   BaseLib::Security::PAcls &acls);

  bool basicServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                   BaseLib::Http &httpPacket,
                   std::string &userName,
                   BaseLib::Security::PAcls &acls);

  bool basicServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                   BaseLib::WebSocket &webSocket,
                   std::string &userName,
                   BaseLib::Security::PAcls &acls);

  bool sessionServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                     BaseLib::WebSocket &webSocket,
                     std::string &userName,
                     BaseLib::Security::PAcls &acls);

  bool certificateServer(C1Net::PTcpSocket &socket, std::string &userName, std::string &dn, BaseLib::Security::PAcls &acls, std::string &error);

  void sendBasicUnauthorized(std::shared_ptr<C1Net::TcpSocket> &socket, bool binary);

  void sendWebSocketAuthorized(std::shared_ptr<C1Net::TcpSocket> &socket);

  void sendWebSocketUnauthorized(std::shared_ptr<C1Net::TcpSocket> &socket, std::string reason);

 protected:
  std::atomic_bool _authenticated;
  std::string _hostname;
  std::string _basicAuthHTTPHeader;
  std::vector<char> _basicUnauthBinaryHeader;
  std::vector<char> _basicUnauthHTTPHeader;
  std::unordered_set<uint64_t> _validGroups;
  std::string _userName;
  std::string _password;
  std::pair<std::string, std::string> _basicAuthString;
  BaseLib::Http _http;
  std::shared_ptr<BaseLib::Rpc::RpcEncoder> _rpcEncoder;
  std::shared_ptr<BaseLib::Rpc::JsonDecoder> _jsonDecoder;
};

}

}

#endif
