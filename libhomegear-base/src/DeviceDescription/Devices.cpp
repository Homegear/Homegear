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

#include "Devices.h"
#include "../BaseLib.h"
#include "HomeMatic/HmConverter.h"

namespace BaseLib
{
namespace DeviceDescription
{

Devices::Devices(BaseLib::Systems::DeviceFamilies family)
{
	_family = family;
}

void Devices::init(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void Devices::clear()
{
	for(std::vector<std::shared_ptr<HomegearDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
	{
		(*i).reset();
	}
	_devices.clear();
}

void Devices::load()
{
	try
	{
		std::string path = _bl->settings.deviceDescriptionPath() + std::to_string((int32_t)_family);
		_devices.clear();
		std::string deviceDir(path);
		std::vector<std::string> files;
		try
		{
			files = _bl->io.getFiles(deviceDir);
		}
		catch(const Exception& ex)
		{
			_bl->out.printError("Could not read device description files in directory: \"" + path + "\": " + ex.what());
			return;
		}
		if(files.empty())
		{
			_bl->out.printError("No xml files found in \"" + path + "\".");
			return;
		}
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			std::string filename(deviceDir + "/" + *i);
			std::shared_ptr<HomegearDevice> device = load(filename);
			if(device) _devices.push_back(device);
		}

		if(_devices.empty()) _bl->out.printError("Could not load any devices from xml files in \"" + deviceDir + "\".");
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<HomegearDevice> Devices::load(std::string& filepath)
{
	try
	{
		if(!Io::fileExists(filepath))
		{
			_bl->out.printError("Error: Could not load device description file: File does not exist.");
			return std::shared_ptr<HomegearDevice>();
		}
		if(filepath.size() < 5)
		{
			_bl->out.printError("Error: Could not load device description file: File does not end with \".xml\".");
			return std::shared_ptr<HomegearDevice>();
		}
		std::string extension = filepath.substr(filepath.size() - 4, 4);
		HelperFunctions::toLower(extension);
		if(extension != ".xml")
		{
			_bl->out.printError("Error: Could not load device description file: File does not end with \".xml\".");
			return std::shared_ptr<HomegearDevice>();
		}
		_bl->out.printDebug("Loading XML RPC device " + filepath);
		bool oldFormat = false;
		std::shared_ptr<HomegearDevice> device(new HomegearDevice(_bl, _family, filepath, oldFormat));
		if(oldFormat) return loadHomeMatic(filepath);
		else if(device && device->loaded()) return device;
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<HomegearDevice>();
}

std::shared_ptr<HomegearDevice> Devices::loadHomeMatic(std::string& filepath)
{
	try
	{
		if(filepath.empty()) return std::shared_ptr<HomegearDevice>();
		std::string filename = (filepath.find('/') == std::string::npos) ? filepath : filepath.substr(filepath.find_last_of('/') + 1);
		if(filename == "rf_cmm.xml" || filename == "hmw_central.xml" || filename == "hmw_generic.xml")
		{
			_bl->out.printInfo("Info: Skipping file " + filename + ": File is not needed.");
			return std::shared_ptr<HomegearDevice>();
		}

		std::shared_ptr<HmDeviceDescription::Device> homeMaticDevice(new HmDeviceDescription::Device(_bl, _family, filepath));
		if(!homeMaticDevice || !homeMaticDevice->loaded()) return std::shared_ptr<HomegearDevice>();

		if(filename.compare(0, 3, "rf_") == 0)
		{
			_bl->out.printInfo("Info: Adding parameter ROAMING and variable CENTRAL_ADDRESS_SPOOFED.");
			std::map<uint32_t, std::shared_ptr<HmDeviceDescription::DeviceChannel>>::iterator channelIterator = homeMaticDevice->channels.find(0);
			if(channelIterator != homeMaticDevice->channels.end() && channelIterator->second)
			{
				std::map<HmDeviceDescription::ParameterSet::Type::Enum, std::shared_ptr<HmDeviceDescription::ParameterSet>>::iterator paramsetIterator = channelIterator->second->parameterSets.find(HmDeviceDescription::ParameterSet::Type::Enum::master);
				if(paramsetIterator != channelIterator->second->parameterSets.end() && paramsetIterator->second)
				{
					if(!paramsetIterator->second->getParameter("ROAMING"))
					{
						std::shared_ptr<HmDeviceDescription::HomeMaticParameter> parameter(new HmDeviceDescription::HomeMaticParameter(_bl));
						parameter->id = "ROAMING";
						parameter->uiFlags = (HmDeviceDescription::HomeMaticParameter::UIFlags::Enum)(HmDeviceDescription::HomeMaticParameter::UIFlags::Enum::visible | HmDeviceDescription::HomeMaticParameter::UIFlags::Enum::internal);
						parameter->logicalParameter.reset(new HmDeviceDescription::LogicalParameterBoolean(_bl));
						parameter->logicalParameter->type = HmDeviceDescription::LogicalParameter::Type::Enum::typeBoolean;
						parameter->logicalParameter->defaultValueExists = true;
						parameter->physicalParameter->interface = HmDeviceDescription::PhysicalParameter::Interface::Enum::store;
						parameter->physicalParameter->type = HmDeviceDescription::PhysicalParameter::Type::Enum::typeInteger;
						paramsetIterator->second->parameters.push_back(parameter);
					}
				}

				paramsetIterator = channelIterator->second->parameterSets.find(HmDeviceDescription::ParameterSet::Type::Enum::values);
				if(paramsetIterator != channelIterator->second->parameterSets.end() && paramsetIterator->second)
				{
					if(!paramsetIterator->second->getParameter("CENTRAL_ADDRESS_SPOOFED"))
					{
						std::shared_ptr<HmDeviceDescription::HomeMaticParameter> parameter(new HmDeviceDescription::HomeMaticParameter(_bl));
						parameter->id = "CENTRAL_ADDRESS_SPOOFED";
						parameter->uiFlags = (HmDeviceDescription::HomeMaticParameter::UIFlags::Enum)(HmDeviceDescription::HomeMaticParameter::UIFlags::Enum::visible | HmDeviceDescription::HomeMaticParameter::UIFlags::Enum::service | HmDeviceDescription::HomeMaticParameter::UIFlags::Enum::sticky);
						parameter->operations = (HmDeviceDescription::HomeMaticParameter::Operations::Enum)(HmDeviceDescription::HomeMaticParameter::Operations::Enum::read | HmDeviceDescription::HomeMaticParameter::Operations::Enum::write | HmDeviceDescription::HomeMaticParameter::Operations::Enum::event);
						parameter->control = "NONE";
						parameter->logicalParameter.reset(new HmDeviceDescription::LogicalParameterEnum(_bl));
						parameter->logicalParameter->type = HmDeviceDescription::LogicalParameter::Type::Enum::typeEnum;
						HmDeviceDescription::LogicalParameterEnum* logical = (HmDeviceDescription::LogicalParameterEnum*)parameter->logicalParameter.get();
						logical->defaultValueExists = true;
						HmDeviceDescription::ParameterOption option;
						option.id = "UNSET";
						option.index = 0;
						option.isDefault = true;
						logical->options.push_back(option);
						option = HmDeviceDescription::ParameterOption();
						option.id = "CENTRAL_ADDRESS_SPOOFED";
						option.index = 1;
						logical->options.push_back(option);
						parameter->physicalParameter->interface = HmDeviceDescription::PhysicalParameter::Interface::Enum::internal;
						parameter->physicalParameter->type = HmDeviceDescription::PhysicalParameter::Type::Enum::typeInteger;
						parameter->physicalParameter->valueID = "CENTRAL_ADDRESS_SPOOFED";
						paramsetIterator->second->parameters.push_back(parameter);
					}
				}
			}
		}
		if(filename == "rf_4dis.xml")
		{
			std::map<std::string, std::shared_ptr<HmDeviceDescription::DeviceFrame>>::iterator frameIterator = homeMaticDevice->framesByID.find("KEY_EVENT_SHORT");
			if(frameIterator != homeMaticDevice->framesByID.end() && frameIterator->second)
			{
				HmDeviceDescription::HomeMaticParameter parameter(_bl);
				parameter.type = HmDeviceDescription::PhysicalParameter::Type::Enum::typeInteger;
				parameter.index = 1.2;
				parameter.size = 0.1;
				parameter.constValue = 1;
				frameIterator->second->parameters.push_front(parameter);
			}

			frameIterator = homeMaticDevice->framesByID.find("KEY_EVENT_LONG");
			if(frameIterator != homeMaticDevice->framesByID.end() && frameIterator->second)
			{
				HmDeviceDescription::HomeMaticParameter parameter(_bl);
				parameter.type = HmDeviceDescription::PhysicalParameter::Type::Enum::typeInteger;
				parameter.index = 1.2;
				parameter.size = 0.1;
				parameter.constValue = 1;
				frameIterator->second->parameters.push_front(parameter);
			}

			frameIterator = homeMaticDevice->framesByID.find("KEY_EVENT_LONG_BIDI");
			if(frameIterator != homeMaticDevice->framesByID.end() && frameIterator->second)
			{
				for(std::list<HmDeviceDescription::HomeMaticParameter>::iterator i = frameIterator->second->parameters.begin(); i != frameIterator->second->parameters.end(); ++i)
				{
					if(i->index == 1.5)
					{
						i->index = 1.2;
						i->constValue = 0;
						break;
					}
				}
			}
		}

		HmDeviceDescription::HmConverter converter(_bl);
		std::shared_ptr<HomegearDevice> device(new HomegearDevice(_bl, _family));
		converter.convert(homeMaticDevice, device);
		return device;
	}
    catch(const std::exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return std::shared_ptr<HomegearDevice>();
}

std::shared_ptr<HomegearDevice> Devices::find(Systems::LogicalDeviceType deviceType, uint32_t firmwareVersion, int32_t countFromSysinfo)
{
	try
	{
		std::shared_ptr<HomegearDevice> partialMatch;
		for(std::vector<std::shared_ptr<HomegearDevice>>::iterator i = _devices.begin(); i != _devices.end(); ++i)
		{
			for(SupportedDevices::iterator j = (*i)->supportedDevices.begin(); j != (*i)->supportedDevices.end(); ++j)
			{
				if((*j)->matches(deviceType, firmwareVersion))
				{
					if(countFromSysinfo > -1 && (*i)->dynamicChannelCountIndex > -1 && (*i)->getDynamicChannelCount() != countFromSysinfo)
					{
						if((*i)->getDynamicChannelCount() == -1) partialMatch = *i;
					}
					else return *i;
				}
			}
		}
		if(partialMatch)
		{
			std::shared_ptr<HomegearDevice> newDevice(new HomegearDevice(_bl, _family));
			*newDevice = *partialMatch;
			newDevice->setDynamicChannelCount(countFromSysinfo);
			_devices.push_back(newDevice);
			return newDevice;
		}
	}
	catch(const std::exception& ex)
    {
     _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(const Exception& ex)
    {
     _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
     _bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return nullptr;
}

}
}
