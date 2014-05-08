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

#include "ServerSettings.h"
#include "../GD/GD.h"

namespace RPC
{
ServerSettings::ServerSettings()
{

}

void ServerSettings::reset()
{
	_servers.clear();
}

void ServerSettings::load(std::string filename)
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
			Output::printError("Unable to open RPC server config file: " + filename + ". " + strerror(errno));
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
						if(settings->port > 0)
						{
							settings->index = index;
							_servers[index++] = settings;
						}
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
				if(name == "interface")
				{
					settings->interface = value;
					if(settings->interface.empty()) settings->interface = "::";
					Output::printDebug("Debug: interface of RPC server " + settings->name + " set to " + settings->interface);
				}
				else if(name == "port")
				{
					settings->port = HelperFunctions::getNumber(value);
					Output::printDebug("Debug: port of RPC server " + settings->name + " set to " + std::to_string(settings->port));
				}
				else if(name == "ssl")
				{
					HelperFunctions::toLower(value);
					if(value == "false") settings->ssl = false;
					Output::printDebug("Debug: ssl of RPC server " + settings->name + " set to " + std::to_string(settings->ssl));
				}
				else if(name == "authtype")
				{
					HelperFunctions::toLower(value);
					if(value == "none") settings->authType = Settings::AuthType::none;
					else if(value == "basic") settings->authType = Settings::AuthType::basic;
					Output::printDebug("Debug: authType of RPC server " + settings->name + " set to " + std::to_string(settings->authType));
				}
				else if(name == "validusers")
				{
					std::stringstream stream(value);
					std::string element;
					while(std::getline(stream, element, ','))
					{
						HelperFunctions::toLower(HelperFunctions::trim(element));
						settings->validUsers.push_back(element);
					}
				}
				else if(name == "diffiehellmankeysize")
				{
					settings->diffieHellmanKeySize = HelperFunctions::getNumber(value);
					if(settings->diffieHellmanKeySize < 128) settings->diffieHellmanKeySize = 128;
					if(settings->diffieHellmanKeySize < 1024) Output::printWarning("Diffie-Hellman key size of server " + settings->name + " is smaller than 1024 bit.");
					Output::printDebug("Debug: diffieHellmanKeySize of RPC server " + settings->name + " set to " + std::to_string(settings->diffieHellmanKeySize));
				}
				else
				{
					Output::printWarning("Warning: RPC client setting not found: " + std::string(input));
				}
			}
		}
		if(settings->port > 0)
		{
			settings->index = index;
			_servers[index] = settings;
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
} /* namespace RPC */
