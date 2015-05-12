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

#include "RPCMethods.h"
#include "../GD/GD.h"
#include "../../Version.h"
#include "../../Modules/Base/BaseLib.h"
#include <sys/wait.h> //Needed for BSD

namespace RPC
{

std::shared_ptr<BaseLib::RPC::Variable> RPCSystemGetCapabilities::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::Variable> capabilities(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));

		std::shared_ptr<BaseLib::RPC::Variable> element(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specUrl", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("http://www.xmlrpc.com/spec")))));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specVersion", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(1))));
		capabilities->structValue->insert(BaseLib::RPC::RPCStructElement("xmlrpc", element));

		element.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specUrl", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php")))));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specVersion", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(20010516))));
		capabilities->structValue->insert(BaseLib::RPC::RPCStructElement("faults_interop", element));

		element.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specUrl", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("http://scripts.incutio.com/xmlrpc/introspection.html")))));
		element->structValue->insert(BaseLib::RPC::RPCStructElement("specVersion", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(1))));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSystemListMethods::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::Variable> methods(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));

		for(std::map<std::string, std::shared_ptr<RPCMethod>>::iterator i = _server->getMethods()->begin(); i != _server->getMethods()->end(); ++i)
		{
			methods->arrayValue->push_back(std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(i->first)));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSystemMethodHelp::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(_server->getMethods()->find(parameters->at(0)->stringValue) == _server->getMethods()->end())
		{
			return BaseLib::RPC::Variable::createError(-32602, "Method not found.");
		}

		std::shared_ptr<BaseLib::RPC::Variable> help = _server->getMethods()->at(parameters->at(0)->stringValue)->getHelp();

		if(!help) help.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcString));

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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSystemMethodSignature::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(_server->getMethods()->find(parameters->at(0)->stringValue) == _server->getMethods()->end())
		{
			return BaseLib::RPC::Variable::createError(-32602, "Method not found.");
		}

		std::shared_ptr<BaseLib::RPC::Variable> signature = _server->getMethods()->at(parameters->at(0)->stringValue)->getSignature();

		if(!signature) signature.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));

		if(signature->arrayValue->empty())
		{
			signature->type = BaseLib::RPC::VariableType::rpcString;
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSystemMulticall::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcArray }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		std::shared_ptr<BaseLib::RPC::Variable> returns(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
		{
			if((*i)->type != BaseLib::RPC::VariableType::rpcStruct)
			{
				returns->arrayValue->push_back(BaseLib::RPC::Variable::createError(-32602, "Array element is no struct."));
				continue;
			}
			if((*i)->structValue->size() != 2)
			{
				returns->arrayValue->push_back(BaseLib::RPC::Variable::createError(-32602, "Struct has wrong size."));
				continue;
			}
			if((*i)->structValue->find("methodName") == (*i)->structValue->end() || (*i)->structValue->at("methodName")->type != BaseLib::RPC::VariableType::rpcString)
			{
				returns->arrayValue->push_back(BaseLib::RPC::Variable::createError(-32602, "No method name provided."));
				continue;
			}
			if((*i)->structValue->find("params") == (*i)->structValue->end() || (*i)->structValue->at("params")->type != BaseLib::RPC::VariableType::rpcArray)
			{
				returns->arrayValue->push_back(BaseLib::RPC::Variable::createError(-32602, "No parameters provided."));
				continue;
			}
			std::string methodName = (*i)->structValue->at("methodName")->stringValue;
			std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters = (*i)->structValue->at("params")->arrayValue;

			if(methodName == "system.multicall") returns->arrayValue->push_back(BaseLib::RPC::Variable::createError(-32602, "Recursive calls to system.multicall are not allowed."));
			else if(methods->find(methodName) == methods->end()) returns->arrayValue->push_back(BaseLib::RPC::Variable::createError(-32601, "Requested method not found."));
			else returns->arrayValue->push_back(methods->at(methodName)->invoke(clientID, parameters));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCAbortEventReset::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCActivateLinkParamset::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool longPress = false;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;

			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else remoteSerialNumber = parameters->at(0)->stringValue;

			if(parameters->size() > 2) longPress = parameters->at(2)->booleanValue;
		}
		else if(parameters->size() > 4) longPress = parameters->at(4)->booleanValue;

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central)continue;
			if(useSerialNumber)
			{
				if(central->knowsDevice(serialNumber)) return central->activateLinkParamset(clientID, serialNumber, channel, remoteSerialNumber, remoteChannel, longPress);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->activateLinkParamset(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, longPress);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCAddDevice::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
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
				return BaseLib::RPC::Variable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->addDevice(clientID, serialNumber);
		}

		std::shared_ptr<BaseLib::RPC::Variable> result;
		std::shared_ptr<BaseLib::RPC::Variable> returnResult;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central) result = central->addDevice(clientID, serialNumber);
			if(result && !result->errorStruct) returnResult = result;
		}
		if(returnResult) return returnResult;
		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCAddEvent::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcStruct }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCAddLink::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t nameIndex = 4;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
		if((signed)parameters->size() > nameIndex)
		{
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::RPC::Variable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if((signed)parameters->size() > nameIndex + 1)
		{
			if(parameters->at(nameIndex + 1)->stringValue.size() > 1000) return BaseLib::RPC::Variable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(nameIndex + 1)->stringValue;
		}


		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(senderSerialNumber)) return central->addLink(clientID, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->addLink(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, name, description);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCClientServerInitialized::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCCreateDevice::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)parameters->at(0)->integerValue) == GD::deviceFamilies.end())
		{
			return BaseLib::RPC::Variable::createError(-2, "Device family is unknown.");
		}
		return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)parameters->at(0)->integerValue)->getCentral()->createDevice(clientID, parameters->at(1)->integerValue, parameters->at(2)->stringValue, parameters->at(3)->integerValue, parameters->at(4)->integerValue);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCDeleteDevice::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool useSerialNumber = (parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString) ? true : false;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(parameters->at(0)->stringValue)) return central->deleteDevice(clientID, parameters->at(0)->stringValue, parameters->at(1)->integerValue);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->deleteDevice(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCDeleteMetadata::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->logicalDevice()->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->logicalDevice()->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::RPC::Variable::createError(-2, "Device not found.");

		std::string dataID;
		if(parameters->size() > 1) dataID = parameters->at(1)->stringValue;
		return GD::db.deleteMetadata(peer->getID(), peer->getSerialNumber(), dataID);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCDeleteSystemVariable::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCEnableEvent::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetAllMetadata::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->logicalDevice()->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->logicalDevice()->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::RPC::Variable::createError(-2, "Device not found.");

		std::string peerName = peer->getName();
		std::shared_ptr<BaseLib::RPC::Variable> metadata = GD::db.getAllMetadata(peer->getID(), peer->getSerialNumber());
		if(!peerName.empty())
		{
			if(metadata->structValue->find("NAME") != metadata->structValue->end()) metadata->structValue->at("NAME")->stringValue = peerName;
			else
			{
				if(metadata->errorStruct) metadata.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct)); //Remove error struct
				metadata->structValue->insert(BaseLib::RPC::RPCStructElement("NAME", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(peerName))));
			}
		}
		return metadata;
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetAllScripts::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef SCRIPTENGINE
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		return GD::scriptEngine.getAllScripts();
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without script engine.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetAllSystemVariables::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetAllValues::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerID = 0;
		bool returnWriteOnly = false;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::RPC::VariableType::rpcBoolean)
		{
			returnWriteOnly = parameters->at(0)->booleanValue;
		}
		else if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::RPC::VariableType::rpcInteger)
		{
			peerID = parameters->at(0)->integerValue;
		}
		else if(parameters->size() == 2)
		{
			peerID = parameters->at(0)->integerValue;
			returnWriteOnly = parameters->at(1)->booleanValue;
		}

		std::shared_ptr<BaseLib::RPC::Variable> values(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(peerID > 0 && !central->knowsDevice(peerID)) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::Variable> result = central->getAllValues(clientID, peerID, returnWriteOnly);
			if(result && result->errorStruct)
			{
				if(peerID > 0) return result;
				else GD::out.printWarning("Warning: Error calling method \"getAllValues\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) values->arrayValue->insert(values->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			if(peerID > 0) break;
		}

		if(values->arrayValue->empty() && peerID > 0) return BaseLib::RPC::Variable::createError(-2, "Unknown device.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetDeviceDescription::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
			if(serialNumber == "BidCoS-RF" || serialNumber == "Homegear") //Return version info
			{
				std::shared_ptr<BaseLib::RPC::Variable> description(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
				description->structValue->insert(BaseLib::RPC::RPCStructElement("TYPE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string("Homegear")))));
				description->structValue->insert(BaseLib::RPC::RPCStructElement("FIRMWARE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(std::string(VERSION)))));
				return description;
			}
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getDeviceDescription(clientID, serialNumber, channel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getDeviceDescription(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetDeviceInfo::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcArray }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcArray })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerID = 0;
		int32_t fieldsIndex = -1;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::RPC::VariableType::rpcArray)
		{
			fieldsIndex = 0;
		}
		else if(parameters->size() == 2 || (parameters->size() == 1 && parameters->at(0)->type == BaseLib::RPC::VariableType::rpcInteger))
		{
			if(parameters->size() == 2) fieldsIndex = 1;
			peerID = parameters->at(0)->integerValue;
		}


		std::map<std::string, bool> fields;
		if(fieldsIndex > -1)
		{
			for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->at(fieldsIndex)->arrayValue->begin(); i != parameters->at(fieldsIndex)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		std::shared_ptr<BaseLib::RPC::Variable> deviceInfo(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			if(peerID > 0)
			{
				if(central->knowsDevice(peerID)) return central->getDeviceInfo(clientID, peerID, fields);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				std::shared_ptr<BaseLib::RPC::Variable> result = central->getDeviceInfo(clientID, peerID, fields);
				if(result && result->errorStruct)
				{
					GD::out.printWarning("Warning: Error calling method \"listDevices\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
					continue;
				}
				if(result && !result->arrayValue->empty()) deviceInfo->arrayValue->insert(deviceInfo->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
		}

		if(peerID == 0) return deviceInfo;
		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetEvent::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler.get(parameters->at(0)->stringValue);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetInstallMode::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		if(familyID > -1)
		{
			if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)familyID) == GD::deviceFamilies.end())
			{
				return BaseLib::RPC::Variable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->getInstallMode(clientID);
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				std::shared_ptr<BaseLib::RPC::Variable> result = central->getInstallMode(clientID);
				if(result->integerValue > 0) return result;
			}
		}

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcInteger));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetKeyMismatchDevice::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);
		//TODO Implement method when AES is supported.
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcString));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetLinkInfo::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
					if(central->knowsDevice(senderSerialNumber)) return central->getLinkInfo(clientID, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getLinkInfo(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetLinkPeers::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
					if(central->knowsDevice(serialNumber)) return central->getLinkPeers(clientID, serialNumber, channel);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getLinkPeers(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetLinks::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>(),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t flags = 0;
		uint64_t peerID = 0;

		bool useSerialNumber = false;
		if(parameters->size() > 0)
		{
			if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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

		std::shared_ptr<BaseLib::RPC::Variable> links(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			if(serialNumber.empty() && peerID == 0)
			{
				std::shared_ptr<BaseLib::RPC::Variable> result = central->getLinks(clientID, peerID, channel, flags);
				if(result && !result->arrayValue->empty()) links->arrayValue->insert(links->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
			else
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getLinks(clientID, serialNumber, channel, flags);
				}
				else
				{
					if(central->knowsDevice(peerID)) return central->getLinks(clientID, peerID, channel, flags);
				}
			}
		}

		if(serialNumber.empty() && peerID == 0) return links;
		else return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetMetadata::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->logicalDevice()->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->logicalDevice()->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::RPC::Variable::createError(-2, "Device not found.");

		if(parameters->at(1)->stringValue == "NAME")
		{
			std::string peerName = peer->getName();
			if(peerName.size() > 0) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(peerName));
			std::shared_ptr<BaseLib::RPC::Variable> rpcName = GD::db.getMetadata(peer->getID(), peer->getSerialNumber(), parameters->at(1)->stringValue);
			if(peerName.size() == 0 && !rpcName->errorStruct)
			{
				peer->setName(rpcName->stringValue);
				GD::db.deleteMetadata(peer->getID(), peer->getSerialNumber(), parameters->at(1)->stringValue);
			}
			return rpcName;
		}
		else return GD::db.getMetadata(peer->getID(), peer->getSerialNumber(), parameters->at(1)->stringValue);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetName::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central && central->knowsDevice(parameters->at(0)->integerValue))
			{
				return central->getName(clientID, parameters->at(0)->integerValue);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetParamsetDescription::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::VariableType::rpcString)
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
				if(central->knowsDevice(serialNumber)) return central->getParamsetDescription(clientID, serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamsetDescription(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetParamsetId::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t channel = 0;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::VariableType::rpcString)
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
				if(central->knowsDevice(serialNumber)) return central->getParamsetId(clientID, serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamsetId(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetParamset::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::VariableType::rpcString)
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
				if(central->knowsDevice(serialNumber)) return central->getParamset(clientID, serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getParamset(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetPeerId::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string filterValue = (parameters->size() > 1) ? parameters->at(1)->stringValue : "";

		std::shared_ptr<BaseLib::RPC::Variable> ids(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				std::shared_ptr<BaseLib::RPC::Variable> result = central->getPeerID(clientID, parameters->at(0)->integerValue, filterValue);
				if(result && !result->errorStruct && result->arrayValue->size() > 0) ids->arrayValue->insert(ids->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
		}

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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetServiceMessages::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>(),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool id = false;
		if(parameters->size() == 1) id = parameters->at(0)->booleanValue;

		std::shared_ptr<BaseLib::RPC::Variable> serviceMessages(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			//getServiceMessages really needs a lot of ressources, so wait a little bit after each central
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::Variable> messages = central->getServiceMessages(clientID, id);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetSystemVariable::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetUpdateStatus::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>());
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<BaseLib::RPC::Variable> updateInfo(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));

		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("DEVICE_COUNT", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((uint32_t)GD::bl->deviceUpdateInfo.devicesToUpdate))));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CURRENT_UPDATE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((uint32_t)GD::bl->deviceUpdateInfo.currentUpdate))));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CURRENT_DEVICE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((uint32_t)GD::bl->deviceUpdateInfo.currentDevice))));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("CURRENT_DEVICE_PROGRESS", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable((uint32_t)GD::bl->deviceUpdateInfo.currentDeviceProgress))));

		std::shared_ptr<BaseLib::RPC::Variable> results(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		updateInfo->structValue->insert(BaseLib::RPC::RPCStructElement("RESULTS", results));
		for(std::map<uint64_t, std::pair<int32_t, std::string>>::iterator i = GD::bl->deviceUpdateInfo.results.begin(); i != GD::bl->deviceUpdateInfo.results.end(); ++i)
		{
			std::shared_ptr<BaseLib::RPC::Variable> result(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
			result->structValue->insert(BaseLib::RPC::RPCStructElement("RESULT_CODE", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(i->second.first))));
			result->structValue->insert(BaseLib::RPC::RPCStructElement("RESULT_STRING", std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(i->second.second))));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetValue::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		bool requestFromDevice = false;
		bool asynchronously = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
			if(parameters->size() >= 3) requestFromDevice = parameters->at(2)->booleanValue;
			if(parameters->size() >= 4) asynchronously = parameters->at(3)->booleanValue;
		}
		else
		{
			if(parameters->size() >= 4) requestFromDevice = parameters->at(3)->booleanValue;
			if(parameters->size() >= 5) asynchronously = parameters->at(4)->booleanValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->getValue(clientID, serialNumber, channel, parameters->at(1)->stringValue, requestFromDevice, asynchronously);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->getValue(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue, requestFromDevice, asynchronously);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCGetVersion::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string version(VERSION);
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable("Homegear " + version));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

RPCInit::~RPCInit()
{
	try
	{
		_disposing = true;
		_initServerThreadMutex.lock();
		if(_initServerThread.joinable()) _initServerThread.join();
		_initServerThreadMutex.unlock();
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

std::shared_ptr<BaseLib::RPC::Variable> RPCInit::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Method is disposing.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::split(parameters->at(0)->stringValue, ':');
		if(server.first.empty() || server.second.empty()) return BaseLib::RPC::Variable::createError(-32602, "Server address or port is empty.");
		if(server.first.size() < 8) return BaseLib::RPC::Variable::createError(-32602, "Server address too short.");
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
		server.second = std::to_string(BaseLib::Math::getNumber(server.second));
		if(server.second.empty() || server.second == "0") return BaseLib::RPC::Variable::createError(-32602, "Port number is invalid.");

		if(parameters->size() == 1 || parameters->at(1)->stringValue.empty())
		{
			GD::rpcClient.removeServer(server);
		}
		else
		{
			std::shared_ptr<RemoteRPCServer> eventServer = GD::rpcClient.addServer(server, path, parameters->at(1)->stringValue);
			if(!eventServer)
			{
				GD::out.printError("Error: Could not create event server.");
				return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
			}
			if(server.first.compare(0, 5, "https") == 0 || server.first.compare(0, 7, "binarys") == 0) eventServer->useSSL = true;
			if(server.first.compare(0, 6, "binary") == 0 ||
			   server.first.compare(0, 7, "binarys") == 0 ||
			   server.first.compare(0, 10, "xmlrpc_bin") == 0) eventServer->binary = true;
			if(parameters->size() > 2)
			{
				eventServer->keepAlive = (parameters->at(2)->integerValue & 1);
				eventServer->binary = (parameters->at(2)->integerValue & 2);
				eventServer->useID = (parameters->at(2)->integerValue & 4);
				eventServer->subscribePeers = (parameters->at(2)->integerValue & 8);
				eventServer->json = (parameters->at(2)->integerValue & 16);
				eventServer->reconnectInfinitely = (parameters->at(2)->integerValue & 128);
			}
			//Reconnect on CCU2 as it doesn't reconnect automatically
			if(parameters->at(1)->stringValue == "1008" || parameters->at(1)->stringValue == "Homegear_java") eventServer->reconnectInfinitely = true;
			_initServerThreadMutex.lock();
			try
			{
				if(_disposing)
				{
					_initServerThreadMutex.unlock();
					return BaseLib::RPC::Variable::createError(-32500, "I'm disposing.");
				}
				if(_initServerThread.joinable()) _initServerThread.join();
				_initServerThread = std::thread(&RPC::Client::initServerMethods, &GD::rpcClient, server);
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
			_initServerThreadMutex.unlock();
		}

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListBidcosInterfaces::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		if(GD::deviceFamilies.find(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS) != GD::deviceFamilies.end() && GD::deviceFamilies.at(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS)) return GD::deviceFamilies.at(BaseLib::Systems::DeviceFamilies::HomeMaticBidCoS)->listBidcosInterfaces();
		else return BaseLib::RPC::Variable::createError(-32601, "Family HomeMatic BidCoS does not exist.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListClientServers::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		std::string id;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListDevices::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean }),
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcArray }),
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		bool channels = true;
		std::map<std::string, bool> fields;
		if(parameters->size() == 2)
		{
			channels = parameters->at(0)->booleanValue;
			for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		std::shared_ptr<BaseLib::RPC::Variable> devices(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::Variable> result = central->listDevices(clientID, channels, fields);
			if(result && result->errorStruct)
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListEvents::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListFamilies::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListInterfaces::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
					std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger })
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCListTeams::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::Variable> teams(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcArray));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::Variable> result = central->listTeams(clientID);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCLogLevel::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }));
			if(error != ParameterError::Enum::noError) return getError(error);
			int32_t debugLevel = parameters->at(0)->integerValue;
			if(debugLevel < 0) debugLevel = 2;
			if(debugLevel > 5) debugLevel = 5;
			GD::bl->debugLevel = debugLevel;
		}
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(GD::bl->debugLevel));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCPutParamset::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcStruct }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcStruct }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcStruct })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
		if(parameters->at(parameterSetIndex)->type == BaseLib::RPC::VariableType::rpcString)
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
				if(central->knowsDevice(serialNumber)) return central->putParamset(clientID, serialNumber, channel, type, remoteSerialNumber, remoteChannel, parameters->back());
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->putParamset(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel, parameters->back());
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCRemoveEvent::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCRemoveLink::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
				if(central->knowsDevice(senderSerialNumber)) return central->removeLink(clientID, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->removeLink(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCReportValueUsage::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central && central->knowsDevice(serialNumber)) return central->reportValueUsage(clientID, serialNumber);
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCRssiInfo::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<BaseLib::RPC::Variable> response(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			std::shared_ptr<BaseLib::RPC::Variable> result = central->rssiInfo(clientID);
			if(result && result->errorStruct)
			{
				GD::out.printWarning("Warning: Error calling method \"rssiInfo\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->structValue->empty()) response->structValue->insert(result->structValue->begin(), result->structValue->end());
		}

		return response;
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCRunScript::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef SCRIPTENGINE
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool internalEngine = false;

		bool wait = false;
		std::string filename;
		std::string arguments;

		filename = parameters->at(0)->stringValue;
		std::string ending = "";
		int32_t pos = filename.find_last_of('.');
		if(pos != (signed)std::string::npos) ending = filename.substr(pos);
		GD::bl->hf.toLower(ending);
		if(ending == ".php" || ending == ".php5") internalEngine = true;

		if((signed)parameters->size() == 2)
		{
			if(parameters->at(1)->type == BaseLib::RPC::VariableType::rpcBoolean) wait = parameters->at(1)->booleanValue;
			else arguments = parameters->at(1)->stringValue;
		}
		if((signed)parameters->size() == 3)
		{
			arguments = parameters->at(1)->stringValue;
			wait = parameters->at(2)->booleanValue;
		}

		std::string path = GD::bl->settings.scriptPath() + filename;

		int32_t exitCode = 0;
		if(internalEngine)
		{
			if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Executing script \"" + path + "\" with parameters \"" + arguments + "\" using internal script engine.");
			std::shared_ptr<std::vector<char>> scriptOutput(new std::vector<char>());
			GD::scriptEngine.execute(path, arguments, scriptOutput, &exitCode, wait);
			if(scriptOutput->size() > 0)
			{
				std::string outputString(&scriptOutput->at(0), &scriptOutput->at(0) + scriptOutput->size());
				return BaseLib::RPC::PVariable(new BaseLib::RPC::Variable(outputString));
			}
			else return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(exitCode));
		}
		else
		{
			if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Executing program/script \"" + path + "\" with parameters \"" + arguments + "\".");
			std::string command = path + " " + arguments;
			if(!wait) command += "&";

			struct stat statStruct;
			if(stat(path.c_str(), &statStruct) < 0) return BaseLib::RPC::Variable::createError(-32400, "Could not execute script: " + std::string(strerror(errno)));
			uint32_t uid = getuid();
			uint32_t gid = getgid();
			if((statStruct.st_mode & S_IXOTH) == 0)
			{
				if(statStruct.st_gid != gid || (statStruct.st_gid == gid && (statStruct.st_mode & S_IXGRP) == 0))
				{
					if(statStruct.st_uid != uid || (statStruct.st_uid == uid && (statStruct.st_mode & S_IXUSR) == 0)) return BaseLib::RPC::Variable::createError(-32400, "Could not execute script. No permission or executable bit is not set.");
				}
			}
			if((statStruct.st_mode & (S_IXGRP | S_IXUSR)) == 0) //At least in Debian it is not possible to execute scripts, when the execution bit is only set for "other".
				return BaseLib::RPC::Variable::createError(-32400, "Could not execute script. Executable bit is not set for user or group.");
			if((statStruct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0) return BaseLib::RPC::Variable::createError(-32400, "Could not execute script. The file mode is not set to executable.");
			exitCode = std::system(command.c_str());
			int32_t signal = WIFSIGNALED(exitCode);
			if(signal) return BaseLib::RPC::Variable::createError(-32400, "Script exited with signal: " + std::to_string(signal));
			exitCode = WEXITSTATUS(exitCode);
			return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(exitCode));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without script engine.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSearchDevices::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		if(familyID > -1)
		{
			if(GD::deviceFamilies.find((BaseLib::Systems::DeviceFamilies)familyID) == GD::deviceFamilies.end())
			{
				return BaseLib::RPC::Variable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->searchDevices(clientID);
		}

		std::shared_ptr<BaseLib::RPC::Variable> result(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcInteger));
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central) result->integerValue += central->searchDevices(clientID)->integerValue;
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetId::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central && central->knowsDevice(parameters->at(0)->integerValue))
			{
				return central->setId(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetInstallMode::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcInteger }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean }),
			std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t enableIndex = -1;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcBoolean) enableIndex = 0;
		else if(parameters->size() == 2) enableIndex = 1;
		int32_t familyIDIndex = (parameters->at(0)->type == BaseLib::RPC::VariableType::rpcInteger) ? 0 : -1;
		int32_t timeIndex = -1;
		if(parameters->size() >= 2 && parameters->at(1)->type == BaseLib::RPC::VariableType::rpcInteger) timeIndex = 1;
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
				return BaseLib::RPC::Variable::createError(-2, "Device family is unknown.");
			}
			return GD::deviceFamilies.at((BaseLib::Systems::DeviceFamilies)familyID)->getCentral()->setInstallMode(enable, time);
		}

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central) central->setInstallMode(enable, time);
		}

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetInterface::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setInterface(clientID, parameters->at(0)->integerValue, parameters->at(1)->stringValue);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetLinkInfo::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString })

		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t nameIndex = 4;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
		if((signed)parameters->size() > nameIndex)
		{
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::RPC::Variable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if((signed)parameters->size() > nameIndex + 1)
		{
			if(parameters->at(nameIndex + 1)->stringValue.size() > 1000) return BaseLib::RPC::Variable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(nameIndex + 1)->stringValue;
		}


		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(senderSerialNumber)) return central->setLinkInfo(clientID, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setLinkInfo(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, name, description);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Sender device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetMetadata::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->logicalDevice()->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->logicalDevice()->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::RPC::Variable::createError(-2, "Device not found.");

		if(parameters->at(1)->stringValue == "NAME")
		{
			peer->setName(parameters->at(2)->stringValue);
			return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
		}
		return GD::db.setMetadata(peer->getID(), peer->getSerialNumber(), parameters->at(1)->stringValue, parameters->at(2));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetName::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central && central->knowsDevice(parameters->at(0)->integerValue))
			{
				return central->setName(clientID, parameters->at(0)->integerValue, parameters->at(1)->stringValue);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetSystemVariable::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetTeam::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t deviceChannel = -1;
		std::string deviceSerialNumber;
		uint64_t teamID = 0;
		int32_t teamChannel = -1;
		std::string teamSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
				if(central->knowsDevice(deviceSerialNumber)) return central->setTeam(clientID, deviceSerialNumber, deviceChannel, teamSerialNumber, teamChannel);
			}
			else
			{
				if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setTeam(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, teamID, teamChannel);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSetValue::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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

		std::shared_ptr<BaseLib::RPC::Variable> value;
		if(useSerialNumber && parameters->size() == 3) value = parameters->at(2);
		else if(!useSerialNumber && parameters->size() == 4) value = parameters->at(3);
		else value.reset(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));

		for(std::map<BaseLib::Systems::DeviceFamilies, std::unique_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = GD::deviceFamilies.begin(); i != GD::deviceFamilies.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::Central> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->knowsDevice(serialNumber)) return central->setValue(clientID, serialNumber, channel, parameters->at(1)->stringValue, value);
				}
				else
				{
					if(central->knowsDevice(parameters->at(0)->integerValue)) return central->setValue(clientID, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue, value);
				}
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCSubscribePeers::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue.empty()) return BaseLib::RPC::Variable::createError(-32602, "Server id is empty.");
		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::split(parameters->at(0)->stringValue, ':');
		BaseLib::HelperFunctions::toLower(server.first);

		int32_t pos = server.second.find_first_of('/');
		if(pos > 0)
		{
			server.second = server.second.substr(0, pos);
			GD::out.printDebug("Debug: Server port set to: " + server.second);
		}
		if(!server.second.empty()) //Port number specified
		{
			server.second = std::to_string(BaseLib::Math::getNumber(server.second));
			if(server.second.empty() || server.second == "0") return BaseLib::RPC::Variable::createError(-32602, "Port number is invalid.");
		}

		std::shared_ptr<RemoteRPCServer> eventServer = GD::rpcClient.getServer(server);
		if(!eventServer) return BaseLib::RPC::Variable::createError(-1, "Event server is unknown.");
		for(BaseLib::RPC::RPCArray::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
		{
			if((*i)->integerValue != 0) eventServer->subscribedPeers.insert((*i)->integerValue);
		}

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCTriggerEvent::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
#else
    return BaseLib::RPC::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

std::shared_ptr<BaseLib::RPC::Variable> RPCTriggerRPCEvent::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcArray }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue == "deleteDevices")
		{
			if(parameters->at(1)->arrayValue->size() != 2) return BaseLib::RPC::Variable::createError(-1, "Wrong parameter count. You need to pass (in this order): Array deviceAddresses, Struct deviceInfo");
			GD::rpcClient.broadcastDeleteDevices(parameters->at(1)->arrayValue->at(0), parameters->at(1)->arrayValue->at(1));
		}
		else if(parameters->at(0)->stringValue == "newDevices")
		{
			GD::rpcClient.broadcastNewDevices(parameters->at(1));
		}
		else if(parameters->at(0)->stringValue == "updateDevice")
		{
			if(parameters->at(1)->arrayValue->size() != 4) return BaseLib::RPC::Variable::createError(-1, "Wrong parameter count. You need to pass (in this order): Integer peerID, Integer channel, String address, Integer flags");
			GD::rpcClient.broadcastUpdateDevice(parameters->at(1)->arrayValue->at(0)->integerValue, parameters->at(1)->arrayValue->at(1)->integerValue, parameters->at(1)->arrayValue->at(2)->stringValue, (RPC::Client::Hint::Enum)parameters->at(1)->arrayValue->at(3)->integerValue);
		}
		else return BaseLib::RPC::Variable::createError(-1, "Invalid function.");

		return BaseLib::RPC::PVariable(new BaseLib::RPC::Variable());
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCUnsubscribePeers::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue.empty()) return BaseLib::RPC::Variable::createError(-32602, "Server id is empty.");
		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::split(parameters->at(0)->stringValue, ':');
		BaseLib::HelperFunctions::toLower(server.first);

		int32_t pos = server.second.find_first_of('/');
		if(pos > 0)
		{
			server.second = server.second.substr(0, pos);
			GD::out.printDebug("Debug: Server port set to: " + server.second);
		}
		if(!server.second.empty()) //Port number specified
		{
			server.second = std::to_string(BaseLib::Math::getNumber(server.second));
			if(server.second.empty() || server.second == "0") return BaseLib::RPC::Variable::createError(-32602, "Port number is invalid.");
		}

		std::shared_ptr<RemoteRPCServer> eventServer = GD::rpcClient.getServer(server);
		if(!eventServer) return BaseLib::RPC::Variable::createError(-1, "Event server is unknown.");
		for(BaseLib::RPC::RPCArray::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
		{
			if((*i)->integerValue != 0) eventServer->subscribedPeers.erase((*i)->integerValue);
		}

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCUpdateFirmware::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool manual = false;
		bool array = true;
		if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcInteger || parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString)
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
				for(std::vector<std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
				{
					if((*i)->type == BaseLib::RPC::VariableType::rpcInteger && (*i)->integerValue != 0 && central->knowsDevice((*i)->integerValue)) ids.push_back((*i)->integerValue);
					else if((*i)->type == BaseLib::RPC::VariableType::rpcString && central->knowsDevice((*i)->stringValue)) ids.push_back(central->getPeerID(clientID, (*i)->stringValue)->integerValue);
				}
				if(ids.size() > 0 && ids.size() != parameters->at(0)->arrayValue->size()) return BaseLib::RPC::Variable::createError(-2, "Please provide only devices of one device family.");
				if(ids.size() > 0) return central->updateFirmware(clientID, ids, manual);
			}
			else if((parameters->at(0)->type == BaseLib::RPC::VariableType::rpcInteger && central->knowsDevice(parameters->at(0)->integerValue)) ||
					(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString && central->knowsDevice(parameters->at(0)->stringValue)))
			{
				std::vector<uint64_t> ids;
				if(parameters->at(0)->type == BaseLib::RPC::VariableType::rpcString) ids.push_back(central->getPeerID(clientID, parameters->at(0)->stringValue)->integerValue);
				else ids.push_back(parameters->at(0)->integerValue);
				return central->updateFirmware(clientID, ids, manual);
			}
		}

		return BaseLib::RPC::Variable::createError(-2, "Device not found.");
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> RPCWriteLog::invoke(int32_t clientID, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::RPC::VariableType>>({
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString }),
				std::vector<BaseLib::RPC::VariableType>({ BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->size() == 2) GD::out.printMessage(parameters->at(0)->stringValue, parameters->at(1)->integerValue, true);
		else GD::out.printMessage(parameters->at(0)->stringValue);

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

} /* namespace RPC */
