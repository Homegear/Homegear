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

#include "ClientSettings.h"
#include "../HelperFunctions.h"
#include "../GD.h"

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
			HelperFunctions::printError("Unable to open RPC client config file: " + filename + ". " + strerror(errno));
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
				HelperFunctions::toLower(name);
				HelperFunctions::trim(name);
				std::string value(&input[ptr]);
				HelperFunctions::trim(value);
				if(name == "hostname")
				{
					settings->hostname = HelperFunctions::toLower(value);
					HelperFunctions::printDebug("Debug: hostname of RPC client " + settings->name + " set to " + settings->hostname);
				}
				else if(name == "forcessl")
				{
					HelperFunctions::toLower(value);
					if(value == "false") settings->forceSSL = false;
					HelperFunctions::printDebug("Debug: forceSSL of RPC client " + settings->name + " set to " + std::to_string(settings->forceSSL));
				}
				else if(name == "authtype")
				{
					HelperFunctions::toLower(value);
					if(value == "basic") settings->authType = Settings::AuthType::basic;
					HelperFunctions::printDebug("Debug: authType of RPC client " + settings->name + " set to " + std::to_string(settings->authType));
				}
				else if(name == "verifycertificate")
				{
					HelperFunctions::toLower(value);
					if(value == "false") settings->verifyCertificate = false;
					HelperFunctions::printDebug("Debug: verifyCertificate of RPC client " + settings->name + " set to " + std::to_string(settings->verifyCertificate));
				}
				else
				{
					HelperFunctions::printWarning("Warning: RPC client setting not found: " + std::string(input));
				}
			}
		}
		if(!settings->hostname.empty()) _clients[settings->hostname] = settings;

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	HelperFunctions::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
} /* namespace RPC */
