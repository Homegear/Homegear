#include <memory>

/* Copyright 2013-2020 Homegear GmbH
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

#include "Miscellaneous.h"
#include "MiscCentral.h"
#include "Gd.h"

namespace Misc
{

Miscellaneous::Miscellaneous(BaseLib::SharedObjects* bl, BaseLib::Systems::IFamilyEventSink* eventHandler) : BaseLib::Systems::DeviceFamily(bl, eventHandler, MISC_FAMILY_ID, MISC_FAMILY_NAME)
{
	Gd::bl = bl;
	Gd::family = this;
	Gd::out.init(bl);
	Gd::out.setPrefix("Module Miscellaneous: ");
	Gd::out.printDebug("Debug: Loading module...");
}

Miscellaneous::~Miscellaneous()
{
}

void Miscellaneous::dispose()
{
	if(_disposed) return;
	DeviceFamily::dispose();
}

void Miscellaneous::reloadRpcDevices()
{
	_bl->out.printInfo("Reloading XML RPC devices...");
	_rpcDevices->load();
}

std::shared_ptr<BaseLib::Systems::ICentral> Miscellaneous::initializeCentral(uint32_t deviceId, int32_t address, std::string serialNumber)
{
	return std::make_shared<MiscCentral>(deviceId, serialNumber, this);
}

void Miscellaneous::createCentral()
{
	try
	{
		_central.reset(new MiscCentral(0, "VMC0000001", this));
		Gd::out.printMessage("Created Miscellaneous central with id " + std::to_string(_central->getId()) + ".");
	}
	catch(const std::exception& ex)
    {
    	Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

PVariable Miscellaneous::getPairingInfo()
{
	try
	{
		if(!_central) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
		PVariable info = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		//{{{ Pairing methods
			PVariable pairingMethods = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

			PVariable createDeviceMetadata = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			PVariable createDeviceMetadataInfo = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
			PVariable createDeviceFields = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
			createDeviceFields->arrayValue->reserve(3);
			createDeviceFields->arrayValue->push_back(std::make_shared<BaseLib::Variable>("deviceType"));
			createDeviceFields->arrayValue->push_back(std::make_shared<BaseLib::Variable>("serialNumber"));
			createDeviceMetadataInfo->structValue->emplace("fields", createDeviceFields);
			createDeviceMetadata->structValue->emplace("metadataInfo", createDeviceMetadataInfo);

			pairingMethods->structValue->emplace("createDevice", createDeviceMetadata);
			info->structValue->emplace("pairingMethods", pairingMethods);
		//}}}

		return info;
	}
	catch(const std::exception& ex)
	{
		Gd::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return Variable::createError(-32500, "Unknown application error.");
}
}
