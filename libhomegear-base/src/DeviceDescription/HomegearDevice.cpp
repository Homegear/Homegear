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

#include "HomegearDevice.h"
#include "../BaseLib.h"
#include "../Encoding/RapidXml/rapidxml_print.hpp"

namespace BaseLib
{
namespace DeviceDescription
{

int32_t HomegearDevice::getDynamicChannelCount()
{
	return _dynamicChannelCount;
}

void HomegearDevice::setDynamicChannelCount(int32_t value)
{
	try
	{
		_dynamicChannelCount = value;
		PFunction function;
		uint32_t index = 0;
		for(Functions::iterator i = functions.begin(); i != functions.end(); ++i)
		{
			if(!i->second) continue;
			if(i->second->dynamicChannelCountIndex > -1)
			{
				index = i->first;
				function = i->second;
				break;
			}
		}

		for(uint32_t i = index + 1; i < index + value; i++)
		{
			if(functions.find(i) == functions.end())
			{
				functions[i] = function;
				for(Parameters::iterator j = function->variables->parameters.begin(); j != function->variables->parameters.end(); ++j)
				{
					for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->getPackets.begin(); k != j->second->getPackets.end(); ++k)
					{
						PacketsById::iterator packet = packetsById.find((*k)->id);
						if(packet != packetsById.end())
						{
							valueRequestPackets[i][(*k)->id] = packet->second;
						}
					}
				}
			}
			else _bl->out.printError("Error: Tried to add channel with the same index twice. Index: " + std::to_string(i));
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
}

HomegearDevice::HomegearDevice(BaseLib::Obj* baseLib, Systems::DeviceFamilies deviceFamily)
{
	_bl = baseLib;
	family = deviceFamily;
	runProgram.reset(new RunProgram(baseLib));
}

HomegearDevice::HomegearDevice(BaseLib::Obj* baseLib, Systems::DeviceFamilies deviceFamily, xml_node<>* node) : HomegearDevice(baseLib, deviceFamily)
{
	if(node) parseXML(node);
}

HomegearDevice::HomegearDevice(BaseLib::Obj* baseLib, Systems::DeviceFamilies deviceFamily, std::string xmlFilename, bool& oldFormat) : HomegearDevice(baseLib, deviceFamily)
{
	try
	{
		load(xmlFilename, oldFormat);

		if(!_loaded) return;
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

HomegearDevice::~HomegearDevice() {

}

void HomegearDevice::load(std::string xmlFilename, bool& oldFormat)
{
	xml_document<> doc;
	try
	{
		std::ifstream fileStream(xmlFilename, std::ios::in | std::ios::binary);
		if(fileStream)
		{
			uint32_t length;
			fileStream.seekg(0, std::ios::end);
			length = fileStream.tellg();
			fileStream.seekg(0, std::ios::beg);
			char buffer[length + 1];
			fileStream.read(&buffer[0], length);
			fileStream.close();
			buffer[length] = '\0';
			doc.parse<parse_no_entity_translation | parse_validate_closing_tags>(buffer);
			if(doc.first_node("device"))
			{
				oldFormat = true;
				doc.clear();
				return;
			}
			else if(!doc.first_node("homegearDevice"))
			{
				_bl->out.printError("Error: Device XML file \"" + xmlFilename + "\" does not start with \"homegearDevice\".");
				doc.clear();
				return;
			}
			parseXML(doc.first_node("homegearDevice"));
		}
		else _bl->out.printError("Error reading file " + xmlFilename + ": " + strerror(errno));
		_loaded = true;
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
    doc.clear();
}

void HomegearDevice::safe(std::string& filename)
{
	try
	{
		xml_document<> doc;

		if(Io::fileExists(filename))
		{
			if(!Io::deleteFile(filename))
			{
				_bl->out.printError("Error: File \"" + filename + "\" already exists and cannot be deleted.");
				return;
			}
		}

		std::vector<std::string> tempStrings; //We need to store all newly created strings as RapidXml doesn't copy strings.

		xml_node<>* homegearDevice = doc.allocate_node(node_element, "homegearDevice");
		doc.append_node(homegearDevice);

		tempStrings.push_back(std::to_string(version));
		xml_attribute<>* versionAttr = doc.allocate_attribute("version", tempStrings.back().c_str());
		homegearDevice->append_attribute(versionAttr);

		for(SupportedDevices::iterator i = supportedDevices.begin(); i != supportedDevices.end(); ++i)
		{
			xml_node<>* node = doc.allocate_node(node_element, "device");
			homegearDevice->append_node(node);

			xml_attribute<>* attr = doc.allocate_attribute("id", (*i)->id.c_str());
			node->append_attribute(attr);

			xml_node<>* subnode = doc.allocate_node(node_element, "description", (*i)->description.c_str());
			node->append_node(subnode);

			tempStrings.push_back(std::to_string((*i)->typeNumber));
			subnode = doc.allocate_node(node_element, "typeNumber", tempStrings.back().c_str());
			node->append_node(subnode);

			if((*i)->minFirmwareVersion != 0)
			{
				tempStrings.push_back(std::to_string((*i)->minFirmwareVersion));
				subnode = doc.allocate_node(node_element, "minFirmwareVersion", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if((*i)->maxFirmwareVersion != 0)
			{
				tempStrings.push_back(std::to_string((*i)->maxFirmwareVersion));
				subnode = doc.allocate_node(node_element, "maxFirmwareVersion", tempStrings.back().c_str());
				node->append_node(subnode);
			}
		}

		std::ofstream fileStream(filename, std::ios::out | std::ios::binary);
		if(fileStream)
		{
			fileStream << doc;
		}
		fileStream.close();
		doc.clear();
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

void HomegearDevice::parseXML(xml_node<>* node)
{
	try
	{
		for(xml_attribute<>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
		{
			std::string attributeName(attr->name());
			std::string attributeValue(attr->value());
			if(attributeName == "version") version = Math::getNumber(attributeValue);
			else if(attributeName == "xmlns") {}
			else _bl->out.printWarning("Warning: Unknown attribute for \"homegearDevice\": " + std::string(attr->name()));
		}
		std::map<std::string, PConfigParameters> configParameters;
		std::map<std::string, PVariables> variables;
		std::map<std::string, PLinkParameters> linkParameters;
		for(xml_node<>* subNode = node->first_node(); subNode; subNode = subNode->next_sibling())
		{
			std::string nodeName(subNode->name());
			if(nodeName == "supportedDevices")
			{
				for(xml_node<>* typeNode = subNode->first_node("device"); typeNode; typeNode = typeNode->next_sibling("device"))
				{
					PSupportedDevice supportedDevice(new SupportedDevice(_bl, typeNode, this));
					supportedDevices.push_back(supportedDevice);
				}
			}
			else if(nodeName == "runProgram")
			{
				runProgram.reset(new RunProgram(_bl, subNode));
			}
			else if(nodeName == "properties")
			{
				for(xml_node<>* propertyNode = subNode->first_node(); propertyNode; propertyNode = propertyNode->next_sibling())
				{
					std::string propertyName(propertyNode->name());
					std::string propertyValue(propertyNode->value());
					if(propertyName == "receiveMode")
					{
						if(propertyValue == "always") receiveModes = (ReceiveModes::Enum)(receiveModes | ReceiveModes::Enum::always);
						else if(propertyValue == "wakeOnRadio") receiveModes = (ReceiveModes::Enum)(receiveModes | ReceiveModes::Enum::wakeOnRadio);
						else if(propertyValue == "config") receiveModes = (ReceiveModes::Enum)(receiveModes | ReceiveModes::Enum::config);
						else if(propertyValue == "wakeUp") receiveModes = (ReceiveModes::Enum)(receiveModes | ReceiveModes::Enum::wakeUp);
						else if(propertyValue == "wakeUp2") receiveModes = (ReceiveModes::Enum)(receiveModes | ReceiveModes::Enum::wakeUp2);
						else if(propertyValue == "lazyConfig") receiveModes = (ReceiveModes::Enum)(receiveModes | ReceiveModes::Enum::lazyConfig);
						else _bl->out.printWarning("Warning: Unknown receiveMode: " + propertyValue);
					}
					else if(propertyName == "encryption") { if(propertyValue == "true") encryption = true; }
					else if(propertyName == "timeout") timeout = Math::getUnsignedNumber(propertyValue);
					else if(propertyName == "memorySize") memorySize = Math::getUnsignedNumber(propertyValue);
					else if(propertyName == "visible")  { if(propertyValue == "false") visible = false; }
					else if(propertyName == "deletable") { if(propertyValue == "false") deletable = false; }
					else if(propertyName == "internal") { if(propertyValue == "true") internal = true; }
					else if(propertyName == "needsTime") { if(propertyValue == "true") needsTime = true; }
					else if(propertyName == "hasBattery") { if(propertyValue == "true") hasBattery = true; }
					else _bl->out.printWarning("Warning: Unknown device property: " + propertyName);
				}
				if(!(receiveModes & ReceiveModes::Enum::always)) hasBattery = true;
			}
			else if(nodeName == "functions")
			{
				for(xml_node<>* functionNode = subNode->first_node("function"); functionNode; functionNode = functionNode->next_sibling("function"))
				{
					uint32_t channel = 0;
					PFunction function(new Function(_bl, functionNode, channel));
					for(uint32_t i = channel; i < channel + function->channelCount; i++)
					{
						if(functions.find(i) == functions.end()) functions[i] = function;
						else _bl->out.printError("Error: Tried to add function with the same channel twice. Channel: " + std::to_string(i));
					}
					if(function->dynamicChannelCountIndex > -1)
					{
						if(dynamicChannelCountIndex > -1) _bl->out.printError("Error: dynamicChannelCount is defined for two channels. That is not allowed.");
						dynamicChannelCountIndex = function->dynamicChannelCountIndex;
						dynamicChannelCountSize = function->dynamicChannelCountSize;
						if(dynamicChannelCountIndex < -1) dynamicChannelCountIndex = -1;
						if(dynamicChannelCountSize <= 0) dynamicChannelCountSize = 1;
					}
				}
			}
			else if(nodeName == "parameterGroups")
			{
				for(xml_node<>* parameterGroupNode = subNode->first_node(); parameterGroupNode; parameterGroupNode = parameterGroupNode->next_sibling())
				{
					std::string parameterGroupName(parameterGroupNode->name());
					if(parameterGroupName == "configParameters")
					{
						PConfigParameters config(new ConfigParameters(_bl, nullptr, parameterGroupNode));
						configParameters[config->id] = config;
					}
					else if(parameterGroupName == "variables")
					{
						PVariables config(new Variables(_bl, nullptr, parameterGroupNode));
						variables[config->id] = config;
					}
					else if(parameterGroupName == "linkParameters")
					{
						PLinkParameters config(new LinkParameters(_bl, nullptr, parameterGroupNode));
						linkParameters[config->id] = config;
					}
					else _bl->out.printWarning("Warning: Unknown parameter group: " + parameterGroupName);
				}
			}
			else if(nodeName == "packets")
			{
				for(xml_node<>* packetNode = subNode->first_node("packet"); packetNode; packetNode = packetNode->next_sibling("packet"))
				{
					PPacket packet(new Packet(_bl, packetNode));

					packetsByMessageType.insert(std::pair<uint32_t, PPacket>(packet->type, packet));
					packetsById[packet->id] = packet;
					if(!packet->function1.empty()) packetsByFunction1.insert(std::pair<std::string, PPacket>(packet->function1, packet));
					if(!packet->function2.empty()) packetsByFunction2.insert(std::pair<std::string, PPacket>(packet->function2, packet));
				}
			}
			else if(nodeName == "group")
			{
				group.reset(new HomegearDevice(_bl, family, subNode));
			}
			else _bl->out.printWarning("Warning: Unknown node name for \"homegearDevice\": " + nodeName);
		}
		for(Functions::iterator i = functions.begin(); i != functions.end(); ++i)
		{
			if(!i->second->configParametersId.empty())
			{
				std::map<std::string, PConfigParameters>::iterator configIterator = configParameters.find(i->second->configParametersId);
				if(configIterator == configParameters.end())
				{
					_bl->out.printWarning("Warning: configParameters with id \"" + i->second->configParametersId + "\" does not exist.");
					continue;
				}
				i->second->configParameters = configIterator->second;
				configIterator->second->setParent(i->second.get());
			}
			if(!i->second->variablesId.empty())
			{
				std::map<std::string, PVariables>::iterator configIterator = variables.find(i->second->variablesId);
				if(configIterator == variables.end())
				{
					_bl->out.printWarning("Warning: variables with id \"" + i->second->variablesId + "\" does not exist.");
					continue;
				}
				i->second->variables = configIterator->second;
				configIterator->second->setParent(i->second.get());

				for(Parameters::iterator j = i->second->variables->parameters.begin(); j != i->second->variables->parameters.end(); ++j)
				{
					for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->getPackets.begin(); k != j->second->getPackets.end(); ++k)
					{
						PacketsById::iterator packetIterator = packetsById.find((*k)->id);
						if(packetIterator != packetsById.end())
						{
							packetIterator->second->associatedVariables.push_back(j->second);
							valueRequestPackets[i->first][(*k)->id] = packetIterator->second;
						}
					}
					for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->setPackets.begin(); k != j->second->setPackets.end(); ++k)
					{
						PacketsById::iterator packetIterator = packetsById.find((*k)->id);
						if(packetIterator != packetsById.end()) packetIterator->second->associatedVariables.push_back(j->second);
					}
					for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->eventPackets.begin(); k != j->second->eventPackets.end(); ++k)
					{
						PacketsById::iterator packetIterator = packetsById.find((*k)->id);
						if(packetIterator != packetsById.end()) packetIterator->second->associatedVariables.push_back(j->second);
					}
				}
			}
			if(!i->second->linkParametersId.empty())
			{
				std::map<std::string, PLinkParameters>::iterator configIterator = linkParameters.find(i->second->linkParametersId);
				if(configIterator == linkParameters.end())
				{
					_bl->out.printWarning("Warning: linkParameters with id \"" + i->second->linkParametersId + "\" does not exist.");
					continue;
				}
				i->second->linkParameters = configIterator->second;
				configIterator->second->setParent(i->second.get());
			}
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
}

PSupportedDevice HomegearDevice::getType(Systems::LogicalDeviceType deviceType, int32_t firmwareVersion)
{
	try
	{
		for(SupportedDevices::iterator j = supportedDevices.begin(); j != supportedDevices.end(); ++j)
		{
			if((*j)->matches(deviceType, firmwareVersion)) return *j;
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
	return PSupportedDevice();
}

}
}
