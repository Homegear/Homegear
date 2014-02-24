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

#include "RPCMethods.h"
#include "../GD/GD.h"

namespace RPC
{

std::shared_ptr<RPCVariable> RPCSystemGetCapabilities::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<RPCVariable> capabilities(new RPCVariable(RPCVariableType::rpcStruct));

		std::shared_ptr<RPCVariable> element(new RPCVariable(RPCVariableType::rpcStruct));
		element->structValue->insert(RPCStructElement("specUrl", std::shared_ptr<RPCVariable>(new RPCVariable(std::string("http://www.xmlrpc.com/spec")))));
		element->structValue->insert(RPCStructElement("specVersion", std::shared_ptr<RPCVariable>(new RPCVariable(1))));
		capabilities->structValue->insert(RPCStructElement("xmlrpc", element));

		element.reset(new RPCVariable(RPCVariableType::rpcStruct));
		element->structValue->insert(RPCStructElement("specUrl", std::shared_ptr<RPCVariable>(new RPCVariable(std::string("http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php")))));
		element->structValue->insert(RPCStructElement("specVersion", std::shared_ptr<RPCVariable>(new RPCVariable(20010516))));
		capabilities->structValue->insert(RPCStructElement("faults_interop", element));

		element.reset(new RPCVariable(RPCVariableType::rpcStruct));
		element->structValue->insert(RPCStructElement("specUrl", std::shared_ptr<RPCVariable>(new RPCVariable(std::string("http://scripts.incutio.com/xmlrpc/introspection.html")))));
		element->structValue->insert(RPCStructElement("specVersion", std::shared_ptr<RPCVariable>(new RPCVariable(1))));
		capabilities->structValue->insert(RPCStructElement("introspection", element));

		return capabilities;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSystemListMethods::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<RPCVariable> methods(new RPCVariable(RPCVariableType::rpcArray));

		for(std::map<std::string, std::shared_ptr<RPCMethod>>::iterator i = _server->getMethods()->begin(); i != _server->getMethods()->end(); ++i)
		{
			methods->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(i->first)));
		}

		return methods;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSystemMethodHelp::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(_server->getMethods()->find(parameters->at(0)->stringValue) == _server->getMethods()->end())
		{
			return RPCVariable::createError(-32602, "Method not found.");
		}

		std::shared_ptr<RPCVariable> help = _server->getMethods()->at(parameters->at(0)->stringValue)->getHelp();

		if(!help) help.reset(new RPCVariable(RPCVariableType::rpcString));

		return help;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSystemMethodSignature::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(_server->getMethods()->find(parameters->at(0)->stringValue) == _server->getMethods()->end())
		{
			return RPCVariable::createError(-32602, "Method not found.");
		}

		std::shared_ptr<RPCVariable> signature = _server->getMethods()->at(parameters->at(0)->stringValue)->getSignature();

		if(!signature) signature.reset(new RPCVariable(RPCVariableType::rpcArray));

		if(signature->arrayValue->empty())
		{
			signature->type = RPCVariableType::rpcString;
			signature->stringValue = "undef";
		}

		return signature;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSystemMulticall::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcArray }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		std::shared_ptr<RPCVariable> returns(new RPCVariable(RPCVariableType::rpcArray));
		for(std::vector<std::shared_ptr<RPCVariable>>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
		{
			if((*i)->type != RPCVariableType::rpcStruct)
			{
				returns->arrayValue->push_back(RPCVariable::createError(-32602, "Array element is no struct."));
				continue;
			}
			if((*i)->structValue->size() != 2)
			{
				returns->arrayValue->push_back(RPCVariable::createError(-32602, "Struct has wrong size."));
				continue;
			}
			if((*i)->structValue->find("methodName") == (*i)->structValue->end() || (*i)->structValue->at("methodName")->type != RPCVariableType::rpcString)
			{
				returns->arrayValue->push_back(RPCVariable::createError(-32602, "No method name provided."));
				continue;
			}
			if((*i)->structValue->find("params") == (*i)->structValue->end() || (*i)->structValue->at("params")->type != RPCVariableType::rpcArray)
			{
				returns->arrayValue->push_back(RPCVariable::createError(-32602, "No parameters provided."));
				continue;
			}
			std::string methodName = (*i)->structValue->at("methodName")->stringValue;
			std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters = (*i)->structValue->at("params")->arrayValue;

			if(methodName == "system.multicall") returns->arrayValue->push_back(RPCVariable::createError(-32602, "Recursive calls to system.multicall are not allowed."));
			else if(methods->find(methodName) == methods->end()) returns->arrayValue->push_back(RPCVariable::createError(-32601, "Requested method not found."));
			else returns->arrayValue->push_back(methods->at(methodName)->invoke(parameters));
		}

		return returns;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCAbortEventReset::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.abortReset(parameters->at(0)->stringValue);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCAddDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;

		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
		}
		else serialNumber = parameters->at(0)->stringValue;

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(serialNumber)) return central->addDevice(serialNumber);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCAddEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcStruct }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.add(parameters->at(0));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCAddLink::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			senderSerialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else senderSerialNumber = parameters->at(0)->stringValue;

		pos = parameters->at(1)->stringValue.find(':');
		if(pos > -1)
		{
			receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
			if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
		}
		else receiverSerialNumber = parameters->at(1)->stringValue;

		std::string name;
		if(parameters->size() > 2)
		{
			if(parameters->at(2)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(2)->stringValue;
		}
		std::string description;
		if(parameters->size() > 3)
		{
			if(parameters->at(3)->stringValue.size() > 1000) return RPC::RPCVariable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(3)->stringValue;
		}


		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(senderSerialNumber)) return central->addLink(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCClientServerInitialized::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string id = parameters->at(0)->stringValue;
		return GD::rpcClient.clientServerInitialized(id);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCDeleteDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcInteger }));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(parameters->at(0)->stringValue)) return central->deleteDevice(parameters->at(0)->stringValue, parameters->at(1)->integerValue);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCDeleteMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		if(parameters->at(0)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(parameters->size() > 1 && parameters->at(1)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");
		std::string dataID;
		if(parameters->size() > 1) dataID = parameters->at(1)->stringValue;

		return Metadata::deleteMetadata(parameters->at(0)->stringValue, dataID);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCEnableEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.enable(parameters->at(0)->stringValue, parameters->at(1)->booleanValue);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetAllMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return Metadata::getAllMetadata(parameters->at(0)->stringValue);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetDeviceDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
			std::vector<RPCVariableType>({ RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getDeviceDescription(serialNumber, channel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getDeviceDescription(parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetInstallMode::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);


		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central)
			{
				std::shared_ptr<RPCVariable> result = central->getInstallMode();
				if(result->integerValue > 0) return result;
			}
		}

		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcInteger));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetKeyMismatchDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);
		//TODO Implement method when AES is supported.
		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcString));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetLinkInfo::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			senderSerialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else senderSerialNumber = parameters->at(0)->stringValue;

		pos = parameters->at(1)->stringValue.find(':');
		if(pos > -1)
		{
			receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
			if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
		}
		else receiverSerialNumber = parameters->at(1)->stringValue;

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(senderSerialNumber)) return central->getLinkInfo(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
		}

		return RPC::RPCVariable::createError(-2, "Sender device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetLinkPeers::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t pos = -1;
		if(parameters->size() > 0)
		{
			pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(serialNumber)) return central->getLinkPeers(serialNumber, channel);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetLinks::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcInteger }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcString }),
				std::vector<RPCVariableType>()
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t pos = -1;
		int32_t flags = 0;
		if(parameters->size() > 0)
		{
			pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;

			if(parameters->size() == 2) flags = parameters->at(1)->integerValue;
		}

		std::shared_ptr<RPC::RPCVariable> links(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			if(serialNumber.empty())
			{
				std::shared_ptr<RPC::RPCVariable> result = central->getLinks(serialNumber, channel, flags);
				if(result && !result->arrayValue->empty()) links->arrayValue->insert(links->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
			else if(central->knowsDevice(serialNumber)) return central->getLinks(serialNumber, channel, flags);
		}

		if(serialNumber.empty()) return links;
		else return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return Metadata::getMetadata(parameters->at(0)->stringValue, parameters->at(1)->stringValue);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetParamsetDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		uint64_t remoteID = 0;
		int32_t parameterSetIndex = useSerialNumber ? 1 : 2;
		ParameterSet::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == RPCVariableType::rpcString)
		{
			type = ParameterSet::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == ParameterSet::Type::Enum::none)
			{
				type = ParameterSet::Type::Enum::link;
				int32_t pos = parameters->at(1)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
					if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(1)->stringValue;
			}
		}
		else
		{
			type = ParameterSet::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getParamsetDescription(serialNumber, channel, type, remoteSerialNumber, remoteChannel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamsetDescription(parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
				}
			}
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetParamsetId::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t channel = 0;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		int32_t pos = parameters->at(0)->stringValue.find(':');

		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		ParameterSet::Type::Enum type = ParameterSet::typeFromString(parameters->at(1)->stringValue);
		if(type == ParameterSet::Type::Enum::none)
		{
			type = ParameterSet::Type::Enum::link;
			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else remoteSerialNumber = parameters->at(1)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(serialNumber)) return central->getParamsetId(serialNumber, channel, type, remoteSerialNumber, remoteChannel);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetParamset::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		ParameterSet::Type::Enum type = ParameterSet::typeFromString(parameters->at(1)->stringValue);
		if(type == ParameterSet::Type::Enum::none)
		{
			type = ParameterSet::Type::Enum::link;
			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else remoteSerialNumber = parameters->at(1)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(serialNumber)) return central->getParamset(serialNumber, channel, type, remoteSerialNumber, remoteChannel);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetPeerId::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
			std::vector<RPCVariableType>({ RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<RPC::RPCVariable> ids(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central)
			{
				std::shared_ptr<RPCVariable> result;
				if(useSerialNumber) result = central->getPeerID(serialNumber);
				else result = central->getPeerID(parameters->at(0)->integerValue);
				if(!result->errorStruct) ids->arrayValue->push_back(result);
			}
		}

		if(ids->arrayValue->empty()) return RPC::RPCVariable::createError(-2, "Device not found.");
		return ids;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetServiceMessages::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<RPC::RPCVariable> serviceMessages(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(!central) continue;
			//getServiceMessages really needs a lot of ressources, so wait a little bit after each central
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<RPC::RPCVariable> messages = central->getServiceMessages();
			if(!messages->arrayValue->empty()) serviceMessages->arrayValue->insert(serviceMessages->arrayValue->end(), messages->arrayValue->begin(), messages->arrayValue->end());
		}

		return serviceMessages;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcString }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getValue(serialNumber, channel, parameters->at(1)->stringValue);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getValue(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue);
				}
			}
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

RPCInit::~RPCInit()
{
	try
	{
		if(_initServerThread.joinable()) _initServerThread.join();
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

std::shared_ptr<RPCVariable> RPCInit::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::pair<std::string, std::string> server = HelperFunctions::split(parameters->at(0)->stringValue, ':');
		if(server.first.empty() || server.second.empty()) return RPCVariable::createError(-32602, "Server address or port is empty.");
		if(server.first.size() < 5) return RPCVariable::createError(-32602, "Server address too short.");
		HelperFunctions::toLower(server.first);

		std::string path = "/RPC2";
		int32_t pos = server.second.find_first_of('/');
		if(pos > 0)
		{
			path = server.second.substr(pos);
			Output::printDebug("Debug: Server path set to: " + path);
			server.second = server.second.substr(0, pos);
			Output::printDebug("Debug: Server port set to: " + server.second);
		}
		server.second = std::to_string(HelperFunctions::getNumber(server.second));
		if(server.second.empty() || server.second == "0") return RPCVariable::createError(-32602, "Port number is invalid.");

		if(parameters->at(1)->stringValue.empty())
		{
			GD::rpcClient.removeServer(server);
		}
		else
		{
			std::shared_ptr<RemoteRPCServer> eventServer = GD::rpcClient.addServer(server, path, parameters->at(1)->stringValue);
			if(eventServer && server.first.compare(0, 5, "https") == 0) eventServer->useSSL = true;
			if(parameters->size() > 2)
			{
				if(parameters->at(2)->integerValue & 1) eventServer->keepAlive = true;
				if(parameters->at(2)->integerValue & 2) eventServer->binary = true;
				if(parameters->at(2)->integerValue & 4) eventServer->useID = true;
			}
			if(_initServerThread.joinable()) _initServerThread.join();
			_initServerThread = std::thread(&RPC::Client::initServerMethods, &GD::rpcClient, server);
		}

		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListBidcosInterfaces::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		return GD::deviceFamilies.at(DeviceFamilies::HomeMaticBidCoS)->listBidcosInterfaces();
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListClientServers::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		std::string id;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
			if(error != ParameterError::Enum::noError) return getError(error);
			id = parameters->at(0)->stringValue;
		}
		return GD::rpcClient.listClientServers(id);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListDevices::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
					std::vector<RPCVariableType>({ RPCVariableType::rpcBoolean }),
					std::vector<RPCVariableType>({ RPCVariableType::rpcString })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		std::shared_ptr<RPC::RPCVariable> devices(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<RPC::RPCVariable> result = central->listDevices();
			if(result && !result->arrayValue->empty()) devices->arrayValue->insert(devices->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}

		return devices;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListEvents::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
					std::vector<RPCVariableType>({ RPCVariableType::rpcInteger }),
					std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger }),
					std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcString })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		int32_t type = -1;
		uint64_t peerID = 0;
		int32_t channel = -1;
		std::string variable;
		if(parameters->size() > 0)
		{
			if(parameters->size() == 1) type = parameters->at(0)->integerValue;
			else
			{
				peerID = parameters->at(0)->integerValue;
				channel = parameters->at(1)->integerValue;
			}
			if(parameters->size() == 3) variable = parameters->at(2)->stringValue;
		}
		return GD::eventHandler.list(type, peerID, channel, variable);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListInterfaces::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		return GD::physicalDevices.listInterfaces();
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListTeams::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<RPC::RPCVariable> teams(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<RPC::RPCVariable> result = central->listTeams();
			if(!result->arrayValue->empty()) teams->arrayValue->insert(teams->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}

		return teams;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCLogLevel::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcInteger }));
			if(error != ParameterError::Enum::noError) return getError(error);
			GD::rpcLogLevel = parameters->at(0)->integerValue;
		}
		return std::shared_ptr<RPCVariable>(new RPCVariable(GD::rpcLogLevel));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCPutParamset::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcStruct }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		ParameterSet::Type::Enum type = ParameterSet::typeFromString(parameters->at(1)->stringValue);
		if(type == ParameterSet::Type::Enum::none)
		{
			type = ParameterSet::Type::Enum::link;
			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else remoteSerialNumber = parameters->at(1)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(serialNumber)) return central->putParamset(serialNumber, channel, type, remoteSerialNumber, remoteChannel, parameters->at(2));
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCRemoveEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.remove(parameters->at(0)->stringValue);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCRemoveLink::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			senderSerialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else senderSerialNumber = parameters->at(0)->stringValue;

		pos = parameters->at(1)->stringValue.find(':');
		if(pos > -1)
		{
			receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
			if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
		}
		else receiverSerialNumber = parameters->at(1)->stringValue;

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(senderSerialNumber)) return central->removeLink(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
		}

		return RPC::RPCVariable::createError(-2, "Sender device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCRunScript::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
			std::vector<RPCVariableType>({ RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcBoolean }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool wait = false;
		std::string filename;
		std::string arguments;

		filename = parameters->at(0)->stringValue;

		if(parameters->size() == 2)
		{
			if(parameters->at(1)->type == RPCVariableType::rpcBoolean) wait = parameters->at(1)->booleanValue;
			else arguments = parameters->at(1)->stringValue;
		}
		if(parameters->size() == 3)
		{
			arguments = parameters->at(1)->stringValue;
			wait = parameters->at(2)->booleanValue;
		}

		std::string path = GD::settings.scriptPath() + filename;
		std::string command = path + " " + arguments;
		if(!wait) command += "&";

		struct stat statStruct;
		if(stat(path.c_str(), &statStruct) < 0) return RPCVariable::createError(-32400, "Could not execute script: " + std::string(strerror(errno)));
		int32_t uid = getuid();
		int32_t gid = getgid();
		if((statStruct.st_mode & S_IXOTH) == 0)
		{
			if(statStruct.st_gid != gid || (statStruct.st_gid == gid && (statStruct.st_mode & S_IXGRP) == 0))
			{
				if(statStruct.st_uid != uid || (statStruct.st_uid == uid && (statStruct.st_mode & S_IXUSR) == 0)) return RPCVariable::createError(-32400, "Could not execute script. No permission or executable bit is not set.");
			}
		}
		if((statStruct.st_mode & (S_IXGRP | S_IXUSR)) == 0) //At least in Debian it is not possible to execute scripts, when the execution bit is only set for "other".
			return RPCVariable::createError(-32400, "Could not execute script. Executable bit is not set for user or group.");
		if((statStruct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) return RPCVariable::createError(-32400, "Could not execute script. The file mode is not set to executable.");
		int32_t exitCode = std::system(command.c_str());
		int32_t signal = WIFSIGNALED(exitCode);
		if(signal) return RPCVariable::createError(-32400, "Script exited with signal: " + std::to_string(signal));
		exitCode = WEXITSTATUS(exitCode);
		return std::shared_ptr<RPCVariable>(new RPCVariable(exitCode));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSearchDevices::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<RPC::RPCVariable> result(new RPC::RPCVariable(RPC::RPCVariableType::rpcInteger));
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central) result->integerValue += central->searchDevices()->integerValue;
		}

		return result;
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetInstallMode::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
			std::vector<RPCVariableType>({ RPCVariableType::rpcBoolean }),
			std::vector<RPCVariableType>({ RPCVariableType::rpcBoolean, RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t time = (parameters->size() > 1) ? parameters->at(1)->integerValue : 60;
		if(time < 5) time = 60;
		if(time > 3600) time = 3600;
		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central) central->setInstallMode(parameters->at(0)->booleanValue, time);
		}

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetLinkInfo::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			senderSerialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else senderSerialNumber = parameters->at(0)->stringValue;

		pos = parameters->at(1)->stringValue.find(':');
		if(pos > -1)
		{
			receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
			if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
		}
		else receiverSerialNumber = parameters->at(1)->stringValue;

		if(parameters->at(2)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "Name has more than 250 characters.");
		std::string	name = parameters->at(2)->stringValue;

		std::string description;
		if(parameters->size() > 3)
		{
			if(parameters->at(3)->stringValue.size() > 1000) return RPC::RPCVariable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(3)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(senderSerialNumber)) return central->setLinkInfo(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
		}

		return RPC::RPCVariable::createError(-2, "Sender device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcVariant }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return Metadata::setMetadata(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetTeam::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t deviceChannel = -1;
		std::string deviceSerialNumber;
		int32_t teamChannel = -1;
		std::string teamSerialNumber;
		int32_t pos = -1;

		pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			deviceSerialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) deviceChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else deviceSerialNumber = parameters->at(0)->stringValue;

		if(parameters->size() > 1)
		{
			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				teamSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) teamChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else teamSerialNumber = parameters->at(1)->stringValue;
		}

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central && central->knowsDevice(deviceSerialNumber)) return central->setTeam(deviceSerialNumber, deviceChannel, teamSerialNumber, teamChannel);
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<RPCVariableType>>({
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcVariant }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcString, RPCVariableType::rpcVariant }),
				std::vector<RPCVariableType>({ RPCVariableType::rpcInteger, RPCVariableType::rpcInteger, RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<RPC::RPCVariable> value;
		if(useSerialNumber && parameters->size() == 3) value = parameters->at(2);
		else if(!useSerialNumber && parameters->size() == 4) value = parameters->at(3);
		else value.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));

		for(std::map<DeviceFamilies, std::shared_ptr<DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->setValue(serialNumber, channel, parameters->at(1)->stringValue, value);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setValue(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue, value);
				}
			}
		}

		return RPC::RPCVariable::createError(-2, "Device not found.");
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCTriggerEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.trigger(parameters->at(0)->stringValue);
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

} /* namespace RPC */
