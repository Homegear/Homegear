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
#include "../../Version.h"
#include "../../Modules/Base/BaseLib.h"

namespace RPC
{

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSystemGetCapabilities::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::RPCVariable> capabilities(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

		std::shared_ptr<BaseLib::RPC::RPCVariable> element(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specUrl", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("http://www.xmlrpc.com/spec")))));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specVersion", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(1))));
		capabilities->structValue->insert(BaseLib::RPC::RPCStructElement("xmlrpc", element));

		element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specUrl", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php")))));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specVersion", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(20010516))));
		capabilities->structValue->insert(BaseLib::RPC::RPCStructElement("faults_interop", element));

		element.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specUrl", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(std::string("http://scripts.incutio.com/xmlrpc/introspection.html")))));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specVersion", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(1))));
		capabilities->structValue->insert(BaseLib::RPC::RPCStructElement("introspection", element));

		return capabilities;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSystemListMethods::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::RPCVariable> methods(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

		for(std::map<std::string, std::shared_ptr<RPCMethod>>::iterator i = _server->getMethods()->begin(); i != _server->getMethods()->end(); ++i)
		{
			methods->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->first)));
		}

		return methods;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSystemMethodHelp::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(_server->getMethods()->find(parameters->at(0)->stringValue) == _server->getMethods()->end())
		{
			return BaseLib::RPC::RPCVariable::createError(-32602, "Method not found.");
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> help = _server->getMethods()->at(parameters->at(0)->stringValue)->getHelp();

		if(!help) help.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));

		return help;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSystemMethodSignature::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(_server->getMethods()->find(parameters->at(0)->stringValue) == _server->getMethods()->end())
		{
			return BaseLib::RPC::RPCVariable::createError(-32602, "Method not found.");
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> signature = _server->getMethods()->at(parameters->at(0)->stringValue)->getSignature();

		if(!signature) signature.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));

		if(signature->arrayValue->empty())
		{
			signature->type = BaseLib::RPC::RPCVariableType::rpcString;
			signature->stringValue = "undef";
		}

		return signature;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSystemMulticall::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcArray }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		std::shared_ptr<BaseLib::RPC::RPCVariable> returns(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
		{
			if((*i)->type != BaseLib::RPC::RPCVariableType::rpcStruct)
			{
				returns->arrayValue->push_back(BaseLib::RPC::RPCVariable::createError(-32602, "Array element is no struct."));
				continue;
			}
			if((*i)->structValue->size() != 2)
			{
				returns->arrayValue->push_back(BaseLib::RPC::RPCVariable::createError(-32602, "Struct has wrong size."));
				continue;
			}
			if((*i)->structValue->find("methodName") == (*i)->structValue->end() || (*i)->structValue->at("methodName")->type != BaseLib::RPC::RPCVariableType::rpcString)
			{
				returns->arrayValue->push_back(BaseLib::RPC::RPCVariable::createError(-32602, "No method name provided."));
				continue;
			}
			if((*i)->structValue->find("params") == (*i)->structValue->end() || (*i)->structValue->at("params")->type != BaseLib::RPC::RPCVariableType::rpcArray)
			{
				returns->arrayValue->push_back(BaseLib::RPC::RPCVariable::createError(-32602, "No parameters provided."));
				continue;
			}
			std::string methodName = (*i)->structValue->at("methodName")->stringValue;
			std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters = (*i)->structValue->at("params")->arrayValue;

			if(methodName == "system.multicall") returns->arrayValue->push_back(BaseLib::RPC::RPCVariable::createError(-32602, "Recursive calls to system.multicall are not allowed."));
			else if(methods->find(methodName) == methods->end()) returns->arrayValue->push_back(BaseLib::RPC::RPCVariable::createError(-32601, "Requested method not found."));
			else returns->arrayValue->push_back(methods->at(methodName)->invoke(parameters));
		}

		return returns;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCAbortEventReset::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.abortReset(parameters->at(0)->stringValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCAddDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t serialNumberIndex = (parameters->size() == 2) ? 1 : 0;
		int32_t familyID = (parameters->size() == 2) ? parameters->at(0)->integerValue : -1;

		std::string serialNumber;
		int32_t pos = parameters->at(serialNumberIndex)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(serialNumberIndex)->stringValue.substr(0, pos);
		}
		else serialNumber = parameters->at(serialNumberIndex)->stringValue;

		if(familyID > -1)
		{
			if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)familyID) == GD::deviceFamilies.end())
			{
				return BaseLib::RPC::RPCVariable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->addDevice(serialNumber);
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central) return central->addDevice(serialNumber);
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCAddEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcStruct }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.add(parameters->at(0));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCAddLink::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t nameIndex = 4;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			nameIndex = 2;
			int32_t pos = parameters->at(0)->stringValue.find(':');
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
		}

		std::string name;
		if(parameters->size() > nameIndex)
		{
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if(parameters->size() > nameIndex + 1)
		{
			if(parameters->at(nameIndex + 1)->stringValue.size() > 1000) return BaseLib::RPC::RPCVariable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(nameIndex + 1)->stringValue;
		}


		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(senderSerialNumber)) return central->addLink(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->addLink(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, name, description);
				}
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCClientServerInitialized::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string id = parameters->at(0)->stringValue;
		return GD::rpcClient.clientServerInitialized(id);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCDeleteDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool useSerialNumber = (parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString) ? true : false;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(parameters->at(0)->stringValue)) return central->deleteDevice(parameters->at(0)->stringValue, parameters->at(1)->integerValue);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->deleteDevice(parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCDeleteMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		bool deviceFound = false;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber))
					{
						deviceFound = true;
						break;
					}
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue))
					{
						deviceFound = true;
						break;
					}
				}
			}
		}

		if(!deviceFound) return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");

		std::string objectID(useSerialNumber ? parameters->at(0)->stringValue : std::to_string(parameters->at(0)->integerValue));
		std::string dataID;
		if(parameters->size() > 1) dataID = parameters->at(1)->stringValue;
		return GD::db.deleteMetadata(objectID, dataID);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCDeleteSystemVariable::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::db.deleteSystemVariable(parameters->at(0)->stringValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCEnableEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.enable(parameters->at(0)->stringValue, parameters->at(1)->booleanValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetAllMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		bool deviceFound = false;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber))
					{
						deviceFound = true;
						break;
					}
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue))
					{
						deviceFound = true;
						break;
					}
				}
			}
		}

		if(!deviceFound) return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");

		std::string objectID(useSerialNumber ? parameters->at(0)->stringValue : std::to_string(parameters->at(0)->integerValue));
		return GD::db.getAllMetadata(objectID);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetAllSystemVariables::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		return GD::db.getAllSystemVariables();
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetAllValues::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		}

		bool returnWriteOnly = false;
		if(parameters->size() > 0) returnWriteOnly = parameters->at(0)->booleanValue;

		std::shared_ptr<BaseLib::RPC::RPCVariable> values(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->getAllValues(returnWriteOnly);
			if(result->errorStruct)
			{
				GD::out.printWarning("Warning: Error calling method \"listDevices\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) values->arrayValue->insert(values->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}

		return values;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetDeviceDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
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

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetDeviceInfo::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcArray }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcArray })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint32_t peerID = 0;
		int32_t fieldsIndex = -1;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcArray)
		{
			fieldsIndex = 0;
		}
		else if(parameters->size() == 2 || (parameters->size() == 1 && parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcInteger))
		{
			if(parameters->size() == 2) fieldsIndex = 1;
			peerID = parameters->at(0)->integerValue;
		}


		std::map<std::string, bool> fields;
		if(fieldsIndex > -1)
		{
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->at(fieldsIndex)->arrayValue->begin(); i != parameters->at(fieldsIndex)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> deviceInfo(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(peerID > 0)
			{
				if(central->knowsDevice(peerID)) return central->getDeviceInfo(peerID, fields);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->getDeviceInfo(peerID, fields);
				if(result->errorStruct)
				{
					GD::out.printWarning("Warning: Error calling method \"listDevices\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
					continue;
				}
				if(result && !result->arrayValue->empty()) deviceInfo->arrayValue->insert(deviceInfo->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
		}

		if(peerID == 0) return deviceInfo;
		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetInstallMode::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		if(familyID > -1)
		{
			if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)familyID) == GD::deviceFamilies.end())
			{
				return BaseLib::RPC::RPCVariable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->getInstallMode();
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->getInstallMode();
				if(result->integerValue > 0) return result;
			}
		}

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetKeyMismatchDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);
		//TODO Implement method when AES is supported.
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcString));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetLinkInfo::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
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
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(senderSerialNumber)) return central->getLinkInfo(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getLinkInfo(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
				}
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetLinkPeers::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
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
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getLinkPeers(serialNumber, channel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getLinkPeers(parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetLinks::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>(),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t flags = 0;
		uint64_t peerID = 0;

		bool useSerialNumber = false;
		if(parameters->size() > 0)
		{
			if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
			{
				useSerialNumber = true;
				int32_t pos = parameters->at(0)->stringValue.find(':');
				if(pos > -1)
				{
					serialNumber = parameters->at(0)->stringValue.substr(0, pos);
					if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
				}
				else serialNumber = parameters->at(0)->stringValue;

				if(parameters->size() == 2) flags = parameters->at(1)->integerValue;
			}
			else
			{
				peerID = parameters->at(0)->integerValue;
				if(peerID > 0 && parameters->size() > 1)
				{
					channel = parameters->at(1)->integerValue;
					if(parameters->size() == 3) flags = parameters->at(2)->integerValue;
				}
			}
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> links(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			if(serialNumber.empty() && peerID == 0)
			{
				std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->getLinks(peerID, channel, flags);
				if(result && !result->arrayValue->empty()) links->arrayValue->insert(links->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
			else
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getLinks(serialNumber, channel, flags);
				}
				else
				{
					if(central->knowsDevice(peerID)) return central->getLinks(peerID, channel, flags);
				}
			}
		}

		if(serialNumber.empty() && peerID == 0) return links;
		else return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		bool deviceFound = false;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber))
					{
						deviceFound = true;
						break;
					}
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue))
					{
						deviceFound = true;
						break;
					}
				}
			}
		}

		if(!deviceFound) return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");

		std::string objectID(useSerialNumber ? parameters->at(0)->stringValue : std::to_string(parameters->at(0)->integerValue));
		return GD::db.getMetadata(objectID, parameters->at(1)->stringValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetParamsetDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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
		BaseLib::RPC::ParameterSet::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			type = BaseLib::RPC::ParameterSet::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::none)
			{
				type = BaseLib::RPC::ParameterSet::Type::Enum::link;
				int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
			}
		}
		else
		{
			type = BaseLib::RPC::ParameterSet::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(serialNumber)) return central->getParamsetDescription(serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamsetDescription(parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetParamsetId::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t channel = 0;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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
		BaseLib::RPC::ParameterSet::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			type = BaseLib::RPC::ParameterSet::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::none)
			{
				type = BaseLib::RPC::ParameterSet::Type::Enum::link;
				int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
			}
		}
		else
		{
			type = BaseLib::RPC::ParameterSet::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(serialNumber)) return central->getParamsetId(serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamsetId(parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetParamset::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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
		BaseLib::RPC::ParameterSet::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			type = BaseLib::RPC::ParameterSet::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::none)
			{
				type = BaseLib::RPC::ParameterSet::Type::Enum::link;
				int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
			}
		}
		else
		{
			type = BaseLib::RPC::ParameterSet::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(serialNumber)) return central->getParamset(serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamset(parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetPeerId::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> ids(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				std::shared_ptr<BaseLib::RPC::RPCVariable> result;
				if(useSerialNumber) result = central->getPeerID(serialNumber);
				else result = central->getPeerID(parameters->at(0)->integerValue);
				if(!result->errorStruct) ids->arrayValue->push_back(result);
			}
		}

		if(ids->arrayValue->empty()) return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
		return ids;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetServiceMessages::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>(),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean })
		}));

		bool id = false;
		if(parameters->size() == 1) id = parameters->at(0)->booleanValue;

		std::shared_ptr<BaseLib::RPC::RPCVariable> serviceMessages(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			//getServiceMessages really needs a lot of ressources, so wait a little bit after each central
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::RPCVariable> messages = central->getServiceMessages(id);
			if(!messages->arrayValue->empty()) serviceMessages->arrayValue->insert(serviceMessages->arrayValue->end(), messages->arrayValue->begin(), messages->arrayValue->end());
		}

		return serviceMessages;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetSystemVariable::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::db.getSystemVariable(parameters->at(0)->stringValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetUpdateStatus::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>());
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<BaseLib::RPC::RPCVariable> updateInfo(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));

		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("DEVICE_COUNT", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)GD::bl->deviceUpdateInfo.devicesToUpdate))));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CURRENT_UPDATE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)GD::bl->deviceUpdateInfo.currentUpdate))));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CURRENT_DEVICE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)GD::bl->deviceUpdateInfo.currentDevice))));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CURRENT_DEVICE_PROGRESS", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable((uint32_t)GD::bl->deviceUpdateInfo.currentDeviceProgress))));

		std::shared_ptr<BaseLib::RPC::RPCVariable> results(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("RESULTS", results));
		for(std::map<uint64_t, std::pair<int32_t, std::string>>::iterator i = GD::bl->deviceUpdateInfo.results.begin(); i != GD::bl->deviceUpdateInfo.results.end(); ++i)
		{
			std::shared_ptr<BaseLib::RPC::RPCVariable> result(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcStruct));
			result->structValue->insert(BaseLib::RPC::RPCStructElement("RESULT_CODE", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->second.first))));
			result->structValue->insert(BaseLib::RPC::RPCStructElement("RESULT_STRING", std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(i->second.second))));
			results->structValue->insert(BaseLib::RPC::RPCStructElement(std::to_string(i->first), result));
		}

		return updateInfo;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
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

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCGetVersion::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string version(VERSION);
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable("Homegear " + version));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

RPCInit::~RPCInit()
{
	try
	{
		if(_initServerThread.joinable()) _initServerThread.join();
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

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCInit::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::split(parameters->at(0)->stringValue, ':');
		if(server.first.empty() || server.second.empty()) return BaseLib::RPC::RPCVariable::createError(-32602, "Server address or port is empty.");
		if(server.first.size() < 8) return BaseLib::RPC::RPCVariable::createError(-32602, "Server address too short.");
		BaseLib::HelperFunctions::toLower(server.first);

		std::string path = "/RPC2";
		int32_t pos = server.second.find_first_of('/');
		if(pos > 0)
		{
			path = server.second.substr(pos);
			GD::out.printDebug("Debug: Server path set to: " + path);
			server.second = server.second.substr(0, pos);
			GD::out.printDebug("Debug: Server port set to: " + server.second);
		}
		server.second = std::to_string(BaseLib::HelperFunctions::getNumber(server.second));
		if(server.second.empty() || server.second == "0") return BaseLib::RPC::RPCVariable::createError(-32602, "Port number is invalid.");

		if(parameters->at(1)->stringValue.empty())
		{
			GD::rpcClient.removeServer(server);
		}
		else
		{
			std::shared_ptr<RemoteRPCServer> eventServer = GD::rpcClient.addServer(server, path, parameters->at(1)->stringValue);
			if(!eventServer)
			{
				GD::out.printError("Error: Could not create event server.");
				return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
			}
			if(server.first.compare(0, 5, "https") == 0 || server.first.compare(0, 7, "binarys") == 0) eventServer->useSSL = true;
			if(server.first.compare(0, 6, "binary") == 0 || server.first.compare(0, 7, "binarys") == 0) eventServer->binary = true;
			if(parameters->size() > 2)
			{
				if(parameters->at(2)->integerValue & 1) eventServer->keepAlive = true;
				if(parameters->at(2)->integerValue & 2) eventServer->binary = true;
				if(parameters->at(2)->integerValue & 4) eventServer->useID = true;
			}
			if(_initServerThread.joinable()) _initServerThread.join();
			_initServerThread = std::thread(&RPC::Client::initServerMethods, &GD::rpcClient, server);
		}

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListBidcosInterfaces::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		if(GD::deviceFamilies.at(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS)) return GD::deviceFamilies.at(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS)->listBidcosInterfaces();
		else return BaseLib::RPC::RPCVariable::createError(-32601, "Family HomeMatic BidCoS does not exist.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListClientServers::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		std::string id;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
			if(error != ParameterError::Enum::noError) return getError(error);
			id = parameters->at(0)->stringValue;
		}
		return GD::rpcClient.listClientServers(id);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListDevices::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean }),
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean, BaseLib::RPC::RPCVariableType::rpcArray }),
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		bool channels = true;
		std::map<std::string, bool> fields;
		if(parameters->size() == 2)
		{
			channels = parameters->at(0)->booleanValue;
			for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> devices(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->listDevices(channels, fields);
			if(result->errorStruct)
			{
				GD::out.printWarning("Warning: Error calling method \"listDevices\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) devices->arrayValue->insert(devices->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}

		return devices;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListEvents::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger }),
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger }),
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString })
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListFamilies::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		return GD::familyController.listFamilies();
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListInterfaces::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
					std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		return GD::physicalInterfaces.listInterfaces(familyID);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCListTeams::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::RPCVariable> teams(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::RPCVariable> result = central->listTeams();
			if(!result->arrayValue->empty()) teams->arrayValue->insert(teams->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
		}

		return teams;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCLogLevel::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger }));
			if(error != ParameterError::Enum::noError) return getError(error);
			int32_t debugLevel = parameters->at(0)->integerValue;
			if(debugLevel < 0) debugLevel = 2;
			if(debugLevel > 5) debugLevel = 5;
			GD::bl->debugLevel = debugLevel;
		}
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(GD::bl->debugLevel));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCPutParamset::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcStruct }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcStruct }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcStruct })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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
		BaseLib::RPC::ParameterSet::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			type = BaseLib::RPC::ParameterSet::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::none)
			{
				type = BaseLib::RPC::ParameterSet::Type::Enum::link;
				int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
			}
		}
		else
		{
			type = BaseLib::RPC::ParameterSet::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central)continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(serialNumber)) return central->putParamset(serialNumber, channel, type, remoteSerialNumber, remoteChannel, parameters->back());
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->putParamset(parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel, parameters->back());
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCRemoveEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.remove(parameters->at(0)->stringValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCRemoveLink::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
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
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(senderSerialNumber)) return central->removeLink(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->removeLink(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCRunScript::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcBoolean }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool wait = false;
		std::string filename;
		std::string arguments;

		filename = parameters->at(0)->stringValue;

		if(parameters->size() == 2)
		{
			if(parameters->at(1)->type == BaseLib::RPC::RPCVariableType::rpcBoolean) wait = parameters->at(1)->booleanValue;
			else arguments = parameters->at(1)->stringValue;
		}
		if(parameters->size() == 3)
		{
			arguments = parameters->at(1)->stringValue;
			wait = parameters->at(2)->booleanValue;
		}

		std::string path = GD::bl->settings.scriptPath() + filename;
		std::string command = path + " " + arguments;
		if(!wait) command += "&";

		struct stat statStruct;
		if(stat(path.c_str(), &statStruct) < 0) return BaseLib::RPC::RPCVariable::createError(-32400, "Could not execute script: " + std::string(strerror(errno)));
		int32_t uid = getuid();
		int32_t gid = getgid();
		if((statStruct.st_mode & S_IXOTH) == 0)
		{
			if(statStruct.st_gid != gid || (statStruct.st_gid == gid && (statStruct.st_mode & S_IXGRP) == 0))
			{
				if(statStruct.st_uid != uid || (statStruct.st_uid == uid && (statStruct.st_mode & S_IXUSR) == 0)) return BaseLib::RPC::RPCVariable::createError(-32400, "Could not execute script. No permission or executable bit is not set.");
			}
		}
		if((statStruct.st_mode & (S_IXGRP | S_IXUSR)) == 0) //At least in Debian it is not possible to execute scripts, when the execution bit is only set for "other".
			return BaseLib::RPC::RPCVariable::createError(-32400, "Could not execute script. Executable bit is not set for user or group.");
		if((statStruct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) return BaseLib::RPC::RPCVariable::createError(-32400, "Could not execute script. The file mode is not set to executable.");
		int32_t exitCode = std::system(command.c_str());
		int32_t signal = WIFSIGNALED(exitCode);
		if(signal) return BaseLib::RPC::RPCVariable::createError(-32400, "Script exited with signal: " + std::to_string(signal));
		exitCode = WEXITSTATUS(exitCode);
		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(exitCode));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSearchDevices::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		if(familyID > -1)
		{
			if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)familyID) == GD::deviceFamilies.end())
			{
				return BaseLib::RPC::RPCVariable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->searchDevices();
		}

		std::shared_ptr<BaseLib::RPC::RPCVariable> result(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcInteger));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central) result->integerValue += central->searchDevices()->integerValue;
		}

		return result;
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetInstallMode::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcBoolean, BaseLib::RPC::RPCVariableType::rpcInteger }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcBoolean }),
			std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcBoolean, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t enableIndex = -1;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcBoolean) enableIndex = 0;
		else if(parameters->size() == 2) enableIndex = 1;
		int32_t familyIDIndex = (parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcInteger) ? 0 : -1;
		int32_t timeIndex = -1;
		if(parameters->size() == 2 && parameters->at(1)->type == BaseLib::RPC::RPCVariableType::rpcInteger) timeIndex = 1;
		else if(parameters->size() == 3) timeIndex = 2;

		bool enable = (enableIndex > -1) ? parameters->at(enableIndex)->booleanValue : false;
		int32_t familyID = (familyIDIndex > -1) ? parameters->at(familyIDIndex)->integerValue : -1;

		uint32_t time = (timeIndex > -1) ? parameters->at(timeIndex)->integerValue : 60;
		if(time < 5) time = 60;
		if(time > 3600) time = 3600;

		if(familyID > -1)
		{
			if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)familyID) == GD::deviceFamilies.end())
			{
				return BaseLib::RPC::RPCVariable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->setInstallMode(enable, time);
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central) central->setInstallMode(enable, time);
		}

		return std::shared_ptr<BaseLib::RPC::RPCVariable>(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetInterface::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setInterface(parameters->at(0)->integerValue, parameters->at(1)->stringValue);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetLinkInfo::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString })

		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t nameIndex = 4;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			nameIndex = 2;
			int32_t pos = parameters->at(0)->stringValue.find(':');
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
		}

		std::string name;
		if(parameters->size() > nameIndex)
		{
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::RPC::RPCVariable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if(parameters->size() > nameIndex + 1)
		{
			if(parameters->at(nameIndex + 1)->stringValue.size() > 1000) return BaseLib::RPC::RPCVariable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(nameIndex + 1)->stringValue;
		}


		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(senderSerialNumber)) return central->setLinkInfo(senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setLinkInfo(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, name, description);
				}
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Sender device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcVariant }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcVariant })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		bool deviceFound = false;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber))
					{
						deviceFound = true;
						break;
					}
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue))
					{
						deviceFound = true;
						break;
					}
				}
			}
		}

		if(!deviceFound) return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");

		std::string objectID(useSerialNumber ? parameters->at(0)->stringValue : std::to_string(parameters->at(0)->integerValue));
		return GD::db.setMetadata(objectID, parameters->at(1)->stringValue, parameters->at(2));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetName::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central && central->knowsDevice(parameters->at(0)->integerValue))
			{
				return central->setName(parameters->at(0)->integerValue, parameters->at(1)->stringValue);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetSystemVariable::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcVariant }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::db.setSystemVariable(parameters->at(0)->stringValue, parameters->at(1));
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetTeam::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t deviceChannel = -1;
		std::string deviceSerialNumber;
		uint64_t teamID = 0;
		int32_t teamChannel = -1;
		std::string teamSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
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
		}
		else if(parameters->size() > 2)
		{
			teamID = (parameters->at(2)->integerValue == -1) ? 0 : parameters->at(2)->integerValue;
			teamChannel = parameters->at(3)->integerValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(deviceSerialNumber)) return central->setTeam(deviceSerialNumber, deviceChannel, teamSerialNumber, teamChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setTeam(parameters->at(0)->integerValue, parameters->at(1)->integerValue, teamID, teamChannel);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCSetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcVariant }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcString }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString, BaseLib::RPC::RPCVariableType::rpcVariant }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcString)
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

		std::shared_ptr<BaseLib::RPC::RPCVariable> value;
		if(useSerialNumber && parameters->size() == 3) value = parameters->at(2);
		else if(!useSerialNumber && parameters->size() == 4) value = parameters->at(3);
		else value.reset(new BaseLib::RPC::RPCVariable(BaseLib::RPC::RPCVariableType::rpcVoid));

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
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

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCTriggerEvent::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.trigger(parameters->at(0)->stringValue);
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::RPCVariable> RPCUpdateFirmware::invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::RPCVariableType>>({
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcInteger, BaseLib::RPC::RPCVariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::RPCVariableType>({ BaseLib::RPC::RPCVariableType::rpcArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool manual = false;
		bool array = true;
		if(parameters->at(0)->type == BaseLib::RPC::RPCVariableType::rpcInteger)
		{
			array = false;
			if(parameters->size() == 2 && parameters->at(1)->booleanValue) manual = true;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(array)
			{
				std::vector<uint64_t> ids;
				for(std::vector<std::shared_ptr<BaseLib::RPC::RPCVariable>>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
				{
					if((*i)->integerValue != 0 && central->knowsDevice((*i)->integerValue)) ids.push_back((*i)->integerValue);
				}
				if(ids.size() > 0 && ids.size() != parameters->at(0)->arrayValue->size()) return BaseLib::RPC::RPCVariable::createError(-2, "Please provide only devices of one device family.");
				if(ids.size() > 0) return central->updateFirmware(ids, manual);
			}
			else if(central->knowsDevice(parameters->at(0)->integerValue))
			{
				std::vector<uint64_t> ids;
				ids.push_back(parameters->at(0)->integerValue);
				return central->updateFirmware(ids, manual);
			}
		}

		return BaseLib::RPC::RPCVariable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

} /* namespace RPC */
