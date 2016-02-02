/* Copyright 2013-2016 Sathya Laufer
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

#include "ClientSettings.h"
#include "../GD/GD.h"

namespace RPC
{
ClientSettings::ClientSettings()
{

}

void ClientSettings::reset()
{
	_clients.clear();
}

void ClientSettings::load(std::string filename)
{
	try
	{
		reset();
		char input[1024];
		FILE *fin;
		int32_t len, ptr;
		bool found = false;

		if (!(fin = fopen(filename.c_str(), "r")))
		{
			GD::out.printError("Unable to open RPC client config file: " + filename + ". " + strerror(errno));
			return;
		}

		std::shared_ptr<Settings> settings(new Settings());
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
						if(!settings->hostname.empty()) _clients[settings->hostname] = settings;
						settings.reset(new Settings());
						settings->name = std::string(&input[1]);
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
				BaseLib::HelperFunctions::toLower(name);
				BaseLib::HelperFunctions::trim(name);
				std::string value(&input[ptr]);
				BaseLib::HelperFunctions::trim(value);
				if(name == "hostname")
				{
					settings->hostname = BaseLib::HelperFunctions::toLower(value);
					GD::out.printDebug("Debug: hostname of RPC client " + settings->name + " set to " + settings->hostname);
				}
				else if(name == "cafile")
				{
					settings->caFile = value;
					GD::out.printDebug("Debug: caFile of RPC client " + settings->name + " set to " + settings->caFile);
				}
				else if(name == "forcessl")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "false") settings->forceSSL = false;
					GD::out.printDebug("Debug: forceSSL of RPC client " + settings->name + " set to " + std::to_string(settings->forceSSL));
				}
				else if(name == "authtype")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "basic") settings->authType = Settings::AuthType::basic;
					GD::out.printDebug("Debug: authType of RPC client " + settings->name + " set to " + std::to_string(settings->authType));
				}
				else if(name == "verifycertificate")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "false") settings->verifyCertificate = false;
					GD::out.printDebug("Debug: verifyCertificate of RPC client " + settings->name + " set to " + std::to_string(settings->verifyCertificate));
				}
				else if(name == "username")
				{
					settings->userName = BaseLib::HelperFunctions::toLower(value);
					GD::out.printDebug("Debug: userName of RPC client " + settings->name + " set to " + settings->userName);
				}
				else if(name == "password")
				{
					settings->password = BaseLib::HelperFunctions::toLower(value);
					if(settings->password.front() == '"' && settings->password.back() == '"')
					{
						settings->password = settings->password.substr(1, settings->password.size() - 2);
						BaseLib::HelperFunctions::stringReplace(settings->password, "\\\"", "\"");
						BaseLib::HelperFunctions::stringReplace(settings->password, "\\\\", "\\");
					}
					GD::out.printDebug("Debug: password of RPC client " + settings->name + " was set.");
				}
				else
				{
					GD::out.printWarning("Warning: RPC client setting not found: " + std::string(input));
				}
			}
		}
		if(!settings->hostname.empty()) _clients[settings->hostname] = settings;

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
} /* namespace RPC */
