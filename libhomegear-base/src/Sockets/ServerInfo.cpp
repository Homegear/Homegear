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

#include "ServerInfo.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Rpc
{
ServerInfo::ServerInfo()
{
}

ServerInfo::ServerInfo(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void ServerInfo::reset()
{
	_servers.clear();
}

void ServerInfo::init(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void ServerInfo::load(std::string filename)
{
	try
	{
		reset();
		int32_t index = 0;
		char input[1024];
		FILE *fin;
		int32_t len, ptr;
		bool found = false;

		if (!(fin = fopen(filename.c_str(), "r")))
		{
			_bl->out.printError("Unable to open RPC server config file: " + filename + ". " + strerror(errno));
			return;
		}

		std::shared_ptr<Info> info(new Info());
		while (fgets(input, 1024, fin))
		{
			if(input[0] == '#') continue;
			len = strlen(input);
			if (len < 2) continue;
			if (input[len-1] == '\n') input[len-1] = '\0';
			ptr = 0;
			if(input[0] == '[')
			{
				while(ptr < len)
				{
					if (input[ptr] == ']')
					{
						input[ptr] = '\0';
						if(info->port > 0)
						{
							info->index = index;
							_servers[index++] = info;
						}
						info.reset(new Info());
						info->name = std::string(&input[1]);
						break;
					}
					ptr++;
				}
				continue;
			}
			found = false;
			while(ptr < len)
			{
				if (input[ptr] == '=')
				{
					found = true;
					input[ptr++] = '\0';
					break;
				}
				ptr++;
			}
			if(found)
			{
				std::string name(input);
				HelperFunctions::toLower(name);
				HelperFunctions::trim(name);
				std::string value(&input[ptr]);
				HelperFunctions::trim(value);
				if(name == "interface")
				{
					info->interface = value;
					if(info->interface.empty()) info->interface = "::";
					_bl->out.printDebug("Debug: interface of server " + info->name + " set to " + info->interface);
				}
				else if(name == "port")
				{
					info->port = Math::getNumber(value);
					_bl->out.printDebug("Debug: port of server " + info->name + " set to " + std::to_string(info->port));
				}
				else if(name == "ssl")
				{
					HelperFunctions::toLower(value);
					if(value == "false") info->ssl = false;
					_bl->out.printDebug("Debug: ssl of server " + info->name + " set to " + std::to_string(info->ssl));
				}
				else if(name == "authtype")
				{
					HelperFunctions::toLower(value);
					if(value == "none") info->authType = Info::AuthType::none;
					else if(value == "basic") info->authType = Info::AuthType::basic;
					_bl->out.printDebug("Debug: authType of server " + info->name + " set to " + std::to_string(info->authType));
				}
				else if(name == "validusers")
				{
					std::stringstream stream(value);
					std::string element;
					while(std::getline(stream, element, ','))
					{
						HelperFunctions::toLower(HelperFunctions::trim(element));
						info->validUsers.push_back(element);
					}
				}
				else if(name == "diffiehellmankeysize")
				{
					info->diffieHellmanKeySize = Math::getNumber(value);
					if(info->diffieHellmanKeySize < 128) info->diffieHellmanKeySize = 128;
					if(info->diffieHellmanKeySize < 1024) _bl->out.printWarning("Diffie-Hellman key size of server " + info->name + " is smaller than 1024 bit.");
					_bl->out.printDebug("Debug: diffieHellmanKeySize of server " + info->name + " set to " + std::to_string(info->diffieHellmanKeySize));
				}
				else if(name == "contentpath")
				{
					info->contentPath = value;
					if(info->contentPath.back() != '/') info->contentPath.push_back('/');
					_bl->out.printDebug("Debug: contentPath of RPC server " + info->name + " set to " + info->contentPath);
				}
				else if(name == "xmlrpcserver")
				{
					HelperFunctions::toLower(value);
					if(value == "false") info->xmlrpcServer = false;
					_bl->out.printDebug("Debug: xmlrpcServer of server " + info->name + " set to " + std::to_string(info->xmlrpcServer));
				}
				else if(name == "jsonrpcserver")
				{
					HelperFunctions::toLower(value);
					if(value == "false") info->jsonrpcServer = false;
					_bl->out.printDebug("Debug: jsonrpcServer of server " + info->name + " set to " + std::to_string(info->jsonrpcServer));
				}
				else if(name == "webserver")
				{
					HelperFunctions::toLower(value);
					if(value == "true") info->webServer = true;
					_bl->out.printDebug("Debug: webServer of server " + info->name + " set to " + std::to_string(info->webServer));
				}
				else if(name == "websocket")
				{
					HelperFunctions::toLower(value);
					if(value == "true") info->webSocket = true;
					_bl->out.printDebug("Debug: webSocket of server " + info->name + " set to " + std::to_string(info->webSocket));
				}
				else if(name == "websocketauthtype")
				{
					HelperFunctions::toLower(value);
					if(value == "none") info->websocketAuthType = Info::AuthType::none;
					else if(value == "basic") info->websocketAuthType = Info::AuthType::basic;
					else if(value == "session") info->websocketAuthType = Info::AuthType::session;
					_bl->out.printDebug("Debug: websocketauthtype of server " + info->name + " set to " + std::to_string(info->websocketAuthType));
				}
				else if(name == "redirectto")
				{
					info->redirectTo = value;
					_bl->out.printDebug("Debug: redirectToPort of server " + info->name + " set to " + info->redirectTo);
				}
				else
				{
					_bl->out.printWarning("Warning: RPC client setting not found: " + std::string(input));
				}
			}
		}
		if(info->port > 0)
		{
			info->index = index;
			_servers[index] = info;
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
}
}
