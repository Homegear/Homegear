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

		PParameter parameter(new Parameter(_bl, nullptr));
		parameter->id = "PAIRED_TO_CENTRAL";
		parameter->visible = false;
		parameter->logical.reset(new LogicalBoolean(_bl));
		parameter->logical->type = ILogical::Type::Enum::tBoolean;
		parameter->physical.reset(new PhysicalBoolean(_bl));
		parameter->physical->operationType = IPhysical::OperationType::Enum::internal;
		parameter->physical->type = IPhysical::Type::Enum::tBoolean;
		parameter->physical->groupId = "PAIRED_TO_CENTRAL";
		parameter->physical->list = 0;
		parameter->physical->index = 2;
		functions[0]->configParameters->parameters[parameter->id] = parameter;

		parameter.reset(new Parameter(_bl, nullptr));
		parameter->id = "CENTRAL_ADDRESS_BYTE_1";
		parameter->visible = false;
		parameter->logical.reset(new LogicalInteger(_bl));
		parameter->logical->type = ILogical::Type::Enum::tInteger;
		parameter->physical.reset(new PhysicalInteger(_bl));
		parameter->physical->operationType = IPhysical::OperationType::Enum::internal;
		parameter->physical->type = IPhysical::Type::Enum::tInteger;
		parameter->physical->groupId = "CENTRAL_ADDRESS_BYTE_1";
		parameter->physical->list = 0;
		parameter->physical->index = 10;
		functions[0]->configParameters->parameters[parameter->id] = parameter;

		parameter.reset(new Parameter(_bl, nullptr));
		parameter->id = "CENTRAL_ADDRESS_BYTE_2";
		parameter->visible = false;
		parameter->logical.reset(new LogicalInteger(_bl));
		parameter->logical->type = ILogical::Type::Enum::tInteger;
		parameter->physical.reset(new PhysicalInteger(_bl));
		parameter->physical->operationType = IPhysical::OperationType::Enum::internal;
		parameter->physical->type = IPhysical::Type::Enum::tInteger;
		parameter->physical->groupId = "CENTRAL_ADDRESS_BYTE_2";
		parameter->physical->list = 0;
		parameter->physical->index = 11;
		functions[0]->configParameters->parameters[parameter->id] = parameter;

		parameter.reset(new Parameter(_bl, nullptr));
		parameter->id = "CENTRAL_ADDRESS_BYTE_3";
		parameter->visible = false;
		parameter->logical.reset(new LogicalInteger(_bl));
		parameter->logical->type = ILogical::Type::Enum::tInteger;
		parameter->physical.reset(new PhysicalInteger(_bl));
		parameter->physical->operationType = IPhysical::OperationType::Enum::internal;
		parameter->physical->type = IPhysical::Type::Enum::tInteger;
		parameter->physical->groupId = "CENTRAL_ADDRESS_BYTE_3";
		parameter->physical->list = 0;
		parameter->physical->index = 12;
		functions[0]->configParameters->parameters[parameter->id] = parameter;

		if(!encryption) return;
		//For HomeMatic BidCoS: Set AES default value to "false", so a new AES key is set on pairing
		for(Functions::iterator i = functions.begin(); i != functions.end(); ++i)
		{
			if(!i->second || i->first == 0) continue;
			parameter = i->second->configParameters->getParameter("AES_ACTIVE");
			if(!parameter)
			{
				parameter.reset(new Parameter(_bl, nullptr));
				i->second->configParameters->parameters["AES_ACTIVE"] = parameter;
			}
			parameter->id = "AES_ACTIVE";
			parameter->internal = true;
			parameter->casts.clear();
			parameter->casts.push_back(std::shared_ptr<BooleanInteger>(new BooleanInteger(_bl)));
			parameter->logical.reset(new LogicalBoolean(_bl));
			parameter->logical->defaultValueExists = true;
			parameter->physical->operationType = IPhysical::OperationType::Enum::config;
			parameter->physical->type = IPhysical::Type::Enum::tInteger;
			parameter->physical->groupId = "AES_ACTIVE";
			parameter->physical->list = 1;
			parameter->physical->index = 8;
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
    doc.clear();
}

void HomegearDevice::save(std::string& filename)
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

		saveDevice(&doc, homegearDevice, this, tempStrings);

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

void HomegearDevice::saveDevice(xml_document<>* doc, xml_node<>* parentNode, HomegearDevice* device, std::vector<std::string>& tempStrings)
{
	try
	{
		tempStrings.push_back(std::to_string(device->version));
		xml_attribute<>* versionAttr = doc->allocate_attribute("version", tempStrings.back().c_str());
		parentNode->append_attribute(versionAttr);

		xml_node<>* node = doc->allocate_node(node_element, "supportedDevices");
		parentNode->append_node(node);
		for(SupportedDevices::iterator i = device->supportedDevices.begin(); i != device->supportedDevices.end(); ++i)
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "device");
			node->append_node(subnode);

			xml_attribute<>* attr = doc->allocate_attribute("id", (*i)->id.c_str());
			subnode->append_attribute(attr);

			xml_node<>* deviceNode = doc->allocate_node(node_element, "description", (*i)->description.c_str());
			subnode->append_node(deviceNode);

			if((*i)->typeNumber != (uint32_t)-1)
			{
				tempStrings.push_back("0x" + BaseLib::HelperFunctions::getHexString((*i)->typeNumber));
				deviceNode = doc->allocate_node(node_element, "typeNumber", tempStrings.back().c_str());
				subnode->append_node(deviceNode);
			}

			if((*i)->minFirmwareVersion != 0)
			{
				tempStrings.push_back("0x" + BaseLib::HelperFunctions::getHexString((*i)->minFirmwareVersion));
				deviceNode = doc->allocate_node(node_element, "minFirmwareVersion", tempStrings.back().c_str());
				subnode->append_node(deviceNode);
			}

			if((*i)->maxFirmwareVersion != 0)
			{
				tempStrings.push_back("0x" + BaseLib::HelperFunctions::getHexString((*i)->maxFirmwareVersion));
				deviceNode = doc->allocate_node(node_element, "maxFirmwareVersion", tempStrings.back().c_str());
				subnode->append_node(deviceNode);
			}
		}

		if(device->runProgram && !device->runProgram->path.empty())
		{
			node = doc->allocate_node(node_element, "runProgram");
			parentNode->append_node(node);

			xml_node<>* subnode = doc->allocate_node(node_element, "path", device->runProgram->path.c_str());
			node->append_node(subnode);

			if(device->runProgram->arguments.size() > 0)
			{
				subnode = doc->allocate_node(node_element, "arguments");
				node->append_node(subnode);
				for(std::vector<std::string>::iterator i = device->runProgram->arguments.begin(); i != device->runProgram->arguments.end(); ++i)
				{
					xml_node<>* argumentnode = doc->allocate_node(node_element, "argument", i->c_str());
					subnode->append_node(argumentnode);
				}
			}

			tempStrings.push_back(device->runProgram->startType == RunProgram::StartType::Enum::once ? "once" : (device->runProgram->startType == RunProgram::StartType::Enum::interval ? "interval" : "permanent"));
			subnode = doc->allocate_node(node_element, "startType", tempStrings.back().c_str());
			node->append_node(subnode);

			if(device->runProgram->interval > 0)
			{
				tempStrings.push_back(std::to_string(device->runProgram->interval));
				subnode = doc->allocate_node(node_element, "interval", tempStrings.back().c_str());
				node->append_node(subnode);
			}
		}

		// {{{ Properties
			node = doc->allocate_node(node_element, "properties");
			parentNode->append_node(node);

			if(device->receiveModes != ReceiveModes::Enum::always)
			{
				if(device->receiveModes & ReceiveModes::Enum::always)
				{
					tempStrings.push_back("always");
					xml_node<>* subnode = doc->allocate_node(node_element, "receiveMode", tempStrings.back().c_str());
					node->append_node(subnode);
				}
				if(device->receiveModes & ReceiveModes::Enum::wakeOnRadio)
				{
					tempStrings.push_back("wakeOnRadio");
					xml_node<>* subnode = doc->allocate_node(node_element, "receiveMode", tempStrings.back().c_str());
					node->append_node(subnode);
				}
				if(device->receiveModes & ReceiveModes::Enum::config)
				{
					tempStrings.push_back("config");
					xml_node<>* subnode = doc->allocate_node(node_element, "receiveMode", tempStrings.back().c_str());
					node->append_node(subnode);
				}
				if(device->receiveModes & ReceiveModes::Enum::wakeUp)
				{
					tempStrings.push_back("wakeUp");
					xml_node<>* subnode = doc->allocate_node(node_element, "receiveMode", tempStrings.back().c_str());
					node->append_node(subnode);
				}
				if(device->receiveModes & ReceiveModes::Enum::wakeUp2)
				{
					tempStrings.push_back("wakeUp2");
					xml_node<>* subnode = doc->allocate_node(node_element, "receiveMode", tempStrings.back().c_str());
					node->append_node(subnode);
				}
				if(device->receiveModes & ReceiveModes::Enum::lazyConfig)
				{
					tempStrings.push_back("lazyConfig");
					xml_node<>* subnode = doc->allocate_node(node_element, "receiveMode", tempStrings.back().c_str());
					node->append_node(subnode);
				}
			}

			if(device->encryption)
			{
				tempStrings.push_back("true");
				xml_node<>* subnode = doc->allocate_node(node_element, "encryption", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(device->timeout > 0)
			{
				tempStrings.push_back(std::to_string(device->timeout));
				xml_node<>* subnode = doc->allocate_node(node_element, "timeout", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(device->memorySize != 1024)
			{
				tempStrings.push_back(std::to_string(device->memorySize));
				xml_node<>* subnode = doc->allocate_node(node_element, "memorySize", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(!device->visible)
			{
				tempStrings.push_back("false");
				xml_node<>* subnode = doc->allocate_node(node_element, "visible", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(!device->deletable)
			{
				tempStrings.push_back("false");
				xml_node<>* subnode = doc->allocate_node(node_element, "deletable", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(device->internal)
			{
				tempStrings.push_back("true");
				xml_node<>* subnode = doc->allocate_node(node_element, "internal", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(device->needsTime)
			{
				tempStrings.push_back("true");
				xml_node<>* subnode = doc->allocate_node(node_element, "needsTime", tempStrings.back().c_str());
				node->append_node(subnode);
			}

			if(device->hasBattery)
			{
				tempStrings.push_back("true");
				xml_node<>* subnode = doc->allocate_node(node_element, "hasBattery", tempStrings.back().c_str());
				node->append_node(subnode);
			}
		// }}}

		std::map<std::string, PConfigParameters> configParameters;
		std::map<std::string, PVariables> variables;
		std::map<std::string, PLinkParameters> linkParameters;

		node = doc->allocate_node(node_element, "functions");
		parentNode->append_node(node);
		int32_t skip = 0;
		for(Functions::iterator i = device->functions.begin(); i != device->functions.end(); i++)
		{
			if(skip == 0) skip = i->second->channelCount - 1;
			else if(skip > 0)
			{
				skip--;
				continue;
			}
			xml_node<>* subnode = doc->allocate_node(node_element, "function");
			node->append_node(subnode);
			saveFunction(doc, subnode, i->second, tempStrings, configParameters, variables, linkParameters);
		}

		// {{{ Packets
		node = doc->allocate_node(node_element, "packets");
		parentNode->append_node(node);
		for(PacketsById::iterator i = device->packetsById.begin(); i != device->packetsById.end(); ++i)
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "packet");
			node->append_node(subnode);

			xml_attribute<>* attr = doc->allocate_attribute("id", i->first.c_str());
			subnode->append_attribute(attr);

			tempStrings.push_back(i->second->direction == Packet::Direction::Enum::fromCentral ? "fromCentral" : "toCentral");
			xml_node<>* packetNode = doc->allocate_node(node_element, "direction", tempStrings.back().c_str());
			subnode->append_node(packetNode);

			if(i->second->length != -1)
			{
				tempStrings.push_back(std::to_string(i->second->length));
				packetNode = doc->allocate_node(node_element, "length", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->type != -1)
			{
				tempStrings.push_back("0x" + BaseLib::HelperFunctions::getHexString(i->second->type));
				packetNode = doc->allocate_node(node_element, "type", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->subtype != -1)
			{
				tempStrings.push_back("0x" + BaseLib::HelperFunctions::getHexString(i->second->subtype));
				packetNode = doc->allocate_node(node_element, "subtype", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->subtypeIndex != -1)
			{
				tempStrings.push_back((i->second->subtypeSize == 1.0 || i->second->subtypeSize == -1) ? std::to_string(i->second->subtypeIndex) : std::to_string(i->second->subtypeIndex) + ':' + Math::toString(i->second->subtypeSize, 1));
				packetNode = doc->allocate_node(node_element, "subtypeIndex", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(!i->second->function1.empty())
			{
				packetNode = doc->allocate_node(node_element, "function1", i->second->function1.c_str());
				subnode->append_node(packetNode);
			}

			if(!i->second->function2.empty())
			{
				packetNode = doc->allocate_node(node_element, "function2", i->second->function2.c_str());
				subnode->append_node(packetNode);
			}

			if(!i->second->metaString1.empty())
			{
				packetNode = doc->allocate_node(node_element, "metaString1", i->second->metaString1.c_str());
				subnode->append_node(packetNode);
			}

			if(!i->second->metaString2.empty())
			{
				packetNode = doc->allocate_node(node_element, "metaString2", i->second->metaString2.c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->responseType != -1)
			{
				tempStrings.push_back(std::to_string(i->second->responseType));
				packetNode = doc->allocate_node(node_element, "responseType", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->responseSubtype != -1)
			{
				tempStrings.push_back(std::to_string(i->second->responseSubtype));
				packetNode = doc->allocate_node(node_element, "responseSubtype", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->channel != -1)
			{
				tempStrings.push_back(std::to_string(i->second->channel));
				packetNode = doc->allocate_node(node_element, "channel", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->channelIndex != -1)
			{
				tempStrings.push_back(i->second->channelSize == 1.0 ? std::to_string(i->second->channelIndex) : std::to_string(i->second->channelIndex) + ':' + Math::toString(i->second->channelSize, 1));
				packetNode = doc->allocate_node(node_element, "channelIndex", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->doubleSend)
			{
				tempStrings.push_back("true");
				packetNode = doc->allocate_node(node_element, "doubleSend", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->splitAfter != -1)
			{
				tempStrings.push_back(std::to_string(i->second->splitAfter));
				packetNode = doc->allocate_node(node_element, "splitAfter", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(i->second->maxPackets != -1)
			{
				tempStrings.push_back(std::to_string(i->second->maxPackets));
				packetNode = doc->allocate_node(node_element, "maxPackets", tempStrings.back().c_str());
				subnode->append_node(packetNode);
			}

			if(!i->second->binaryPayloads.empty())
			{
				packetNode = doc->allocate_node(node_element, "binaryPayload");
				subnode->append_node(packetNode);

				for(BinaryPayloads::iterator j = i->second->binaryPayloads.begin(); j != i->second->binaryPayloads.end(); ++j)
				{
					xml_node<>* payloadNode = doc->allocate_node(node_element, "element");
					packetNode->append_node(payloadNode);

					if((*j)->index != 0)
					{
						tempStrings.push_back(Math::toString((*j)->index, 1));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "index", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->index != 0 && (*j)->size != 1.0)
					{
						tempStrings.push_back(Math::toString((*j)->size, 1));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "size", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->index2 != 0)
					{
						tempStrings.push_back(Math::toString((*j)->index2, 1));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "index2", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->index2 != 0 && (*j)->size2 != 0)
					{
						tempStrings.push_back(Math::toString((*j)->size2, 1));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "size2", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->index2Offset != -1)
					{
						tempStrings.push_back(std::to_string((*j)->index2Offset));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "index2Offset", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if(!(*j)->parameterId.empty())
					{
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "parameterId", (*j)->parameterId.c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueInteger != -1)
					{
						tempStrings.push_back(std::to_string((*j)->constValueInteger));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueInteger", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueDecimal != -1)
					{
						tempStrings.push_back(std::to_string((*j)->constValueDecimal));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueDecimal", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if(!(*j)->constValueString.empty())
					{
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueString", (*j)->constValueString.c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->isSigned)
					{
						tempStrings.push_back("true");
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "isSigned", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->omitIfSet)
					{
						tempStrings.push_back(std::to_string((*j)->omitIf));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "omitIf", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}
				}
			}

			if(!i->second->jsonPayloads.empty())
			{
				packetNode = doc->allocate_node(node_element, "jsonPayload");
				subnode->append_node(packetNode);

				for(JsonPayloads::iterator j = i->second->jsonPayloads.begin(); j != i->second->jsonPayloads.end(); ++j)
				{
					xml_node<>* payloadNode = doc->allocate_node(node_element, "element");
					packetNode->append_node(payloadNode);

					xml_node<>* payloadElementNode = doc->allocate_node(node_element, "key", (*j)->key.c_str());
					payloadNode->append_node(payloadElementNode);

					if(!(*j)->subkey.empty())
					{
						payloadElementNode = doc->allocate_node(node_element, "subkey", (*j)->subkey.c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if(!(*j)->parameterId.empty())
					{
						payloadElementNode = doc->allocate_node(node_element, "parameterId", (*j)->parameterId.c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueBooleanSet)
					{
						tempStrings.push_back((*j)->constValueBoolean ? "true" : "false");
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueBoolean", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueIntegerSet)
					{
						tempStrings.push_back(std::to_string((*j)->constValueInteger));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueInteger", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueDecimalSet)
					{
						tempStrings.push_back(std::to_string((*j)->constValueDecimal));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueDecimal", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if(!(*j)->constValueStringSet)
					{
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueString", (*j)->constValueString.c_str());
						payloadNode->append_node(payloadElementNode);
					}
				}
			}

			if(!i->second->httpPayloads.empty())
			{
				packetNode = doc->allocate_node(node_element, "httpPayload");
				subnode->append_node(packetNode);

				for(HttpPayloads::iterator j = i->second->httpPayloads.begin(); j != i->second->httpPayloads.end(); ++j)
				{
					xml_node<>* payloadNode = doc->allocate_node(node_element, "element");
					packetNode->append_node(payloadNode);

					xml_node<>* payloadElementNode = doc->allocate_node(node_element, "key", (*j)->key.c_str());
					payloadNode->append_node(payloadElementNode);

					if(!(*j)->parameterId.empty())
					{
						payloadElementNode = doc->allocate_node(node_element, "parameterId", (*j)->parameterId.c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueBooleanSet)
					{
						tempStrings.push_back((*j)->constValueBoolean ? "true" : "false");
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueBoolean", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueIntegerSet)
					{
						tempStrings.push_back(std::to_string((*j)->constValueInteger));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueInteger", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if((*j)->constValueDecimalSet)
					{
						tempStrings.push_back(std::to_string((*j)->constValueDecimal));
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueDecimal", tempStrings.back().c_str());
						payloadNode->append_node(payloadElementNode);
					}

					if(!(*j)->constValueStringSet)
					{
						xml_node<>* payloadElementNode = doc->allocate_node(node_element, "constValueString", (*j)->constValueString.c_str());
						payloadNode->append_node(payloadElementNode);
					}
				}
			}
		}
		/// }}}

		/// {{{ ParameterGroups
		node = doc->allocate_node(node_element, "parameterGroups");
		parentNode->append_node(node);

		for(std::map<std::string, PConfigParameters>::iterator i = configParameters.begin(); i != configParameters.end(); ++i)
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "configParameters");
			node->append_node(subnode);

			xml_attribute<>* attr = doc->allocate_attribute("id", i->first.c_str());
			subnode->append_attribute(attr);

			if(i->second->memoryAddressStart != -1)
			{
				tempStrings.push_back(std::to_string(i->second->memoryAddressStart));
				attr = doc->allocate_attribute("memoryAddressStart", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->memoryAddressStep != -1)
			{
				tempStrings.push_back(std::to_string(i->second->memoryAddressStep));
				attr = doc->allocate_attribute("memoryAddressStep", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			for(std::vector<PParameter>::iterator j = i->second->parametersOrdered.begin(); j != i->second->parametersOrdered.end(); ++j)
			{
				xml_node<>* parameterNode = doc->allocate_node(node_element, "parameter");
				subnode->append_node(parameterNode);
				saveParameter(doc, parameterNode, (*j), tempStrings);
			}

			for(Scenarios::iterator j = i->second->scenarios.begin(); j != i->second->scenarios.end(); ++j)
			{
				xml_node<>* parameterNode = doc->allocate_node(node_element, "scenario");
				subnode->append_node(parameterNode);
				saveScenario(doc, parameterNode, j->second, tempStrings);
			}
		}

		for(std::map<std::string, PVariables>::iterator i = variables.begin(); i != variables.end(); ++i)
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "variables");
			node->append_node(subnode);

			xml_attribute<>* attr = doc->allocate_attribute("id", i->first.c_str());
			subnode->append_attribute(attr);

			if(i->second->memoryAddressStart != -1)
			{
				tempStrings.push_back(std::to_string(i->second->memoryAddressStart));
				attr = doc->allocate_attribute("memoryAddressStart", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->memoryAddressStep != -1)
			{
				tempStrings.push_back(std::to_string(i->second->memoryAddressStep));
				attr = doc->allocate_attribute("memoryAddressStep", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			for(std::vector<PParameter>::iterator j = i->second->parametersOrdered.begin(); j != i->second->parametersOrdered.end(); ++j)
			{
				xml_node<>* parameterNode = doc->allocate_node(node_element, "parameter");
				subnode->append_node(parameterNode);
				saveParameter(doc, parameterNode, (*j), tempStrings);
			}

			for(Scenarios::iterator j = i->second->scenarios.begin(); j != i->second->scenarios.end(); ++j)
			{
				xml_node<>* parameterNode = doc->allocate_node(node_element, "scenario");
				subnode->append_node(parameterNode);
				saveScenario(doc, parameterNode, j->second, tempStrings);
			}
		}

		for(std::map<std::string, PLinkParameters>::iterator i = linkParameters.begin(); i != linkParameters.end(); ++i)
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "linkParameters");
			node->append_node(subnode);

			xml_attribute<>* attr = doc->allocate_attribute("id", i->first.c_str());
			subnode->append_attribute(attr);

			if(i->second->memoryAddressStart != -1)
			{
				tempStrings.push_back(std::to_string(i->second->memoryAddressStart));
				attr = doc->allocate_attribute("memoryAddressStart", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->memoryAddressStep != -1)
			{
				tempStrings.push_back(std::to_string(i->second->memoryAddressStep));
				attr = doc->allocate_attribute("memoryAddressStep", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->peerChannelMemoryOffset != -1)
			{
				tempStrings.push_back(std::to_string(i->second->peerChannelMemoryOffset));
				attr = doc->allocate_attribute("peerChannelMemoryOffset", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->channelMemoryOffset != -1)
			{
				tempStrings.push_back(std::to_string(i->second->channelMemoryOffset));
				attr = doc->allocate_attribute("channelMemoryOffset", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->peerAddressMemoryOffset != -1)
			{
				tempStrings.push_back(std::to_string(i->second->peerAddressMemoryOffset));
				attr = doc->allocate_attribute("peerAddressMemoryOffset", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			if(i->second->maxLinkCount != -1)
			{
				tempStrings.push_back(std::to_string(i->second->maxLinkCount));
				attr = doc->allocate_attribute("maxLinkCount", tempStrings.back().c_str());
				subnode->append_attribute(attr);
			}

			for(std::vector<PParameter>::iterator j = i->second->parametersOrdered.begin(); j != i->second->parametersOrdered.end(); ++j)
			{
				xml_node<>* parameterNode = doc->allocate_node(node_element, "parameter");
				subnode->append_node(parameterNode);
				saveParameter(doc, parameterNode, (*j), tempStrings);
			}

			for(Scenarios::iterator j = i->second->scenarios.begin(); j != i->second->scenarios.end(); ++j)
			{
				xml_node<>* parameterNode = doc->allocate_node(node_element, "scenario");
				subnode->append_node(parameterNode);
				saveScenario(doc, parameterNode, j->second, tempStrings);
			}
		}
		/// }}}


		if(device->group)
		{
			node = doc->allocate_node(node_element, "group");
			parentNode->append_node(node);
			saveDevice(doc, node, device->group.get(), tempStrings);
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

void HomegearDevice::saveParameter(xml_document<>* doc, xml_node<>* parentNode, PParameter& parameter, std::vector<std::string>& tempStrings)
{
	try
	{
		xml_attribute<>* attr = doc->allocate_attribute("id", parameter->id.c_str());
		parentNode->append_attribute(attr);

		// {{{ Properties
			xml_node<>* propertiesNode = doc->allocate_node(node_element, "properties");
			parentNode->append_node(propertiesNode);

			if(!parameter->readable)
			{
				tempStrings.push_back("false");
				xml_node<>* node = doc->allocate_node(node_element, "readable", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(!parameter->writeable)
			{
				tempStrings.push_back("false");
				xml_node<>* node = doc->allocate_node(node_element, "writeable", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(!parameter->addonWriteable)
			{
				tempStrings.push_back("false");
				xml_node<>* node = doc->allocate_node(node_element, "addonWriteable", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(!parameter->visible)
			{
				tempStrings.push_back("false");
				xml_node<>* node = doc->allocate_node(node_element, "visible", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(parameter->internal)
			{
				tempStrings.push_back("true");
				xml_node<>* node = doc->allocate_node(node_element, "internal", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(parameter->parameterGroupSelector)
			{
				tempStrings.push_back("true");
				xml_node<>* node = doc->allocate_node(node_element, "parameterGroupSelector", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(parameter->service)
			{
				tempStrings.push_back("true");
				xml_node<>* node = doc->allocate_node(node_element, "service", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(parameter->sticky)
			{
				tempStrings.push_back("true");
				xml_node<>* node = doc->allocate_node(node_element, "sticky", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(parameter->transform)
			{
				tempStrings.push_back("true");
				xml_node<>* node = doc->allocate_node(node_element, "transform", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(parameter->isSigned)
			{
				tempStrings.push_back("true");
				xml_node<>* node = doc->allocate_node(node_element, "signed", tempStrings.back().c_str());
				propertiesNode->append_node(node);
			}

			if(!parameter->control.empty())
			{
				xml_node<>* node = doc->allocate_node(node_element, "control", parameter->control.c_str());
				propertiesNode->append_node(node);
			}

			if(!parameter->unit.empty())
			{
				xml_node<>* node = doc->allocate_node(node_element, "unit", parameter->unit.c_str());
				propertiesNode->append_node(node);
			}

			xml_node<>* node = nullptr;
			if(!parameter->casts.empty())
			{
				node = doc->allocate_node(node_element, "casts");
				propertiesNode->append_node(node);
				for(Casts::iterator i = parameter->casts.begin(); i != parameter->casts.end(); ++i)
				{
					{
						PDecimalIntegerScale decimalIntegerScale;
						decimalIntegerScale = std::dynamic_pointer_cast<DecimalIntegerScale>(*i);
						if(decimalIntegerScale)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "decimalIntegerScale");
							node->append_node(castNode);
							if(decimalIntegerScale->factor != 0)
							{
								tempStrings.push_back(Math::toString(decimalIntegerScale->factor, 6));
								xml_node<>* subnode = doc->allocate_node(node_element, "factor", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(decimalIntegerScale->offset != 0)
							{
								tempStrings.push_back(Math::toString(decimalIntegerScale->offset, 6));
								xml_node<>* subnode = doc->allocate_node(node_element, "offset", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							continue;
						}
					}

					{
						PIntegerIntegerScale integerIntegerScale;
						integerIntegerScale = std::dynamic_pointer_cast<IntegerIntegerScale>(*i);
						if(integerIntegerScale)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "integerIntegerScale");
							node->append_node(castNode);
							if(integerIntegerScale->operation != IntegerIntegerScale::Operation::Enum::none)
							{
								tempStrings.push_back(integerIntegerScale->operation == IntegerIntegerScale::Operation::Enum::division ? "division" : "multiplication");
								xml_node<>* subnode = doc->allocate_node(node_element, "operation", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(integerIntegerScale->factor != 10)
							{
								tempStrings.push_back(std::to_string(integerIntegerScale->factor));
								xml_node<>* subnode = doc->allocate_node(node_element, "factor", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(integerIntegerScale->offset != 0)
							{
								tempStrings.push_back(std::to_string(integerIntegerScale->offset));
								xml_node<>* subnode = doc->allocate_node(node_element, "offset", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							continue;
						}
					}

					{
						PIntegerIntegerMap integerIntegerMap;
						integerIntegerMap = std::dynamic_pointer_cast<IntegerIntegerMap>(*i);
						if(integerIntegerMap)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "integerIntegerMap");
							node->append_node(castNode);
							if(integerIntegerMap->direction != IntegerIntegerMap::Direction::Enum::none)
							{
								tempStrings.push_back(integerIntegerMap->direction == IntegerIntegerMap::Direction::Enum::fromDevice ? "fromDevice" : (integerIntegerMap->direction == IntegerIntegerMap::Direction::Enum::toDevice ? "toDevice" : "both"));
								xml_node<>* subnode = doc->allocate_node(node_element, "direction", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							for(std::map<int32_t, int32_t>::iterator j = integerIntegerMap->integerValueMapFromDevice.begin(); j != integerIntegerMap->integerValueMapFromDevice.end(); ++j)
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "value");
								castNode->append_node(subnode);

								tempStrings.push_back(std::to_string(j->first));
								xml_node<>* valueNode = doc->allocate_node(node_element, "physical", tempStrings.back().c_str());
								subnode->append_node(valueNode);

								tempStrings.push_back(std::to_string(j->second));
								valueNode = doc->allocate_node(node_element, "logical", tempStrings.back().c_str());
								subnode->append_node(valueNode);
							}
							continue;
						}
					}

					{
						PBooleanInteger booleanInteger;
						booleanInteger = std::dynamic_pointer_cast<BooleanInteger>(*i);
						if(booleanInteger)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "booleanInteger");
							node->append_node(castNode);

							if(booleanInteger->trueValue != 0)
							{
								tempStrings.push_back(std::to_string(booleanInteger->trueValue));
								xml_node<>* subnode = doc->allocate_node(node_element, "trueValue", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(booleanInteger->falseValue != 0)
							{
								tempStrings.push_back(std::to_string(booleanInteger->falseValue));
								xml_node<>* subnode = doc->allocate_node(node_element, "falseValue", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(booleanInteger->invert)
							{
								tempStrings.push_back("true");
								xml_node<>* subnode = doc->allocate_node(node_element, "invert", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(booleanInteger->threshold != 1)
							{
								tempStrings.push_back(std::to_string(booleanInteger->threshold));
								xml_node<>* subnode = doc->allocate_node(node_element, "threshold", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							continue;
						}
					}

					{
						PBooleanString booleanString;
						booleanString = std::dynamic_pointer_cast<BooleanString>(*i);
						if(booleanString)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "booleanString");
							node->append_node(castNode);

							if(!booleanString->trueValue.empty())
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "trueValue", booleanString->trueValue.c_str());
								castNode->append_node(subnode);
							}

							if(!booleanString->falseValue.empty())
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "falseValue", booleanString->falseValue.c_str());
								castNode->append_node(subnode);
							}

							if(booleanString->invert)
							{
								tempStrings.push_back("true");
								xml_node<>* subnode = doc->allocate_node(node_element, "invert", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							continue;
						}
					}

					{
						PDecimalConfigTime decimalConfigTime;
						decimalConfigTime = std::dynamic_pointer_cast<DecimalConfigTime>(*i);
						if(decimalConfigTime)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "decimalConfigTime");
							node->append_node(castNode);

							for(std::vector<double>::iterator j = decimalConfigTime->factors.begin(); j != decimalConfigTime->factors.end(); ++j)
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "factors");
								castNode->append_node(subnode);

								tempStrings.push_back(std::to_string(*j));
								xml_node<>* factorNode = doc->allocate_node(node_element, "factor", tempStrings.back().c_str());
								subnode->append_node(factorNode);
							}

							if(decimalConfigTime->valueSize != 0)
							{
								tempStrings.push_back(Math::toString(decimalConfigTime->valueSize, 6));
								xml_node<>* subnode = doc->allocate_node(node_element, "valueSize", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							continue;
						}
					}

					{
						PIntegerTinyFloat integerTinyFloat;
						integerTinyFloat = std::dynamic_pointer_cast<IntegerTinyFloat>(*i);
						if(integerTinyFloat)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "integerTinyFloat");
							node->append_node(castNode);

							if(integerTinyFloat->mantissaStart != 5)
							{
								tempStrings.push_back(std::to_string(integerTinyFloat->mantissaStart));
								xml_node<>* subnode = doc->allocate_node(node_element, "mantissaStart", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(integerTinyFloat->mantissaSize != 11)
							{
								tempStrings.push_back(std::to_string(integerTinyFloat->mantissaSize));
								xml_node<>* subnode = doc->allocate_node(node_element, "mantissaSize", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(integerTinyFloat->exponentStart != 0)
							{
								tempStrings.push_back(std::to_string(integerTinyFloat->exponentStart));
								xml_node<>* subnode = doc->allocate_node(node_element, "exponentStart", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(integerTinyFloat->exponentSize != 5)
							{
								tempStrings.push_back(std::to_string(integerTinyFloat->exponentSize));
								xml_node<>* subnode = doc->allocate_node(node_element, "exponentSize", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							continue;
						}
					}

					{
						PStringUnsignedInteger stringUnsignedInteger;
						stringUnsignedInteger = std::dynamic_pointer_cast<StringUnsignedInteger>(*i);
						if(stringUnsignedInteger)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "stringUnsignedInteger");
							node->append_node(castNode);

							continue;
						}
					}

					{
						PBlindTest blindTest;
						blindTest = std::dynamic_pointer_cast<BlindTest>(*i);
						if(blindTest)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "blindTest");
							node->append_node(castNode);

							if(blindTest->value != 0)
							{
								tempStrings.push_back(std::to_string(blindTest->value));
								xml_node<>* subnode = doc->allocate_node(node_element, "value", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							continue;
						}
					}

					{
						POptionString optionString;
						optionString = std::dynamic_pointer_cast<OptionString>(*i);
						if(optionString)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "optionString");
							node->append_node(castNode);

							continue;
						}
					}

					{
						POptionInteger optionInteger;
						optionInteger = std::dynamic_pointer_cast<OptionInteger>(*i);
						if(optionInteger)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "optionInteger");
							node->append_node(castNode);
							for(std::map<int32_t, int32_t>::iterator j = optionInteger->valueMapFromDevice.begin(); j != optionInteger->valueMapFromDevice.end(); ++j)
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "value");
								castNode->append_node(subnode);

								tempStrings.push_back(std::to_string(j->first));
								xml_node<>* valueNode = doc->allocate_node(node_element, "physical", tempStrings.back().c_str());
								subnode->append_node(valueNode);

								tempStrings.push_back(std::to_string(j->second));
								valueNode = doc->allocate_node(node_element, "logical", tempStrings.back().c_str());
								subnode->append_node(valueNode);
							}
							continue;
						}
					}

					{
						PStringJsonArrayDecimal stringJsonArrayDecimal;
						stringJsonArrayDecimal = std::dynamic_pointer_cast<StringJsonArrayDecimal>(*i);
						if(stringJsonArrayDecimal)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "stringJsonArrayDecimal");
							node->append_node(castNode);

							continue;
						}
					}

					{
						PRpcBinary rpcBinary;
						rpcBinary = std::dynamic_pointer_cast<RpcBinary>(*i);
						if(rpcBinary)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "rpcBinary");
							node->append_node(castNode);

							continue;
						}
					}

					{
						PToggle toggleCast;
						toggleCast = std::dynamic_pointer_cast<Toggle>(*i);
						if(toggleCast)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "toggle");
							node->append_node(castNode);

							xml_node<>* subnode = doc->allocate_node(node_element, "parameter", toggleCast->parameter.c_str());
							castNode->append_node(subnode);

							if(toggleCast->on != 200)
							{
								tempStrings.push_back(std::to_string(toggleCast->on));
								subnode = doc->allocate_node(node_element, "on", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}

							if(toggleCast->off != 0)
							{
								tempStrings.push_back(std::to_string(toggleCast->off));
								subnode = doc->allocate_node(node_element, "off", tempStrings.back().c_str());
								castNode->append_node(subnode);
							}
							continue;
						}
					}

					{
						PCfm cfm;
						cfm = std::dynamic_pointer_cast<Cfm>(*i);
						if(cfm)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "cfm");
							node->append_node(castNode);

							continue;
						}
					}

					{
						PCcrtdnParty ccrtdnParty;
						ccrtdnParty = std::dynamic_pointer_cast<CcrtdnParty>(*i);
						if(ccrtdnParty)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "ccrtdnParty");
							node->append_node(castNode);

							continue;
						}
					}

					{
						PStringReplace stringReplace;
						stringReplace = std::dynamic_pointer_cast<StringReplace>(*i);
						if(stringReplace)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "stringReplace");
							node->append_node(castNode);

							if(!stringReplace->search.empty())
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "search", stringReplace->search.c_str());
								castNode->append_node(subnode);
							}

							if(!stringReplace->replace.empty())
							{
								xml_node<>* subnode = doc->allocate_node(node_element, "replace", stringReplace->replace.c_str());
								castNode->append_node(subnode);
							}

							continue;
						}
					}

					{
						PHexStringByteArray hexStringByteArray;
						hexStringByteArray = std::dynamic_pointer_cast<HexStringByteArray>(*i);
						if(hexStringByteArray)
						{
							xml_node<>* castNode = doc->allocate_node(node_element, "hexStringByteArray");
							node->append_node(castNode);
							continue;
						}
					}
				}
			}
		// }}}

		// {{{ Logical
			if(parameter->logical->type == ILogical::Type::Enum::tBoolean)
			{
				node = doc->allocate_node(node_element, "logicalBoolean");
				parentNode->append_node(node);

				if(parameter->logical->defaultValueExists)
				{
					tempStrings.push_back(parameter->logical->getDefaultValue()->booleanValue ? "true" : "false");
					xml_node<>* subnode = doc->allocate_node(node_element, "defaultValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->logical->setToValueOnPairingExists)
				{
					tempStrings.push_back(parameter->logical->getSetToValueOnPairing()->booleanValue ? "true" : "false");
					xml_node<>* subnode = doc->allocate_node(node_element, "setToValueOnPairing", tempStrings.back().c_str());
					node->append_node(subnode);
				}
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tAction)
			{
				node = doc->allocate_node(node_element, "logicalAction");
				parentNode->append_node(node);
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tInteger)
			{
				node = doc->allocate_node(node_element, "logicalInteger");
				parentNode->append_node(node);

				LogicalInteger* logical = (LogicalInteger*)parameter->logical.get();

				if(logical->minimumValue != -2147483648)
				{
					tempStrings.push_back(std::to_string(logical->minimumValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "minimumValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->maximumValue != 2147483647)
				{
					tempStrings.push_back(std::to_string(logical->maximumValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "maximumValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->defaultValueExists)
				{
					tempStrings.push_back(std::to_string(logical->getDefaultValue()->integerValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "defaultValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->setToValueOnPairingExists)
				{
					tempStrings.push_back(std::to_string(logical->getSetToValueOnPairing()->integerValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "setToValueOnPairing", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(!logical->specialValuesStringMap.empty())
				{
					xml_node<>* subnode = doc->allocate_node(node_element, "specialValues", tempStrings.back().c_str());
					node->append_node(subnode);

					for(std::unordered_map<std::string, int32_t>::iterator i = logical->specialValuesStringMap.begin(); i != logical->specialValuesStringMap.end(); ++i)
					{
						tempStrings.push_back(std::to_string(i->second));
						xml_node<>* specialValueNode = doc->allocate_node(node_element, "specialValue", tempStrings.back().c_str());
						subnode->append_node(specialValueNode);

						attr = doc->allocate_attribute("id", i->first.c_str());
						specialValueNode->append_attribute(attr);
					}
				}
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tFloat)
			{
				node = doc->allocate_node(node_element, "logicalDecimal");
				parentNode->append_node(node);

				LogicalDecimal* logical = (LogicalDecimal*)parameter->logical.get();

				if(logical->minimumValue != 1.175494351e-38f)
				{
					tempStrings.push_back(std::to_string(logical->minimumValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "minimumValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->maximumValue != 3.40282347e+38f)
				{
					tempStrings.push_back(std::to_string(logical->maximumValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "maximumValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->defaultValueExists)
				{
					tempStrings.push_back(std::to_string(logical->getDefaultValue()->floatValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "defaultValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->setToValueOnPairingExists)
				{
					tempStrings.push_back(std::to_string(logical->getSetToValueOnPairing()->floatValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "setToValueOnPairing", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(!logical->specialValuesStringMap.empty())
				{
					xml_node<>* subnode = doc->allocate_node(node_element, "specialValues", tempStrings.back().c_str());
					node->append_node(subnode);

					for(std::unordered_map<std::string, double>::iterator i = logical->specialValuesStringMap.begin(); i != logical->specialValuesStringMap.end(); ++i)
					{
						tempStrings.push_back(std::to_string(i->second));
						xml_node<>* specialValueNode = doc->allocate_node(node_element, "specialValue", tempStrings.back().c_str());
						subnode->append_node(specialValueNode);

						attr = doc->allocate_attribute("id", i->first.c_str());
						specialValueNode->append_attribute(attr);
					}
				}
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tString)
			{
				node = doc->allocate_node(node_element, "logicalString");
				parentNode->append_node(node);

				if(parameter->logical->defaultValueExists)
				{
					xml_node<>* subnode = doc->allocate_node(node_element, "defaultValue", parameter->logical->getDefaultValue()->stringValue.c_str());
					node->append_node(subnode);
				}

				if(parameter->logical->setToValueOnPairingExists)
				{
					xml_node<>* subnode = doc->allocate_node(node_element, "setToValueOnPairing", parameter->logical->getSetToValueOnPairing()->stringValue.c_str());
					node->append_node(subnode);
				}
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tEnum)
			{
				node = doc->allocate_node(node_element, "logicalEnumeration");
				parentNode->append_node(node);

				LogicalEnumeration* logical = (LogicalEnumeration*)parameter->logical.get();

				if(logical->defaultValueExists)
				{
					tempStrings.push_back(std::to_string(logical->getDefaultValue()->integerValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "defaultValue", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(logical->setToValueOnPairingExists)
				{
					tempStrings.push_back(std::to_string(logical->getSetToValueOnPairing()->integerValue));
					xml_node<>* subnode = doc->allocate_node(node_element, "setToValueOnPairing", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				for(std::vector<EnumerationValue>::iterator i = logical->values.begin(); i != logical->values.end(); ++i)
				{
					xml_node<>* subnode = doc->allocate_node(node_element, "value", tempStrings.back().c_str());
					node->append_node(subnode);

					xml_node<>* valueNode = doc->allocate_node(node_element, "id", i->id.c_str());
					subnode->append_node(valueNode);

					tempStrings.push_back(std::to_string(i->index));
					valueNode = doc->allocate_node(node_element, "index", tempStrings.back().c_str());
					subnode->append_node(valueNode);
				}
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tArray)
			{
				node = doc->allocate_node(node_element, "logicalArray");
				parentNode->append_node(node);
			}
			else if(parameter->logical->type == ILogical::Type::Enum::tStruct)
			{
				node = doc->allocate_node(node_element, "logicalStruct");
				parentNode->append_node(node);
			}
		// }}}

		// {{{ Physical
			node = nullptr;
			if(parameter->physical->type == IPhysical::Type::Enum::tInteger)
			{
				node = doc->allocate_node(node_element, "physicalInteger");
				parentNode->append_node(node);
			}
			else if(parameter->physical->type == IPhysical::Type::Enum::tBoolean)
			{
				node = doc->allocate_node(node_element, "physicalBoolean");
				parentNode->append_node(node);
			}
			else if(parameter->physical->type == IPhysical::Type::Enum::tString)
			{
				node = doc->allocate_node(node_element, "physicalString");
				parentNode->append_node(node);
			}

			if(node)
			{
				attr = doc->allocate_attribute("groupId", parameter->physical->groupId.c_str());
				node->append_attribute(attr);

				if(parameter->physical->index != 0)
				{
					tempStrings.push_back(Math::toString(parameter->physical->index, 1));
					xml_node<>* subnode = doc->allocate_node(node_element, "index", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->sizeDefined)
				{
					tempStrings.push_back(Math::toString(parameter->physical->size, 1));
					xml_node<>* subnode = doc->allocate_node(node_element, "size", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->mask != -1)
				{
					tempStrings.push_back(std::to_string(parameter->physical->mask));
					xml_node<>* subnode = doc->allocate_node(node_element, "mask", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->list != -1)
				{
					tempStrings.push_back(std::to_string(parameter->physical->list));
					xml_node<>* subnode = doc->allocate_node(node_element, "list", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->operationType != IPhysical::OperationType::Enum::none)
				{
					if(parameter->physical->operationType == IPhysical::OperationType::Enum::command) tempStrings.push_back("command");
					else if(parameter->physical->operationType == IPhysical::OperationType::Enum::centralCommand) tempStrings.push_back("centralCommand");
					else if(parameter->physical->operationType == IPhysical::OperationType::Enum::internal) tempStrings.push_back("internal");
					else if(parameter->physical->operationType == IPhysical::OperationType::Enum::config) tempStrings.push_back("config");
					else if(parameter->physical->operationType == IPhysical::OperationType::Enum::configString) tempStrings.push_back("configString");
					else if(parameter->physical->operationType == IPhysical::OperationType::Enum::store) tempStrings.push_back("store");
					else if(parameter->physical->operationType == IPhysical::OperationType::Enum::memory) tempStrings.push_back("memory");
					xml_node<>* subnode = doc->allocate_node(node_element, "operationType", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->endianess != IPhysical::Endianess::Enum::big)
				{
					tempStrings.push_back("little");
					xml_node<>* subnode = doc->allocate_node(node_element, "endianess", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->memoryIndex != 0)
				{
					tempStrings.push_back(Math::toString(parameter->physical->memoryIndex, 1));
					xml_node<>* subnode = doc->allocate_node(node_element, "memoryIndex", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->memoryIndexOperation != IPhysical::MemoryIndexOperation::Enum::none)
				{
					tempStrings.push_back(parameter->physical->memoryIndexOperation == IPhysical::MemoryIndexOperation::Enum::addition ? "addition" : "subtraction");
					xml_node<>* subnode = doc->allocate_node(node_element, "memoryIndexOperation", tempStrings.back().c_str());
					node->append_node(subnode);
				}

				if(parameter->physical->memoryChannelStep != 0)
				{
					tempStrings.push_back(Math::toString(parameter->physical->memoryChannelStep, 1));
					xml_node<>* subnode = doc->allocate_node(node_element, "memoryChannelStep", tempStrings.back().c_str());
					node->append_node(subnode);
				}
			}
		//}}}

		// {{{ Packets
			if(!parameter->getPackets.empty() || !parameter->setPackets.empty() || !parameter->eventPackets.empty())
			{
				node = doc->allocate_node(node_element, "packets");
				parentNode->append_node(node);

				for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator i = parameter->getPackets.begin(); i != parameter->getPackets.end(); ++i)
				{
					saveParameterPacket(doc, node, *i, tempStrings);
				}

				for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator i = parameter->setPackets.begin(); i != parameter->setPackets.end(); ++i)
				{
					saveParameterPacket(doc, node, *i, tempStrings);
				}

				for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator i = parameter->eventPackets.begin(); i != parameter->eventPackets.end(); ++i)
				{
					saveParameterPacket(doc, node, *i, tempStrings);
				}
			}
		// }}}
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

void HomegearDevice::saveParameterPacket(xml_document<>* doc, xml_node<>* parentNode, std::shared_ptr<Parameter::Packet>& packet, std::vector<std::string>& tempStrings)
{
	try
	{
		xml_node<>* subnode = doc->allocate_node(node_element, "packet");
		parentNode->append_node(subnode);

		xml_attribute<>* attr = doc->allocate_attribute("id", packet->id.c_str());
		subnode->append_attribute(attr);

		tempStrings.push_back((packet->type == Parameter::Packet::Type::Enum::get) ? "get" : ((packet->type == Parameter::Packet::Type::Enum::set) ? "set" : "event"));
		xml_node<>* packetNode = doc->allocate_node(node_element, "type", tempStrings.back().c_str());
		subnode->append_node(packetNode);

		if(!packet->responseId.empty())
		{
			packetNode = doc->allocate_node(node_element, "responseId", packet->responseId.c_str());
			subnode->append_node(packetNode);
		}

		if(!packet->autoReset.empty())
		{
			packetNode = doc->allocate_node(node_element, "autoReset");
			subnode->append_node(packetNode);

			for(std::vector<std::string>::iterator j = packet->autoReset.begin(); j != packet->autoReset.end(); ++j)
			{
				xml_node<>* autoResetNode = doc->allocate_node(node_element, "parameterId", j->c_str());
				packetNode->append_node(autoResetNode);
			}
		}

		if(!packet->delayedAutoReset.first.empty())
		{
			packetNode = doc->allocate_node(node_element, "delayedAutoReset");
			subnode->append_node(packetNode);

			xml_node<>* autoResetNode = doc->allocate_node(node_element, "resetDelayParameterId", packet->delayedAutoReset.first.c_str());
			packetNode->append_node(autoResetNode);

			tempStrings.push_back(std::to_string(packet->delayedAutoReset.second));
			autoResetNode = doc->allocate_node(node_element, "resetTo", tempStrings.back().c_str());
			packetNode->append_node(autoResetNode);
		}

		if(packet->conditionOperator != Parameter::Packet::ConditionOperator::Enum::none)
		{
			if(packet->conditionOperator == Parameter::Packet::ConditionOperator::Enum::e) tempStrings.push_back("e");
			else if(packet->conditionOperator == Parameter::Packet::ConditionOperator::Enum::g) tempStrings.push_back("g");
			else if(packet->conditionOperator == Parameter::Packet::ConditionOperator::Enum::l) tempStrings.push_back("l");
			else if(packet->conditionOperator == Parameter::Packet::ConditionOperator::Enum::ge) tempStrings.push_back("ge");
			else if(packet->conditionOperator == Parameter::Packet::ConditionOperator::Enum::le) tempStrings.push_back("le");
			packetNode = doc->allocate_node(node_element, "conditionOperator", tempStrings.back().c_str());
			subnode->append_node(packetNode);
		}

		if(packet->conditionValue != -1)
		{
			tempStrings.push_back(std::to_string(packet->conditionValue));
			packetNode = doc->allocate_node(node_element, "conditionValue", tempStrings.back().c_str());
			subnode->append_node(packetNode);
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

void HomegearDevice::saveScenario(xml_document<>* doc, xml_node<>* parentNode, PScenario& scenario, std::vector<std::string>& tempStrings)
{
	try
	{
		xml_attribute<>* attr = doc->allocate_attribute("id", scenario->id.c_str());
		parentNode->append_attribute(attr);

		for(ScenarioEntries::iterator i = scenario->scenarioEntries.begin(); i != scenario->scenarioEntries.end(); ++i)
		{
			xml_node<>* parameterNode = doc->allocate_node(node_element, "parameter", i->second.c_str());
			parentNode->append_node(parameterNode);

			attr = doc->allocate_attribute("id", i->first.c_str());
			parameterNode->append_attribute(attr);
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

void HomegearDevice::saveFunction(xml_document<>* doc, xml_node<>* parentNode, PFunction& function, std::vector<std::string>& tempStrings, std::map<std::string, PConfigParameters>& configParameters, std::map<std::string, PVariables>& variables, std::map<std::string, PLinkParameters>& linkParameters)
{
	try
	{
		tempStrings.push_back(std::to_string(function->channel));
		xml_attribute<>* attr = doc->allocate_attribute("channel", tempStrings.back().c_str());
		parentNode->append_attribute(attr);

		attr = doc->allocate_attribute("type", function->type.c_str());
		parentNode->append_attribute(attr);

		tempStrings.push_back(std::to_string(function->channelCount));
		attr = doc->allocate_attribute("channelCount", tempStrings.back().c_str());
		parentNode->append_attribute(attr);

		// {{{ Properties
		{
			xml_node<>* propertiesNode = doc->allocate_node(node_element, "properties");
			parentNode->append_node(propertiesNode);

			if(function->encryptionEnabledByDefault)
			{
				tempStrings.push_back("true");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "encryptionEnabledByDefault", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(!function->visible)
			{
				tempStrings.push_back("false");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "visible", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->internal)
			{
				tempStrings.push_back("true");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "internal", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->deletable)
			{
				tempStrings.push_back("true");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "deletable", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->dynamicChannelCountIndex != -1)
			{
				tempStrings.push_back(std::to_string(function->dynamicChannelCountIndex) + ':' + Math::toString(function->dynamicChannelCountSize, 1));
				xml_node<>* propertyNode = doc->allocate_node(node_element, "dynamicChannelCount", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(!function->countFromVariable.empty())
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "countFromVariable", function->countFromVariable.c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->physicalChannelIndexOffset != 0)
			{
				tempStrings.push_back(std::to_string(function->physicalChannelIndexOffset));
				xml_node<>* propertyNode = doc->allocate_node(node_element, "physicalChannelIndexOffset", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->grouped)
			{
				tempStrings.push_back("true");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "grouped", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->direction != Function::Direction::Enum::none)
			{
				tempStrings.push_back(function->direction == Function::Direction::Enum::sender ? "sender" : "receiver");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "direction", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->forceEncryption)
			{
				tempStrings.push_back("true");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "forceEncryption", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(!function->defaultLinkScenarioElementId.empty())
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "defaultLinkScenarioElementId", function->defaultLinkScenarioElementId.c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(!function->defaultGroupedLinkScenarioElementId1.empty())
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "defaultGroupedLinkScenarioElementId1", function->defaultGroupedLinkScenarioElementId1.c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(!function->defaultGroupedLinkScenarioElementId2.empty())
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "defaultGroupedLinkScenarioElementId2", function->defaultGroupedLinkScenarioElementId2.c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->hasGroup)
			{
				tempStrings.push_back("true");
				xml_node<>* propertyNode = doc->allocate_node(node_element, "hasGroup", tempStrings.back().c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(!function->groupId.empty())
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "groupId", function->groupId.c_str());
				propertiesNode->append_node(propertyNode);
			}

			if(function->linkSenderFunctionTypes.size() > 0)
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "linkSenderFunctionTypes");
				propertiesNode->append_node(propertyNode);

				for(LinkFunctionTypes::iterator j = function->linkSenderFunctionTypes.begin(); j != function->linkSenderFunctionTypes.end(); ++j)
				{
					xml_node<>* typeNode = doc->allocate_node(node_element, "type", j->c_str());
					propertyNode->append_node(typeNode);
				}
			}

			if(function->linkReceiverFunctionTypes.size() > 0)
			{
				xml_node<>* propertyNode = doc->allocate_node(node_element, "linkReceiverFunctionTypes");
				propertiesNode->append_node(propertyNode);

				for(LinkFunctionTypes::iterator j = function->linkReceiverFunctionTypes.begin(); j != function->linkReceiverFunctionTypes.end(); ++j)
				{
					xml_node<>* typeNode = doc->allocate_node(node_element, "type", j->c_str());
					propertyNode->append_node(typeNode);
				}
			}
		}
		// }}}

		if(!function->configParametersId.empty())
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "configParameters", function->configParametersId.c_str());
			parentNode->append_node(subnode);
			configParameters[function->configParametersId] = function->configParameters;
		}

		if(!function->variablesId.empty())
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "variables", function->variablesId.c_str());
			parentNode->append_node(subnode);
			variables[function->variablesId] = function->variables;
		}

		if(!function->linkParametersId.empty())
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "linkParameters", function->linkParametersId.c_str());
			parentNode->append_node(subnode);
			linkParameters[function->linkParametersId] = function->linkParameters;
		}

		if(function->alternativeFunction)
		{
			xml_node<>* subnode = doc->allocate_node(node_element, "alternativeFunction");
			parentNode->append_node(subnode);
			saveFunction(doc, subnode, function->alternativeFunction, tempStrings, configParameters, variables, linkParameters);
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
						PConfigParameters config(new ConfigParameters(_bl, parameterGroupNode));
						configParameters[config->id] = config;
					}
					else if(parameterGroupName == "variables")
					{
						PVariables config(new Variables(_bl, parameterGroupNode));
						variables[config->id] = config;
					}
					else if(parameterGroupName == "linkParameters")
					{
						PLinkParameters config(new LinkParameters(_bl, parameterGroupNode));
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
			postProcessFunction(i->second, configParameters, variables, linkParameters);
			if(i->second->alternativeFunction) postProcessFunction(i->second->alternativeFunction, configParameters, variables, linkParameters);
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

void HomegearDevice::postProcessFunction(PFunction& function, std::map<std::string, PConfigParameters>& configParameters, std::map<std::string, PVariables>& variables, std::map<std::string, PLinkParameters>& linkParameters)
{
	try
	{
		if(!function->configParametersId.empty())
		{
			std::map<std::string, PConfigParameters>::iterator configIterator = configParameters.find(function->configParametersId);
			if(configIterator == configParameters.end()) _bl->out.printWarning("Warning: configParameters with id \"" + function->configParametersId + "\" does not exist.");
			else
			{
				function->configParameters = configIterator->second;
				if(configIterator->second->parameterGroupSelector) function->parameterGroupSelector = configIterator->second->parameterGroupSelector;
			}
		}
		if(!function->variablesId.empty())
		{
			std::map<std::string, PVariables>::iterator configIterator = variables.find(function->variablesId);
			if(configIterator == variables.end()) _bl->out.printWarning("Warning: variables with id \"" + function->variablesId + "\" does not exist.");
			else
			{
				function->variables = configIterator->second;
				if(configIterator->second->parameterGroupSelector) function->parameterGroupSelector = configIterator->second->parameterGroupSelector;

				for(Parameters::iterator j = function->variables->parameters.begin(); j != function->variables->parameters.end(); ++j)
				{
					for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->getPackets.begin(); k != j->second->getPackets.end(); ++k)
					{
						PacketsById::iterator packetIterator = packetsById.find((*k)->id);
						if(packetIterator != packetsById.end())
						{
							packetIterator->second->associatedVariables.push_back(j->second);
							valueRequestPackets[function->channel][(*k)->id] = packetIterator->second;
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
		}
		if(!function->linkParametersId.empty())
		{
			std::map<std::string, PLinkParameters>::iterator configIterator = linkParameters.find(function->linkParametersId);
			if(configIterator == linkParameters.end()) _bl->out.printWarning("Warning: linkParameters with id \"" + function->linkParametersId + "\" does not exist.");
			else
			{
				function->linkParameters = configIterator->second;
				if(configIterator->second->parameterGroupSelector) function->parameterGroupSelector = configIterator->second->parameterGroupSelector;
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
