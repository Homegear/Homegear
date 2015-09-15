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

#ifndef IWEBSERVEREVENTSINK_H_
#define IWEBSERVEREVENTSINK_H_

#include "ServerInfo.h"

namespace BaseLib
{
namespace Rpc
{
/**
 * This class provides hooks into the web server so get and post requests can be passed into family modules.
 */
class IWebserverEventSink : public IEventSinkBase
{
public:
	/**
	 * Called on every HTTP GET request processed by the web server.
	 * @param httpRequest The http request received by the webserver.
	 * @param socket The socket to the client.
	 * @param path The GET path called by the client.
	 * @return Return true when the request was handled. When false is returned, the request will be handled by the web server.
	 */
	virtual bool onGet(PServerInfo& serverInfo, HTTP& httpRequest, std::shared_ptr<SocketOperations>& socket, std::string& path) { return false; }

	/**
	 * Called on every HTTP POST request processed by the web server.
	 * @param httpRequest The http request received by the webserver.
	 * @param socket The socket to the client.
	 * @param path The POST path called by the client.
	 * @return Return true when the request was handled. When false is returned, the request will be handled by the web server.
	 */
	virtual bool onPost(PServerInfo& serverInfo, HTTP& httpRequest, std::shared_ptr<SocketOperations>& socket, std::string& path) { return false; }
};
}
}
#endif


