/* Copyright 2013-2019 Homegear GmbH
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

#ifndef RESTSERVER_H_
#define RESTSERVER_H_

#include <homegear-base/BaseLib.h>

namespace Homegear
{

namespace Rpc
{

class RestServer
{
public:
    RestServer(std::shared_ptr<BaseLib::Rpc::ServerInfo::Info>& serverInfo);

    virtual ~RestServer();

    void process(BaseLib::PRpcClientInfo clientInfo, BaseLib::Http& http, std::shared_ptr<BaseLib::TcpSocket> socket);

private:
    BaseLib::Output _out;
    BaseLib::Rpc::PServerInfo _serverInfo;
    BaseLib::Http _http;
    std::unique_ptr<BaseLib::Rpc::JsonEncoder> _jsonEncoder;
    std::unique_ptr<BaseLib::Rpc::JsonDecoder> _jsonDecoder;

    void getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content);

    void getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content, std::vector<std::string>& additionalHeaders);

    void send(std::shared_ptr<BaseLib::TcpSocket>& socket, std::vector<char>& data);
};

}

}
#endif
