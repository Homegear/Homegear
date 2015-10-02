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

#include "HmConverter.h"
#include "../../BaseLib.h"

namespace BaseLib
{
namespace HmDeviceDescription
{

HmConverter::HmConverter(BaseLib::Obj* baseLib)
{
	_bl = baseLib;
}

void HmConverter::convert(std::shared_ptr<Device> homematicDevice, std::shared_ptr<HomegearDevice> homegearDevice)
{
	/*
	 * Ignored:
	 *
	 * version
	 * deviceClass
	 * peeringSysinfoExpectChannel
	 */

	if(!homematicDevice || !homegearDevice) return;
	homegearDevice->version = homematicDevice->version;
	homegearDevice->hasBattery = homematicDevice->hasBattery;
	homegearDevice->timeout = homematicDevice->cyclicTimeout;
	homegearDevice->memorySize = (uint32_t)homematicDevice->eepSize;
	homegearDevice->receiveModes = (HomegearDevice::ReceiveModes::Enum)homematicDevice->rxModes;
	homegearDevice->visible = (homematicDevice->uiFlags & Device::UIFlags::Enum::visible);
	homegearDevice->internal = (homematicDevice->uiFlags & Device::UIFlags::Enum::internal);
	homegearDevice->deletable = !(homematicDevice->uiFlags & Device::UIFlags::Enum::dontdelete);
	homegearDevice->dynamicChannelCountIndex = homematicDevice->countFromSysinfoIndex;
	homegearDevice->dynamicChannelCountSize = homematicDevice->countFromSysinfoSize;
	homegearDevice->encryption = homematicDevice->supportsAES;
	homegearDevice->needsTime = homematicDevice->needsTime;

	for(std::vector<std::shared_ptr<DeviceType>>::iterator i = homematicDevice->supportedTypes.begin(); i != homematicDevice->supportedTypes.end(); ++i)
	{
		/*
		 * Ignored:
		 *
		 * updateable
		 * priority
		 */

		PSupportedDevice supportedDevice(new SupportedDevice(_bl, homegearDevice.get()));
		supportedDevice->id = (*i)->id;
		supportedDevice->description = (*i)->name;
		int32_t typeId = -1;
		bool ignore = false;
		if((*i)->typeID == -1)
		{
			for(std::vector<HomeMaticParameter>::iterator j = (*i)->parameters.begin(); j != (*i)->parameters.end(); ++j)
			{
				//When the device type is not at index 10 of the pairing packet, the device is not supported
				//The "priority" attribute is ignored, for the standard devices "priority" seems not important
				if(j->index == 10.0) { typeId = j->constValue; }
				else if(j->index == 9.0 && (*i)->firmware == -1)
				{
					switch((DeviceType::BooleanOperator::Enum)j->booleanOperator)
					{
						case DeviceType::BooleanOperator::Enum::e:
							supportedDevice->minFirmwareVersion = j->constValue;
							supportedDevice->maxFirmwareVersion = j->constValue;
							break;
						case DeviceType::BooleanOperator::Enum::g:
							supportedDevice->minFirmwareVersion = j->constValue + 1;
							break;
						case DeviceType::BooleanOperator::Enum::l:
							supportedDevice->maxFirmwareVersion = j->constValue - 1;
							break;
						case DeviceType::BooleanOperator::Enum::ge:
							supportedDevice->minFirmwareVersion = j->constValue;
							break;
						case DeviceType::BooleanOperator::Enum::le:
							supportedDevice->maxFirmwareVersion = j->constValue;
							break;
					}
				}
				else if(j->index == 0)
				{
					if(typeId == -1) typeId = 0;
					typeId += (j->constValue << 8);
				}
				else if(j->index == 1.0)
				{
					if(typeId == -1) typeId = 0;
					typeId += j->constValue;
				}
				else if(j->index == 2.0 && (*i)->firmware == -1)
				{
					switch((DeviceType::BooleanOperator::Enum)j->booleanOperator)
					{
						case DeviceType::BooleanOperator::Enum::e:
							supportedDevice->minFirmwareVersion = j->constValue;
							supportedDevice->maxFirmwareVersion = j->constValue;
							break;
						case DeviceType::BooleanOperator::Enum::g:
							supportedDevice->minFirmwareVersion = j->constValue + 1;
							break;
						case DeviceType::BooleanOperator::Enum::l:
							supportedDevice->maxFirmwareVersion = j->constValue - 1;
							break;
						case DeviceType::BooleanOperator::Enum::ge:
							supportedDevice->minFirmwareVersion = j->constValue;
							break;
						case DeviceType::BooleanOperator::Enum::le:
							supportedDevice->maxFirmwareVersion = j->constValue;
							break;
					}
				}
				else if(j->index == 22.0) ignore = true;
			}
			(*i)->typeID = typeId;
			if(ignore) continue;
		}
		if((*i)->typeID == -1)
		{
			_bl->out.printError("Warning: typeID is -1 (" + (*i)->id + "). Device might not work.");
		}
		supportedDevice->typeNumber = (*i)->typeID;

		if((*i)->firmware != -1)
		{
			switch((*i)->booleanOperator)
			{
				case DeviceType::BooleanOperator::Enum::e:
					supportedDevice->minFirmwareVersion = (*i)->firmware;
					supportedDevice->maxFirmwareVersion = (*i)->firmware;
					break;
				case DeviceType::BooleanOperator::Enum::g:
					supportedDevice->minFirmwareVersion = (*i)->firmware + 1;
					break;
				case DeviceType::BooleanOperator::Enum::l:
					supportedDevice->maxFirmwareVersion = (*i)->firmware - 1;
					break;
				case DeviceType::BooleanOperator::Enum::ge:
					supportedDevice->minFirmwareVersion = (*i)->firmware;
					break;
				case DeviceType::BooleanOperator::Enum::le:
					supportedDevice->maxFirmwareVersion = (*i)->firmware;
					break;
			}
		}
		homegearDevice->supportedDevices.push_back(supportedDevice);
	}

	if(homematicDevice->runProgram)
	{
		homegearDevice->runProgram->path = homematicDevice->runProgram->path;
		homegearDevice->runProgram->arguments = homematicDevice->runProgram->arguments;
		homegearDevice->runProgram->startType = (RunProgram::StartType::Enum)homematicDevice->runProgram->startType;
		homegearDevice->runProgram->interval = homematicDevice->runProgram->interval;
	}

	for(std::map<uint32_t, std::shared_ptr<DeviceChannel>>::iterator i = homematicDevice->channels.begin(); i != homematicDevice->channels.end(); ++i)
	{
		PFunction function(new Function(_bl));
		convertChannel(i->second, function);
		homegearDevice->functions[i->first] = function;
	}

	for(std::map<std::string, std::shared_ptr<DeviceFrame>>::iterator i = homematicDevice->framesByID.begin(); i != homematicDevice->framesByID.end(); ++i)
	{
		PPacket packet(new Packet(_bl));
		convertPacket(i->second, packet);
		homegearDevice->packetsById[packet->id] = packet;
		homegearDevice->packetsByMessageType.insert(std::pair<uint32_t, PPacket>(packet->type, packet));
		if(!packet->function1.empty()) homegearDevice->packetsByFunction1.insert(std::pair<std::string, PPacket>(packet->function1, packet));
		if(!packet->function2.empty()) homegearDevice->packetsByFunction2.insert(std::pair<std::string, PPacket>(packet->function2, packet));
	}

	for(Functions::iterator i = homegearDevice->functions.begin(); i != homegearDevice->functions.end(); ++i)
	{
		if(i->second->variables->parameters.empty()) continue;
		for(Parameters::iterator j = i->second->variables->parameters.begin(); j != i->second->variables->parameters.end(); ++j)
		{
			if(!j->second) continue;
			for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->getPackets.begin(); k != j->second->getPackets.end(); ++k)
			{
				PacketsById::iterator packet = homegearDevice->packetsById.find((*k)->id);
				if(packet != homegearDevice->packetsById.end())
				{
					packet->second->associatedVariables.push_back(j->second);
					homegearDevice->valueRequestPackets[i->first][(*k)->id] = packet->second;
				}

			}
			for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->setPackets.begin(); k != j->second->setPackets.end(); ++k)
			{
				PacketsById::iterator packet = homegearDevice->packetsById.find((*k)->id);
				if(packet != homegearDevice->packetsById.end()) packet->second->associatedVariables.push_back(j->second);

			}
			for(std::vector<std::shared_ptr<Parameter::Packet>>::iterator k = j->second->eventPackets.begin(); k != j->second->eventPackets.end(); ++k)
			{
				PacketsById::iterator packet = homegearDevice->packetsById.find((*k)->id);
				if(packet != homegearDevice->packetsById.end())
				{
					packet->second->associatedVariables.push_back(j->second);
					packet->second->channelIndexOffset = i->second->physicalChannelIndexOffset;
					for(BinaryPayloads::iterator l = packet->second->binaryPayloads.begin(); l != packet->second->binaryPayloads.end(); ++l)
					{
						if((*l)->parameterId == j->second->id)
						{
							if((*l)->isSigned) j->second->isSigned = true;
							if((*l)->size > 0)
							{
								j->second->physical->sizeDefined = true;
								j->second->physical->size = (*l)->size;
							}
						}
					}
				}
			}
		}
	}

	if(homematicDevice->team)
	{
		homegearDevice->group.reset(new HomegearDevice(_bl, homegearDevice->family));
		convert(homematicDevice->team, homegearDevice->group);
		if(homegearDevice->group->receiveModes == HomegearDevice::ReceiveModes::Enum::always) homegearDevice->group->receiveModes = homegearDevice->receiveModes;
	}
}

void HmConverter::convertChannel(std::shared_ptr<DeviceChannel> homematicChannel, PFunction homegearFunction)
{
	/*
	 * Ignored:
	 *
	 * aesCBC
	 * autoregister
	 * class
	 * uiFlags => deletable
	 */

	if(homematicChannel->subconfig)
	{
		homegearFunction->alternativeFunction.reset(new Function(_bl));
		homematicChannel->subconfig->startIndex = homematicChannel->startIndex;
		homematicChannel->subconfig->type = homematicChannel->type;
		homematicChannel->subconfig->count = homematicChannel->count;
		convertChannel(homematicChannel->subconfig, homegearFunction->alternativeFunction);
	}
	homegearFunction->channel = homematicChannel->startIndex;
	homegearFunction->channelCount = homematicChannel->count;
	homegearFunction->countFromVariable = homematicChannel->countFromVariable;
	homegearFunction->defaultLinkScenarioElementId = homematicChannel->function;
	homegearFunction->defaultGroupedLinkScenarioElementId1 = homematicChannel->pairFunction1;
	homegearFunction->defaultGroupedLinkScenarioElementId2 = homematicChannel->pairFunction2;
	homegearFunction->direction = (Function::Direction::Enum)homematicChannel->direction;
	homegearFunction->dynamicChannelCountIndex = homematicChannel->countFromSysinfo;
	homegearFunction->dynamicChannelCountSize = homematicChannel->countFromSysinfoSize;
	homegearFunction->encryptionEnabledByDefault = homematicChannel->aesDefault;
	homegearFunction->forceEncryption = homematicChannel->aesAlways;
	homegearFunction->groupId = homematicChannel->teamTag;
	homegearFunction->grouped = homematicChannel->paired;
	homegearFunction->hasGroup = homematicChannel->hasTeam;
	homegearFunction->internal = (homematicChannel->uiFlags & DeviceChannel::UIFlags::Enum::internal);
	if(homematicChannel->linkRoles)
	{
		for(std::vector<std::string>::iterator i = homematicChannel->linkRoles->sourceNames.begin(); i != homematicChannel->linkRoles->sourceNames.end(); ++i)
		{
			homegearFunction->linkSenderFunctionTypes.insert(*i);
		}
		for(std::vector<std::string>::iterator i = homematicChannel->linkRoles->targetNames.begin(); i != homematicChannel->linkRoles->targetNames.end(); ++i)
		{
			homegearFunction->linkReceiverFunctionTypes.insert(*i);
		}
	}
	homegearFunction->physicalChannelIndexOffset = homematicChannel->physicalIndexOffset;
	homegearFunction->type = homematicChannel->type;
	homegearFunction->visible = (homematicChannel->uiFlags & DeviceChannel::UIFlags::Enum::visible) && !homematicChannel->hidden;

	std::map<ParameterSet::Type::Enum, std::shared_ptr<ParameterSet>>::iterator parameterSet = homematicChannel->parameterSets.find(ParameterSet::Type::master);
	if(parameterSet != homematicChannel->parameterSets.end() && parameterSet->second)
	{
		if(parameterSet->second->id.empty()) parameterSet->second->id = "config";
		homegearFunction->configParametersId = parameterSet->second->id + '-' + std::to_string(homegearFunction->channel);
		homegearFunction->configParameters->id = parameterSet->second->id + '-' + std::to_string(homegearFunction->channel);
		homegearFunction->configParameters->memoryAddressStart = parameterSet->second->addressStart;
		homegearFunction->configParameters->memoryAddressStep = parameterSet->second->addressStep;
		for(std::vector<std::shared_ptr<HmDeviceDescription::HomeMaticParameter>>::iterator i = parameterSet->second->parameters.begin(); i != parameterSet->second->parameters.end(); ++i)
		{
			PParameter parameter(new BaseLib::DeviceDescription::Parameter(_bl, homegearFunction->configParameters.get()));
			convertParameter(*i, parameter);
			if(parameter->id.empty()) continue;
			if(parameter->id == "BURST_RX" || parameter->id == "LIVE_MODE_RX") parameter->id = "WAKE_ON_RADIO";
			if(parameter->parameterGroupSelector)
			{
				homegearFunction->configParametersId += "-t" + std::to_string(homegearFunction->channel);
				homegearFunction->configParameters->id += "-t" + std::to_string(homegearFunction->channel);
			}
			homegearFunction->configParameters->parameters[parameter->id] = parameter;
			homegearFunction->configParameters->parametersOrdered.push_back(parameter);
			if(parameter->physical->list > -1) homegearFunction->configParameters->lists[parameter->physical->list].push_back(parameter);
		}
		for(std::map<std::string, DefaultValue>::iterator i = parameterSet->second->defaultValues.begin(); i != parameterSet->second->defaultValues.end(); ++i)
		{
			PScenario scenario(new Scenario(_bl));
			scenario->id = i->first;
			for(std::vector<std::pair<std::string, std::string>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				scenario->scenarioEntries[j->first] = j->second;
			}
			homegearFunction->configParameters->scenarios[i->first] = scenario;
		}
	}
	parameterSet = homematicChannel->parameterSets.find(ParameterSet::Type::values);
	if(parameterSet != homematicChannel->parameterSets.end() && parameterSet->second)
	{

		homegearFunction->variablesId = parameterSet->second->id + '-' + std::to_string(homegearFunction->channel);
		homegearFunction->variables->id = parameterSet->second->id + '-' + std::to_string(homegearFunction->channel);
		homegearFunction->variables->memoryAddressStart = parameterSet->second->addressStart;
		homegearFunction->variables->memoryAddressStep = parameterSet->second->addressStep;
		for(std::vector<std::shared_ptr<HmDeviceDescription::HomeMaticParameter>>::iterator i = parameterSet->second->parameters.begin(); i != parameterSet->second->parameters.end(); ++i)
		{
			PParameter parameter(new BaseLib::DeviceDescription::Parameter(_bl, homegearFunction->variables.get()));
			convertParameter(*i, parameter);
			if(parameter->id.empty()) continue;
			homegearFunction->variables->parameters[parameter->id] = parameter;
			homegearFunction->variables->parametersOrdered.push_back(parameter);
			if(parameter->physical->list > -1) homegearFunction->variables->lists[parameter->physical->list].push_back(parameter);
		}
		for(std::map<std::string, DefaultValue>::iterator i = parameterSet->second->defaultValues.begin(); i != parameterSet->second->defaultValues.end(); ++i)
		{
			PScenario scenario(new Scenario(_bl));
			scenario->id = i->first;
			for(std::vector<std::pair<std::string, std::string>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				scenario->scenarioEntries[j->first] = j->second;
			}
			homegearFunction->variables->scenarios[i->first] = scenario;
		}
	}
	parameterSet = homematicChannel->parameterSets.find(ParameterSet::Type::link);
	if(parameterSet != homematicChannel->parameterSets.end() && parameterSet->second)
	{
		if(parameterSet->second->id.empty()) parameterSet->second->id = "link-config";
		homegearFunction->linkParametersId = parameterSet->second->id + '-' + std::to_string(homegearFunction->channel);
		homegearFunction->linkParameters->id = parameterSet->second->id + '-' + std::to_string(homegearFunction->channel);
		homegearFunction->linkParameters->memoryAddressStart = parameterSet->second->addressStart;
		homegearFunction->linkParameters->memoryAddressStep = parameterSet->second->addressStep;
		homegearFunction->linkParameters->peerChannelMemoryOffset = parameterSet->second->peerChannelOffset;
		homegearFunction->linkParameters->channelMemoryOffset = parameterSet->second->channelOffset;
		homegearFunction->linkParameters->peerAddressMemoryOffset = parameterSet->second->peerAddressOffset;
		homegearFunction->linkParameters->maxLinkCount = parameterSet->second->count;
		for(std::vector<std::shared_ptr<HmDeviceDescription::HomeMaticParameter>>::iterator i = parameterSet->second->parameters.begin(); i != parameterSet->second->parameters.end(); ++i)
		{
			PParameter parameter(new BaseLib::DeviceDescription::Parameter(_bl, homegearFunction->linkParameters.get()));
			convertParameter(*i, parameter);
			if(parameter->id.empty()) continue;
			homegearFunction->linkParameters->parameters[parameter->id] = parameter;
			homegearFunction->linkParameters->parametersOrdered.push_back(parameter);
			if(parameter->physical->list > -1) homegearFunction->linkParameters->lists[parameter->physical->list].push_back(parameter);
		}
		for(std::map<std::string, DefaultValue>::iterator i = parameterSet->second->defaultValues.begin(); i != parameterSet->second->defaultValues.end(); ++i)
		{
			PScenario scenario(new Scenario(_bl));
			scenario->id = i->first;
			for(std::vector<std::pair<std::string, std::string>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				scenario->scenarioEntries[j->first] = j->second;
			}
			homegearFunction->linkParameters->scenarios[i->first] = scenario;
		}
		if(!homematicChannel->enforceLinks.empty())
		{
			PScenario scenario(new Scenario(_bl));
			scenario->id = "default";
			for(std::vector<std::shared_ptr<EnforceLink>>::iterator i = homematicChannel->enforceLinks.begin(); i != homematicChannel->enforceLinks.end(); ++i)
			{
				scenario->scenarioEntries[(*i)->id] = (*i)->value;
			}
			homegearFunction->linkParameters->scenarios[scenario->id] = scenario;
		}
	}
	if(homematicChannel->specialParameter && !homematicChannel->specialParameter->id.empty())
	{
		homegearFunction->parameterGroupSelector = homegearFunction->configParameters->parameters.at(homematicChannel->specialParameter->id);
		homegearFunction->parameterGroupSelector->parameterGroupSelector = true;
	}
}

void HmConverter::convertParameter(std::shared_ptr<HomeMaticParameter> homematicParameter, PParameter parameter)
{
	parameter->addonWriteable = true;
	parameter->control = homematicParameter->control;
	parameter->hasDelayedAutoResetParameters = homematicParameter->hasDominoEvents;
	parameter->id = homematicParameter->id;
	parameter->internal = homematicParameter->uiFlags & HomeMaticParameter::UIFlags::internal;
	parameter->isSigned = homematicParameter->isSigned;
	parameter->parameterGroupSelector = homematicParameter->uiFlags & HomeMaticParameter::UIFlags::transform;
	parameter->readable = (homematicParameter->operations & HomeMaticParameter::Operations::read) || (homematicParameter->operations & HomeMaticParameter::Operations::event);
	parameter->service = homematicParameter->uiFlags & HomeMaticParameter::UIFlags::service;
	parameter->sticky = homematicParameter->uiFlags & HomeMaticParameter::UIFlags::sticky;
	parameter->unit = homematicParameter->logicalParameter->unit;
	parameter->visible = !(homematicParameter->uiFlags & HomeMaticParameter::UIFlags::invisible) && !homematicParameter->hidden;
	parameter->writeable = homematicParameter->operations & HomeMaticParameter::Operations::write;

	for(std::vector<std::shared_ptr<ParameterConversion>>::iterator i = homematicParameter->conversion.begin(); i != homematicParameter->conversion.end(); ++i)
	{
		if((*i)->type == ParameterConversion::Type::floatIntegerScale)
		{
			PDecimalIntegerScale cast(new DecimalIntegerScale(_bl));
			cast->factor = (*i)->factor;
			cast->offset = (*i)->offset;
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::integerIntegerScale)
		{
			if((*i)->mul > 0)
			{
				PIntegerIntegerScale cast(new IntegerIntegerScale(_bl));
				cast->operation = IntegerIntegerScale::Operation::multiplication;
				cast->factor = (*i)->mul;
				cast->offset = (*i)->offset;
				parameter->casts.push_back(cast);
			}
			if((*i)->div > 0)
			{
				PIntegerIntegerScale cast(new IntegerIntegerScale(_bl));
				cast->operation = IntegerIntegerScale::Operation::division;
				cast->factor = (*i)->div;
				cast->offset = (*i)->offset;
				parameter->casts.push_back(cast);
			}
			if((*i)->factor != 0)
			{
				PIntegerIntegerScale cast(new IntegerIntegerScale(_bl));
				cast->operation = IntegerIntegerScale::Operation::multiplication;
				cast->factor = (*i)->factor;
				cast->offset = (*i)->offset;
				parameter->casts.push_back(cast);
			}
		}
		else if((*i)->type == ParameterConversion::Type::integerIntegerMap)
		{
			PIntegerIntegerMap cast(new IntegerIntegerMap(_bl));
			if((*i)->fromDevice && (*i)->toDevice) cast->direction = IntegerIntegerMap::Direction::both;
			else if((*i)->fromDevice) cast->direction = IntegerIntegerMap::Direction::fromDevice;
			else if((*i)->toDevice) cast->direction = IntegerIntegerMap::Direction::toDevice;
			for(std::unordered_map<int32_t, int32_t>::iterator j = (*i)->integerValueMapDevice.begin(); j != (*i)->integerValueMapDevice.end(); ++j)
			{
				cast->integerValueMapFromDevice[j->first] = j->second;
			}
			for(std::unordered_map<int32_t, int32_t>::iterator j = (*i)->integerValueMapParameter.begin(); j != (*i)->integerValueMapParameter.end(); ++j)
			{
				cast->integerValueMapToDevice[j->first] = j->second;
			}
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::booleanInteger)
		{
			PBooleanInteger cast(new BooleanInteger(_bl));
			cast->trueValue = (*i)->valueTrue;
			cast->falseValue = (*i)->valueFalse;
			cast->invert = (*i)->invert;
			cast->threshold = (*i)->threshold;
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::booleanString)
		{
			PBooleanString cast(new BooleanString(_bl));
			cast->trueValue = (*i)->stringValueTrue;
			cast->falseValue = (*i)->stringValueFalse;
			cast->invert = (*i)->invert;
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::floatConfigTime)
		{
			PDecimalConfigTime cast(new DecimalConfigTime(_bl));
			cast->valueSize = (*i)->valueSize;
			cast->factors = (*i)->factors;
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::integerTinyFloat)
		{
			PIntegerTinyFloat cast(new IntegerTinyFloat(_bl));
			cast->mantissaStart = (*i)->mantissaStart;
			cast->mantissaSize = (*i)->mantissaSize;
			cast->exponentStart = (*i)->exponentStart;
			cast->exponentSize = (*i)->exponentSize;
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::stringUnsignedInteger)
		{
			PStringUnsignedInteger cast(new StringUnsignedInteger(_bl));
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::blindTest)
		{
			PBlindTest cast(new BlindTest(_bl));
			cast->value = Math::getNumber((*i)->stringValue);
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::optionString)
		{
			POptionString cast(new OptionString(_bl));
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::optionInteger)
		{
			POptionInteger cast(new OptionInteger(_bl));
			for(std::unordered_map<int32_t, int32_t>::iterator j = (*i)->integerValueMapDevice.begin(); j != (*i)->integerValueMapDevice.end(); ++j)
			{
				cast->valueMapFromDevice[j->first] = j->second;
			}
			for(std::unordered_map<int32_t, int32_t>::iterator j = (*i)->integerValueMapParameter.begin(); j != (*i)->integerValueMapParameter.end(); ++j)
			{
				cast->valueMapToDevice[j->first] = j->second;
			}
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::stringJsonArrayFloat)
		{
			PStringJsonArrayDecimal cast(new StringJsonArrayDecimal(_bl));
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::rpcBinary)
		{
			PRpcBinary cast(new RpcBinary(_bl));
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::toggle)
		{
			PToggle cast(new Toggle(_bl));
			cast->parameter = (*i)->stringValue;
			cast->on = (*i)->on;
			cast->off = (*i)->off;
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::cfm)
		{
			PCfm cast(new Cfm(_bl));
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::ccrtdnParty)
		{
			PCcrtdnParty cast(new CcrtdnParty(_bl));
			parameter->casts.push_back(cast);
		}
		else if((*i)->type == ParameterConversion::Type::hexstringBytearray)
		{
			PHexStringByteArray cast(new HexStringByteArray(_bl));
			parameter->casts.push_back(cast);
		}
	}

	if(homematicParameter->logicalParameter)
	{
		if(homematicParameter->logicalParameter->type == LogicalParameter::Type::Enum::typeBoolean)
		{
			PLogicalBoolean logical(new LogicalBoolean(_bl));
			parameter->logical = logical;
			logical->defaultValueExists = homematicParameter->logicalParameter->defaultValueExists;
			logical->defaultValue = homematicParameter->logicalParameter->getDefaultValue()->booleanValue;
			logical->setToValueOnPairingExists = homematicParameter->logicalParameter->enforce;
			logical->setToValueOnPairing = homematicParameter->logicalParameter->getEnforceValue()->booleanValue;
		}
		else if(homematicParameter->logicalParameter->type == LogicalParameter::Type::Enum::typeAction)
		{
			parameter->logical.reset(new LogicalAction(_bl));
		}
		else if(homematicParameter->logicalParameter->type == LogicalParameter::Type::Enum::typeInteger)
		{
			PLogicalInteger logical(new LogicalInteger(_bl));
			LogicalParameterInteger* homematicLogical = (LogicalParameterInteger*)homematicParameter->logicalParameter.get();
			parameter->logical = logical;
			logical->defaultValueExists = homematicLogical->defaultValueExists;
			logical->defaultValue = homematicLogical->getDefaultValue()->integerValue;
			logical->setToValueOnPairingExists = homematicLogical->enforce;
			logical->setToValueOnPairing = homematicLogical->getEnforceValue()->integerValue;
			logical->minimumValue = homematicLogical->min;
			logical->maximumValue = homematicLogical->max;
			logical->specialValuesStringMap = homematicLogical->specialValues;
			for(std::unordered_map<std::string, int32_t>::iterator i = logical->specialValuesStringMap.begin(); i != logical->specialValuesStringMap.end(); ++i)
			{
				logical->specialValuesIntegerMap[i->second] = i->first;
			}
		}
		else if(homematicParameter->logicalParameter->type == LogicalParameter::Type::Enum::typeFloat)
		{
			PLogicalDecimal logical(new LogicalDecimal(_bl));
			LogicalParameterFloat* homematicLogical = (LogicalParameterFloat*)homematicParameter->logicalParameter.get();
			parameter->logical = logical;
			logical->defaultValueExists = homematicLogical->defaultValueExists;
			logical->defaultValue = homematicLogical->getDefaultValue()->floatValue;
			logical->setToValueOnPairingExists = homematicLogical->enforce;
			logical->setToValueOnPairing = homematicLogical->getEnforceValue()->floatValue;
			logical->minimumValue = homematicLogical->min;
			logical->maximumValue = homematicLogical->max;
			logical->specialValuesStringMap = homematicLogical->specialValues;
			for(std::unordered_map<std::string, double>::iterator i = logical->specialValuesStringMap.begin(); i != logical->specialValuesStringMap.end(); ++i)
			{
				logical->specialValuesFloatMap[i->second] = i->first;
			}
		}
		else if(homematicParameter->logicalParameter->type == LogicalParameter::Type::Enum::typeString)
		{
			PLogicalString logical(new LogicalString(_bl));
			parameter->logical = logical;
			logical->defaultValueExists = homematicParameter->logicalParameter->defaultValueExists;
			logical->defaultValue = homematicParameter->logicalParameter->getDefaultValue()->stringValue;
			logical->setToValueOnPairingExists = homematicParameter->logicalParameter->enforce;
			logical->setToValueOnPairing = homematicParameter->logicalParameter->getEnforceValue()->stringValue;
		}
		else if(homematicParameter->logicalParameter->type == LogicalParameter::Type::Enum::typeEnum)
		{
			PLogicalEnumeration logical(new LogicalEnumeration(_bl));
			LogicalParameterEnum* homematicLogical = (LogicalParameterEnum*)homematicParameter->logicalParameter.get();
			parameter->logical = logical;
			logical->defaultValueExists = homematicLogical->defaultValueExists;
			logical->defaultValue = homematicLogical->getDefaultValue()->integerValue;
			logical->setToValueOnPairingExists = homematicLogical->enforce;
			logical->setToValueOnPairing = homematicLogical->getEnforceValue()->integerValue;
			logical->minimumValue = homematicLogical->min;
			logical->maximumValue = homematicLogical->max;
			for(std::vector<ParameterOption>::iterator i = homematicLogical->options.begin(); i != homematicLogical->options.end(); ++i)
			{
				EnumerationValue enumerationValue;
				enumerationValue.id = i->id;
				enumerationValue.index = i->index;
				logical->values.push_back(enumerationValue);
			}
		}
	}

	if(homematicParameter->physicalParameter)
	{
		if(homematicParameter->physicalParameter->type == PhysicalParameter::Type::Enum::typeInteger) parameter->physical.reset(new PhysicalInteger(_bl));
		else if(homematicParameter->physicalParameter->type == PhysicalParameter::Type::Enum::typeBoolean) parameter->physical.reset(new PhysicalBoolean(_bl));
		else if(homematicParameter->physicalParameter->type == PhysicalParameter::Type::Enum::typeString) parameter->physical.reset(new PhysicalString(_bl));
		parameter->physical->endianess = homematicParameter->physicalParameter->endian == PhysicalParameter::Endian::big ? IPhysical::Endianess::big : IPhysical::Endianess::little;
		if(!homematicParameter->physicalParameter->id.empty() && homematicParameter->physicalParameter->id != homematicParameter->id) _bl->out.printWarning("Warning: id of physical does not match parameter id (" + homematicParameter->id + ").");
		if(!homematicParameter->physicalParameter->valueID.empty() && !homematicParameter->physicalParameter->id.empty() && homematicParameter->physicalParameter->id != homematicParameter->id) _bl->out.printWarning("Warning: id and value_id of physical are set. That's not allowed.");
		parameter->physical->groupId = homematicParameter->physicalParameter->valueID;
		if(parameter->physical->groupId.empty()) parameter->physical->groupId = homematicParameter->physicalParameter->id;
		parameter->physical->index = homematicParameter->physicalParameter->index;
		if(homematicParameter->physicalParameter->list != 9999) parameter->physical->list = homematicParameter->physicalParameter->list;
		parameter->physical->mask = homematicParameter->physicalParameter->mask;
		parameter->physical->memoryChannelStep = homematicParameter->physicalParameter->address.step;
		parameter->physical->memoryIndex = homematicParameter->physicalParameter->address.index;
		parameter->physical->memoryIndexOperation = (IPhysical::MemoryIndexOperation::Enum)homematicParameter->physicalParameter->address.operation;
		parameter->physical->operationType = (IPhysical::OperationType::Enum)homematicParameter->physicalParameter->interface;
		parameter->physical->size = homematicParameter->physicalParameter->size;
		parameter->physical->sizeDefined = homematicParameter->physicalParameter->sizeDefined;
	}

	if(!homematicParameter->physicalParameter->setRequest.empty())
	{
		std::shared_ptr<Parameter::Packet> packet(new Parameter::Packet);
		packet->type = Parameter::Packet::Type::Enum::set;
		packet->id = homematicParameter->physicalParameter->setRequest;
		packet->autoReset = homematicParameter->physicalParameter->resetAfterSend;
		parameter->setPackets.push_back(packet);
	}
	if(!homematicParameter->physicalParameter->setRequestsEx.empty())
	{
		for(std::vector<std::shared_ptr<SetRequestEx>>::iterator i = homematicParameter->physicalParameter->setRequestsEx.begin(); i != homematicParameter->physicalParameter->setRequestsEx.end(); ++i)
		{
			std::shared_ptr<Parameter::Packet> packet(new Parameter::Packet);
			packet->type = Parameter::Packet::Type::Enum::set;
			packet->id = (*i)->frame;
			packet->autoReset = homematicParameter->physicalParameter->resetAfterSend;
			packet->conditionOperator = (Parameter::Packet::ConditionOperator::Enum)(*i)->conditionOperator;
			packet->conditionValue = (*i)->value;
			parameter->setPackets.push_back(packet);
		}
	}
	if(!homematicParameter->physicalParameter->getRequest.empty())
	{
		std::shared_ptr<Parameter::Packet> packet(new Parameter::Packet);
		packet->type = Parameter::Packet::Type::Enum::get;
		packet->id = homematicParameter->physicalParameter->getRequest;
		packet->responseId = homematicParameter->physicalParameter->getResponse;
		packet->autoReset = homematicParameter->physicalParameter->resetAfterSend;
		parameter->getPackets.push_back(packet);
	}
	if(!homematicParameter->physicalParameter->eventFrames.empty())
	{
		for(std::vector<std::shared_ptr<PhysicalParameterEvent>>::iterator i = homematicParameter->physicalParameter->eventFrames.begin(); i != homematicParameter->physicalParameter->eventFrames.end(); ++i)
		{
			std::shared_ptr<Parameter::Packet> packet(new Parameter::Packet);
			packet->type = Parameter::Packet::Type::Enum::event;
			packet->id = (*i)->frame;
			packet->delayedAutoReset.first = (*i)->dominoEventDelayID;
			packet->delayedAutoReset.second = (*i)->dominoEventValue;
			parameter->eventPackets.push_back(packet);
		}
	}
}

void HmConverter::convertPacket(std::shared_ptr<DeviceFrame> homematicFrame, PPacket packet)
{
	packet->channel = homematicFrame->fixedChannel;
	packet->channelIndex = homematicFrame->channelField;
	packet->channelSize = homematicFrame->channelFieldSize;
	if(homematicFrame->direction == DeviceFrame::Direction::Enum::fromDevice) packet->direction = Packet::Direction::Enum::toCentral;
	else if(homematicFrame->direction == DeviceFrame::Direction::Enum::toDevice) packet->direction = Packet::Direction::Enum::fromCentral;
	else if(homematicFrame->direction == DeviceFrame::Direction::Enum::none) packet->direction = Packet::Direction::Enum::none;
	packet->doubleSend = homematicFrame->doubleSend;
	packet->function1 = homematicFrame->function1;
	packet->function2 = homematicFrame->function2;
	packet->id = homematicFrame->id;
	packet->length = homematicFrame->size;
	packet->maxPackets = homematicFrame->maxPackets;
	packet->metaString1 = homematicFrame->metaString1;
	packet->metaString2 = homematicFrame->metaString2;
	packet->responseSubtype = homematicFrame->responseSubtype;
	packet->responseType = homematicFrame->responseType;
	packet->splitAfter = homematicFrame->splitAfter;
	packet->subtype = homematicFrame->subtype;
	packet->subtypeIndex = homematicFrame->subtypeIndex;
	packet->subtypeSize = homematicFrame->subtypeFieldSize;
	packet->type = homematicFrame->type;
	for(std::list<HomeMaticParameter>::iterator i = homematicFrame->parameters.begin(); i != homematicFrame->parameters.end(); ++i)
	{
		if(i->field.empty())
		{
			PBinaryPayload payload(new BinaryPayload(_bl));
			payload->parameterId = i->param;
			if(!i->additionalParameter.empty() && !i->param.empty())
			{
				_bl->out.printWarning("Warning: param and PARAM are set for frame \"" + homematicFrame->id + "\".");
			}
			if(!i->additionalParameter.empty()) payload->parameterId = i->additionalParameter;
			payload->index = i->index;
			payload->size = i->size;
			payload->index2 = i->index2;
			payload->size2 = i->size2;
			payload->index2Offset = i->index2Offset;
			payload->constValueInteger = i->constValue;
			payload->constValueString = i->constValueString;
			payload->isSigned = i->isSigned;
			payload->omitIf = i->omitIf;
			payload->omitIfSet = i->omitIfSet;
			packet->binaryPayloads.push_back(payload);
		}
		else
		{
			PJsonPayload payload(new JsonPayload(_bl));
			payload->parameterId = i->param;
			if(!i->additionalParameter.empty() && !i->param.empty())
			{
				_bl->out.printWarning("Warning: param and PARAM are set for frame \"" + homematicFrame->id + "\".");
			}
			if(!i->additionalParameter.empty()) payload->parameterId = i->additionalParameter;
			payload->key = i->field;
			payload->subkey = i->subfield;
			if(i->constValue != -1)
			{
				if(i->type == PhysicalParameter::Type::typeBoolean)
				{
					payload->constValueBoolean = (bool)i->constValue;
					payload->constValueBooleanSet = true;
				}
				else
				{
					payload->constValueInteger = i->constValue;
					payload->constValueIntegerSet = true;
				}
			}
			payload->constValueString = i->constValueString;
			if(!payload->constValueString.empty()) payload->constValueStringSet = true;
			packet->jsonPayloads.push_back(payload);
		}
	}
}

}
}
