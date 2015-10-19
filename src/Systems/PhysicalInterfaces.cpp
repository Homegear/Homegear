/* Copyright 2013-2015 Sathya Laufer
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

#include "PhysicalInterfaces.h"
#include "../GD/GD.h"

PhysicalInterfaces::PhysicalInterfaces()
{
}

void PhysicalInterfaces::dispose()
{
	if(_disposing) return;
	_disposing = true;
	_physicalInterfacesMutex.lock();
	_physicalInterfaces.clear();
	_physicalInterfacesMutex.unlock();
}

void PhysicalInterfaces::reset()
{
	_physicalInterfacesMutex.lock();
	_physicalInterfaces.clear();
	_physicalInterfacesMutex.unlock();
}

void PhysicalInterfaces::load(std::string filename)
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
			GD::out.printError("Unable to open physical device config file: " + filename + ". " + strerror(errno));
			return;
		}

		std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings(new BaseLib::Systems::PhysicalInterfaceSettings());
		while (fgets(input, 1024, fin))
		{
			if(input[0] == '#' || input[0] == '-' || input[0] == '_') continue;
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
						if(!settings->type.empty() && settings->family != -1)
						{
							if(settings->id.empty()) settings->id = settings->type;
							std::shared_ptr<BaseLib::Systems::IPhysicalInterface> device = GD::deviceFamilies.at(settings->family)->createPhysicalDevice(settings);
							_physicalInterfacesMutex.lock();
							if(device) _physicalInterfaces[settings->family][settings->id] = device;
							_physicalInterfacesMutex.unlock();
						}
						settings.reset(new BaseLib::Systems::PhysicalInterfaceSettings());
						std::string name(&input[1]);
						BaseLib::HelperFunctions::toLower(name);
						std::map<std::string, int32_t>::iterator familyIterator = GD::deviceFamiliesByName.find(name);
						if(familyIterator == GD::deviceFamiliesByName.end()) GD::out.printError("Error in physicalinterfaces.conf: No module found for device family: " + name);
						else
						{
							settings->family = familyIterator->second;
							GD::out.printDebug("Debug: Reading config for physical device family " + GD::deviceFamilies.at(settings->family)->getName());
						}
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

				if(name == "id")
				{
					if(!value.empty())
					{
						settings->id = value;
						GD::out.printDebug("Debug: id of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->id);
					}
				}
				else if(name == "default")
				{
					if(value == "true") settings->isDefault = true;
					GD::out.printDebug("Debug: default of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->isDefault));
				}
				else if(name == "devicetype")
				{
					if(settings->type.length() > 0) GD::out.printWarning("Warning: deviceType in \"physicalinterfaces.conf\" is set multiple times within one device block. Is the family header missing (e. g. \"[HomeMaticBidCoS]\")?");
					if(settings->family == -1) GD::out.printWarning("Warning: deviceType is set in \"physicalinterfaces.conf\" without defining the device family. Is the family header missing (e. g. \"[HomeMaticBidCoS]\")?");
					BaseLib::HelperFunctions::toLower(value);
					settings->type = value;
					GD::out.printDebug("Debug: deviceType of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->type);
				}
				else if(name == "device")
				{
					settings->device = value;
					GD::out.printDebug("Debug: device of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->device);
				}
				else if(name == "responsedelay")
				{
					settings->responseDelay = BaseLib::Math::getNumber(value);
					if(settings->responseDelay > 10000) settings->responseDelay = 10000;
					GD::out.printDebug("Debug: responseDelay of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->responseDelay));
				}
				else if(name == "txpowersetting")
				{
					settings->txPowerSetting = BaseLib::Math::getNumber(value);
					GD::out.printDebug("Debug: txPowerSetting of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->txPowerSetting));
				}
				else if(name == "highgainmodegpio")
				{
					settings->highGainModeGpio = BaseLib::Math::getNumber(value);
					GD::out.printDebug("Debug: highGainModeGpio of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->highGainModeGpio));
				}
				else if(name == "oscillatorfrequency")
				{
					settings->oscillatorFrequency = BaseLib::Math::getNumber(value);
					if(settings->oscillatorFrequency < 0) settings->oscillatorFrequency = -1;
					GD::out.printDebug("Debug: oscillatorFrequency of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->oscillatorFrequency));
				}
				else if(name == "oneway")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "true") settings->oneWay = true;
					GD::out.printDebug("Debug: oneWay of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->oneWay));
				}
				else if(name == "enablerxvalue")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					settings->enableRXValue = number;
					GD::out.printDebug("Debug: enableRXValue of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->enableRXValue));
				}
				else if(name == "enabletxvalue")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					settings->enableTXValue = number;
					GD::out.printDebug("Debug: enableTXValue of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->enableTXValue));
				}
				else if(name == "fastsending")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "true") settings->fastSending = true;
					GD::out.printDebug("Debug: fastSending of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->fastSending));
				}
				else if(name == "waitforbus")
				{
					settings->waitForBus = BaseLib::Math::getNumber(value);
					if(settings->waitForBus < 60) settings->waitForBus = 60;
					else if(settings->waitForBus > 210) settings->waitForBus = 210;
					GD::out.printDebug("Debug: waitForBus of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->waitForBus));
				}
				else if(name == "timeout")
				{
					settings->timeout = BaseLib::Math::getNumber(value);
					if(settings->timeout > 100000) settings->timeout = 100000;
					GD::out.printDebug("Debug: timeout of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->timeout));
				}
				else if(name == "interruptpin")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					if(number >= 0)
					{
						settings->interruptPin = number;
						GD::out.printDebug("Debug: interruptPin of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->interruptPin));
					}
				}
				else if(name == "gpio1")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					if(number > 0)
					{
						settings->gpio[1].number = number;
						GD::out.printDebug("Debug: GPIO1 of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->gpio[1].number));
					}
				}
				else if(name == "gpio2")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					if(number > 0)
					{
						settings->gpio[2].number = number;
						GD::out.printDebug("Debug: GPIO2 of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->gpio[2].number));
					}
				}
				else if(name == "gpio3")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					if(number > 0)
					{
						settings->gpio[3].number = number;
						GD::out.printDebug("Debug: GPIO3 of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->gpio[3].number));
					}
				}
				else if(name == "stackposition")
				{
					int32_t number = BaseLib::Math::getNumber(value);
					if(number > 0)
					{
						settings->stackPosition = number;
						GD::out.printDebug("Debug: stackPosition of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->stackPosition));
					}
				}
				else if(name == "host")
				{
					settings->host = value;
					GD::out.printDebug("Debug: Host of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->host);
				}
				else if(name == "port")
				{
					settings->port = value;
					GD::out.printDebug("Debug: Port of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->port);
				}
				else if(name == "portkeepalive")
				{
					settings->portKeepAlive = value;
					GD::out.printDebug("Debug: PortKeepAlive of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->portKeepAlive);
				}
				else if(name == "oldrfkey")
				{
					settings->oldRFKey = value;
					GD::out.printDebug("Debug: OldRFKey of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->oldRFKey);
				}
				else if(name == "rfkey")
				{
					settings->rfKey = value;
					GD::out.printDebug("Debug: RFKey of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->rfKey);
				}
				else if(name == "currentrfkeyindex")
				{
					settings->currentRFKeyIndex = BaseLib::Math::getNumber(value);
					GD::out.printDebug("Debug: CurrentRFKeyIndex of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->currentRFKeyIndex));
				}
				else if(name == "lankey")
				{
					settings->lanKey = value;
					GD::out.printDebug("Debug: LANKey of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->lanKey);
				}
				else if(name == "ssl")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "true") settings->ssl = true;
					GD::out.printDebug("Debug: SSL of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->ssl));
				}
				else if(name == "cafile")
				{
					settings->caFile = value;
					GD::out.printDebug("Debug: CAFile of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->caFile);
				}
				else if(name == "verifycertificate")
				{
					BaseLib::HelperFunctions::toLower(value);
					if(value == "false") settings->verifyCertificate = false;
					GD::out.printDebug("Debug: VerifyCertificate of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->verifyCertificate));
				}
				else if(name == "listenthreadpriority")
				{
					settings->listenThreadPriority = BaseLib::Math::getNumber(value);
					if(settings->listenThreadPriority > 99) settings->listenThreadPriority = 99;
					if(settings->listenThreadPriority < 0) settings->listenThreadPriority = 0;
					GD::out.printDebug("Debug: ListenThreadPriority of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->listenThreadPriority));
				}
				else if(name == "listenthreadpolicy")
				{
					settings->listenThreadPolicy = BaseLib::Threads::getThreadPolicyFromString(value);
					settings->listenThreadPriority = BaseLib::Threads::parseThreadPriority(settings->listenThreadPriority, settings->listenThreadPolicy);
					GD::out.printDebug("Debug: ListenThreadPolicy of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + std::to_string(settings->listenThreadPolicy));
				}
				else if(name == "ttsprogram")
				{
					settings->ttsProgram = value;
					GD::out.printDebug("Debug: ttsProgram of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->ttsProgram);
				}
				else if(name == "datapath")
				{
					settings->dataPath = value;
					if(settings->dataPath.back() != '/') settings->dataPath.push_back('/');
					GD::out.printDebug("Debug: dataPath of family " + GD::deviceFamilies.at(settings->family)->getName() + " set to " + settings->dataPath);
				}
				else
				{
					GD::out.printWarning("Warning: Unknown physical device setting: " + std::string(input));
				}
			}
		}
		if(!settings->type.empty() && settings->family != -1)
		{
			if(settings->id.empty()) settings->id = settings->type;
			std::shared_ptr<BaseLib::Systems::IPhysicalInterface> device = GD::deviceFamilies.at(settings->family)->createPhysicalDevice(settings);
			_physicalInterfacesMutex.lock();
			if(device) _physicalInterfaces[settings->family][settings->id] = device;
			_physicalInterfacesMutex.unlock();
		}

		fclose(fin);
	}
	catch(const std::exception& ex)
    {
		_physicalInterfacesMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_physicalInterfacesMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_physicalInterfacesMutex.unlock();
    	GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>> PhysicalInterfaces::get(int32_t family)
{
	std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>> devices;
	try
	{
		_physicalInterfacesMutex.lock();
		if(_physicalInterfaces.find(family) != _physicalInterfaces.end()) devices = _physicalInterfaces[family];
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
    _physicalInterfacesMutex.unlock();
    return devices;
}

void PhysicalInterfaces::clear(int32_t family)
{
	try
	{
		_physicalInterfacesMutex.lock();
		if(_physicalInterfaces.find(family) != _physicalInterfaces.end())
		{
			for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = _physicalInterfaces.at(family).begin(); j != _physicalInterfaces.at(family).end(); ++j)
			{
				j->second->stopListening();
			}
			_physicalInterfaces.erase(family);
		}
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
    _physicalInterfacesMutex.unlock();
}

uint32_t PhysicalInterfaces::count()
{
	uint32_t size = 0;
	try
	{
		_physicalInterfacesMutex.lock();
		size = _physicalInterfaces.size();
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
    _physicalInterfacesMutex.unlock();
    return size;
}

uint32_t PhysicalInterfaces::count(int32_t family)
{
	uint32_t size = 0;
	try
	{
		_physicalInterfacesMutex.lock();
		if(_physicalInterfaces.find(family) != _physicalInterfaces.end()) size = _physicalInterfaces.at(family).size();
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
    _physicalInterfacesMutex.unlock();
    return size;
}

bool PhysicalInterfaces::isOpen()
{
	try
	{
		if(_physicalInterfaces.empty()) return true;
		_physicalInterfacesMutex.lock();
		for(std::map<int32_t, std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>>::iterator i = _physicalInterfaces.begin(); i != _physicalInterfaces.end(); ++i)
		{
			for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!j->second->isNetworkDevice() && !j->second->isOpen())
				{
					_physicalInterfacesMutex.unlock();
					return false;
				}
			}
		}
		_physicalInterfacesMutex.unlock();
		return true;
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
    _physicalInterfacesMutex.unlock();
	return false;
}

void PhysicalInterfaces::startListening()
{
	try
	{
		_physicalInterfacesMutex.lock();
		for(std::map<int32_t, std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>>::iterator i = _physicalInterfaces.begin(); i != _physicalInterfaces.end(); ++i)
		{
			for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				j->second->startListening();
			}
		}
		_physicalInterfacesMutex.unlock();
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

void PhysicalInterfaces::stopListening()
{
	try
	{
		_physicalInterfacesMutex.lock();
		for(std::map<int32_t, std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>>::iterator i = _physicalInterfaces.begin(); i != _physicalInterfaces.end(); ++i)
		{
			for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				j->second->stopListening();
			}
		}
		_physicalInterfacesMutex.unlock();
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

void PhysicalInterfaces::setup(int32_t userID, int32_t groupID)
{
	try
	{
		_physicalInterfacesMutex.lock();
		for(std::map<int32_t, std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>>::iterator i = _physicalInterfaces.begin(); i != _physicalInterfaces.end(); ++i)
		{
			for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!j->second)
				{
					GD::out.printCritical("Critical: Could not setup device for device family with index " + std::to_string((int32_t)i->first) + " device pointer was empty.");
					continue;
				}
				GD::out.printDebug("Debug: Setting up physical device for device family with index " + std::to_string((int32_t)i->first));
				j->second->setup(userID, groupID);
			}
		}
		_physicalInterfacesMutex.unlock();
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

BaseLib::PVariable PhysicalInterfaces::listInterfaces(int32_t familyID)
{
	try
	{
		BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));

		for(std::map<int32_t, std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>>::iterator i = _physicalInterfaces.begin(); i != _physicalInterfaces.end(); ++i)
		{
			for(std::map<std::string, std::shared_ptr<BaseLib::Systems::IPhysicalInterface>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(GD::deviceFamilies.find(i->first) == GD::deviceFamilies.end()) continue;
				if(familyID > -1 && ((int32_t)i->first) != familyID) continue;
				std::shared_ptr<BaseLib::Systems::Central> central = GD::deviceFamilies.at(i->first)->getCentral();
				if(!central) continue;
				BaseLib::PVariable interface(new BaseLib::Variable(BaseLib::VariableType::tStruct));

				interface->structValue->insert(BaseLib::StructElement("FAMILYID", BaseLib::PVariable(new BaseLib::Variable((int32_t)i->first))));
				interface->structValue->insert(BaseLib::StructElement("ID", BaseLib::PVariable(new BaseLib::Variable(j->second->getID()))));
				interface->structValue->insert(BaseLib::StructElement("PHYSICALADDRESS", BaseLib::PVariable(new BaseLib::Variable(central->physicalAddress()))));
				interface->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(j->second->getType()))));
				interface->structValue->insert(BaseLib::StructElement("CONNECTED", BaseLib::PVariable(new BaseLib::Variable(j->second->isOpen()))));
				interface->structValue->insert(BaseLib::StructElement("DEFAULT", BaseLib::PVariable(new BaseLib::Variable(j->second->isDefault()))));
				interface->structValue->insert(BaseLib::StructElement("LASTPACKETSENT", BaseLib::PVariable(new BaseLib::Variable((uint32_t)(j->second->lastPacketSent() / 1000)))));
				interface->structValue->insert(BaseLib::StructElement("LASTPACKETRECEIVED", BaseLib::PVariable(new BaseLib::Variable((uint32_t)(j->second->lastPacketReceived() / 1000)))));
				array->arrayValue->push_back(interface);
			}
		}
		return array;
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}
