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

#include "../GD/GD.h"
#include "MqttSettings.h"

MqttSettings::MqttSettings()
{

}

void MqttSettings::reset()
{
	_enabled = false;
	_brokerHostname = "localhost";
	_brokerPort = "1883";
	_prefix = "homegear/";
	_homegearId = "";
	_username = "";
	_password = "";
	_retain = true;
	_enableSSL = false;
	_caFile = "";
	_verifyCertificate = true;
	_certPath = "";
	_keyPath = "";
}

void MqttSettings::load(std::string filename)
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
			GD::bl->out.printError("Unable to open config file: " + filename + ". " + strerror(errno));
			return;
		}

		while (fgets(input, 1024, fin))
		{
			if(input[0] == '#') continue;
			len = strlen(input);
			if (len < 2) continue;
			if (input[len-1] == '\n') input[len-1] = '\0';
			ptr = 0;
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
				if(name == "enabled")
				{
					if(BaseLib::HelperFunctions::toLower(value) == "true") _enabled = true;
					GD::bl->out.printDebug("Debug (MQTT settings): enabled set to " + std::to_string(_enabled));
				}
				else if(name == "processingthreadcount")
				{
					int32_t integerValue = BaseLib::Math::getNumber(value, false);
					if(integerValue > 0) _processingThreadCount = integerValue;
					GD::bl->out.printDebug("Debug (MQTT settings): processingThreadCount set to " + std::to_string(_processingThreadCount));
				}
				else if(name == "brokerhostname")
				{
					_brokerHostname = value;
					GD::bl->out.printDebug("Debug (MQTT settings): brokerHostname set to " + _brokerHostname);
				}
				else if(name == "brokerport")
				{
					_brokerPort = value;
					GD::bl->out.printDebug("Debug (MQTT settings): brokerPort set to " + _brokerPort);
				}
				else if(name == "clientname")
				{
					_clientName = value;
					GD::bl->out.printDebug("Debug (MQTT settings): clientName set to " + _clientName);
				}
				else if(name == "prefix")
				{
					_prefix = value;
					if(!_prefix.empty() && _prefix.back() != '/') _prefix.push_back('/');
					if(_prefix == "/") _prefix = "";
					GD::bl->out.printDebug("Debug (MQTT settings): prefix set to " + _prefix);
				}
				else if(name == "homegearid")
				{
					_homegearId = value;
					if(_homegearId.length() > 23)
					{
						GD::bl->out.printWarning("Warning (MQTT settings): homegearId is longer than 23 characters. This is outside of the specification. Most servers support this though.");
					}
					GD::bl->out.printDebug("Debug (MQTT settings): homegearId set to " + _homegearId);
				}
				else if(name == "username")
				{
					_username = value;
					GD::bl->out.printDebug("Debug (MQTT settings): username set to " + _username);
				}
				else if(name == "password")
				{
					_password = value;
					GD::bl->out.printDebug("Debug (MQTT settings): password set to " + _password);
				}
				else if(name == "retain")
				{
					if(BaseLib::HelperFunctions::toLower(value) == "false") _retain = false;
					GD::bl->out.printDebug("Debug (MQTT settings): retain set to " + std::to_string(_retain));
				}
				else if(name == "plaintopic")
				{
					_plainTopic = (BaseLib::HelperFunctions::toLower(value) == "true");
					GD::bl->out.printDebug("Debug (MQTT settings): plainTopic set to " + std::to_string(_plainTopic));
				}
				else if(name == "jsontopic")
				{
					_jsonTopic = (BaseLib::HelperFunctions::toLower(value) == "true");
					GD::bl->out.printDebug("Debug (MQTT settings): jsonTopic set to " + std::to_string(_jsonTopic));
				}
				else if(name == "jsonobjtopic")
				{
					_jsonobjTopic = (BaseLib::HelperFunctions::toLower(value) == "true");
					GD::bl->out.printDebug("Debug (MQTT settings): jsonobjTopic set to " + std::to_string(_jsonobjTopic));
				}
				else if(name == "enablessl")
				{
					if(value == "true") _enableSSL = true;
					GD::bl->out.printDebug("Debug (MQTT settings): enableSSL set to " + _enableSSL);
				}
				else if(name == "cafile")
				{
					_caFile = value;
					GD::bl->out.printDebug("Debug (MQTT settings): caFile set to " + _caFile);
				}
				else if(name == "verifycertificate")
				{
					if(value == "false") _verifyCertificate = false;
					GD::bl->out.printDebug("Debug (MQTT settings): verifyCertificate set to " + std::to_string(_verifyCertificate));
				}
				else if(name == "certpath")
				{
					_certPath = value;
					GD::bl->out.printDebug("Debug (MQTT settings): certPath set to " + _certPath);
				}
				else if(name == "keypath")
				{
					_keyPath = value;
					GD::bl->out.printDebug("Debug (MQTT settings): keyPath set to " + _keyPath);
				}
				else
				{
					GD::bl->out.printWarning("Warning: Setting not found: " + std::string(input));
				}
			}
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const BaseLib::Exception& ex)
    {
    	GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}
