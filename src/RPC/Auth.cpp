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

#include <gnutls/x509.h>
#include "Auth.h"
#include "../GD/GD.h"

namespace Homegear {

namespace Rpc {

Auth::Auth(std::unordered_set<uint64_t> &validGroups) {
  _authenticated = false;
  _validGroups = validGroups;
  if (_validGroups.empty()) _validGroups.emplace(1);
  _rpcEncoder = std::make_shared<BaseLib::Rpc::RpcEncoder>(GD::bl.get());
  _jsonDecoder = std::make_shared<BaseLib::Rpc::JsonDecoder>(GD::bl.get());
}

bool Auth::validUser(std::string &userName) {
  auto groups = User::getGroups(userName);
  for (auto group: groups) {
    if (_validGroups.find(group) != _validGroups.end()) return true;
  }

  return false;
}

void Auth::sendBasicUnauthorized(std::shared_ptr<C1Net::TcpSocket> &socket, bool binary) {
  if (binary) {
    if (_basicUnauthBinaryHeader.empty()) {
      BaseLib::PVariable error = BaseLib::Variable::createError(-32603, "Unauthorized");
      _rpcEncoder->encodeResponse(error, _basicUnauthBinaryHeader);
    }
    try {
      socket->Send((uint8_t *)_basicUnauthBinaryHeader.data(), _basicUnauthBinaryHeader.size());
    }
    catch (const BaseLib::SocketOperationException &ex) {
      throw AuthException(std::string("Authorization failed because of socket exception: ") + ex.what());
    }
  } else {
    if (_basicUnauthHTTPHeader.empty()) {
      std::string header;
      header.append("HTTP/1.1 401 Unauthorized\r\n");
      header.append("Connection: Close\r\n");
      std::string content
          ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>Unauthorized</title></head><body>Unauthorized</body></html>");
      header.append("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n");
      header.append(content);
      _basicUnauthHTTPHeader.insert(_basicUnauthHTTPHeader.begin(), header.begin(), header.end());
    }
    try {
      socket->Send((uint8_t *)_basicUnauthHTTPHeader.data(), _basicUnauthHTTPHeader.size());
    }
    catch (const BaseLib::SocketOperationException &ex) {
      throw AuthException(std::string("Authorization failed because of socket exception: ") + ex.what());
    }
  }
}

bool Auth::basicServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                       std::shared_ptr<BaseLib::Rpc::RpcHeader> &binaryHeader,
                       std::string &userName,
                       BaseLib::Security::PAcls &acls) {
  if (_authenticated) return true; //We might be authenticated through certificate authentication
  userName = "";
  if (!acls) acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  acls->clear();
  if (_validGroups.empty()) throw AuthException("No valid groups specified in \"rpcservers.conf\".");
  if (!binaryHeader) throw AuthException("Header is nullptr.");
  if (binaryHeader->authorization.empty()) {
    sendBasicUnauthorized(socket, true);
    throw AuthException("No header field \"Authorization\"");
  }
  std::pair<std::string, std::string> authData = BaseLib::HelperFunctions::splitLast(binaryHeader->authorization, ' ');
  BaseLib::HelperFunctions::toLower(authData.first);
  if (authData.first != "basic") {
    sendBasicUnauthorized(socket, true);
    throw AuthException("Authorization type is not basic but: " + authData.first);
  }
  std::string decodedData;
  BaseLib::Base64::decode(authData.second, decodedData);
  std::pair<std::string, std::string> credentials = BaseLib::HelperFunctions::splitLast(decodedData, ':');
  BaseLib::HelperFunctions::toLower(credentials.first);
  if (credentials.first.empty() || credentials.second.empty()) throw AuthException("User name or password is empty.");
  if (!validUser(credentials.first)) {
    sendBasicUnauthorized(socket, true);
    throw AuthException(
        "User's " + credentials.first + " group is not in the list of valid groups in /etc/homegear/rpcservers.conf.");
  }
  if (User::verify(credentials.first, credentials.second)) {
    userName = credentials.first;
    if (!acls->fromUser(userName)) {
      _userName = "";
      throw AuthException("Could not set ACLs.");
    }
    _authenticated = true;
    return true;
  }
  sendBasicUnauthorized(socket, true);
  return false;
}

bool Auth::basicServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                       BaseLib::Http &httpPacket,
                       std::string &userName,
                       BaseLib::Security::PAcls &acls) {
  if (_authenticated) return true; //We might be authenticated through certificate authentication
  userName = "";
  if (!acls) acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  acls->clear();
  if (_validGroups.empty()) throw AuthException("No valid groups specified in \"rpcservers.conf\".");
  _http.reset();
  const uint32_t bufferLength = 1024;
  std::array<char, bufferLength + 1> buffer{};
  bool more_data = false;
  if (httpPacket.getHeader().authorization.empty()) {
    if (_basicAuthHTTPHeader.empty()) {
      _basicAuthHTTPHeader.append("HTTP/1.1 401 Authorization Required\r\n");
      _basicAuthHTTPHeader.append("WWW-Authenticate: Basic realm=\"Authentication Required\"\r\n");
      _basicAuthHTTPHeader.append("Connection: Keep-Alive\r\n");
      std::string content
          ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\"><html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"><title>Authorization Required</title></head><body>Authorization Required</body></html>");
      _basicAuthHTTPHeader.append("Content-Length: " + std::to_string(content.size()) + "\r\n\r\n");
      _basicAuthHTTPHeader.append(content);
    }
    std::shared_ptr<std::vector<char>> data(new std::vector<char>());
    data->insert(data->begin(), _basicAuthHTTPHeader.begin(), _basicAuthHTTPHeader.end());
    try {
      socket->Send((uint8_t *)data->data(), data->size());
      int32_t bytesRead = socket->Read((uint8_t *)buffer.data(), buffer.size() - 1, more_data);
      //Some clients send only one byte in the first packet
      if (bytesRead == 1) bytesRead += socket->Read((uint8_t *)buffer.data() + 1, buffer.size() - 2, more_data);
      buffer.at(bytesRead) = '\0';
      try {
        _http.process(buffer.data(), buffer.size() - 1);
      }
      catch (BaseLib::HttpException &ex) {
        throw AuthException(std::string("Authorization failed because of HTTP exception: ") + ex.what());
      }
    }
    catch (const BaseLib::SocketOperationException &ex) {
      throw AuthException(std::string("Authorization failed because of socket exception: ") + ex.what());
    }
  } else _http = httpPacket;
  if (_http.getHeader().authorization.empty()) {
    sendBasicUnauthorized(socket, false);
    throw AuthException("No header field \"Authorization\"");
  }
  std::pair<std::string, std::string>
      authData = BaseLib::HelperFunctions::splitLast(_http.getHeader().authorization, ' ');
  BaseLib::HelperFunctions::toLower(authData.first);
  if (authData.first != "basic") {
    sendBasicUnauthorized(socket, false);
    throw AuthException("Authorization type is not basic but: " + authData.first);
  }
  std::string decodedData;
  BaseLib::Base64::decode(authData.second, decodedData);
  std::pair<std::string, std::string> credentials = BaseLib::HelperFunctions::splitLast(decodedData, ':');
  BaseLib::HelperFunctions::toLower(credentials.first);
  if (credentials.first.empty() || credentials.second.empty()) throw AuthException("User name or password is empty.");
  if (!validUser(credentials.first)) {
    sendBasicUnauthorized(socket, false);
    throw AuthException(
        "User's " + credentials.first + " group is not in the list of valid groups in /etc/homegear/rpcservers.conf.");
  }
  if (User::verify(credentials.first, credentials.second)) {
    userName = credentials.first;
    if (!acls->fromUser(userName)) {
      _userName = "";
      throw AuthException("Could not set ACLs.");
    }
    _authenticated = true;
    return true;
  }
  sendBasicUnauthorized(socket, false);
  return false;
}

bool Auth::basicServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                       BaseLib::WebSocket &webSocket,
                       std::string &userName,
                       BaseLib::Security::PAcls &acls) {
  if (_authenticated) return true; //We might be authenticated through certificate authentication
  userName = "";
  if (!acls) acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  acls->clear();
  if (_validGroups.empty()) throw AuthException("No valid groups specified in \"rpcservers.conf\".");
  if (webSocket.getContent().empty()) {
    sendWebSocketUnauthorized(socket, "No data received.");
    return false;
  }
  BaseLib::PVariable variable;
  try {
    variable = _jsonDecoder->decode(webSocket.getContent());
  }
  catch (BaseLib::Rpc::JsonDecoderException &ex) {
    sendWebSocketUnauthorized(socket, "Error decoding json (is packet in json format?).");
    return false;
  }
  if (variable->type != BaseLib::VariableType::tStruct) {
    sendWebSocketUnauthorized(socket, "Received data is no json object.");
    return false;
  }
  if (variable->structValue->find("user") == variable->structValue->end()
      || variable->structValue->find("password") == variable->structValue->end()) {
    sendWebSocketUnauthorized(socket, "Either 'user' or 'password' is not specified.");
    return false;
  }
  BaseLib::HelperFunctions::toLower(variable->structValue->at("user")->stringValue);
  std::string websocketUser = variable->structValue->at("user")->stringValue;
  std::string websocketPassword = variable->structValue->at("password")->stringValue;
  if (websocketUser.empty() || websocketPassword.empty()) throw AuthException("User name or password is empty.");
  if (!validUser(websocketUser)) {
    sendBasicUnauthorized(socket, false);
    throw AuthException(
        "User's " + websocketUser + " group is not in the list of valid groups in /etc/homegear/rpcservers.conf.");
  }
  if (User::verify(websocketUser, websocketPassword)) {
    userName = websocketUser;
    if (!acls->fromUser(websocketUser)) {
      userName = "";
      throw AuthException("Could not set ACLs.");
    }
    sendWebSocketAuthorized(socket);
    _authenticated = true;
    return true;
  }
  sendWebSocketUnauthorized(socket, "Wrong user name or password.");
  return false;
}

bool Auth::sessionServer(std::shared_ptr<C1Net::TcpSocket> &socket,
                         BaseLib::WebSocket &webSocket,
                         std::string &userName,
                         BaseLib::Security::PAcls &acls) {
  userName = "";
  if (!acls) acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  acls->clear();
  if (webSocket.getContent().empty()) {
    sendWebSocketUnauthorized(socket, "No data received.");
    return false;
  }
#ifndef NO_SCRIPTENGINE
  BaseLib::PVariable variable;
  try {
    variable = _jsonDecoder->decode(webSocket.getContent());
  }
  catch (BaseLib::Rpc::JsonDecoderException &ex) {
    sendWebSocketUnauthorized(socket, "Error decoding json (is packet in json format?).");
    return false;
  }
  if (variable->type != BaseLib::VariableType::tStruct) {
    sendWebSocketUnauthorized(socket, "Received data is no json object.");
    return false;
  }
  if (variable->structValue->find("user") != variable->structValue->end()) {
    std::string webSocketUser = GD::scriptEngineServer->checkSessionId(variable->structValue->at("user")->stringValue);
    BaseLib::HelperFunctions::toLower(webSocketUser);
    if (!webSocketUser.empty()) {
      if (!validUser(webSocketUser)) {
        sendBasicUnauthorized(socket, false);
        throw AuthException(
            "User's " + webSocketUser + " group is not in the list of valid groups in /etc/homegear/rpcservers.conf.");
      }
      userName = webSocketUser;
      if (!acls->fromUser(userName)) {
        userName = "";
        throw AuthException("Could not set ACLs (user: " + userName + ")");
      }
      sendWebSocketAuthorized(socket);
      _authenticated = true;
      return true;
    } else {
      sendWebSocketUnauthorized(socket, "Session id is invalid.");
      return false;
    }
  } else {
    sendWebSocketUnauthorized(socket, "No session id specified.");
    return false;
  }
#else
  sendWebSocketUnauthorized(socket, "Homegear is compiled without script engine. Session authentication is not possible.");
  throw AuthException("Homegear is compiled without script engine. Session authentication is not possible.");
  return false;
#endif
}

bool Auth::certificateServer(C1Net::PTcpSocket &socket, std::string &userName, std::string &dn, BaseLib::Security::PAcls &acls, std::string &error) {
  userName = "";
  if (!acls) acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  acls->clear();
  if (_validGroups.empty()) throw AuthException("No valid groups specified in \"rpcservers.conf\".");
  const gnutls_datum_t *derClientCertificates = gnutls_certificate_get_peers(socket->GetTlsSessionHandle(), nullptr);
  if (!derClientCertificates) {
    error = "Error retrieving client certificate.";
    return false;
  }
  gnutls_x509_crt_t clientCertificates;
  unsigned int certMax = 1;
  if (gnutls_x509_crt_list_import(&clientCertificates, &certMax, derClientCertificates, GNUTLS_X509_FMT_DER, 0) < 1) {
    error = "Error importing client certificate.";
    return false;
  }
  gnutls_datum_t distinguishedName;
  if (gnutls_x509_crt_get_dn2(clientCertificates, &distinguishedName) != GNUTLS_E_SUCCESS) {
    error = "Error getting client certificate's distinguished name.";
    return false;
  }

  std::string certUserName = std::string((char *)distinguishedName.data, distinguishedName.size);
  dn = certUserName;
  BaseLib::HelperFunctions::toLower(certUserName);
  if (validUser(certUserName)) userName = certUserName;
  else {
    bool userFound = false;

    auto outerFields = BaseLib::HelperFunctions::splitAll(certUserName, ',');
    for (auto &outerField: outerFields) {
      auto innerFields = BaseLib::HelperFunctions::splitFirst(outerField, '=');
      BaseLib::HelperFunctions::trim(innerFields.first);
      BaseLib::HelperFunctions::toLower(innerFields.first);
      if (innerFields.first != "cn") continue;
      BaseLib::HelperFunctions::trim(innerFields.second);
      BaseLib::HelperFunctions::toLower(innerFields.second);
      certUserName = innerFields.second;
      userFound = true;
      break;
    }

    if (userFound && validUser(certUserName)) userName = certUserName;
    else {
      error = "User name is not in a valid group as defined in \"rpcservers.conf\": " + certUserName;
      return false;
    }
  }

  if (!acls->fromUser(userName)) {
    userName = "";
    error = "Error getting ACLs for client or the user doesn't exist. User (= common name or distinguished name): "
        + certUserName;
    return false;
  }

  _authenticated = true;
  return true;
}

void Auth::sendWebSocketAuthorized(std::shared_ptr<C1Net::TcpSocket> &socket) {
  std::vector<char> output;
  std::string json("{\"auth\":\"success\"}");
  std::vector<char> data(json.data(), json.data() + json.size());
  BaseLib::WebSocket::encode(data, BaseLib::WebSocket::Header::Opcode::text, output);
  socket->Send((uint8_t *)output.data(), output.size());
}

void Auth::sendWebSocketUnauthorized(std::shared_ptr<C1Net::TcpSocket> &socket, std::string reason) {
  std::vector<char> output;
  std::string json("{\"auth\":\"failure\",\"reason\":\"" + reason + "\"}");
  std::vector<char> data(json.data(), json.data() + json.size());
  BaseLib::WebSocket::encode(data, BaseLib::WebSocket::Header::Opcode::text, output);
  socket->Send((uint8_t *)output.data(), output.size());
}

}

}