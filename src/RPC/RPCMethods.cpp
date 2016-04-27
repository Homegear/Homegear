/* Copyright 2013-2016 Sathya Laufer
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

#include "RPCMethods.h"
#include "../GD/GD.h"
#include "homegear-base/BaseLib.h"
#ifdef BSDSYSTEM
	#include <sys/wait.h>
#endif

namespace RPC
{

BaseLib::PVariable RPCDevTest::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		bool value = false;
		if(parameters->size() > 0) value = parameters->at(0)->booleanValue;

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>{ "PRESS_SHORT" });
		std::shared_ptr<std::vector<BaseLib::PVariable>> values(new std::vector<BaseLib::PVariable>{ BaseLib::PVariable(new BaseLib::Variable(value)) });
		GD::rpcClient->broadcastEvent(90, 1, "OPNWEATHES:1", valueKeys, values);
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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

BaseLib::PVariable RPCSystemGetCapabilities::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		BaseLib::PVariable capabilities(new BaseLib::Variable(BaseLib::VariableType::tStruct));

		BaseLib::PVariable element(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		element->structValue->insert(BaseLib::StructElement("specUrl", BaseLib::PVariable(new BaseLib::Variable(std::string("http://www.xmlrpc.com/spec")))));
		element->structValue->insert(BaseLib::StructElement("specVersion", BaseLib::PVariable(new BaseLib::Variable(1))));
		capabilities->structValue->insert(BaseLib::StructElement("xmlrpc", element));

		element.reset(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		element->structValue->insert(BaseLib::StructElement("specUrl", BaseLib::PVariable(new BaseLib::Variable(std::string("http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php")))));
		element->structValue->insert(BaseLib::StructElement("specVersion", BaseLib::PVariable(new BaseLib::Variable(20010516))));
		capabilities->structValue->insert(BaseLib::StructElement("faults_interop", element));

		element.reset(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		element->structValue->insert(BaseLib::StructElement("specUrl", BaseLib::PVariable(new BaseLib::Variable(std::string("http://scripts.incutio.com/xmlrpc/introspection.html")))));
		element->structValue->insert(BaseLib::StructElement("specVersion", BaseLib::PVariable(new BaseLib::Variable(1))));
		capabilities->structValue->insert(BaseLib::StructElement("introspection", element));

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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSystemListMethods::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		BaseLib::PVariable methodInfo(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		for(std::map<std::string, std::shared_ptr<RPCMethod>>::iterator i = methods->begin(); i != methods->end(); ++i)
		{
			methodInfo->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(i->first)));
		}

		return methodInfo;
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

BaseLib::PVariable RPCSystemMethodHelp::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		if(methods->find(parameters->at(0)->stringValue) == methods->end())
		{
			return BaseLib::Variable::createError(-32602, "Method not found.");
		}

		BaseLib::PVariable help = _server->getMethods()->at(parameters->at(0)->stringValue)->getHelp();

		if(!help) help.reset(new BaseLib::Variable(BaseLib::VariableType::tString));

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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSystemMethodSignature::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		if(methods->find(parameters->at(0)->stringValue) == methods->end())
		{
			return BaseLib::Variable::createError(-32602, "Method not found.");
		}

		BaseLib::PVariable signature = _server->getMethods()->at(parameters->at(0)->stringValue)->getSignature();

		if(!signature) signature.reset(new BaseLib::Variable(BaseLib::VariableType::tArray));

		if(signature->arrayValue->empty())
		{
			signature->type = BaseLib::VariableType::tString;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSystemMulticall::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tArray }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<std::map<std::string, std::shared_ptr<RPCMethod>>> methods = _server->getMethods();
		BaseLib::PVariable returns(new BaseLib::Variable(BaseLib::VariableType::tArray));
		for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
		{
			if((*i)->type != BaseLib::VariableType::tStruct)
			{
				returns->arrayValue->push_back(BaseLib::Variable::createError(-32602, "Array element is no struct."));
				continue;
			}
			if((*i)->structValue->size() != 2)
			{
				returns->arrayValue->push_back(BaseLib::Variable::createError(-32602, "Struct has wrong size."));
				continue;
			}
			if((*i)->structValue->find("methodName") == (*i)->structValue->end() || (*i)->structValue->at("methodName")->type != BaseLib::VariableType::tString)
			{
				returns->arrayValue->push_back(BaseLib::Variable::createError(-32602, "No method name provided."));
				continue;
			}
			if((*i)->structValue->find("params") == (*i)->structValue->end() || (*i)->structValue->at("params")->type != BaseLib::VariableType::tArray)
			{
				returns->arrayValue->push_back(BaseLib::Variable::createError(-32602, "No parameters provided."));
				continue;
			}
			std::string methodName = (*i)->structValue->at("methodName")->stringValue;
			std::shared_ptr<std::vector<BaseLib::PVariable>> parameters = (*i)->structValue->at("params")->arrayValue;

			if(methodName == "system.multicall") returns->arrayValue->push_back(BaseLib::Variable::createError(-32602, "Recursive calls to system.multicall are not allowed."));
			else if(methods->find(methodName) == methods->end()) returns->arrayValue->push_back(BaseLib::Variable::createError(-32601, "Requested method not found."));
			else returns->arrayValue->push_back(methods->at(methodName)->invoke(clientInfo, parameters));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCAbortEventReset::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler->abortReset(parameters->at(0)->stringValue);
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCActivateLinkParamset::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool longPress = false;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
			// {{{ HomeMatic-Konfigurator Bugfix
			std::string copy = remoteSerialNumber;
			BaseLib::HelperFunctions::toUpper(copy);
			if(copy == serialNumber) remoteSerialNumber = copy;
			// }}}

			if(parameters->size() > 2) longPress = parameters->at(2)->booleanValue;
		}
		else if(parameters->size() > 4) longPress = parameters->at(4)->booleanValue;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central)continue;
			if(useSerialNumber)
			{
				if(central->peerExists(serialNumber)) return central->activateLinkParamset(clientInfo, serialNumber, channel, remoteSerialNumber, remoteChannel, longPress);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->activateLinkParamset(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, longPress);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCAddDevice::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(familyID > -1)
		{
			if(families.find(familyID) == families.end())
			{
				return BaseLib::Variable::createError(-2, "Device family is unknown.");
			}
			return families.at(familyID)->getCentral()->addDevice(clientInfo, serialNumber);
		}

		BaseLib::PVariable result;
		BaseLib::PVariable returnResult;
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central) result = central->addDevice(clientInfo, serialNumber);
			if(result && !result->errorStruct) returnResult = result;
		}
		if(returnResult) return returnResult;
		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCAddEvent::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tStruct }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler->add(parameters->at(0));
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCAddLink::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t nameIndex = 4;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::Variable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if((signed)parameters->size() > nameIndex + 1)
		{
			if(parameters->at(nameIndex + 1)->stringValue.size() > 1000) return BaseLib::Variable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(nameIndex + 1)->stringValue;
		}


		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(senderSerialNumber)) return central->addLink(clientInfo, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->addLink(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, name, description);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCCopyConfig::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel1 = -1;
		std::string serialNumber1;
		int32_t channel2 = -1;
		std::string serialNumber2;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber1 = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel1 = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber1 = parameters->at(0)->stringValue;
		}

		if(parameters->at(2)->type == BaseLib::VariableType::tString)
		{
			int32_t pos = parameters->at(2)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber2 = parameters->at(2)->stringValue.substr(0, pos);
				if(parameters->at(2)->stringValue.size() > (unsigned)pos + 1) channel2 = std::stoll(parameters->at(2)->stringValue.substr(pos + 1));
			}
			else serialNumber2 = parameters->at(2)->stringValue;
		}

		BaseLib::PVariable parameterSet1;
		BaseLib::PVariable parameterSet2;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central)continue;
			if(useSerialNumber)
			{
				if(central->peerExists(serialNumber1)) parameterSet1 = central->getParamset(clientInfo, serialNumber1, channel1, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, "", -1);
				if(central->peerExists(serialNumber2)) parameterSet2 = central->getParamset(clientInfo, serialNumber2, channel2, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, "", -1);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) parameterSet1 = central->getParamset(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, 0, -1);
				if(central->peerExists((uint64_t)parameters->at(2)->integerValue)) parameterSet2 = central->getParamset(clientInfo, parameters->at(2)->integerValue, parameters->at(3)->integerValue, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, 0, -1);
			}
		}

		if(!parameterSet1) return BaseLib::Variable::createError(-3, "Source parameter set not found.");
		if(!parameterSet2) return BaseLib::Variable::createError(-3, "Target parameter set not found.");

		BaseLib::PVariable resultSet(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		for(BaseLib::Struct::iterator i = parameterSet1->structValue->begin(); i != parameterSet1->structValue->end(); ++i)
		{
			BaseLib::Struct::iterator set2Iterator = parameterSet2->structValue->find(i->first);
			if(set2Iterator == parameterSet2->structValue->end() || *(set2Iterator->second) == *(i->second) || set2Iterator->second->type != i->second->type) continue;

			resultSet->structValue->insert(BaseLib::StructElement(i->first, i->second));
		}

		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->peerExists(serialNumber2)) return central->putParamset(clientInfo, serialNumber2, channel2, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, "", -1, resultSet);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(2)->integerValue)) return central->putParamset(clientInfo, parameters->at(2)->integerValue, parameters->at(3)->integerValue, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, 0, -1, resultSet);
			}
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCClientServerInitialized::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string id = parameters->at(0)->stringValue;
		return GD::rpcClient->clientServerInitialized(id);
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

BaseLib::PVariable RPCCreateDevice::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(families.find(parameters->at(0)->integerValue) == families.end())
		{
			return BaseLib::Variable::createError(-2, "Device family is unknown.");
		}
		return families.at(parameters->at(0)->integerValue)->getCentral()->createDevice(clientInfo, parameters->at(1)->integerValue, parameters->at(2)->stringValue, parameters->at(3)->integerValue, parameters->at(4)->integerValue);
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

BaseLib::PVariable RPCDeleteDevice::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool useSerialNumber = (parameters->at(0)->type == BaseLib::VariableType::tString) ? true : false;
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(parameters->at(0)->stringValue)) return central->deleteDevice(clientInfo, parameters->at(0)->stringValue, parameters->at(1)->integerValue);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->deleteDevice(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCDeleteMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		std::string dataID;
		if(parameters->size() > 1) dataID = parameters->at(1)->stringValue;
		serialNumber = peer->getSerialNumber();
		return GD::bl->db->deleteMetadata(peer->getID(), serialNumber, dataID);
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

BaseLib::PVariable RPCDeleteSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->deleteSystemVariable(parameters->at(0)->stringValue);
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

BaseLib::PVariable RPCEnableEvent::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler->enable(parameters->at(0)->stringValue, parameters->at(1)->booleanValue);
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCGetAllConfig::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerID = 0;
		if(parameters->size() > 0) peerID = parameters->at(0)->integerValue;

		BaseLib::PVariable config(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(peerID > 0 && !central->peerExists(peerID)) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable result = central->getAllConfig(clientInfo, peerID);
			if(result && result->errorStruct)
			{
				if(peerID > 0) return result;
				else GD::out.printWarning("Warning: Error calling method \"getAllConfig\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) config->arrayValue->insert(config->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			if(peerID > 0) break;
		}

		if(config->arrayValue->empty() && peerID > 0) return BaseLib::Variable::createError(-2, "Unknown device.");
		return config;
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

BaseLib::PVariable RPCGetAllMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		std::string peerName = peer->getName();
		BaseLib::PVariable metadata = GD::bl->db->getAllMetadata(peer->getID());
		if(!peerName.empty())
		{
			if(metadata->structValue->find("NAME") != metadata->structValue->end()) metadata->structValue->at("NAME")->stringValue = peerName;
			else
			{
				if(metadata->errorStruct) metadata.reset(new BaseLib::Variable(BaseLib::VariableType::tStruct)); //Remove error struct
				metadata->structValue->insert(BaseLib::StructElement("NAME", BaseLib::PVariable(new BaseLib::Variable(peerName))));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetAllScripts::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		return GD::scriptEngineServer->getAllScripts();
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

BaseLib::PVariable RPCGetAllSystemVariables::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		return GD::bl->db->getAllSystemVariables();
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

BaseLib::PVariable RPCGetAllValues::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerID = 0;
		bool returnWriteOnly = false;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tBoolean)
		{
			returnWriteOnly = parameters->at(0)->booleanValue;
		}
		else if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tInteger)
		{
			peerID = parameters->at(0)->integerValue;
		}
		else if(parameters->size() == 2)
		{
			peerID = parameters->at(0)->integerValue;
			returnWriteOnly = parameters->at(1)->booleanValue;
		}

		BaseLib::PVariable values(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(peerID > 0 && !central->peerExists(peerID)) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable result = central->getAllValues(clientInfo, peerID, returnWriteOnly);
			if(result && result->errorStruct)
			{
				if(peerID > 0) return result;
				else GD::out.printWarning("Warning: Error calling method \"getAllValues\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) values->arrayValue->insert(values->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			if(peerID > 0) break;
		}

		if(values->arrayValue->empty() && peerID > 0) return BaseLib::Variable::createError(-2, "Unknown device.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetConfigParameter::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->getConfigParameter(clientInfo, serialNumber, channel, parameters->at(1)->stringValue);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getConfigParameter(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetDeviceDescription::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
				BaseLib::PVariable description(new BaseLib::Variable(BaseLib::VariableType::tStruct));
				description->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(std::string("Homegear")))));
				description->structValue->insert(BaseLib::StructElement("FIRMWARE", BaseLib::PVariable(new BaseLib::Variable(std::string(VERSION)))));
				return description;
			}
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->getDeviceDescription(clientInfo, serialNumber, channel);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getDeviceDescription(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetDeviceInfo::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tArray }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerID = 0;
		int32_t fieldsIndex = -1;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tArray)
		{
			fieldsIndex = 0;
		}
		else if(parameters->size() == 2 || (parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tInteger))
		{
			if(parameters->size() == 2) fieldsIndex = 1;
			peerID = parameters->at(0)->integerValue;
		}


		std::map<std::string, bool> fields;
		if(fieldsIndex > -1)
		{
			for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(fieldsIndex)->arrayValue->begin(); i != parameters->at(fieldsIndex)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		BaseLib::PVariable deviceInfo(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(peerID > 0)
			{
				if(central->peerExists(peerID)) return central->getDeviceInfo(clientInfo, peerID, fields);
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(3));
				BaseLib::PVariable result = central->getDeviceInfo(clientInfo, peerID, fields);
				if(result && result->errorStruct)
				{
					GD::out.printWarning("Warning: Error calling method \"listDevices\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
					continue;
				}
				if(result && !result->arrayValue->empty()) deviceInfo->arrayValue->insert(deviceInfo->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
		}

		if(peerID == 0) return deviceInfo;
		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetEvent::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler->get(parameters->at(0)->stringValue);
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCGetInstallMode::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(familyID > -1)
		{
			if(families.find(familyID) == families.end())
			{
				return BaseLib::Variable::createError(-2, "Device family is unknown.");
			}
			return families.at(familyID)->getCentral()->getInstallMode(clientInfo);
		}

		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable result = central->getInstallMode(clientInfo);
				if(result->integerValue > 0) return result;
			}
		}

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tInteger));
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

BaseLib::PVariable RPCGetKeyMismatchDevice::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);
		//TODO Implement method when AES is supported.
		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tString));
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

BaseLib::PVariable RPCGetLinkInfo::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(senderSerialNumber)) return central->getLinkInfo(clientInfo, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getLinkInfo(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Sender device not found.");
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

BaseLib::PVariable RPCGetLinkPeers::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->getLinkPeers(clientInfo, serialNumber, channel);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getLinkPeers(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCGetLinks::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>(),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t flags = 0;
		uint64_t peerID = 0;

		bool useSerialNumber = false;
		if(parameters->size() > 0)
		{
			if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		BaseLib::PVariable links(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			if(serialNumber.empty() && peerID == 0)
			{
				BaseLib::PVariable result = central->getLinks(clientInfo, peerID, channel, flags);
				if(result && !result->arrayValue->empty()) links->arrayValue->insert(links->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
			else
			{
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->getLinks(clientInfo, serialNumber, channel, flags);
				}
				else
				{
					if(central->peerExists(peerID)) return central->getLinks(clientInfo, peerID, channel, flags);
				}
			}
		}

		if(serialNumber.empty() && peerID == 0) return links;
		else return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCGetMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		if(parameters->at(1)->stringValue == "NAME")
		{
			std::string peerName = peer->getName();
			if(peerName.size() > 0) return BaseLib::PVariable(new BaseLib::Variable(peerName));
			BaseLib::PVariable rpcName = GD::bl->db->getMetadata(peer->getID(), parameters->at(1)->stringValue);
			if(peerName.size() == 0 && !rpcName->errorStruct)
			{
				peer->setName(rpcName->stringValue);
				serialNumber = peer->getSerialNumber();
				GD::bl->db->deleteMetadata(peer->getID(), serialNumber, parameters->at(1)->stringValue);
			}
			return rpcName;
		}
		else return GD::bl->db->getMetadata(peer->getID(), parameters->at(1)->stringValue);
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

BaseLib::PVariable RPCGetName::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t)parameters->at(0)->integerValue))
			{
				return central->getName(clientInfo, parameters->at(0)->integerValue);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCGetPairingMethods::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
		if(family) return family->getPairingMethods();

		return BaseLib::Variable::createError(-2, "Device family not found.");
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

BaseLib::PVariable RPCGetParamsetDescription::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->size() == 5)
		{
			std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
			if(!family) return BaseLib::Variable::createError(-2, "Device family not found.");
			return family->getParamsetDescription(clientInfo, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(4)->stringValue));
		}
		else
		{
			int32_t channel = -1;
			std::string serialNumber;
			int32_t remoteChannel = -1;
			std::string remoteSerialNumber;
			bool useSerialNumber = false;
			if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
			BaseLib::DeviceDescription::ParameterGroup::Type::Enum type;
			if(parameters->at(parameterSetIndex)->type == BaseLib::VariableType::tString)
			{
				type = BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(parameterSetIndex)->stringValue);
				if(type == BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none)
				{
					type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
					int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
					if(pos > -1)
					{
						remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
						if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
					}
					else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
					// {{{ HomeMatic-Konfigurator Bugfix
					std::string copy = remoteSerialNumber;
					BaseLib::HelperFunctions::toUpper(copy);
					if(copy == serialNumber) remoteSerialNumber = copy;
					// }}}
				}
			}
			else
			{
				type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
				remoteID = parameters->at(2)->integerValue;
				remoteChannel = parameters->at(3)->integerValue;
			}

			std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
			for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
			{
				std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
				if(!central) continue;
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->getParamsetDescription(clientInfo, serialNumber, channel, type, remoteSerialNumber, remoteChannel);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getParamsetDescription(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
				}
			}
		}
		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCGetParamsetId::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t channel = 0;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
		BaseLib::DeviceDescription::ParameterGroup::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::VariableType::tString)
		{
			type = BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none)
			{
				type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
				int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
				// {{{ HomeMatic-Konfigurator Bugfix
				std::string copy = remoteSerialNumber;
				BaseLib::HelperFunctions::toUpper(copy);
				if(copy == serialNumber) remoteSerialNumber = copy;
				// }}}
			}
		}
		else
		{
			type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->peerExists(serialNumber)) return central->getParamsetId(clientInfo, serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getParamsetId(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetParamset::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
		BaseLib::DeviceDescription::ParameterGroup::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::VariableType::tString)
		{
			type = BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none)
			{
				type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
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
			type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->peerExists(serialNumber)) return central->getParamset(clientInfo, serialNumber, channel, type, remoteSerialNumber, remoteChannel);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getParamset(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetPeerId::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string filterValue = (parameters->size() > 1) ? parameters->at(1)->stringValue : "";

		BaseLib::PVariable ids(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable result = central->getPeerId(clientInfo, parameters->at(0)->integerValue, filterValue);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetServiceMessages::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>(),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool id = false;
		if(parameters->size() == 1) id = parameters->at(0)->booleanValue;

		BaseLib::PVariable serviceMessages(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			//getServiceMessages really needs a lot of ressources, so wait a little bit after each central
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable messages = central->getServiceMessages(clientInfo, id);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->getSystemVariable(parameters->at(0)->stringValue);
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

BaseLib::PVariable RPCGetUpdateStatus::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>());
		if(error != ParameterError::Enum::noError) return getError(error);

		BaseLib::PVariable updateInfo(new BaseLib::Variable(BaseLib::VariableType::tStruct));

		updateInfo->structValue->insert(BaseLib::StructElement("DEVICE_COUNT", BaseLib::PVariable(new BaseLib::Variable((uint32_t)GD::bl->deviceUpdateInfo.devicesToUpdate))));
		updateInfo->structValue->insert(BaseLib::StructElement("CURRENT_UPDATE", BaseLib::PVariable(new BaseLib::Variable((uint32_t)GD::bl->deviceUpdateInfo.currentUpdate))));
		updateInfo->structValue->insert(BaseLib::StructElement("CURRENT_DEVICE", BaseLib::PVariable(new BaseLib::Variable((uint32_t)GD::bl->deviceUpdateInfo.currentDevice))));
		updateInfo->structValue->insert(BaseLib::StructElement("CURRENT_DEVICE_PROGRESS", BaseLib::PVariable(new BaseLib::Variable((uint32_t)GD::bl->deviceUpdateInfo.currentDeviceProgress))));

		BaseLib::PVariable results(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		updateInfo->structValue->insert(BaseLib::StructElement("RESULTS", results));
		for(std::map<uint64_t, std::pair<int32_t, std::string>>::iterator i = GD::bl->deviceUpdateInfo.results.begin(); i != GD::bl->deviceUpdateInfo.results.end(); ++i)
		{
			BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tStruct));
			result->structValue->insert(BaseLib::StructElement("RESULT_CODE", BaseLib::PVariable(new BaseLib::Variable(i->second.first))));
			result->structValue->insert(BaseLib::StructElement("RESULT_STRING", BaseLib::PVariable(new BaseLib::Variable(i->second.second))));
			results->structValue->insert(BaseLib::StructElement(std::to_string(i->first), result));
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetValue::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		bool requestFromDevice = false;
		bool asynchronously = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->getValue(clientInfo, serialNumber, channel, parameters->at(1)->stringValue, requestFromDevice, asynchronously);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->getValue(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue, requestFromDevice, asynchronously);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCGetVersion::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string version(VERSION);
		return BaseLib::PVariable(new BaseLib::Variable("Homegear " + version));
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

RPCInit::~RPCInit()
{
	try
	{
		_disposing = true;
		_initServerThreadMutex.lock();
		GD::bl->threadManager.join(_initServerThread);
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

BaseLib::PVariable RPCInit::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(_disposing) return BaseLib::Variable::createError(-32500, "Method is disposing.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(GD::bl->settings.clientAddressesToReplace().find(parameters->at(0)->stringValue) != GD::bl->settings.clientAddressesToReplace().end())
		{
			std::string newAddress = GD::bl->settings.clientAddressesToReplace().at(parameters->at(0)->stringValue);
			std::string remoteIP = RPC::Server::getClientIPAll(clientInfo->id);
			if(remoteIP.empty()) return BaseLib::Variable::createError(-32500, "Could not get client's IP address.");
			GD::bl->hf.stringReplace(newAddress, "$remoteip", remoteIP);
			GD::out.printInfo("Info: Replacing address " + parameters->at(0)->stringValue + " with " + newAddress);
			parameters->at(0)->stringValue = newAddress;
		}

		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::splitLast(parameters->at(0)->stringValue, ':');
		if(server.first.empty() || server.second.empty()) return BaseLib::Variable::createError(-32602, "Server address or port is empty.");
		if(server.first.size() < 8) return BaseLib::Variable::createError(-32602, "Server address too short.");
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
		if(server.second.empty() || server.second == "0") return BaseLib::Variable::createError(-32602, "Port number is invalid.");

		if(parameters->size() == 1 || parameters->at(1)->stringValue.empty())
		{
			GD::rpcClient->removeServer(server);
		}
		else
		{
			std::shared_ptr<RemoteRpcServer> eventServer = GD::rpcClient->addServer(server, path, parameters->at(1)->stringValue);
			if(!eventServer)
			{
				GD::out.printError("Error: Could not create event server.");
				return BaseLib::Variable::createError(-32500, "Unknown application error.");
			}
			if(server.first.compare(0, 5, "https") == 0 || server.first.compare(0, 7, "binarys") == 0) eventServer->useSSL = true;
			if(server.first.compare(0, 6, "binary") == 0 ||
			   server.first.compare(0, 7, "binarys") == 0 ||
			   server.first.compare(0, 10, "xmlrpc_bin") == 0) eventServer->binary = true;
			if(parameters->size() > 2)
			{
				eventServer->keepAlive = (parameters->at(2)->integerValue & 1);
				eventServer->binary = (parameters->at(2)->integerValue & 2);
				eventServer->newFormat = (parameters->at(2)->integerValue & 4);
				eventServer->subscribePeers = (parameters->at(2)->integerValue & 8);
				eventServer->json = (parameters->at(2)->integerValue & 16);
				eventServer->reconnectInfinitely = (parameters->at(2)->integerValue & 128);
			}
			// {{{ Reconnect on CCU2 as it doesn't reconnect automatically
			if((parameters->at(1)->stringValue.size() == 4 && BaseLib::Math::isNumber(parameters->at(1)->stringValue, false) && server.second == "1999") || parameters->at(1)->stringValue == "Homegear_java") eventServer->reconnectInfinitely = true;
			// }}}
			// {{{ Keep connection to IP-Symcon
			if(server.second == "5544" && parameters->at(1)->stringValue == "IPS") eventServer->reconnectInfinitely = true;
			// }}}

			_initServerThreadMutex.lock();
			try
			{
				if(_disposing)
				{
					_initServerThreadMutex.unlock();
					return BaseLib::Variable::createError(-32500, "I'm disposing.");
				}
				GD::bl->threadManager.join(_initServerThread);
				GD::bl->threadManager.start(_initServerThread, false, &RPC::Client::initServerMethods, GD::rpcClient.get(), server);
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

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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

BaseLib::PVariable RPCListBidcosInterfaces::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		//Return dummy data
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			BaseLib::PVariable array(new BaseLib::Variable(BaseLib::VariableType::tArray));
			BaseLib::PVariable interface(new BaseLib::Variable(BaseLib::VariableType::tStruct));
			array->arrayValue->push_back(interface);
			interface->structValue->insert(BaseLib::StructElement("ADDRESS", BaseLib::PVariable(new BaseLib::Variable(central->getSerialNumber()))));
			interface->structValue->insert(BaseLib::StructElement("DESCRIPTION", BaseLib::PVariable(new BaseLib::Variable(std::string("Homegear default BidCoS interface")))));
			interface->structValue->insert(BaseLib::StructElement("CONNECTED", BaseLib::PVariable(new BaseLib::Variable(true))));
			interface->structValue->insert(BaseLib::StructElement("DEFAULT", BaseLib::PVariable(new BaseLib::Variable(true))));
			return array;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCListClientServers::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		std::string id;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
			if(error != ParameterError::Enum::noError) return getError(error);
			id = parameters->at(0)->stringValue;
		}
		return GD::rpcClient->listClientServers(id);
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

BaseLib::PVariable RPCListDevices::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean }),
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray }),
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		bool channels = true;
		std::map<std::string, bool> fields;
		if(parameters->size() == 2)
		{
			channels = parameters->at(0)->booleanValue;
			for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		BaseLib::PVariable devices(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable result = central->listDevices(clientInfo, channels, fields);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCListEvents::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
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
		return GD::eventHandler->list(type, peerID, channel, variable);
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCListFamilies::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		return GD::familyController->listFamilies();
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

BaseLib::PVariable RPCListInterfaces::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		return GD::familyController->listInterfaces(familyID);
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

BaseLib::PVariable RPCListKnownDeviceTypes::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean }),
					std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray }),
			}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		bool channels = true;
		std::map<std::string, bool> fields;
		if(parameters->size() > 0) channels = parameters->at(0)->booleanValue;
		if(parameters->size() == 2)
		{
			for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
		}

		BaseLib::PVariable devices(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable result = i->second->listKnownDeviceTypes(clientInfo, channels, fields);
			if(result && result->errorStruct)
			{
				GD::out.printWarning("Warning: Error calling method \"listKnownDeviceTypes\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCListTeams::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		BaseLib::PVariable teams(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable result = central->listTeams(clientInfo);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCLogLevel::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }));
			if(error != ParameterError::Enum::noError) return getError(error);
			int32_t debugLevel = parameters->at(0)->integerValue;
			if(debugLevel < 0) debugLevel = 2;
			if(debugLevel > 5) debugLevel = 5;
			GD::bl->debugLevel = debugLevel;
		}
		return BaseLib::PVariable(new BaseLib::Variable(GD::bl->debugLevel));
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

BaseLib::PVariable RPCPutParamset::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t remoteChannel = -1;
		std::string remoteSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
		BaseLib::DeviceDescription::ParameterGroup::Type::Enum type;
		if(parameters->at(parameterSetIndex)->type == BaseLib::VariableType::tString)
		{
			type = BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(parameterSetIndex)->stringValue);
			if(type == BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none)
			{
				type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
				int32_t pos = parameters->at(parameterSetIndex)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue.substr(0, pos);
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned)pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
				// {{{ HomeMatic-Konfigurator Bugfix
				std::string copy = remoteSerialNumber;
				BaseLib::HelperFunctions::toUpper(copy);
				if(copy == serialNumber) remoteSerialNumber = copy;
				// }}}
			}
		}
		else
		{
			type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
			remoteID = parameters->at(2)->integerValue;
			remoteChannel = parameters->at(3)->integerValue;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central)continue;
			if(useSerialNumber)
			{
				if(central->peerExists(serialNumber)) return central->putParamset(clientInfo, serialNumber, channel, type, remoteSerialNumber, remoteChannel, parameters->back());
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->putParamset(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, type, remoteID, remoteChannel, parameters->back());
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCRemoveEvent::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler->remove(parameters->at(0)->stringValue);
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCRemoveLink::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->peerExists(senderSerialNumber)) return central->removeLink(clientInfo, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->removeLink(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
			}
		}

		return BaseLib::Variable::createError(-2, "Sender device not found.");
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

BaseLib::PVariable RPCReportValueUsage::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger }),
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists(serialNumber)) return central->reportValueUsage(clientInfo, serialNumber);
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCRssiInfo::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		BaseLib::PVariable response(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			BaseLib::PVariable result = central->rssiInfo(clientInfo);
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCRunScript::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean })
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
		if(ending == ".php" || ending == ".php5" || ending == ".php7" || ending == ".hgs") internalEngine = true;

		if((signed)parameters->size() == 2)
		{
			if(parameters->at(1)->type == BaseLib::VariableType::tBoolean) wait = parameters->at(1)->booleanValue;
			else arguments = parameters->at(1)->stringValue;
		}
		if((signed)parameters->size() == 3)
		{
			arguments = parameters->at(1)->stringValue;
			wait = parameters->at(2)->booleanValue;
		}

		std::string relativePath = '/' + filename;
		std::string fullPath = GD::bl->settings.scriptPath() + filename;

		BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		BaseLib::PVariable exitCode(new BaseLib::Variable(0));
		BaseLib::PVariable output(new BaseLib::Variable(BaseLib::VariableType::tString));
		result->structValue->insert(BaseLib::StructElement("EXITCODE", exitCode));
		result->structValue->insert(BaseLib::StructElement("OUTPUT", output));
		if(internalEngine)
		{
			if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Executing script \"" + fullPath + "\" with parameters \"" + arguments + "\" using internal script engine.");
			BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli, fullPath, relativePath, arguments));
			if(wait) scriptInfo->returnOutput = true;
			GD::scriptEngineServer->executeScript(scriptInfo, wait);
			if(!scriptInfo->started)
			{
				output->stringValue = "Error: Could not execute script. Check log file for more details.";
				exitCode->integerValue = -1;
				return result;
			}
			if(wait) exitCode->integerValue = scriptInfo->exitCode;
			if(!scriptInfo->output.empty())
			{
				output->stringValue = std::move(scriptInfo->output);
				return result;
			}
			else
			{
				if(!wait)
				{
					output->stringValue = "Error executing script. Check log file for more details.";
					exitCode->integerValue = -1;
				}
				return result;
			}
		}
		else
		{
			if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Executing program/script \"" + fullPath + "\" with parameters \"" + arguments + "\".");
			std::string command = fullPath + " " + arguments;
			if(!wait) command += "&";

			struct stat statStruct;
			if(stat(fullPath.c_str(), &statStruct) < 0)
			{
				output->stringValue = "Could not execute script: " + std::string(strerror(errno));
				exitCode->integerValue = -32400;
				return result;
			}
			uint32_t uid = getuid();
			uint32_t gid = getgid();
			if((statStruct.st_mode & S_IXOTH) == 0)
			{
				if(statStruct.st_gid != gid || (statStruct.st_gid == gid && (statStruct.st_mode & S_IXGRP) == 0))
				{
					if(statStruct.st_uid != uid || (statStruct.st_uid == uid && (statStruct.st_mode & S_IXUSR) == 0))
					{
						output->stringValue = "Could not execute script. No permission - or executable bit is not set.";
						exitCode->integerValue = -32400;
						return result;
					}
				}
			}
			if((statStruct.st_mode & (S_IXGRP | S_IXUSR)) == 0) //At least in Debian it is not possible to execute scripts, when the execution bit is only set for "other".
			{
				output->stringValue = "Could not execute script. Executable bit is not set for user or group.";
				exitCode->integerValue = -32400;
				return result;
			}
			if((statStruct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)
			{
				output->stringValue = "Could not execute script. The file mode is not set to executable.";
				exitCode->integerValue = -32400;
				return result;
			}

			exitCode->integerValue = BaseLib::HelperFunctions::exec(command.c_str(), output->stringValue);
			return result;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSearchDevices::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger })
			}));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(familyID > -1)
		{
			if(families.find(familyID) == families.end())
			{
				return BaseLib::Variable::createError(-2, "Device family is unknown.");
			}
			return families.at(familyID)->getCentral()->searchDevices(clientInfo);
		}

		BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tInteger));
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central) result->integerValue += central->searchDevices(clientInfo)->integerValue;
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
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCSetId::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t)parameters->at(0)->integerValue))
			{
				return central->setId(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCSetInstallMode::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean }),
			std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t enableIndex = -1;
		if(parameters->at(0)->type == BaseLib::VariableType::tBoolean) enableIndex = 0;
		else if(parameters->size() == 2) enableIndex = 1;
		int32_t familyIDIndex = (parameters->at(0)->type == BaseLib::VariableType::tInteger) ? 0 : -1;
		int32_t timeIndex = -1;
		if(parameters->size() >= 2 && parameters->at(1)->type == BaseLib::VariableType::tInteger) timeIndex = 1;
		else if(parameters->size() == 3) timeIndex = 2;

		bool enable = (enableIndex > -1) ? parameters->at(enableIndex)->booleanValue : false;
		int32_t familyID = (familyIDIndex > -1) ? parameters->at(familyIDIndex)->integerValue : -1;

		uint32_t time = (timeIndex > -1) ? parameters->at(timeIndex)->integerValue : 60;
		if(time < 5) time = 60;
		if(time > 3600) time = 3600;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(familyID > -1)
		{
			if(families.find(familyID) == families.end())
			{
				return BaseLib::Variable::createError(-2, "Device family is unknown.");
			}
			return families.at(familyID)->getCentral()->setInstallMode(clientInfo, enable, time);
		}

		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central) central->setInstallMode(clientInfo, enable, time);
		}

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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

BaseLib::PVariable RPCSetInterface::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->setInterface(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->stringValue);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCSetLinkInfo::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString })

		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t senderChannel = -1;
		std::string senderSerialNumber;
		int32_t receiverChannel = -1;
		std::string receiverSerialNumber;
		int32_t nameIndex = 4;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::Variable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if((signed)parameters->size() > nameIndex + 1)
		{
			if(parameters->at(nameIndex + 1)->stringValue.size() > 1000) return BaseLib::Variable::createError(-32602, "Description has more than 1000 characters.");
			description = parameters->at(nameIndex + 1)->stringValue;
		}


		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(senderSerialNumber)) return central->setLinkInfo(clientInfo, senderSerialNumber, senderChannel, receiverSerialNumber, receiverChannel, name, description);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->setLinkInfo(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, name, description);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Sender device not found.");
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

BaseLib::PVariable RPCSetMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string serialNumber;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1) serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			else serialNumber = parameters->at(0)->stringValue;
		}

		std::shared_ptr<BaseLib::Systems::Peer> peer;
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					peer = central->getPeer(serialNumber);
					if(peer) break;
				}
				else
				{
					peer = central->getPeer((uint64_t)parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		if(parameters->at(1)->stringValue == "NAME")
		{
			peer->setName(parameters->at(2)->stringValue);
			return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
		}
		serialNumber = peer->getSerialNumber();
		return GD::bl->db->setMetadata(peer->getID(), serialNumber, parameters->at(1)->stringValue, parameters->at(2));
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

BaseLib::PVariable RPCSetName::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t)parameters->at(0)->integerValue))
			{
				return central->setName(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->stringValue);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCSetSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tVariant }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->setSystemVariable(parameters->at(0)->stringValue, parameters->at(1));
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

BaseLib::PVariable RPCSetTeam::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t deviceChannel = -1;
		std::string deviceSerialNumber;
		uint64_t teamID = 0;
		int32_t teamChannel = -1;
		std::string teamSerialNumber;

		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(useSerialNumber)
			{
				if(central->peerExists(deviceSerialNumber)) return central->setTeam(clientInfo, deviceSerialNumber, deviceChannel, teamSerialNumber, teamChannel);
			}
			else
			{
				if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->setTeam(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, teamID, teamChannel);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCSetValue::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
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

		BaseLib::PVariable value;
		if(useSerialNumber && parameters->size() >= 3) value = parameters->at(2);
		else if(!useSerialNumber && parameters->size() >= 4) value = parameters->at(3);
		else value.reset(new BaseLib::Variable(BaseLib::VariableType::tVoid));

		bool wait = true;
		if(useSerialNumber && parameters->size() == 4) wait = parameters->at(3)->booleanValue;
		else if(!useSerialNumber && parameters->size() == 5) wait = parameters->at(4)->booleanValue;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(useSerialNumber)
				{
					if(central->peerExists(serialNumber)) return central->setValue(clientInfo, serialNumber, channel, parameters->at(1)->stringValue, value, wait);
				}
				else
				{
					if(central->peerExists((uint64_t)parameters->at(0)->integerValue)) return central->setValue(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue, value, wait);
				}
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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
    return BaseLib::Variable::createError(-32500, "Unknown application error. Check the address format.");
}

BaseLib::PVariable RPCSubscribePeers::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-32602, "Server id is empty.");
		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::splitLast(parameters->at(0)->stringValue, ':');
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
			if(server.second.empty() || server.second == "0") return BaseLib::Variable::createError(-32602, "Port number is invalid.");
		}

		std::shared_ptr<RemoteRpcServer> eventServer = GD::rpcClient->getServer(server);
		if(!eventServer) return BaseLib::Variable::createError(-1, "Event server is unknown.");
		for(BaseLib::Array::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
		{
			if((*i)->integerValue != 0) eventServer->subscribedPeers.insert((*i)->integerValue);
		}

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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

BaseLib::PVariable RPCTriggerEvent::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
#ifdef EVENTHANDLER
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::eventHandler->trigger(parameters->at(0)->stringValue);
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
#else
    return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without event handler.");
#endif
}

BaseLib::PVariable RPCTriggerRpcEvent::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tArray }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue == "deleteDevices")
		{
			if(parameters->at(1)->arrayValue->size() != 2) return BaseLib::Variable::createError(-1, "Wrong parameter count. You need to pass (in this order): Array<String> deviceAddresses, Array<Struct> deviceInfo");
			GD::rpcClient->broadcastDeleteDevices(parameters->at(1)->arrayValue->at(0), parameters->at(1)->arrayValue->at(1));
		}
		else if(parameters->at(0)->stringValue == "newDevices")
		{
			GD::rpcClient->broadcastNewDevices(parameters->at(1));
		}
		else if(parameters->at(0)->stringValue == "updateDevice")
		{
			if(parameters->at(1)->arrayValue->size() != 4) return BaseLib::Variable::createError(-1, "Wrong parameter count. You need to pass (in this order): Integer peerID, Integer channel, String address, Integer flags");
			GD::rpcClient->broadcastUpdateDevice(parameters->at(1)->arrayValue->at(0)->integerValue, parameters->at(1)->arrayValue->at(1)->integerValue, parameters->at(1)->arrayValue->at(2)->stringValue, (RPC::Client::Hint::Enum)parameters->at(1)->arrayValue->at(3)->integerValue);
		}
		else return BaseLib::Variable::createError(-1, "Invalid function.");

		return BaseLib::PVariable(new BaseLib::Variable());
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

BaseLib::PVariable RPCUnsubscribePeers::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue.empty()) return BaseLib::Variable::createError(-32602, "Server id is empty.");
		std::pair<std::string, std::string> server = BaseLib::HelperFunctions::splitLast(parameters->at(0)->stringValue, ':');
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
			if(server.second.empty() || server.second == "0") return BaseLib::Variable::createError(-32602, "Port number is invalid.");
		}

		std::shared_ptr<RemoteRpcServer> eventServer = GD::rpcClient->getServer(server);
		if(!eventServer) return BaseLib::Variable::createError(-1, "Event server is unknown.");
		for(BaseLib::Array::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
		{
			if((*i)->integerValue != 0) eventServer->subscribedPeers.erase((*i)->integerValue);
		}

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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

BaseLib::PVariable RPCUpdateFirmware::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tArray })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool manual = false;
		bool array = true;
		if(parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			array = false;
			if(parameters->size() == 2 && parameters->at(1)->booleanValue) manual = true;
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(array)
			{
				std::vector<uint64_t> ids;
				for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(0)->arrayValue->begin(); i != parameters->at(0)->arrayValue->end(); ++i)
				{
					if((*i)->type == BaseLib::VariableType::tInteger && (*i)->integerValue != 0 && central->peerExists((uint64_t)(*i)->integerValue)) ids.push_back((*i)->integerValue);
					else if((*i)->type == BaseLib::VariableType::tString && central->peerExists((*i)->stringValue)) ids.push_back(central->getPeerId(clientInfo, (*i)->stringValue)->integerValue);
				}
				if(ids.size() > 0 && ids.size() != parameters->at(0)->arrayValue->size()) return BaseLib::Variable::createError(-2, "Please provide only devices of one device family.");
				if(ids.size() > 0) return central->updateFirmware(clientInfo, ids, manual);
			}
			else if((parameters->at(0)->type == BaseLib::VariableType::tInteger && central->peerExists((uint64_t)parameters->at(0)->integerValue)) ||
					(parameters->at(0)->type == BaseLib::VariableType::tString && central->peerExists(parameters->at(0)->stringValue)))
			{
				std::vector<uint64_t> ids;
				if(parameters->at(0)->type == BaseLib::VariableType::tString) ids.push_back(central->getPeerId(clientInfo, parameters->at(0)->stringValue)->integerValue);
				else ids.push_back(parameters->at(0)->integerValue);
				return central->updateFirmware(clientInfo, ids, manual);
			}
		}

		return BaseLib::Variable::createError(-2, "Device not found.");
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

BaseLib::PVariable RPCWriteLog::invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString }),
				std::vector<BaseLib::VariableType>({ BaseLib::VariableType::tString, BaseLib::VariableType::tInteger })
		}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->size() == 2) GD::out.printMessage(parameters->at(0)->stringValue, parameters->at(1)->integerValue, true);
		else GD::out.printMessage(parameters->at(0)->stringValue);

		return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
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

} /* namespace RPC */
