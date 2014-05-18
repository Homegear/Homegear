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

#include "PhysicalDevices.h"
#include "../GD/GD.h"

PhysicalDevices::PhysicalDevices()
{
}

void PhysicalDevices::reset()
{
	_physicalDevicesMutex.lock();
	_physicalDevices.clear();
	_physicalDevicesMutex.unlock();
}

void PhysicalDevices::load(std::string filename)
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
			BaseLib::Output::printError("Unable to open physical device config file: " + filename + ". " + strerror(errno));
			return;
		}

		std::shared_ptr<BaseLib::Systems::PhysicalDeviceSettings> settings(new BaseLib::Systems::PhysicalDeviceSettings());
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
						if((!settings->device.empty() || (!settings->hostname.empty() && !settings->port.empty())) && !settings->type.empty() && settings->family != BaseLib::Systems::DeviceFamilies::none && GD::deviceFamilies.find(settings->family) != GD::deviceFamilies.end())
						{
							std::shared_ptr<BaseLib::Systems::PhysicalDevice> device = GD::deviceFamilies.at(settings->family)->createPhysicalDevice(settings);
							_physicalDevicesMutex.lock();
							if(device) _physicalDevices[settings->family] = device;
							_physicalDevicesMutex.unlock();
						}
						settings.reset(new BaseLib::Systems::PhysicalDeviceSettings());
						std::string name(&input[1]);
						BaseLib::HelperFunctions::toLower(name);
						if(name == "homematicbidcos") settings->family = BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS;
						else if(name == "homematicwired") settings->family = BaseLib::Systems::DeviceFamilies::HomeMaticWired;
						else if(name == "insteon") settings->family = BaseLib::Systems::DeviceFamilies::Insteon;
						else if(name == "fs20") settings->family = BaseLib::Systems::DeviceFamilies::FS20;
						else if(name == "max") settings->family = BaseLib::Systems::DeviceFamilies::MAX;
						if(GD::deviceFamilies.find(settings->family) != GD::deviceFamilies.end()) BaseLib::Output::printDebug("Debug: Reading config for physical device family " + GD::deviceFamilies.at(settings->family)->getName());
						else BaseLib::Output::printError("Error in physicaldevices.conf: No module found for device family: " + name);
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
			if(found && GD::deviceFamilies.find(settings->family) != GD::deviceFamilies.end())
			{
				std::string name(input);
				BaseLib::HelperFunctions::toLower(name);
				BaseLib::HelperFunctions::trim(name);
				std::string value(&input[ptr]);
				BaseLib::HelperFunctions::trim(value);

				if(name == "devicetype")
				{
					BaseLib::HelperFunctions::toLower(value);
					settings->type = value;
					BaseLib::Output::printDebug("Debug: deviceType of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->type);
				}
				else if(name == "device")
				{
					settings->device = value;
					BaseLib::Output::printDebug("Debug: device of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->device);
				}
				else if(name == "responsedelay")
				{
					settings->responseDelay = BaseLib::HelperFunctions::getNumber(value);
					if(settings->responseDelay > 10000) settings->responseDelay = 10000;
					BaseLib::Output::printDebug("Debug: responseDelay of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->responseDelay));
				}
				else if(name == "oneway")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "true") settings->oneWay = true;
					BaseLib::Output::printDebug("Debug: oneWay of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->oneWay));
				}
				else if(name == "enablerxvalue")
				{
					int32_t number = BaseLib::HelperFunctions::getNumber(value);
					settings->enableRXValue = number;
					BaseLib::Output::printDebug("Debug: enableRXValue of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->enableRXValue));
				}
				else if(name == "enabletxvalue")
				{
					int32_t number = BaseLib::HelperFunctions::getNumber(value);
					settings->enableTXValue = number;
					BaseLib::Output::printDebug("Debug: enableTXValue of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->enableTXValue));
				}
				else if(name == "gpio1")
				{
					int32_t number = BaseLib::HelperFunctions::getNumber(value);
					if(number > 0)
					{
						settings->gpio[1].number = number;
						BaseLib::Output::printDebug("Debug: GPIO1 of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->gpio[1].number));
					}
				}
				else if(name == "gpio2")
				{
					int32_t number = BaseLib::HelperFunctions::getNumber(value);
					if(number > 0)
					{
						settings->gpio[2].number = number;
						BaseLib::Output::printDebug("Debug: GPIO2 of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->gpio[2].number));
					}
				}
				else if(name == "gpio3")
				{
					int32_t number = BaseLib::HelperFunctions::getNumber(value);
					if(number > 0)
					{
						settings->gpio[3].number = number;
						BaseLib::Output::printDebug("Debug: GPIO3 of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->gpio[3].number));
					}
				}
				else if(name == "hostname")
				{
					settings->hostname = value;
					BaseLib::Output::printDebug("Debug: Hostname of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->hostname);
				}
				else if(name == "port")
				{
					settings->port = value;
					BaseLib::Output::printDebug("Debug: Port of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->port);
				}
				else if(name == "ssl")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "true") settings->ssl = true;
					BaseLib::Output::printDebug("Debug: SSL of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->ssl));
				}
				else if(name == "verifycertificate")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "false") settings->verifyCertificate = false;
					BaseLib::Output::printDebug("Debug: VerifyCertificate of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->verifyCertificate));
				}
				else
				{
					BaseLib::Output::printWarning("Warning: Unknown physical device setting: " + std::string(input));
				}
			}
		}
		if((!settings->device.empty() || (!settings->hostname.empty() && !settings->port.empty())) && !settings->type.empty() && settings->family != BaseLib::Systems::DeviceFamilies::none && GD::deviceFamilies.find(settings->family) != GD::deviceFamilies.end())
		{
			std::shared_ptr<BaseLib::Systems::PhysicalDevice> device = GD::deviceFamilies.at(settings->family)->createPhysicalDevice(settings);
			_physicalDevicesMutex.lock();
			if(device) _physicalDevices[settings->family] = device;
			_physicalDevicesMutex.unlock();
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
		_physicalDevicesMutex.unlock();
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_physicalDevicesMutex.unlock();
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_physicalDevicesMutex.unlock();
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::Systems::PhysicalDevice> PhysicalDevices::get(BaseLib::Systems::DeviceFamilies family)
{
	std::shared_ptr<BaseLib::Systems::PhysicalDevice> device;
	try
	{
		_physicalDevicesMutex.lock();
		if(_physicalDevices.find(family) != _physicalDevices.end()) device = _physicalDevices[family];
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _physicalDevicesMutex.unlock();
    return device;
}

uint32_t PhysicalDevices::count()
{
	uint32_t size = 0;
	try
	{
		_physicalDevicesMutex.lock();
		size = _physicalDevices.size();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _physicalDevicesMutex.unlock();
    return size;
}

bool PhysicalDevices::isOpen()
{
	try
	{
		if(_physicalDevices.empty()) return false;
		_physicalDevicesMutex.lock();
		for(std::map<BaseLib::Systems::DeviceFamilies, std::shared_ptr<BaseLib::Systems::PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			if(!i->second->isOpen())
			{
				_physicalDevicesMutex.unlock();
				return false;
			}
		}
		_physicalDevicesMutex.unlock();
		return true;
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _physicalDevicesMutex.unlock();
	return false;
}

void PhysicalDevices::startListening()
{
	try
	{
		_physicalDevicesMutex.lock();
		for(std::map<BaseLib::Systems::DeviceFamilies, std::shared_ptr<BaseLib::Systems::PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			i->second->startListening();
		}
		_physicalDevicesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PhysicalDevices::stopListening()
{
	try
	{
		_physicalDevicesMutex.lock();
		for(std::map<BaseLib::Systems::DeviceFamilies, std::shared_ptr<BaseLib::Systems::PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			i->second->stopListening();
		}
		_physicalDevicesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void PhysicalDevices::setup(int32_t userID, int32_t groupID)
{
	try
	{
		_physicalDevicesMutex.lock();
		for(std::map<BaseLib::Systems::DeviceFamilies, std::shared_ptr<BaseLib::Systems::PhysicalDevice>>::iterator i = _physicalDevices.begin(); i != _physicalDevices.end(); ++i)
		{
			if(!i->second)
			{
				BaseLib::Output::printCritical("Critical: Could not setup device for device family with index " + std::to_string((int32_t)i->first) + " device pointer was empty.");
				continue;
			}
			BaseLib::Output::printDebug("Debug: Setting up physical device for device family with index " + std::to_string((int32_t)i->first));
			i->second->setup(userID, groupID);
		}
		_physicalDevicesMutex.unlock();
	}
	catch(const std::exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<BaseLib::RPC::RPCVariable> PhysicalDevices::listInterfaces()
{
	try
	{
		std::shared_ptr<BaseLib::RPC::RPCVariable> array(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::shared_ptr<BaseLib::RPC::RPCVariable> interface(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
			std::shared_ptr<BaseLib::Systems::PhysicalDevice> device = get(i->first);
			if(!device) continue;

			interface->structValue->insert(BaseLib::RPC::RPCStructElement("DEVICEFAMILY", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->second->getName()))));
			interface->structValue->insert(BaseLib::RPC::RPCStructElement("PHYSICALADDRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(central->physicalAddress()))));
			interface->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(device->getType()))));
			interface->structValue->insert(BaseLib::RPC::RPCStructElement("CONNECTED", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(device->isOpen()))));
			interface->structValue->insert(BaseLib::RPC::RPCStructElement("DEFAULT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(true))));
			interface->structValue->insert(BaseLib::RPC::RPCStructElement("LASTPACKETSENT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)(device->lastPacketSent() / 1000)))));
			interface->structValue->insert(BaseLib::RPC::RPCStructElement("LASTPACKETRECEIVED", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)(device->lastPacketReceived() / 1000)))));
			array->arrayValue->push_back(interface);
		}
		return array;
	}
	catch(const std::exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		BaseLib::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
