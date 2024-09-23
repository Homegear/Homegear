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

#include "RpcClient.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace Homegear {

namespace Rpc {

RpcClient::RpcClient() {
  try {
    _out.init(GD::bl.get());
    _out.setPrefix("RPC client: ");

    if (!GD::bl) _out.printCritical("Critical: Can't initialize RPC client, because base library is not initialized.");
    _rpcDecoder = std::unique_ptr<BaseLib::Rpc::RpcDecoder>(new BaseLib::Rpc::RpcDecoder(GD::bl.get()));
    _rpcEncoder = std::unique_ptr<BaseLib::Rpc::RpcEncoder>(new BaseLib::Rpc::RpcEncoder(GD::bl.get()));
    _xmlRpcDecoder = std::unique_ptr<BaseLib::Rpc::XmlrpcDecoder>(new BaseLib::Rpc::XmlrpcDecoder(GD::bl.get()));
    _xmlRpcEncoder = std::unique_ptr<BaseLib::Rpc::XmlrpcEncoder>(new BaseLib::Rpc::XmlrpcEncoder(GD::bl.get()));
    _jsonDecoder = std::unique_ptr<BaseLib::Rpc::JsonDecoder>(new BaseLib::Rpc::JsonDecoder(GD::bl.get()));
    _jsonEncoder = std::unique_ptr<BaseLib::Rpc::JsonEncoder>(new BaseLib::Rpc::JsonEncoder(GD::bl.get()));
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

RpcClient::~RpcClient() {
}

std::pair<std::string, std::string> RpcClient::basicAuth(std::string &userName, std::string &password) {
  if (userName.empty() || password.empty()) throw AuthException("User name or password is empty.");
  std::pair<std::string, std::string> basicAuthString;
  basicAuthString.first = "Authorization";
  std::string credentials = userName + ":" + password;
  std::string encodedData;
  BaseLib::Base64::encode(credentials, encodedData);
  basicAuthString.second = "Basic " + encodedData;
  return basicAuthString;
}

void RpcClient::invokeBroadcast(RemoteRpcServer *server,
                                std::string methodName,
                                std::shared_ptr<std::list<BaseLib::PVariable>> parameters) {
  try {
    if (methodName.empty()) {
      //Avoid calling the error callback in Output.
      std::cout << BaseLib::Output::getTimeString() << " " << "Error: Could not invoke RPC method for server "
                << server->hostname << ". methodName is empty." << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << "Error: Could not invoke RPC method for server "
                << server->hostname << ". methodName is empty." << std::endl;
      return;
    }
    if (!server) {
      std::cout << BaseLib::Output::getTimeString() << " "
                << "RPC Client: Could not send packet. Pointer to server is nullptr." << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " "
                << "RPC Client: Could not send packet. Pointer to server is nullptr." << std::endl;
      return;
    }
    std::unique_lock<std::mutex> sendGuard(server->sendMutex);
    if (GD::bl->debugLevel >= 5) {
      _out.printDebug("Debug: Calling RPC method \"" + methodName + "\" on server "
                          + (server->hostname.empty() ? server->address.first : server->hostname) + ".");
      _out.printDebug("Parameters:");
      for (std::list<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i) {
        (*i)->print(true, false);
      }
    }
    //Get settings pointer every time this method is executed, because
    //the settings might change.
    server->settings = GD::clientSettings.get(server->hostname);
    bool retry = false;
    uint32_t retries = server->settings ? server->settings->retries : 3;

    std::vector<char> requestData;
    std::vector<char> responseData;
    if (server->binary) _rpcEncoder->encodeRequest(methodName, parameters, requestData);
    else if (server->webSocket) {
      std::vector<char> json;
      _jsonEncoder->encodeRequest(methodName, parameters, json);
      BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, requestData);
    } else if (server->json) _jsonEncoder->encodeRequest(methodName, parameters, requestData);
    else _xmlRpcEncoder->encodeRequest(methodName, parameters, requestData);
    for (uint32_t i = 0; i < retries; ++i) {
      retry = false;
      if (i == 0) sendRequest(server, requestData, responseData, true, retry);
      else sendRequest(server, requestData, responseData, false, retry);
      if (!retry || server->removed || !server->autoConnect) break;
    }
    if (server->removed) return;
    if (retry && !server->reconnectInfinitely) {
      if (!server->webSocket) {
        std::cout << BaseLib::Output::getTimeString() << " " << "Removing server \"" << server->id
                  << "\" after trying to send a packet " + std::to_string(retries)
                      + " times. Server has to send \"init\" again."
                  << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " " << "Removing server \"" << server->id
                  << "\" after trying to send a packet " + std::to_string(retries)
                      + " times. Server has to send \"init\" again."
                  << std::endl;
      }
      server->removed = true;
      return;
    }
    if (responseData.empty()) {
      if (server->webSocket) {
        server->removed = true;
        sendGuard.unlock();
        _out.printInfo("Info: Connection to server closed. Host: " + server->hostname);
      } else {
        sendGuard.unlock();
        std::cout << BaseLib::Output::getTimeString() << " " << "Warning: Response is empty. RPC method: "
                  << methodName << " Server: " << server->hostname << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " " << "Warning: Response is empty. RPC method: "
                  << methodName << " Server: " << server->hostname << std::endl;
      }
      return;
    }
    BaseLib::PVariable returnValue;
    if (server->binary) returnValue = _rpcDecoder->decodeResponse(responseData);
    else if (server->webSocket || server->json) returnValue = _jsonDecoder->decode(responseData);
    else returnValue = _xmlRpcDecoder->decodeResponse(responseData);

    if (returnValue->errorStruct) {
      std::cout << BaseLib::Output::getTimeString() << " " << "Error in RPC response from " << server->hostname
                << ". faultCode: " << std::to_string(returnValue->structValue->at("faultCode")->integerValue)
                << " faultString: " << returnValue->structValue->at("faultString")->stringValue << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << "Error in RPC response from " << server->hostname
                << ". faultCode: " << std::to_string(returnValue->structValue->at("faultCode")->integerValue)
                << " faultString: " << returnValue->structValue->at("faultString")->stringValue << std::endl;
    } else {
      if (GD::bl->debugLevel >= 5) {
        _out.printDebug("Response was:");
        returnValue->print(true, false);
      }
      server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
    }
  }
  catch (const std::exception &ex) {
    std::cout << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
  }
}

BaseLib::PVariable RpcClient::invoke(RemoteRpcServer *server,
                                     std::string methodName,
                                     std::shared_ptr<std::list<BaseLib::PVariable>> parameters) {
  try {
    if (methodName.empty()) return BaseLib::Variable::createError(-32601, "Method name is empty");
    if (!server) return BaseLib::Variable::createError(-32500, "Could not send packet. Pointer to server is nullptr.");
    std::unique_lock<std::mutex> sendGuard(server->sendMutex);
    if (server->removed)
      return BaseLib::Variable::createError(-32300,
                                            "Server was removed and has to send \"init\" again.");;
    if (GD::bl->debugLevel >= 5) {
      _out.printDebug("Debug: Calling RPC method \"" + methodName + "\" on server "
                          + (server->hostname.empty() ? server->address.first : server->hostname) + ".");
      _out.printDebug("Parameters:");
      for (std::list<BaseLib::PVariable>::iterator i = parameters->begin(); i != parameters->end(); ++i) {
        (*i)->print(true, false);
      }
    }
    //Get settings pointer every time this method is executed, because
    //the settings might change.
    server->settings = GD::clientSettings.get(server->hostname);
    bool retry = false;
    uint32_t retries = server->settings ? server->settings->retries : 3;
    std::vector<char> requestData;
    std::vector<char> responseData;
    if (server->binary) _rpcEncoder->encodeRequest(methodName, parameters, requestData);
    else if (server->webSocket) {
      std::vector<char> json;
      _jsonEncoder->encodeRequest(methodName, parameters, json);
      BaseLib::WebSocket::encode(json, BaseLib::WebSocket::Header::Opcode::text, requestData);
    } else if (server->json) _jsonEncoder->encodeRequest(methodName, parameters, requestData);
    else _xmlRpcEncoder->encodeRequest(methodName, parameters, requestData);
    for (uint32_t i = 0; i < retries; ++i) {
      retry = false;
      if (i == 0) sendRequest(server, requestData, responseData, true, retry);
      else sendRequest(server, requestData, responseData, false, retry);
      if (!retry || server->removed || !server->autoConnect) break;
    }
    if (server->removed)
      return BaseLib::Variable::createError(-32300,
                                            "Server was removed and has to send \"init\" again.");
    if (retry && !server->reconnectInfinitely) {
      if (!server->webSocket) {
        std::cout << BaseLib::Output::getTimeString() << " " << "Removing server \"" << server->id
                  << "\" after trying to send a packet " + std::to_string(retries)
                      + " times. Server has to send \"init\" again."
                  << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " " << "Removing server \"" << server->id
                  << "\" after trying to send a packet " + std::to_string(retries)
                      + " times. Server has to send \"init\" again."
                  << std::endl;
      }
      server->removed = true;
      return BaseLib::Variable::createError(-32300, "Request timed out.");
    }
    if (responseData.empty()) {
      if (server->webSocket) {
        server->removed = true;
        sendGuard.unlock();
        _out.printInfo("Info: Connection to server closed. Host: " + server->hostname);
      }
      return BaseLib::Variable::createError(-32700, "No response data.");
    }
    BaseLib::PVariable returnValue;
    if (server->binary) returnValue = _rpcDecoder->decodeResponse(responseData);
    else if (server->webSocket || server->json) returnValue = _jsonDecoder->decode(responseData);
    else returnValue = _xmlRpcDecoder->decodeResponse(responseData);
    if (returnValue->errorStruct) {
      std::cout << BaseLib::Output::getTimeString() << " " << "Error in RPC response from " << server->hostname
                << ". faultCode: " << std::to_string(returnValue->structValue->at("faultCode")->integerValue)
                << " faultString: " << returnValue->structValue->at("faultString")->stringValue << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << "Error in RPC response from " << server->hostname
                << ". faultCode: " << std::to_string(returnValue->structValue->at("faultCode")->integerValue)
                << " faultString: " << returnValue->structValue->at("faultString")->stringValue << std::endl;
    } else {
      if (GD::bl->debugLevel >= 5) {
        _out.printDebug("Response was:");
        returnValue->print(true, false);
      }
      server->lastPacketSent = BaseLib::HelperFunctions::getTimeSeconds();
    }

    return returnValue;
  }
  catch (const std::exception &ex) {
    std::cout << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
  }
  return BaseLib::Variable::createError(-32700, "No response data.");
}

void RpcClient::sendRequest(RemoteRpcServer *server,
                            std::vector<char> &data,
                            std::vector<char> &responseData,
                            bool insertHeader,
                            bool &retry) {
  try {
    if (!server) {
      std::cout << BaseLib::Output::getTimeString() << " "
                << "RPC Client: Could not send packet. Pointer to server or data is nullptr." << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " "
                << "RPC Client: Could not send packet. Pointer to server or data is nullptr." << std::endl;
      return;
    }
    if (server->removed) return;

    if (server->autoConnect) {
      if (server->hostname.empty()) {
        std::cout << BaseLib::Output::getTimeString() << " " << "RPC Client: Error: hostname is empty."
                  << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " " << "RPC Client: Error: hostname is empty."
                  << std::endl;
        server->removed = true;
        return;
      }
      if (!server->useSSL && server->settings && server->settings->forceSSL) {
        std::cout << BaseLib::Output::getTimeString() << " "
                  << "RPC Client: Tried to send unencrypted packet to " << server->hostname
                  << " with forceSSL enabled for this server. Removing server from list. Server has to send \"init\" again."
                  << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " "
                  << "RPC Client: Tried to send unencrypted packet to " << server->hostname
                  << " with forceSSL enabled for this server. Removing server from list. Server has to send \"init\" again."
                  << std::endl;
        server->removed = true;
        return;
      }
    }

    try {
      if (!server->socket->Connected()) {
        if (server->autoConnect) {
          C1Net::TcpSocketInfo tcp_socket_info;
          if (server->settings) {
            tcp_socket_info.read_timeout = server->settings->timeout;
            tcp_socket_info.write_timeout = server->settings->timeout;
          }

          C1Net::TcpSocketHostInfo tcp_socket_host_info;
          tcp_socket_host_info.host = server->hostname;
          tcp_socket_host_info.port = BaseLib::Math::getUnsignedNumber(server->address.second);
          tcp_socket_host_info.auto_connect = false;
          tcp_socket_host_info.tls = server->useSSL;

          if (server->settings && (!server->settings->caFile.empty() || !server->settings->certFile.empty() || !server->settings->keyFile.empty())) {
            tcp_socket_host_info.ca_file = server->settings->caFile;
            tcp_socket_host_info.client_cert_file = server->settings->certFile;
            tcp_socket_host_info.client_key_file = server->settings->keyFile;
            tcp_socket_host_info.verify_certificate = server->settings->verifyCertificate;
          }

          server->socket = std::make_shared<C1Net::TcpSocket>(tcp_socket_info, tcp_socket_host_info);
          server->socket->Open();
        } else {
          std::cout << BaseLib::Output::getTimeString() << " " << "Connection to server with id "
                    << std::to_string(server->uid) << " closed. Removing server." << std::endl;
          std::cerr << BaseLib::Output::getTimeString() << " " << "Connection to server with id "
                    << std::to_string(server->uid) << " closed. Removing server." << std::endl;
          server->removed = true;
          return;
        }
      }
    }
    catch (const C1Net::Exception &ex) {
      if (!server->reconnectInfinitely) server->removed = true;
      GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
      std::cout << BaseLib::Output::getTimeString() << " " << ex.what()
                << " Removing server. Server has to send \"init\" again." << std::endl;
      std::cerr << BaseLib::Output::getTimeString() << " " << ex.what()
                << " Removing server. Server has to send \"init\" again." << std::endl;
      return;
    }

    if (server->settings && server->settings->authType != ClientSettings::Settings::AuthType::none) {
      if (server->settings->userName.empty() || server->settings->password.empty()) {
        server->socket->Shutdown();
        std::cout << BaseLib::Output::getTimeString() << " "
                  << "Error: No user name or password specified in config file for RPC server "
                  << server->hostname << ". Closing connection." << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " "
                  << "Error: No user name or password specified in config file for RPC server "
                  << server->hostname << ". Closing connection." << std::endl;
        return;
      }
    }

    if (insertHeader) {
      if (server->binary) {
        BaseLib::Rpc::RpcHeader header;
        if (server->settings && (server->settings->authType & ClientSettings::Settings::AuthType::basic)) {
          _out.printDebug("Using Basic Access Authentication.");
          std::pair<std::string, std::string>
              authField = basicAuth(server->settings->userName, server->settings->password);
          header.authorization = authField.second;
        }
        _rpcEncoder->insertHeader(data, header);
      } else if (server->webSocket) {}
      else //XML-RPC, JSON-RPC
      {
        std::string header =
            "POST " + server->path + " HTTP/1.1\r\nUser-Agent: Homegear " + GD::baseLibVersion + "\r\nHost: "
                + server->hostname + ":" + server->address.second + "\r\nContent-Type: "
                + (server->json ? "application/json" : "text/xml") + "\r\nContent-Length: "
                + std::to_string(data.size() + 2) + "\r\nConnection: " + (server->keepAlive ? "Keep-Alive" : "close")
                + "\r\n";
        if (server->settings && (server->settings->authType & ClientSettings::Settings::AuthType::basic)) {
          _out.printDebug("Using Basic Access Authentication.");
          std::pair<std::string, std::string>
              authField = basicAuth(server->settings->userName, server->settings->password);
          header += authField.first + ": " + authField.second + "\r\n";
        }
        header += "\r\n";
        data.reserve(data.size() + header.size() + 2);
        data.push_back('\r');
        data.push_back('\n');
        data.insert(data.begin(), header.begin(), header.end());
      }
    }

    if (GD::bl->debugLevel >= 5) {
      if (server->binary || server->webSocket)
        _out.printDebug("Debug: Sending packet: " + GD::bl->hf.getHexString(data));
      else _out.printDebug("Debug: Sending packet: " + std::string(data.data(), data.size()));
    }
    try {
      server->socket->Send((uint8_t *)data.data(), data.size());
    }
    catch (const C1Net::Exception &ex) {
      retry = true;
      server->socket->Shutdown();
      std::cout << BaseLib::Output::getTimeString() << " " << "Info: Could not send data to XML RPC server "
                << server->hostname << ": " + std::string(ex.what()) << "." << std::endl;
      return;
    }
  }
  catch (const std::exception &ex) {
    if (!server->reconnectInfinitely) server->removed = true;
    else retry = true;
    GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    std::cout << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cout << BaseLib::Output::getTimeString() << " " << "Removing server. Server has to send \"init\" again."
              << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Removing server. Server has to send \"init\" again."
              << std::endl;
    return;
  }

  //Receive response
  try {
    responseData.clear();
    ssize_t receivedBytes = 0;

    int32_t bufferMax = 2048;
    char buffer[bufferMax + 1];
    BaseLib::Rpc::BinaryRpc binaryRpc(GD::bl.get());
    BaseLib::Http http;
    BaseLib::WebSocket webSocket;
    bool more_data = false;

    while (!binaryRpc.isFinished() && !http.isFinished()
        && !webSocket.isFinished()) //This is equal to while(true) for binary packets
    {
      try {
        receivedBytes = server->socket->Read((uint8_t *)buffer, bufferMax, more_data);
        //Some clients send only one byte in the first packet
        if (receivedBytes == 1 && !binaryRpc.processingStarted() && !http.headerIsFinished()
            && !webSocket.dataProcessingStarted())
          receivedBytes += server->socket->Read((uint8_t *)buffer + 1, bufferMax - 1, more_data);
      }
      catch (const C1Net::TimeoutException &ex) {
        _out.printInfo("Info: Reading from RPC server timed out. Server: " + server->hostname);
        retry = true;
        if (!server->keepAlive) server->socket->Shutdown();
        return;
      }
      catch (const C1Net::ClosedException &ex) {
        retry = true;
        if (!server->keepAlive) server->socket->Shutdown();
        std::cout << BaseLib::Output::getTimeString() << " " << "Warning: " << ex.what() << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " " << "Warning: " << ex.what() << std::endl;
        return;
      }
      catch (const C1Net::Exception &ex) {
        retry = true;
        if (!server->keepAlive) server->socket->Shutdown();
        std::cout << BaseLib::Output::getTimeString() << " " << "Error: " << ex.what() << std::endl;
        std::cerr << BaseLib::Output::getTimeString() << " " << "Error: " << ex.what() << std::endl;
        return;
      }

      //We are using string functions to process the buffer. So just to make sure,
      //they don't do something in the memory after buffer, we add '\0'
      buffer[receivedBytes] = '\0';
      if (GD::bl->debugLevel >= 5) {
        std::vector<uint8_t> rawPacket(buffer, buffer + receivedBytes);
        _out.printDebug("Debug: Packet received: " + BaseLib::HelperFunctions::getHexString(rawPacket));
      }

      if (server->binary) {
        try {
          int32_t processedBytes = binaryRpc.process(&buffer[0], receivedBytes);
          if (processedBytes < receivedBytes) {
            std::cout << BaseLib::Output::getTimeString() << " " << "Warning: Received more bytes ("
                      << std::to_string(receivedBytes) << ") than binary packet size ("
                      << std::to_string(processedBytes) << ")." << " Packet was: "
                      << BaseLib::HelperFunctions::getHexString(&buffer[0], receivedBytes) << std::endl;
            std::cerr << BaseLib::Output::getTimeString() << " " << "Warning: Received more bytes ("
                      << std::to_string(receivedBytes) << ") than binary packet size ("
                      << std::to_string(processedBytes) << ")." << std::endl;
          }
        }
        catch (BaseLib::Rpc::BinaryRpcException &ex) {
          if (!server->keepAlive) server->socket->Shutdown();
          std::cout << BaseLib::Output::getTimeString() << " " << "Error processing packet: " << ex.what()
                    << std::endl;
          std::cerr << BaseLib::Output::getTimeString() << " " << "Error processing packet: " << ex.what()
                    << std::endl;
          return;
        }
      } else if (server->webSocket) {
        try {
          webSocket.process(buffer,
                            receivedBytes); //Check for chunked packets (HomeMatic Manager, ioBroker). Necessary, because HTTP header does not contain transfer-encoding.
        }
        catch (BaseLib::WebSocketException &ex) {
          if (!server->keepAlive) server->socket->Shutdown();
          std::cout << BaseLib::Output::getTimeString() << " "
                    << "RPC Client: Could not process WebSocket packet: " << ex.what() << " Buffer: "
                    << std::string(buffer, receivedBytes) << std::endl;
          std::cerr << BaseLib::Output::getTimeString() << " "
                    << "RPC Client: Could not process WebSocket packet: " << ex.what() << " Buffer: "
                    << std::string(buffer, receivedBytes) << std::endl;
          return;
        }
        if (http.getContentSize() > 10485760 || http.getHeader().contentLength > 10485760) {
          if (!server->keepAlive) server->socket->Shutdown();
          std::cout << BaseLib::Output::getTimeString() << " "
                    << "Error: Packet with data larger than 100 MiB received." << std::endl;
          std::cerr << BaseLib::Output::getTimeString() << " "
                    << "Error: Packet with data larger than 100 MiB received." << std::endl;
          return;
        }
        if (webSocket.isFinished() && webSocket.getHeader().opcode == BaseLib::WebSocket::Header::Opcode::ping) {
          _out.printInfo("Info: Websocket ping received.");
          std::vector<char> pong;
          webSocket.encode(webSocket.getContent(), BaseLib::WebSocket::Header::Opcode::pong, pong);
          try {
            server->socket->Send((uint8_t *)pong.data(), pong.size());
          }
          catch (const C1Net::Exception &ex) {
            retry = true;
            server->socket->Shutdown();
            std::cout << BaseLib::Output::getTimeString() << " "
                      << "Info: Could not send data to XML RPC server " << server->hostname << ": "
                      << ex.what() << "." << std::endl;
            return;
          }
          webSocket = BaseLib::WebSocket();
        }
      } else {
        if (!http.headerIsFinished()) {
          if (!strncmp(buffer, "401", 3)
              || !strncmp(&buffer[9], "401", 3)) //"401 Unauthorized" or "HTTP/1.X 401 Unauthorized"
          {
            if (!server->keepAlive) server->socket->Shutdown();
            std::cout << BaseLib::Output::getTimeString() << " " << "Error: Authentication failed. Server "
                      << server->hostname << ". Check user name and password in rpcclients.conf."
                      << std::endl;
            std::cerr << BaseLib::Output::getTimeString() << " " << "Error: Authentication failed. Server "
                      << server->hostname << ". Check user name and password in rpcclients.conf."
                      << std::endl;
            return;
          }
        }

        try {
          http.process(buffer,
                       receivedBytes,
                       !server->json,
                       server->json); //Check for chunked packets (HomeMatic Manager, ioBroker). Necessary, because HTTP header does not contain transfer-encoding.
        }
        catch (BaseLib::HttpException &ex) {
          if (!server->keepAlive) server->socket->Shutdown();
          std::cout << BaseLib::Output::getTimeString() << " "
                    << "XML RPC Client: Could not process HTTP packet: " << ex.what() << " Buffer: "
                    << std::string(buffer, receivedBytes) << std::endl;
          std::cerr << BaseLib::Output::getTimeString() << " "
                    << "XML RPC Client: Could not process HTTP packet: " << ex.what() << " Buffer: "
                    << std::string(buffer, receivedBytes) << std::endl;
          return;
        }
        if (http.getContentSize() > 10485760 || http.getHeader().contentLength > 10485760) {
          if (!server->keepAlive) server->socket->Shutdown();
          std::cout << BaseLib::Output::getTimeString() << " "
                    << "Error: Packet with data larger than 100 MiB received." << std::endl;
          std::cerr << BaseLib::Output::getTimeString() << " "
                    << "Error: Packet with data larger than 100 MiB received." << std::endl;
          return;
        }
      }
    }
    if (!server->keepAlive) server->socket->Shutdown();
    if (GD::bl->debugLevel >= 5) {
      if (server->binary)
        _out.printDebug("Debug: Received packet from server " + server->hostname + ": "
                            + GD::bl->hf.getHexString(binaryRpc.getData()));
      else if (http.isFinished() && !http.getContent().empty())
        _out.printDebug("Debug: Received packet from server " + server->hostname + ":\n"
                            + std::string(&http.getContent().at(0), http.getContent().size()));
      else if (webSocket.isFinished() && !webSocket.getContent().empty())
        _out.printDebug("Debug: Received packet from server " + server->hostname + ":\n"
                            + std::string(&webSocket.getContent().at(0), webSocket.getContent().size()));
    }
    if (server->binary && binaryRpc.isFinished()) responseData = std::move(binaryRpc.getData());
    else if (server->webSocket && webSocket.isFinished()) responseData = std::move(webSocket.getContent());
    else if (http.isFinished()) responseData = std::move(http.getContent());
  }
  catch (const std::exception &ex) {
    if (!server->reconnectInfinitely) server->removed = true;
    GD::bl->fileDescriptorManager.shutdown(server->fileDescriptor);
    std::cout << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Error in file " << __FILE__ << " line " << __LINE__
              << " in function " << __PRETTY_FUNCTION__ << ": " << ex.what() << std::endl;
    std::cout << BaseLib::Output::getTimeString() << " " << "Removing server. Server has to send \"init\" again."
              << std::endl;
    std::cerr << BaseLib::Output::getTimeString() << " " << "Removing server. Server has to send \"init\" again."
              << std::endl;
  }
}

}

}