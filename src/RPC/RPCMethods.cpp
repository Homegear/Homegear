/* Copyright 2013-2017 Sathya Laufer
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
#include <homegear-base/BaseLib.h>
#ifdef BSDSYSTEM
	#include <sys/wait.h>
#endif

namespace Homegear
{

namespace Rpc
{

BaseLib::PVariable RPCDevTest::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("devTest")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCSystemGetCapabilities::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("system.getCapabilities")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
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

BaseLib::PVariable RPCSystemListMethods::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("system.listMethods")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		if(parameters->size() == 1) return std::make_shared<BaseLib::Variable>(GD::rpcServers.begin()->second->methodExists(clientInfo, parameters->at(0)->stringValue));

		BaseLib::PVariable methodInfo(new BaseLib::Variable(BaseLib::VariableType::tArray));
		auto methods = GD::rpcServers.begin()->second->getMethods();
		for(auto& method : *methods)
		{
			methodInfo->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(method.first)));
		}
		std::unordered_map<std::string, std::shared_ptr<RpcMethod>> ipcMethods = GD::ipcServer->getRpcMethods();
		for(auto& method : ipcMethods)
		{
			methodInfo->arrayValue->push_back(std::make_shared<BaseLib::Variable>(method.first));
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

BaseLib::PVariable RPCSystemMethodHelp::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("system.methodHelp")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
		if(error != ParameterError::Enum::noError) return getError(error);

		BaseLib::PVariable help;

		auto methods = GD::rpcServers.begin()->second->getMethods();
		auto methodIterator = methods->find(parameters->at(0)->stringValue);
		if(methodIterator == methods->end())
		{
			std::unordered_map<std::string, std::shared_ptr<RpcMethod>> ipcMethods = GD::ipcServer->getRpcMethods();
			auto methodIterator2 = ipcMethods.find(parameters->at(0)->stringValue);
			if(methodIterator2 == ipcMethods.end()) return BaseLib::Variable::createError(-32602, "Method not found.");
			help = methodIterator2->second->getHelp();
		}
		else help = methodIterator->second->getHelp();

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

BaseLib::PVariable RPCSystemMethodSignature::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("system.methodSignature")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
		if(error != ParameterError::Enum::noError) return getError(error);

		BaseLib::PVariable signature;

		std::shared_ptr<std::map<std::string, std::shared_ptr<RpcMethod>>> methods = GD::rpcServers.begin()->second->getMethods();
		auto methodIterator = methods->find(parameters->at(0)->stringValue);
		if(methodIterator == methods->end())
		{
			std::unordered_map<std::string, std::shared_ptr<RpcMethod>> ipcMethods = GD::ipcServer->getRpcMethods();
			auto methodIterator2 = ipcMethods.find(parameters->at(0)->stringValue);
			if(methodIterator2 == ipcMethods.end()) return BaseLib::Variable::createError(-32602, "Method not found.");
			signature = methodIterator2->second->getSignature();
		}
		else signature = methodIterator->second->getSignature();

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

BaseLib::PVariable RPCSystemMulticall::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("system.multicall")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray}));
		if(error != ParameterError::Enum::noError) return getError(error);

		BaseLib::PVariable returns(new BaseLib::Variable(BaseLib::VariableType::tArray));
		returns->arrayValue->reserve(parameters->at(0)->arrayValue->size());
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
			auto parameters = (*i)->structValue->at("params");

			if(methodName == "system.multicall") returns->arrayValue->push_back(BaseLib::Variable::createError(-32602, "Recursive calls to system.multicall are not allowed."));
			else
			{
				returns->arrayValue->push_back(GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters));
			}
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

BaseLib::PVariable RPCAbortEventReset::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("abortEventReset")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
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

BaseLib::PVariable RPCAcknowledgeGlobalServiceMessage::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("acknowledgeGlobalServiceMessage")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		GD::bl->globalServiceMessages.unset(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue);

		return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCActivateLinkParamset::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("activateLinkParamset")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean})
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;

			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
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
				if(central->peerExists((uint64_t) parameters->at(0)->integerValue)) return central->activateLinkParamset(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue, longPress);
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

BaseLib::PVariable RPCAddCategoryToChannel::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("addCategoryToChannel", parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->categoryExists(parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->addCategoryToChannel(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, parameters->at(2)->integerValue64);
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

BaseLib::PVariable RPCAddCategoryToDevice::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("addCategoryToDevice", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->addCategoryToChannel(clientInfo, (uint64_t) parameters->at(0)->integerValue64, -1, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCAddCategoryToSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("addCategoryToSystemVariable", parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesWriteSet();

		if(!GD::bl->db->categoryExists(parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
		if(!systemVariable) return BaseLib::Variable::createError(-5, "Unknown system variable.");

		if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->at(1)->integerValue64 != 0) systemVariable->categories.emplace(parameters->at(1)->integerValue64);

		return GD::bl->db->setSystemVariableCategories(systemVariable->name, systemVariable->categories);
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

BaseLib::PVariable RPCAddCategoryToVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("addCategoryToVariable", parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->categoryExists(parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
			if(!peer) continue;

			if(checkAcls)
			{
				if(!clientInfo->acls->checkVariableWriteAccess(peer, parameters->at(1)->integerValue, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
			}

			bool result = peer->addCategoryToVariable(parameters->at(1)->integerValue, parameters->at(2)->stringValue, parameters->at(3)->integerValue64);
			return std::make_shared<BaseLib::Variable>(result);
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

BaseLib::PVariable RPCAddChannelToRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("addChannelToRoom", (uint64_t) parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->addChannelToRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, (uint64_t) parameters->at(2)->integerValue64);
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

BaseLib::PVariable RPCAddDevice::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("addDevice")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
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
		return BaseLib::Variable::createError(-2, "No device was paired.");
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

BaseLib::PVariable RPCAddDeviceToRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("addDeviceToRoom", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->addChannelToRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, -1, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCAddEvent::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("addEvent")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct}));
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

BaseLib::PVariable RPCAddLink::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("addLink")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString})
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else senderSerialNumber = parameters->at(0)->stringValue;

			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else receiverSerialNumber = parameters->at(1)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		std::string name;
		if((signed) parameters->size() > nameIndex)
		{
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::Variable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if((signed) parameters->size() > nameIndex + 1)
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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64) || !central->peerExists((uint64_t) parameters->at(2)->integerValue64)) continue;
					if(checkAcls)
					{
						auto peer1 = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						auto peer2 = central->getPeer((uint64_t) parameters->at(2)->integerValue64);
						if(!peer1 || !clientInfo->acls->checkDeviceWriteAccess(peer1)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
						if(!peer2 || !clientInfo->acls->checkDeviceWriteAccess(peer2)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->addLink(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, parameters->at(2)->integerValue64, parameters->at(3)->integerValue, name, description);
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

BaseLib::PVariable RPCAddRoomToStory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("addRoomToStory", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->addRoomToStory((uint64_t) parameters->at(0)->integerValue64, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCAddSystemVariableToRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("addSystemVariableToRoom", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesWriteSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
		if(!systemVariable) return BaseLib::Variable::createError(-5, "Unknown system variable.");

		if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->setSystemVariableRoom(systemVariable->name, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCAddUiElement::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("addUiElement")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::uiController->addUiElement(clientInfo, parameters->at(0)->stringValue, parameters->at(1));
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

BaseLib::PVariable RPCAddVariableToRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("addVariableToRoom", (uint64_t) parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
			if(!peer) continue;

			if(checkAcls)
			{
				if(!clientInfo->acls->checkVariableWriteAccess(peer, parameters->at(1)->integerValue, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
			}

			bool result = peer->setVariableRoom(parameters->at(1)->integerValue, parameters->at(2)->stringValue, (uint64_t) parameters->at(3)->integerValue64);
			return std::make_shared<BaseLib::Variable>(result);
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

BaseLib::PVariable RPCCheckServiceAccess::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("checkServiceAccess")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return std::make_shared<BaseLib::Variable>(clientInfo->acls->checkServiceAccess(parameters->at(0)->stringValue));
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

BaseLib::PVariable RPCCopyConfig::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("copyConfig")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel1 = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber1 = parameters->at(0)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		if(parameters->at(2)->type == BaseLib::VariableType::tString)
		{
			int32_t pos = parameters->at(2)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber2 = parameters->at(2)->stringValue.substr(0, pos);
				if(parameters->at(2)->stringValue.size() > (unsigned) pos + 1) channel2 = std::stoll(parameters->at(2)->stringValue.substr(pos + 1));
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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64) || !central->peerExists((uint64_t) parameters->at(2)->integerValue64)) continue;

				if(clientInfo->acls->roomsCategoriesDevicesWriteSet())
				{
					auto peer1 = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					auto peer2 = central->getPeer((uint64_t) parameters->at(2)->integerValue64);
					if(!peer1 || !clientInfo->acls->checkDeviceWriteAccess(peer1)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					if(!peer2 || !clientInfo->acls->checkDeviceWriteAccess(peer2)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				parameterSet1 = central->getParamset(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, 0, -1, checkAcls);
				parameterSet2 = central->getParamset(clientInfo, parameters->at(2)->integerValue64, parameters->at(3)->integerValue, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, 0, -1, checkAcls);
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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64) || !central->peerExists((uint64_t) parameters->at(2)->integerValue64)) continue;
				return central->putParamset(clientInfo, (uint64_t) parameters->at(2)->integerValue64, parameters->at(3)->integerValue, BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config, 0, -1, resultSet, checkAcls);
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCClientServerInitialized::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("clientServerInitialized")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
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

BaseLib::PVariable RPCCreateCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("createCategory")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->createCategory(parameters->at(0), parameters->size() == 2 ? parameters->at(1) : std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
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

BaseLib::PVariable RPCCreateDevice::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("createDevice")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(families.find(parameters->at(0)->integerValue) == families.end())
		{
			return BaseLib::Variable::createError(-2, "Device family is unknown.");
		}
		std::string interfaceId(parameters->size() > 5 ? parameters->at(5)->stringValue : "");
		return families.at(parameters->at(0)->integerValue)->getCentral()->createDevice(clientInfo, parameters->at(1)->integerValue, parameters->at(2)->stringValue, parameters->at(3)->integerValue, parameters->at(4)->integerValue, interfaceId);
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

BaseLib::PVariable RPCCreateRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("createRoom")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->createRoom(parameters->at(0), parameters->size() == 2 ? parameters->at(1) : std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
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

BaseLib::PVariable RPCCreateStory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("createStory")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->createStory(parameters->at(0), parameters->size() == 2 ? parameters->at(1) : std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
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

BaseLib::PVariable RPCDeleteCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint64_t categoryId = (uint64_t) parameters->at(0)->integerValue64;
		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("deleteCategory", categoryId)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		BaseLib::PVariable result = GD::bl->db->deleteCategory(categoryId);
		if(result->errorStruct) return result;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable devices = central->getChannelsInCategory(clientInfo, categoryId, false);
				for(auto& device : *devices->structValue)
				{
					for(auto& channel : *device.second->arrayValue)
					{
						central->removeCategoryFromChannel(clientInfo, (uint64_t) BaseLib::Math::getNumber64(device.first, false), channel->integerValue, categoryId);
					}
				}

				auto peers = central->getPeers();
				for(auto& peer : peers)
				{
					peer->removeCategoryFromVariables(categoryId);
				}
			}
		}

		GD::bl->db->removeCategoryFromSystemVariables(categoryId);

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

BaseLib::PVariable RPCDeleteData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("deleteData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
		return GD::bl->db->deleteData(parameters->at(0)->stringValue, key);
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

BaseLib::PVariable RPCDeleteDevice::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("deleteDevice")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool useSerialNumber = (parameters->at(0)->type == BaseLib::VariableType::tString) ? true : false;

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->deleteDevice(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue);
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

BaseLib::PVariable RPCDeleteMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("deleteMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
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

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					peer = central->getPeer((uint64_t) parameters->at(0)->integerValue);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		if(checkAcls && !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

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

BaseLib::PVariable RPCDeleteNodeData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("deleteNodeData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
		return GD::bl->db->deleteNodeData(parameters->at(0)->stringValue, key);
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

BaseLib::PVariable RPCDeleteRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint64_t roomId = (uint64_t) parameters->at(0)->integerValue64;
		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("deleteRoom", roomId)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		BaseLib::PVariable result = GD::bl->db->deleteRoom(roomId);
		if(result->errorStruct) return result;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable devices = central->getChannelsInRoom(clientInfo, roomId, false);
				for(auto& device : *devices->structValue)
				{
					for(auto& channel : *device.second->arrayValue)
					{
						central->removeChannelFromRoom(clientInfo, (uint64_t) BaseLib::Math::getNumber64(device.first, false), channel->integerValue, roomId);
					}
				}

				auto peers = central->getPeers();
				for(auto& peer : peers)
				{
					peer->removeRoomFromVariables(roomId);
				}
			}
		}

		GD::bl->db->removeRoomFromSystemVariables(roomId);
		GD::bl->db->removeRoomFromStories(roomId);

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

BaseLib::PVariable RPCDeleteStory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("deleteStory")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->deleteStory((uint64_t) parameters->at(0)->integerValue64);
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

BaseLib::PVariable RPCDeleteSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("deleteSystemVariable")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesReadSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
		if(error != ParameterError::Enum::noError) return getError(error);

		auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
		if(!systemVariable) return std::make_shared<BaseLib::Variable>();

		if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

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

BaseLib::PVariable RPCEnableEvent::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("enableEvent")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean}));
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

BaseLib::PVariable RPCExecuteMiscellaneousDeviceMethod::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifndef NO_SCRIPTENGINE
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("executeMiscellaneousDeviceMethod")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64, BaseLib::VariableType::tString, BaseLib::VariableType::tArray}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(checkAcls)
		{
			bool deviceFound = false;
			std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
			for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
			{
				std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
				if(central)
				{
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

					deviceFound = true;
				}
			}

			if(!deviceFound) return BaseLib::Variable::createError(-1, "Unknown device.");
		}

		return GD::scriptEngineServer->executeDeviceMethod(parameters);
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
	return BaseLib::Variable::createError(-32500, "This version of Homegear is compiled without script engine.");
#endif
}

BaseLib::PVariable RPCGetAllConfig::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllConfig")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerId = 0;
		if(parameters->size() > 0) peerId = parameters->at(0)->integerValue64;

		BaseLib::PVariable config(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(peerId > 0)
			{
				if(!central->peerExists(peerId)) continue;
				if(clientInfo->acls->roomsCategoriesDevicesReadSet())
				{
					auto peer = central->getPeer(peerId);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}
			}
			BaseLib::PVariable result = central->getAllConfig(clientInfo, peerId, checkAcls);
			if(result && result->errorStruct)
			{
				if(peerId > 0) return result;
				else GD::out.printWarning("Warning: Error calling method \"getAllConfig\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) config->arrayValue->insert(config->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			if(peerId > 0) break;
		}

		if(config->arrayValue->empty() && peerId > 0) return BaseLib::Variable::createError(-2, "Unknown device.");
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

BaseLib::PVariable RPCFamilyExists::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("familyExists")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return std::make_shared<BaseLib::Variable>((bool) GD::familyController->getFamily(parameters->at(0)->integerValue));
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

BaseLib::PVariable RPCGetAllMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
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

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		std::string peerName = peer->getName();
		if(clientInfo->clientType == BaseLib::RpcClientType::homematicconfigurator)
		{
			BaseLib::Ansi ansi(true, false);
			std::string ansiName = ansi.toUtf8(peerName); // I know, this absolutely makes no sense, but this is correct!
			peerName = std::move(ansiName);
		}
		BaseLib::PVariable metadata = GD::bl->db->getAllMetadata(clientInfo, peer, checkAcls);
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

BaseLib::PVariable RPCGetAllScripts::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllScripts")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

#ifndef NO_SCRIPTENGINE
		return GD::scriptEngineServer->getAllScripts();
#else
		return BaseLib::Variable::createError(-32500, "Homegear is compiled without script engine.");
#endif
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

BaseLib::PVariable RPCGetAllSystemVariables::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllSystemVariables")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesReadSet();
		bool returnRoomsAndCategories = false;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);

			returnRoomsAndCategories = parameters->at(0)->booleanValue;
		}

		return GD::bl->db->getAllSystemVariables(clientInfo, returnRoomsAndCategories, checkAcls);
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

BaseLib::PVariable RPCGetAllUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::uiController->getAllUiElements(clientInfo, parameters->at(0)->stringValue);
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

BaseLib::PVariable RPCGetAllValues::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAllValues")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray, BaseLib::VariableType::tBoolean})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerId = 0;
		bool isArray = false;
		bool returnWriteOnly = false;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tBoolean)
		{
			returnWriteOnly = parameters->at(0)->booleanValue;
		}
		else if(parameters->size() == 1 && (parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tInteger64))
		{
			peerId = parameters->at(0)->integerValue64;
		}
		else if(parameters->size() == 1 && (parameters->at(0)->type == BaseLib::VariableType::tArray))
		{
			isArray = true;
		}
		else if(parameters->size() == 2 && (parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tInteger64))
		{
			peerId = parameters->at(0)->integerValue64;
			returnWriteOnly = parameters->at(1)->booleanValue;
		}
		else if(parameters->size() == 2 && (parameters->at(0)->type == BaseLib::VariableType::tArray))
		{
			isArray = true;
		}

		BaseLib::PVariable values(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(peerId > 0)
			{
				if(!central->peerExists(peerId)) continue;
				if(clientInfo->acls->roomsCategoriesDevicesReadSet())
				{
					auto peer = central->getPeer(peerId);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}
			}

			BaseLib::PVariable result;
			if(isArray) result = central->getAllValues(clientInfo, parameters->at(0)->arrayValue, returnWriteOnly, checkAcls);
			else
			{
				auto peerIds = std::make_shared<BaseLib::Array>();
				if(peerId > 0) peerIds->push_back(std::make_shared<BaseLib::Variable>(peerId));
				result = central->getAllValues(clientInfo, peerIds, returnWriteOnly, checkAcls);
			}
			if(result && result->errorStruct)
			{
				if(peerId > 0) return result;
				else GD::out.printWarning("Warning: Error calling method \"getAllValues\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
				continue;
			}
			if(result && !result->arrayValue->empty()) values->arrayValue->insert(values->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			if(peerId > 0) break;
		}

		if(values->arrayValue->empty() && peerId > 0) return BaseLib::Variable::createError(-2, "Unknown device.");
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

BaseLib::PVariable RPCGetAvailableUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getAvailableUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::uiController->getAvailableUiElements(clientInfo, parameters->at(0)->stringValue);
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

BaseLib::PVariable RPCGetCategories::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getCategories")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->categoriesReadSet();
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		std::string languageCode = parameters->empty() ? "" : parameters->at(0)->stringValue;

		return GD::bl->db->getCategories(clientInfo, languageCode, checkAcls);
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

BaseLib::PVariable RPCGetCategoryMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getCategoryMetadata", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->getCategoryMetadata((uint64_t) parameters->at(0)->integerValue64);
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

BaseLib::PVariable RPCGetCategoryUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getCategoryUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::uiController->getUiElementsInCategory(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->stringValue);
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

BaseLib::PVariable RPCGetChannelsInCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getChannelsInCategory", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable devices = central->getChannelsInCategory(clientInfo, (uint64_t) parameters->at(0)->integerValue64, checkAcls);
				result->structValue->insert(devices->structValue->begin(), devices->structValue->end());
			}
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

BaseLib::PVariable RPCGetChannelsInRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomReadAccess("getChannelsInRoom", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable devices = central->getChannelsInRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, checkAcls);
				result->structValue->insert(devices->structValue->begin(), devices->structValue->end());
			}
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

BaseLib::PVariable RPCGetConfigParameter::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getConfigParameter")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->getConfigParameter(clientInfo, parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->stringValue);
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

BaseLib::PVariable RPCGetData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
		return GD::bl->db->getData(parameters->at(0)->stringValue, key);
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

BaseLib::PVariable RPCGetDeviceDescription::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getDeviceDescription")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		bool useSerialNumber = false;
		std::map<std::string, bool> fields;
		int32_t fieldsIndex = -1;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
			if(serialNumber == "BidCoS-RF" || serialNumber == "Homegear") //Return version info
			{
				BaseLib::PVariable description(new BaseLib::Variable(BaseLib::VariableType::tStruct));
				description->structValue->insert(BaseLib::StructElement("TYPE", BaseLib::PVariable(new BaseLib::Variable(std::string("Homegear")))));
				description->structValue->insert(BaseLib::StructElement("FIRMWARE", BaseLib::PVariable(new BaseLib::Variable(std::string(VERSION)))));
				return description;
			}

			if(parameters->size() >= 2) fieldsIndex = 1;
		}
		else if(parameters->size() >= 3) fieldsIndex = 2;

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		if(fieldsIndex != -1)
		{
			for(auto field : *(parameters->at(fieldsIndex)->arrayValue))
			{
				if(field->stringValue.empty()) continue;
				fields[field->stringValue] = true;
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
					if(central->peerExists(serialNumber)) return central->getDeviceDescription(clientInfo, serialNumber, channel, fields);
				}
				else
				{
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->getDeviceDescription(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, fields);
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

BaseLib::PVariable RPCGetDeviceInfo::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getDeviceInfo")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		uint64_t peerId = 0;
		int32_t fieldsIndex = -1;
		if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tArray)
		{
			fieldsIndex = 0;
		}
		else if(parameters->size() == 2 || (parameters->size() == 1 && (parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tInteger64)))
		{
			if(parameters->size() == 2) fieldsIndex = 1;
			peerId = parameters->at(0)->integerValue64;
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
			if(peerId > 0)
			{
				if(!central->peerExists(peerId)) continue;

				if(checkAcls)
				{
					auto peer = central->getPeer(peerId);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->getDeviceInfo(clientInfo, peerId, fields, false);
			}
			else
			{
				BaseLib::PVariable result = central->getDeviceInfo(clientInfo, peerId, fields, checkAcls);
				if(result && result->errorStruct)
				{
					GD::out.printWarning("Warning: Error calling method \"getDeviceInfo\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
					continue;
				}
				if(result && !result->arrayValue->empty()) deviceInfo->arrayValue->insert(deviceInfo->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
		}

		if(peerId == 0) return deviceInfo;
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

BaseLib::PVariable RPCGetDevicesInCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getDevicesInCategory", parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(!GD::bl->db->categoryExists(parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable devices = central->getDevicesInCategory(clientInfo, parameters->at(0)->integerValue64, checkAcls);
				if(result->arrayValue->size() + devices->arrayValue->size() > result->arrayValue->capacity()) result->arrayValue->reserve(result->arrayValue->size() + devices->arrayValue->size() + 1000);
				result->arrayValue->insert(result->arrayValue->end(), devices->arrayValue->begin(), devices->arrayValue->end());
			}
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

BaseLib::PVariable RPCGetDevicesInRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomReadAccess("getDevicesInRoom", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				BaseLib::PVariable devices = central->getDevicesInRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, checkAcls);
				if(result->arrayValue->size() + devices->arrayValue->size() > result->arrayValue->capacity()) result->arrayValue->reserve(result->arrayValue->size() + devices->arrayValue->size() + 1000);
				result->arrayValue->insert(result->arrayValue->end(), devices->arrayValue->begin(), devices->arrayValue->end());
			}
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

BaseLib::PVariable RPCGetEvent::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getEvent")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
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

BaseLib::PVariable RPCGetLastEvents::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getLastEvents")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::set<uint64_t> ids;
		for(auto& id : *parameters->at(0)->arrayValue)
		{
			ids.insert(id->integerValue64);
		}

		return GD::rpcClient->getLastEvents(ids, parameters->at(1)->integerValue);
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

BaseLib::PVariable RPCGetInstallMode::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getInstallMode")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
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

BaseLib::PVariable RPCGetKeyMismatchDevice::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getKeyMismatchDevice")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}));
		if(error != ParameterError::Enum::noError) return getError(error);
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

BaseLib::PVariable RPCGetLinkInfo::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getLinkInfo")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else senderSerialNumber = parameters->at(0)->stringValue;

			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else receiverSerialNumber = parameters->at(1)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->getLinkInfo(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->integerValue);
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

BaseLib::PVariable RPCGetLinkPeers::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getLinkPeers")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
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
					if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
				}
				else serialNumber = parameters->at(0)->stringValue;
			}
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->getLinkPeers(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue);
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

BaseLib::PVariable RPCGetLinks::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getLinks")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>(),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t flags = 0;
		uint64_t peerId = 0;

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
					if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
				}
				else serialNumber = parameters->at(0)->stringValue;

				if(parameters->size() == 2) flags = parameters->at(1)->integerValue;
			}
			else
			{
				peerId = parameters->at(0)->integerValue64;
				if(peerId > 0 && parameters->size() > 1)
				{
					channel = parameters->at(1)->integerValue;
					if(parameters->size() == 3) flags = parameters->at(2)->integerValue;
				}
			}
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		BaseLib::PVariable links(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			if(serialNumber.empty() && peerId == 0)
			{
				BaseLib::PVariable result = central->getLinks(clientInfo, peerId, channel, flags, checkAcls);
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
					if(!central->peerExists(peerId)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->getLinks(clientInfo, peerId, channel, flags, false);
				}
			}
		}

		if(serialNumber.empty() && peerId == 0) return links;
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

BaseLib::PVariable RPCGetMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
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

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
					peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		if(checkAcls && !clientInfo->acls->checkVariableReadAccess(peer, -2, parameters->at(1)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->at(1)->stringValue == "NAME")
		{
			std::string peerName = peer->getName();
			if(peerName.size() > 0) return BaseLib::PVariable(new BaseLib::Variable(peerName));
			BaseLib::PVariable rpcName = GD::bl->db->getMetadata(peer->getID(), parameters->at(1)->stringValue);
			if(peerName.size() == 0 && !rpcName->errorStruct)
			{
				peer->setName(-1, rpcName->stringValue);
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

BaseLib::PVariable RPCGetName::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getName")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->getName(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->size() == 2 ? parameters->at(1)->integerValue : -1);
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

BaseLib::PVariable RPCGetNodeData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
		return GD::bl->db->getNodeData(parameters->at(0)->stringValue, key, clientInfo->flowsServer || clientInfo->scriptEngineServer);
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

BaseLib::PVariable RPCGetFlowData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getFlowData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string key = parameters->size() == 2 ? parameters->at(1)->stringValue : "";
		return GD::bl->db->getNodeData(parameters->at(0)->stringValue, key, clientInfo->flowsServer || clientInfo->scriptEngineServer);
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

BaseLib::PVariable RPCGetGlobalData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getGlobalData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>(),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string key = parameters->size() == 1 ? parameters->at(0)->stringValue : "";
		std::string nodeId = "global";
		return GD::bl->db->getNodeData(nodeId, key, clientInfo->flowsServer || clientInfo->scriptEngineServer);
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

BaseLib::PVariable RPCGetNodeEvents::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeEvents")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::rpcClient->getNodeEvents();
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

BaseLib::PVariable RPCGetNodesWithFixedInputs::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getNodesWithFixedInputs")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(GD::nodeBlueServer) return GD::nodeBlueServer->getNodesWithFixedInputs();
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

BaseLib::PVariable RPCGetNodeVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getNodeVariable")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(GD::nodeBlueServer) return GD::nodeBlueServer->getNodeVariable(parameters->at(0)->stringValue, parameters->at(1)->stringValue);
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

BaseLib::PVariable RPCGetPairingInfo::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getPairingInfo")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
		if(family) return family->getPairingInfo();

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

BaseLib::PVariable RPCGetPairingState::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getPairingState")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
            std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
        }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(parameters->at(0)->integerValue);
        if(!family) return BaseLib::Variable::createError(-2, "Device family not found.");
        auto central = family->getCentral();
        if(!central) return BaseLib::Variable::createError(-32501, "Family has no central.");

		return central->getPairingState(clientInfo);
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

BaseLib::PVariable RPCGetParamsetDescription::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getParamsetDescription")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
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
					if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
				}
				else serialNumber = parameters->at(0)->stringValue;
			}

			if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

			uint64_t remoteId = 0;
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
						if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned) pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
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
				remoteId = (uint64_t) parameters->at(2)->integerValue64;
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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					if(clientInfo->acls->roomsCategoriesDevicesReadSet())
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

						if(remoteId != 0)
						{
							peer = central->getPeer(remoteId);
							if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
						}
					}

					return central->getParamsetDescription(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, type, remoteId, remoteChannel, checkAcls);
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

BaseLib::PVariable RPCGetParamsetId::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getParamsetId")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		uint64_t remoteId = 0;
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
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned) pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
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
			remoteId = parameters->at(2)->integerValue64;
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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

					if(remoteId != 0)
					{
						peer = central->getPeer(remoteId);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}
				}

				return central->getParamsetId(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, type, remoteId, remoteChannel);
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

BaseLib::PVariable RPCGetParamset::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getParamset")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		BaseLib::DeviceDescription::ParameterGroup::Type::Enum type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none;
		uint64_t remoteId = 0;
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;

			type = BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(1)->stringValue);
			if(type == BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none)
			{
				type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
				int32_t pos = parameters->at(1)->stringValue.find(':');
				if(pos > -1)
				{
					remoteSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
					if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) remoteChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(1)->stringValue;
			}
		}
		else
		{
			if(parameters->size() >= 4)
			{
				type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
				remoteId = parameters->at(2)->integerValue64;
				remoteChannel = parameters->at(3)->integerValue;
			}
			else if(parameters->size() == 3) type = BaseLib::DeviceDescription::ParameterGroup::typeFromString(parameters->at(2)->stringValue);

			if(type == BaseLib::DeviceDescription::ParameterGroup::Type::Enum::none) type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

				if(clientInfo->acls->roomsCategoriesDevicesReadSet())
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

					if(remoteId != 0)
					{
						peer = central->getPeer(remoteId);
						if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}
				}

				return central->getParamset(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, type, remoteId, remoteChannel, checkAcls);
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

BaseLib::PVariable RPCGetPeerId::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getPeerId")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
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
				BaseLib::PVariable result = central->getPeerId(clientInfo, parameters->at(0)->integerValue, filterValue, checkAcls);
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
	return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

BaseLib::PVariable RPCGetRoomMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomReadAccess("getRoomMetadata", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->getRoomMetadata((uint64_t) parameters->at(0)->integerValue64);
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

BaseLib::PVariable RPCGetRooms::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getRooms")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsReadSet();
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		std::string languageCode = parameters->empty() ? "" : parameters->at(0)->stringValue;

		return GD::bl->db->getRooms(clientInfo, languageCode, checkAcls);
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

BaseLib::PVariable RPCGetRoomsInStory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getRoomsInStory")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsReadSet();

		return GD::bl->db->getRoomsInStory(clientInfo, (uint64_t) parameters->at(0)->integerValue64, checkAcls);
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

BaseLib::PVariable RPCGetRoomUiElements::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getRoomUiElements")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::uiController->getUiElementsInRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->stringValue);
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

BaseLib::PVariable RPCGetServiceMessages::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getServiceMessages")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>(),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool id = false;
		if(parameters->size() == 1) id = parameters->at(0)->booleanValue;

		BaseLib::PVariable serviceMessages(new BaseLib::Variable(BaseLib::VariableType::tArray));

		if(id)
		{
			auto globalMessages = GD::bl->globalServiceMessages.get(clientInfo);
			if(!globalMessages->arrayValue->empty()) serviceMessages->arrayValue->insert(serviceMessages->arrayValue->end(), globalMessages->arrayValue->begin(), globalMessages->arrayValue->end());
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			BaseLib::PVariable messages = central->getServiceMessages(clientInfo, id, checkAcls);
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

BaseLib::PVariable RPCGetSniffedDevices::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getSniffedDevices")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		auto family = families.find(parameters->at(0)->integerValue);
		if(family == families.end()) return BaseLib::Variable::createError(-1, "Unknown device family.");

		std::shared_ptr<BaseLib::Systems::ICentral> central = family->second->getCentral();
		if(central) return central->getSniffedDevices(clientInfo);

		return BaseLib::Variable::createError(-32500, "No central found.");
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

BaseLib::PVariable RPCGetStories::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getStories")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		std::string languageCode = parameters->empty() ? "" : parameters->at(0)->stringValue;

		return GD::bl->db->getStories(languageCode);
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

BaseLib::PVariable RPCGetStoryMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getStoryMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->getStoryMetadata((uint64_t) parameters->at(0)->integerValue64);
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

BaseLib::PVariable RPCGetSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getSystemVariable")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
		if(error != ParameterError::Enum::noError) return getError(error);

		auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
		if(!systemVariable) return std::make_shared<BaseLib::Variable>();

		if(checkAcls && !clientInfo->acls->checkSystemVariableReadAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return systemVariable->value;
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

BaseLib::PVariable RPCGetSystemVariablesInCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getSystemVariablesInCategory", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesReadSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		return GD::bl->db->getSystemVariablesInCategory(clientInfo, (uint64_t) parameters->at(0)->integerValue64, checkAcls);
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

BaseLib::PVariable RPCGetSystemVariablesInRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getSystemVariablesInRoom", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesReadSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		return GD::bl->db->getSystemVariablesInRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, checkAcls);
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

BaseLib::PVariable RPCGetVariablesInCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getVariablesInCategory", parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkDeviceAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		bool checkVariableAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		if(!GD::bl->db->categoryExists(parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				auto variables = central->getVariablesInCategory(clientInfo, parameters->at(0)->integerValue64, checkDeviceAcls, checkVariableAcls);
				result->structValue->insert(variables->structValue->begin(), variables->structValue->end());
			}
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

BaseLib::PVariable RPCGetVariablesInRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryReadAccess("getVariablesInRoom", parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkDeviceAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();
		bool checkVariableAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		if(!GD::bl->db->roomExists(parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		BaseLib::PVariable result = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				auto variables = central->getVariablesInRoom(clientInfo, parameters->at(0)->integerValue64, checkDeviceAcls, checkVariableAcls);
				result->structValue->insert(variables->structValue->begin(), variables->structValue->end());
			}
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

BaseLib::PVariable RPCGetUpdateStatus::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getUpdateStatus")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>());
		if(error != ParameterError::Enum::noError) return getError(error);

		BaseLib::PVariable updateInfo(new BaseLib::Variable(BaseLib::VariableType::tStruct));

		updateInfo->structValue->insert(BaseLib::StructElement("DEVICE_COUNT", BaseLib::PVariable(new BaseLib::Variable((uint32_t) GD::bl->deviceUpdateInfo.devicesToUpdate))));
		updateInfo->structValue->insert(BaseLib::StructElement("CURRENT_UPDATE", BaseLib::PVariable(new BaseLib::Variable((uint32_t) GD::bl->deviceUpdateInfo.currentUpdate))));
		updateInfo->structValue->insert(BaseLib::StructElement("CURRENT_DEVICE", BaseLib::PVariable(new BaseLib::Variable((uint32_t) GD::bl->deviceUpdateInfo.currentDevice))));
		updateInfo->structValue->insert(BaseLib::StructElement("CURRENT_DEVICE_PROGRESS", BaseLib::PVariable(new BaseLib::Variable((uint32_t) GD::bl->deviceUpdateInfo.currentDeviceProgress))));

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

BaseLib::PVariable RPCGetValue::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getValue")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint64_t peerId = parameters->at(0)->integerValue64;
		int32_t channel = 0;
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
			if(parameters->size() >= 3) requestFromDevice = parameters->at(2)->booleanValue;
			if(parameters->size() >= 4) asynchronously = parameters->at(3)->booleanValue;
		}
		else
		{
			channel = parameters->at(1)->integerValue;
			if(parameters->size() >= 4) requestFromDevice = parameters->at(3)->booleanValue;
			if(parameters->size() >= 5) asynchronously = parameters->at(4)->booleanValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		if(!useSerialNumber)
		{
			if(peerId == 0 && channel < 0)
			{
				BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
				requestParameters->arrayValue->push_back(parameters->at(2));
				std::string methodName = "getSystemVariable";
				return GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
			}
			else if(peerId != 0 && channel < 0)
			{
				BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
				requestParameters->arrayValue->reserve(2);
				requestParameters->arrayValue->push_back(parameters->at(0));
				requestParameters->arrayValue->push_back(parameters->at(2));
				std::string methodName = "getMetadata";
				return GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
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
					if(central->peerExists(serialNumber)) return central->getValue(clientInfo, serialNumber, channel, parameters->at(1)->stringValue, requestFromDevice, asynchronously);
				}
				else
				{
					if(!central->peerExists(peerId)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkVariableReadAccess(peer, channel, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->getValue(clientInfo, peerId, channel, parameters->at(2)->stringValue, requestFromDevice, asynchronously);
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

BaseLib::PVariable RPCGetVariableDescription::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getVariableDescription")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkVariableReadAccess(peer, parameters->at(1)->integerValue, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->getVariableDescription(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, parameters->at(2)->stringValue);
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

BaseLib::PVariable RPCGetVersion::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("getVersion")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

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

BaseLib::PVariable RPCInit::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("init")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(_disposing) return BaseLib::Variable::createError(-32500, "Method is disposing.");
		if(!clientInfo) return BaseLib::Variable::createError(-32500, "clientInfo is nullptr.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo->address.empty()) GD::out.printInfo("Info: Client with IP " + clientInfo->address + " is calling \"init\".");

		std::string url;
		std::string interfaceId;
		int32_t flags = 0;
		if(parameters->size() == 1)
		{
			url = parameters->at(0)->stringValue;
		}
		else if(parameters->size() == 2)
		{
			if(parameters->at(1)->type == BaseLib::VariableType::tInteger || parameters->at(1)->type == BaseLib::VariableType::tInteger64)
			{
				interfaceId = parameters->at(0)->stringValue;
				flags = parameters->at(1)->integerValue;
			}
			else
			{
				url = parameters->at(0)->stringValue;
				interfaceId = parameters->at(1)->stringValue;
			}
		}
		else
		{
			url = parameters->at(0)->stringValue;
			interfaceId = parameters->at(1)->stringValue;
			flags = parameters->at(2)->integerValue;
		}

		if(!url.empty() && GD::bl->settings.clientAddressesToReplace().find(url) != GD::bl->settings.clientAddressesToReplace().end())
		{
			std::string newAddress = GD::bl->settings.clientAddressesToReplace().at(url);
			std::string remoteIP = clientInfo->address;
			if(remoteIP.empty()) return BaseLib::Variable::createError(-32500, "Could not get client's IP address.");
			GD::bl->hf.stringReplace(newAddress, "$remoteip", remoteIP);
			GD::out.printInfo("Info: Replacing address " + url + " with " + newAddress);
			url = newAddress;
		}

		std::pair<std::string, std::string> server;
		std::string path;
		if(!url.empty())
		{
			server = BaseLib::HelperFunctions::splitLast(url, ':');
			if(server.first.empty() || server.second.empty()) return BaseLib::Variable::createError(-32602, "Server address or port is empty.");
			if(server.first.size() < 8) return BaseLib::Variable::createError(-32602, "Server address too short.");
			BaseLib::HelperFunctions::toLower(server.first);

			path = "/RPC2";
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
		}

		if(interfaceId.empty())
		{
			GD::rpcClient->removeServer(server);
		}
		else
		{
			if(_reservedIds.find(interfaceId) != _reservedIds.end() || interfaceId.compare(0, sizeof("device-") - 1, "device-") == 0)
			{
				interfaceId = "client-" + interfaceId;
			}

			if(url.empty()) //Send events over connection to server
			{
				clientInfo->sendEventsToRpcServer = true;

				server = std::make_pair(clientInfo->address, std::to_string(clientInfo->port));

				std::shared_ptr<RemoteRpcServer> eventServer = GD::rpcClient->addSingleConnectionServer(server, clientInfo, interfaceId);
				if(!eventServer)
				{
					GD::out.printError("Error: Could not create event server.");
					return BaseLib::Variable::createError(-32500, "Unknown application error.");
				}

				clientInfo->initInterfaceId = interfaceId;

				if(flags != 0)
				{
					clientInfo->initBinaryMode = (flags & 2) || eventServer->binary;
					clientInfo->initNewFormat = true;
					clientInfo->initSubscribePeers = flags & 8;
					clientInfo->initJsonMode = flags & 0x10;
					clientInfo->initSendNewDevices = false;
				}

				eventServer->type = clientInfo->clientType;
				if(flags != 0)
				{
					eventServer->binary = (flags & 2) || eventServer->binary;
					eventServer->newFormat = true;
					eventServer->subscribePeers = flags & 8;
					eventServer->json = flags & 0x10;
					eventServer->sendNewDevices = false;
				}
			}
			else //Send events over seperate connection to a client's server
			{
				std::shared_ptr<RemoteRpcServer> eventServer = GD::rpcClient->addServer(server, clientInfo, path, interfaceId);
				if(!eventServer)
				{
					GD::out.printError("Error: Could not create event server.");
					return BaseLib::Variable::createError(-32500, "Unknown application error.");
				}

				if(server.first.compare(0, 5, "https") == 0 || server.first.compare(0, 7, "binarys") == 0) eventServer->useSSL = true;
				if(server.first.compare(0, 6, "binary") == 0 ||
				   server.first.compare(0, 7, "binarys") == 0 ||
				   server.first.compare(0, 10, "xmlrpc_bin") == 0)
					eventServer->binary = true;

				// {{{ Reconnect on CCU2 as it doesn't reconnect automatically
				if((BaseLib::Math::isNumber(interfaceId, false) && server.second == "1999") || interfaceId == "Homegear_java")
				{
					clientInfo->clientType = BaseLib::RpcClientType::ccu2;
					eventServer->reconnectInfinitely = true;
					if(eventServer->socket)
					{
						eventServer->socket->setReadTimeout(30000000);
						eventServer->socket->setWriteTimeout(30000000);
					}
				}
					// }}}
					// {{{ IP-Symcon
				else if(interfaceId.size() >= 3 && interfaceId.compare(0, 3, "IPS") == 0)
				{
					clientInfo->clientType = BaseLib::RpcClientType::ipsymcon;
					eventServer->sendNewDevices = false;
					if(interfaceId == "IPS") eventServer->reconnectInfinitely = true; //Keep connection to IP-Symcon version 3
				}
					// }}}
					// {{{ Home Assistant
				else if(interfaceId.size() >= 13 && interfaceId.compare(0, 13, "homeassistant") == 0)
				{
					clientInfo->clientType = BaseLib::RpcClientType::homeassistant;
				}
				// }}}

				clientInfo->initUrl = url;
				clientInfo->initInterfaceId = interfaceId;

				if(flags != 0)
				{
					clientInfo->initKeepAlive = flags & 1;
					clientInfo->initBinaryMode = (flags & 2) || eventServer->binary;
					clientInfo->initNewFormat = flags & 4;
					clientInfo->initSubscribePeers = flags & 8;
					clientInfo->initJsonMode = flags & 0x10;
					clientInfo->initSendNewDevices = !(flags & 0x20);
				}

				eventServer->type = clientInfo->clientType;
				if(flags != 0)
				{
					if(!eventServer->keepAlive) eventServer->keepAlive = flags & 1;
					eventServer->binary = (flags & 2) || eventServer->binary;
					eventServer->newFormat = flags & 4;
					eventServer->subscribePeers = flags & 8;
					eventServer->json = flags & 0x10;
					eventServer->sendNewDevices = !(flags & 0x20);
					if(!eventServer->reconnectInfinitely) eventServer->reconnectInfinitely = (flags & 128);
				}
			}

			std::lock_guard<std::mutex> initServerThreadGuard(_initServerThreadMutex);
			try
			{
				if(_disposing) return BaseLib::Variable::createError(-32500, "I'm disposing.");
				GD::bl->threadManager.join(_initServerThread);
				GD::bl->threadManager.start(_initServerThread, false, &Rpc::Client::initServerMethods, GD::rpcClient.get(), server);
			}
			catch(const std::exception& ex)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
			}
			catch(...)
			{
				GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
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

BaseLib::PVariable RPCInvokeFamilyMethod::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("invokeFamilyMethod")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tArray}));
		if(error != ParameterError::Enum::noError) return getError(error);

		auto family = GD::familyController->getFamily(parameters->at(0)->integerValue);
		if(!family) return BaseLib::Variable::createError(-32501, "Unknown family.");

		auto central = family->getCentral();
		if(!central) return BaseLib::Variable::createError(-32501, "Family has no central.");

		return central->invokeFamilyMethod(clientInfo, parameters->at(1)->stringValue, parameters->at(2)->arrayValue);
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

BaseLib::PVariable RPCListBidcosInterfaces::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listBidcosInterfaces")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

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

BaseLib::PVariable RPCListClientServers::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listClientServers")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		std::string id;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
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

BaseLib::PVariable RPCListDevices::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listDevices")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray, BaseLib::VariableType::tInteger}),
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		bool channels = true;
		std::map<std::string, bool> fields;
		int32_t familyId = -1;
		// The HomeMatic configurator passes boolean false to this method, so only accept "channels = false" when there are more than 1 arguments.
		if(parameters->size() > 1 && parameters->at(0)->type == BaseLib::VariableType::tBoolean) channels = parameters->at(0)->booleanValue;
		if(parameters->size() >= 2)
		{
			for(std::vector<BaseLib::PVariable>::iterator i = parameters->at(1)->arrayValue->begin(); i != parameters->at(1)->arrayValue->end(); ++i)
			{
				if((*i)->stringValue.empty()) continue;
				fields[(*i)->stringValue] = true;
			}
			if(parameters->size() == 3) familyId = parameters->at(2)->integerValue;
		}
		else if(parameters->size() == 1 && parameters->at(0)->type == BaseLib::VariableType::tBoolean && !parameters->at(0)->booleanValue)
		{
			//Client is HomeMatic Configurator
			clientInfo->clientType = BaseLib::RpcClientType::homematicconfigurator;
		}

		BaseLib::PVariable devices(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			if(familyId != -1 && i->first != familyId) continue;
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			BaseLib::PVariable result = central->listDevices(clientInfo, channels, fields, checkAcls);
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

BaseLib::PVariable RPCListEvents::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listEvents")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
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
				peerID = parameters->at(0)->integerValue64;
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

BaseLib::PVariable RPCListFamilies::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listFamilies")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}

		int32_t familyId = parameters->size() > 0 ? parameters->at(0)->integerValue : -1;

		return GD::familyController->listFamilies(familyId);
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

BaseLib::PVariable RPCListInterfaces::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listInterfaces")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		int32_t familyID = -1;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
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

BaseLib::PVariable RPCListKnownDeviceTypes::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listKnownDeviceTypes")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		int32_t familyId = parameters->size() == 3 ? parameters->at(0)->integerValue : -1;
		bool channels = true;
		std::set<std::string> fields;
		channels = parameters->size() == 3 ? parameters->at(1)->booleanValue : parameters->at(0)->booleanValue;
		if(parameters->size() >= 2)
		{
			BaseLib::PArray& array = parameters->size() == 3 ? parameters->at(2)->arrayValue : parameters->at(1)->arrayValue;
			for(auto element : *array)
			{
				if(element->stringValue.empty()) continue;
				fields.insert(element->stringValue);
			}
		}

		BaseLib::PVariable devices;
		if(familyId != -1)
		{
			std::shared_ptr<BaseLib::Systems::DeviceFamily> family = GD::familyController->getFamily(familyId);
			if(!family) return BaseLib::Variable::createError(-2, "Device family not found.");
			devices = family->listKnownDeviceTypes(clientInfo, channels, fields);
		}
		else
		{
			devices = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

			std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
			for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
			{
				BaseLib::PVariable result = i->second->listKnownDeviceTypes(clientInfo, channels, fields);
				if(result && result->errorStruct)
				{
					GD::out.printWarning("Warning: Error calling method \"listKnownDeviceTypes\" on device family " + i->second->getName() + ": " + result->structValue->at("faultString")->stringValue);
					continue;
				}
				if(result && !result->arrayValue->empty()) devices->arrayValue->insert(devices->arrayValue->end(), result->arrayValue->begin(), result->arrayValue->end());
			}
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

BaseLib::PVariable RPCListTeams::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("listTeams")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		BaseLib::PVariable teams(new BaseLib::Variable(BaseLib::VariableType::tArray));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			BaseLib::PVariable result = central->listTeams(clientInfo, checkAcls);
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

BaseLib::PVariable RPCLogLevel::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("logLevel")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}));
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

BaseLib::PVariable RPCNodeOutput::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("nodeOutput")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(GD::nodeBlueServer) GD::nodeBlueServer->nodeOutput(parameters->at(0)->stringValue, parameters->at(1)->integerValue, parameters->at(2), parameters->size() == 4 ? parameters->at(3)->booleanValue : false);

		return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCPeerExists::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("peerExists")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger64}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger64})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->size() == 2)
		{
			auto family = GD::familyController->getFamily(parameters->at(0)->integerValue);
			if(!family) return BaseLib::Variable::createError(-1, "Unknown family.");

			auto central = family->getCentral();
			if(!central) return BaseLib::Variable::createError(-1, "Unknown family (2).");

			return std::make_shared<BaseLib::Variable>(central->peerExists((uint64_t) parameters->at(1)->integerValue64));
		}
		else
		{
			std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
			for(auto& family : families)
			{
				auto central = family.second->getCentral();
				if(!central) continue;

				if(central->peerExists((uint64_t) parameters->at(0)->integerValue64)) return std::make_shared<BaseLib::Variable>(true);
			}
			return std::make_shared<BaseLib::Variable>(false);
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

BaseLib::PVariable RPCPing::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("ping")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		std::string id;
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);
			id = parameters->at(0)->stringValue;
		}

		std::string source = clientInfo->initInterfaceId;
		std::shared_ptr<std::vector<std::string>> variables(new std::vector<std::string>{"PONG"});
		BaseLib::PArray values(new BaseLib::Array{BaseLib::PVariable(new BaseLib::Variable(id))});
		GD::familyController->onEvent(source, 0, -1, variables, values);
		GD::familyController->onRPCEvent(source, 0, -1, "CENTRAL", variables, values);

		return BaseLib::PVariable(new BaseLib::Variable(true));
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

BaseLib::PVariable RPCPutParamset::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("putParamset")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct})
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		uint64_t remoteId = 0;
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
					if(parameters->at(parameterSetIndex)->stringValue.size() > (unsigned) pos + 1) remoteChannel = std::stoll(parameters->at(parameterSetIndex)->stringValue.substr(pos + 1));
				}
				else remoteSerialNumber = parameters->at(parameterSetIndex)->stringValue;
				// {{{ HomeMatic-Konfigurator Bugfix
				std::string copy = remoteSerialNumber;
				BaseLib::HelperFunctions::toUpper(copy);
				if(copy == serialNumber) remoteSerialNumber = copy;
				// }}}
			}
		}
		else if(parameters->at(parameterSetIndex)->type == BaseLib::VariableType::tStruct)
		{
			type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::config;
		}
		else
		{
			type = BaseLib::DeviceDescription::ParameterGroup::Type::Enum::link;
			remoteId = parameters->at(2)->integerValue64;
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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

				if(clientInfo->acls->roomsCategoriesDevicesWriteSet())
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

					if(remoteId != 0)
					{
						peer = central->getPeer(remoteId);
						if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}
				}

				return central->putParamset(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, type, remoteId, remoteChannel, parameters->back(), checkAcls);
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

BaseLib::PVariable RPCRemoveCategoryFromChannel::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeCategoryFromChannel", (uint64_t) parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->removeCategoryFromChannel(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, (uint64_t) parameters->at(2)->integerValue64);
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

BaseLib::PVariable RPCRemoveCategoryFromDevice::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeCategoryFromDevice", parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->removeCategoryFromChannel(clientInfo, parameters->at(0)->integerValue64, -1, parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCRemoveCategoryFromSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeCategoryFromSystemVariable", parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesWriteSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
		if(!systemVariable) return BaseLib::Variable::createError(-5, "Unknown system variable.");

		if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->at(1)->integerValue64 != 0) systemVariable->categories.erase((uint64_t) parameters->at(1)->integerValue64);

		return GD::bl->db->setSystemVariableCategories(systemVariable->name, systemVariable->categories);
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

BaseLib::PVariable RPCRemoveCategoryFromVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeCategoryFromVariable", (uint64_t) parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->categoryExists((uint64_t) parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown category.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
			if(!peer) continue;

			if(checkAcls)
			{
				if(!clientInfo->acls->checkVariableWriteAccess(peer, parameters->at(1)->integerValue, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
			}

			bool result = peer->removeCategoryFromVariable(parameters->at(1)->integerValue, parameters->at(2)->stringValue, (uint64_t) parameters->at(3)->integerValue64);
			return std::make_shared<BaseLib::Variable>(result);
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

BaseLib::PVariable RPCRemoveChannelFromRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("removeChannelFromRoom", (uint64_t) parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(2)->integerValue64)) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->removeChannelFromRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, (uint64_t) parameters->at(2)->integerValue64);
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

BaseLib::PVariable RPCRemoveDeviceFromRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeDeviceFromRoom", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		if(!GD::bl->db->roomExists((uint64_t) parameters->at(1)->integerValue)) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->removeChannelFromRoom(clientInfo, (uint64_t) parameters->at(0)->integerValue64, -1, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCRemoveEvent::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("removeEvent")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
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

BaseLib::PVariable RPCRemoveLink::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("removeLink")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else senderSerialNumber = parameters->at(0)->stringValue;

			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else receiverSerialNumber = parameters->at(1)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64) || !central->peerExists((uint64_t) parameters->at(2)->integerValue64)) continue;

				if(checkAcls)
				{
					auto peer1 = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					auto peer2 = central->getPeer((uint64_t) parameters->at(2)->integerValue64);
					if(!peer1 || !clientInfo->acls->checkDeviceWriteAccess(peer1)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					if(!peer2 || !clientInfo->acls->checkDeviceWriteAccess(peer2)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->removeLink(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, (uint64_t) parameters->at(2)->integerValue64, parameters->at(3)->integerValue);
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

BaseLib::PVariable RPCRemoveRoomFromStory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("removeRoomFromStory", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->removeRoomFromStory((uint64_t) parameters->at(0)->integerValue64, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCRemoveSystemVariableFromRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeSystemVariableFromRoom", (uint64_t) parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesWriteSet();

		uint64_t roomId = (uint64_t) parameters->at(1)->integerValue64;

		if(!GD::bl->db->roomExists(roomId)) return BaseLib::Variable::createError(-1, "Unknown room.");

		auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
		if(!systemVariable) return BaseLib::Variable::createError(-5, "Unknown system variable.");

		if(checkAcls && !clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(systemVariable->room == roomId) return GD::bl->db->setSystemVariableRoom(systemVariable->name, 0);
		else return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCRemoveUiElement::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("removeUiElement")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::uiController->removeUiElement(clientInfo, (uint64_t) parameters->at(0)->integerValue64);
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

BaseLib::PVariable RPCRemoveVariableFromRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("removeVariableFromRoom", (uint64_t) parameters->at(3)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		uint64_t roomId = (uint64_t) parameters->at(3)->integerValue64;

		if(!GD::bl->db->roomExists(roomId)) return BaseLib::Variable::createError(-1, "Unknown room.");

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
			if(!peer) continue;

			if(checkAcls)
			{
				if(!clientInfo->acls->checkVariableWriteAccess(peer, parameters->at(1)->integerValue, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
			}

			bool result = false;
			if(peer->getVariableRoom(parameters->at(1)->integerValue, parameters->at(2)->stringValue) == roomId)
			{
				result = peer->setVariableRoom(parameters->at(1)->integerValue, parameters->at(2)->stringValue, 0);
			}
			return std::make_shared<BaseLib::Variable>(result);
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

BaseLib::PVariable RPCReportValueUsage::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("reportValueUsage")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger}),
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
			if(central && central->peerExists(serialNumber))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer(serialNumber);
					if(!peer || !clientInfo->acls->checkDeviceReadAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->reportValueUsage(clientInfo, serialNumber);
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

BaseLib::PVariable RPCRssiInfo::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("rssiInfo")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesReadSet();

		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		BaseLib::PVariable response(new BaseLib::Variable(BaseLib::VariableType::tStruct));
		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(!central) continue;
			BaseLib::PVariable result = central->rssiInfo(clientInfo, checkAcls);
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

BaseLib::PVariable RPCRunScript::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("runScript")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);
		if(GD::bl->shuttingDown)
		{
			return BaseLib::Variable::createError(-32501, "Homegear is shutting down.");
		}

#ifndef NO_SCRIPTENGINE
		bool internalEngine = false;
#endif

		bool wait = false;
		std::string filename;
		std::string arguments;

		filename = parameters->at(0)->stringValue;
		std::string ending = "";
		auto pos = filename.find_last_of('.');
		if(pos != std::string::npos) ending = filename.substr(pos);
		GD::bl->hf.toLower(ending);
		if(ending == ".php" || ending == ".php5" || ending == ".php7" || ending == ".hgs")
		{
#ifndef NO_SCRIPTENGINE
			internalEngine = true;
#else
			GD::out.printWarning("Warning: Not executing PHP script with internal script engine as Homegear is compiled without script engine.");
#endif
		}

		if((signed) parameters->size() == 2)
		{
			if(parameters->at(1)->type == BaseLib::VariableType::tBoolean) wait = parameters->at(1)->booleanValue;
			else arguments = parameters->at(1)->stringValue;
		}
		if((signed) parameters->size() == 3)
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
#ifndef NO_SCRIPTENGINE
		if(internalEngine)
		{
			if(GD::bl->debugLevel >= 4) GD::out.printInfo("Info: Executing script \"" + fullPath + "\" with parameters \"" + arguments + "\" using internal script engine.");
			BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli, fullPath, relativePath, arguments));
			if(wait) scriptInfo->returnOutput = true;
			GD::scriptEngineServer->executeScript(scriptInfo, wait);
			if(!scriptInfo->started)
			{
				output->stringValue = "Error: Could not execute script. Check log file for more details (1).";
				exitCode->integerValue = -1;
				return result;
			}
			if(wait) exitCode->integerValue = scriptInfo->exitCode;
			if(!scriptInfo->output.empty()) output->stringValue = std::move(scriptInfo->output);
			return result;
		}
		else
		{
#endif
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
#ifndef NO_SCRIPTENGINE
		}
#endif
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

BaseLib::PVariable RPCSearchDevices::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("searchDevices")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		int32_t familyID = -1;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
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

BaseLib::PVariable RPCSearchInterfaces::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("searchInterfaces")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		int32_t familyID = -1;
		BaseLib::PVariable metadata;
		if(!parameters->empty())
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																															 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tVariant})
																													 }));
			if(error != ParameterError::Enum::noError) return getError(error);

			familyID = parameters->at(0)->integerValue;
			if(parameters->size() > 1) metadata = parameters->at(1);
		}

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		if(familyID > -1)
		{
			if(families.find(familyID) == families.end())
			{
				return BaseLib::Variable::createError(-2, "Device family is unknown.");
			}
			return families.at(familyID)->getCentral()->searchInterfaces(clientInfo, metadata);
		}

		BaseLib::PVariable result(new BaseLib::Variable(BaseLib::VariableType::tInteger));
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central) result->integerValue += central->searchInterfaces(clientInfo, metadata)->integerValue;
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

BaseLib::PVariable RPCSetCategoryMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("setCategoryMetadata", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->setCategoryMetadata((uint64_t) parameters->at(0)->integerValue64, parameters->at(1));
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

BaseLib::PVariable RPCSetData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->setData(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
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

BaseLib::PVariable RPCSetGlobalServiceMessage::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setGlobalServiceMessage")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		GD::bl->globalServiceMessages.set(parameters->at(0)->integerValue, parameters->at(1)->integerValue, parameters->at(2)->integerValue, parameters->at(3)->stringValue, parameters->at(4), parameters->at(5)->integerValue64);

		return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCSetId::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setId")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();

		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(1)->integerValue64))
			{
				return BaseLib::Variable::createError(101, "New Peer ID is already in use.");
			}

			//Double check and also check in database
			if(GD::bl->db->peerExists(parameters->at(1)->integerValue64)) return BaseLib::Variable::createError(101, "New Peer ID is already in use.");
		}

		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->setId(clientInfo, (uint64_t) parameters->at(0)->integerValue64, (uint64_t) parameters->at(1)->integerValue64);
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

BaseLib::PVariable RPCSetInstallMode::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setInstallMode")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t enableIndex = -1;
		if(parameters->at(0)->type == BaseLib::VariableType::tBoolean) enableIndex = 0;
		else if(parameters->size() >= 2) enableIndex = 1;
		int32_t familyIDIndex = (parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tInteger64) ? 0 : -1;
		int32_t timeIndex = -1;
		if(parameters->size() == 2 && (parameters->at(1)->type == BaseLib::VariableType::tInteger || parameters->at(1)->type == BaseLib::VariableType::tInteger64)) timeIndex = 1;
		else if(parameters->size() >= 3 && (parameters->at(2)->type == BaseLib::VariableType::tInteger || parameters->at(2)->type == BaseLib::VariableType::tInteger64)) timeIndex = 2;
		int32_t metadataIndex = -1;
		if(parameters->size() == 3 && parameters->at(2)->type == BaseLib::VariableType::tStruct) metadataIndex = 2;
		else if(parameters->size() == 4) metadataIndex = 3;

		bool enable = (enableIndex > -1) ? parameters->at(enableIndex)->booleanValue : false;
		int32_t familyID = (familyIDIndex > -1) ? parameters->at(familyIDIndex)->integerValue : -1;
		BaseLib::PVariable metadata;
		if(metadataIndex != -1) metadata = parameters->at(metadataIndex);

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
			return families.at(familyID)->getCentral()->setInstallMode(clientInfo, enable, time, metadata);
		}

		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central) central->setInstallMode(clientInfo, enable, time, metadata);
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

BaseLib::PVariable RPCSetInterface::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setInterface")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central)
			{
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->setInterface(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->stringValue);
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

BaseLib::PVariable RPCSetLanguage::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setLanguage")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(clientInfo) clientInfo->language = parameters->at(0)->stringValue;

		return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCSetLinkInfo::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setLinkInfo")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString})

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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) senderChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else senderSerialNumber = parameters->at(0)->stringValue;

			pos = parameters->at(1)->stringValue.find(':');
			if(pos > -1)
			{
				receiverSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
				if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) receiverChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
			}
			else receiverSerialNumber = parameters->at(1)->stringValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		std::string name;
		if((signed) parameters->size() > nameIndex)
		{
			if(parameters->at(nameIndex)->stringValue.size() > 250) return BaseLib::Variable::createError(-32602, "Name has more than 250 characters.");
			name = parameters->at(nameIndex)->stringValue;
		}
		std::string description;
		if((signed) parameters->size() > nameIndex + 1)
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
					if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64)) continue;

					if(checkAcls)
					{
						auto peer1 = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						auto peer2 = central->getPeer((uint64_t) parameters->at(2)->integerValue64);
						if(!peer1 || !clientInfo->acls->checkDeviceWriteAccess(peer1)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
						if(!peer2 || !clientInfo->acls->checkDeviceWriteAccess(peer2)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->setLinkInfo(clientInfo, (uint64_t) parameters->at(0)->integerValue64, parameters->at(1)->integerValue, (uint64_t) parameters->at(2)->integerValue64, parameters->at(3)->integerValue, name, description);
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

BaseLib::PVariable RPCSetMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
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

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		BaseLib::PVariable value = parameters->size() > 2 ? parameters->at(2) : std::make_shared<BaseLib::Variable>();

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
					peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(peer) break;
				}
			}
		}

		if(!peer) return BaseLib::Variable::createError(-2, "Device not found.");

		if(checkAcls && !clientInfo->acls->checkVariableWriteAccess(peer, -2, parameters->at(1)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->at(1)->stringValue == "NAME")
		{
			// Assume HomeMatic Configurator here.
			BaseLib::Ansi ansi(false, true);
			parameters->at(2)->stringValue = ansi.toAnsi(parameters->at(2)->stringValue); // I know, this absolutely makes no sense, but this is correct!
			peer->setName(-1, parameters->at(2)->stringValue);
			return BaseLib::PVariable(new BaseLib::Variable(BaseLib::VariableType::tVoid));
		}
		serialNumber = peer->getSerialNumber();
		return GD::bl->db->setMetadata(clientInfo, peer->getID(), serialNumber, parameters->at(1)->stringValue, value);
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

BaseLib::PVariable RPCSetName::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setName")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = parameters->size() == 3 ? parameters->at(1)->integerValue : -1;
		std::string name = parameters->size() == 3 ? parameters->at(2)->stringValue : parameters->at(1)->stringValue;

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		for(std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>>::iterator i = families.begin(); i != families.end(); ++i)
		{
			std::shared_ptr<BaseLib::Systems::ICentral> central = i->second->getCentral();
			if(central && central->peerExists((uint64_t) parameters->at(0)->integerValue64))
			{
				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->setName(clientInfo, (uint64_t) parameters->at(0)->integerValue64, channel, name);
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

BaseLib::PVariable RPCSetNodeData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setNodeData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->setNodeData(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
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

BaseLib::PVariable RPCSetFlowData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setFlowData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		return GD::bl->db->setNodeData(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
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

BaseLib::PVariable RPCSetGlobalData::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setGlobalData")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::string nodeId = "global";
		return GD::bl->db->setNodeData(nodeId, parameters->at(0)->stringValue, parameters->at(1));
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

BaseLib::PVariable RPCSetNodeVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setNodeVariable")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(GD::nodeBlueServer) GD::nodeBlueServer->setNodeVariable(parameters->at(0)->stringValue, parameters->at(1)->stringValue, parameters->at(2));
		return std::make_shared<BaseLib::Variable>();
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

BaseLib::PVariable RPCSetRoomMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("setRoomMetadata", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->setRoomMetadata((uint64_t) parameters->at(0)->integerValue64, parameters->at(1));
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

BaseLib::PVariable RPCSetStoryMetadata::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setStoryMetadata")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		return GD::bl->db->setStoryMetadata((uint64_t) parameters->at(0)->integerValue64, parameters->at(1));
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

BaseLib::PVariable RPCSetSystemVariable::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setSystemVariable")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tVariant})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(checkAcls)
		{
			auto systemVariable = GD::bl->db->getSystemVariableInternal(parameters->at(0)->stringValue);
			if(!systemVariable)
			{
				systemVariable = std::make_shared<BaseLib::Database::SystemVariable>();
				systemVariable->name = parameters->at(0)->stringValue;
				systemVariable->value = std::make_shared<BaseLib::Variable>();
			}
			if(!clientInfo->acls->checkSystemVariableWriteAccess(systemVariable)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		}

		BaseLib::PVariable value = parameters->size() > 1 ? parameters->at(1) : std::make_shared<BaseLib::Variable>();

		return GD::bl->db->setSystemVariable(clientInfo, parameters->at(0)->stringValue, value);
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

BaseLib::PVariable RPCSetTeam::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setTeam")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t deviceChannel = -1;
		std::string deviceSerialNumber;
		uint64_t teamId = 0;
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
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) deviceChannel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else deviceSerialNumber = parameters->at(0)->stringValue;

			if(parameters->size() > 1)
			{
				pos = parameters->at(1)->stringValue.find(':');
				if(pos > -1)
				{
					teamSerialNumber = parameters->at(1)->stringValue.substr(0, pos);
					if(parameters->at(1)->stringValue.size() > (unsigned) pos + 1) teamChannel = std::stoll(parameters->at(1)->stringValue.substr(pos + 1));
				}
				else teamSerialNumber = parameters->at(1)->stringValue;
			}
		}
		else if(parameters->size() > 2)
		{
			teamId = (parameters->at(2)->integerValue64 == -1) ? 0 : parameters->at(2)->integerValue64;
			teamChannel = parameters->at(3)->integerValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

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
				if(!central->peerExists((uint64_t) parameters->at(0)->integerValue64) || !central->peerExists(teamId)) continue;

				if(checkAcls)
				{
					auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

					peer = central->getPeer(teamId);
					if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
				}

				return central->setTeam(clientInfo, parameters->at(0)->integerValue64, parameters->at(1)->integerValue, teamId, teamChannel);
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

BaseLib::PVariable RPCSetValue::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("setValue")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->variablesRoomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);
		std::string serialNumber;
		uint64_t peerId = (uint64_t) parameters->at(0)->integerValue64;
		int32_t channel = 0;
		bool useSerialNumber = false;
		if(parameters->at(0)->type == BaseLib::VariableType::tString)
		{
			useSerialNumber = true;
			int32_t pos = parameters->at(0)->stringValue.find(':');
			if(pos > -1)
			{
				serialNumber = parameters->at(0)->stringValue.substr(0, pos);
				if(parameters->at(0)->stringValue.size() > (unsigned) pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
			}
			else serialNumber = parameters->at(0)->stringValue;
		}
		else
		{
			channel = parameters->at(1)->integerValue;
		}

		if(useSerialNumber && checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

		BaseLib::PVariable value;
		if(useSerialNumber && parameters->size() >= 3) value = parameters->at(2);
		else if(!useSerialNumber && parameters->size() >= 4) value = parameters->at(3);
		else value.reset(new BaseLib::Variable(BaseLib::VariableType::tVoid));

		bool wait = true;
		if(useSerialNumber && parameters->size() == 4) wait = parameters->at(3)->booleanValue;
		else if(!useSerialNumber && parameters->size() == 5) wait = parameters->at(4)->booleanValue;

		if(!useSerialNumber)
		{
			if(peerId == 0 && channel < 0)
			{
				BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
				requestParameters->arrayValue->reserve(2);
				requestParameters->arrayValue->push_back(parameters->at(2));
				requestParameters->arrayValue->push_back(value);
				std::string methodName = "setSystemVariable";
				return GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
			}
			else if(peerId != 0 && channel < 0)
			{
				BaseLib::PVariable requestParameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
				requestParameters->arrayValue->reserve(3);
				requestParameters->arrayValue->push_back(parameters->at(0));
				requestParameters->arrayValue->push_back(parameters->at(2));
				requestParameters->arrayValue->push_back(value);
				std::string methodName = "setMetadata";
				return GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, requestParameters);
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
					if(central->peerExists(serialNumber)) return central->setValue(clientInfo, serialNumber, channel, parameters->at(1)->stringValue, value, wait);
				}
				else
				{
					if(!central->peerExists(peerId)) continue;

					if(checkAcls)
					{
						auto peer = central->getPeer(peerId);
						if(!peer || !clientInfo->acls->checkVariableWriteAccess(peer, channel, parameters->at(2)->stringValue)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					return central->setValue(clientInfo, peerId, channel, parameters->at(2)->stringValue, value, wait);
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

BaseLib::PVariable RPCStartSniffing::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("startSniffing")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		auto family = families.find(parameters->at(0)->integerValue);
		if(family == families.end()) return BaseLib::Variable::createError(-1, "Unknown device family.");

		std::shared_ptr<BaseLib::Systems::ICentral> central = family->second->getCentral();
		if(central) return central->startSniffing(clientInfo);

		return BaseLib::Variable::createError(-32500, "No central found.");
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

BaseLib::PVariable RPCStopSniffing::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("stopSniffing")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::map<int32_t, std::shared_ptr<BaseLib::Systems::DeviceFamily>> families = GD::familyController->getFamilies();
		auto family = families.find(parameters->at(0)->integerValue);
		if(family == families.end()) return BaseLib::Variable::createError(-1, "Unknown device family.");

		std::shared_ptr<BaseLib::Systems::ICentral> central = family->second->getCentral();
		if(central) return central->stopSniffing(clientInfo);

		return BaseLib::Variable::createError(-32500, "No central found.");
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

BaseLib::PVariable RPCSubscribePeers::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("subscribePeers")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray})
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
			if((*i)->integerValue64 != 0) eventServer->subscribedPeers.insert((*i)->integerValue64);
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

BaseLib::PVariable RPCTriggerEvent::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
#ifdef EVENTHANDLER
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("triggerEvent")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
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

BaseLib::PVariable RPCTriggerRpcEvent::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("triggerRpcEvent")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray}));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(parameters->at(0)->stringValue == "updateDevice")
		{
			if(parameters->at(1)->arrayValue->size() != 4) return BaseLib::Variable::createError(-1, "Wrong parameter count. You need to pass (in this order): Integer peerID, Integer channel, String address, Integer flags");
			GD::rpcClient->broadcastUpdateDevice((uint64_t) parameters->at(1)->arrayValue->at(0)->integerValue64, parameters->at(1)->arrayValue->at(1)->integerValue, parameters->at(1)->arrayValue->at(2)->stringValue, (Rpc::Client::Hint::Enum) parameters->at(1)->arrayValue->at(3)->integerValue);
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

BaseLib::PVariable RPCUnsubscribePeers::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("unsubscribePeers")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tArray})
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

BaseLib::PVariable RPCUpdateCategory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndCategoryWriteAccess("updateCategory", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->size() == 3) return GD::bl->db->updateCategory((uint64_t) parameters->at(0)->integerValue64, parameters->at(1), parameters->at(2));
		else return GD::bl->db->updateCategory((uint64_t) parameters->at(0)->integerValue64, parameters->at(1), std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
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

BaseLib::PVariable RPCUpdateFirmware::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("updateFirmware")) return BaseLib::Variable::createError(-32603, "Unauthorized.");
		bool checkAcls = clientInfo->acls->roomsCategoriesDevicesWriteSet();

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tArray})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		bool manual = false;
		bool array = true;
		if(parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tInteger64 || parameters->at(0)->type == BaseLib::VariableType::tString)
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
					if(((*i)->type == BaseLib::VariableType::tInteger || (*i)->type == BaseLib::VariableType::tInteger64) && (*i)->integerValue != 0 && central->peerExists((uint64_t) (*i)->integerValue64))
					{
						if(checkAcls)
						{
							auto peer = central->getPeer((uint64_t) (*i)->integerValue64);
							if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
						}

						ids.push_back((uint64_t) (*i)->integerValue64);
					}
					else if((*i)->type == BaseLib::VariableType::tString && central->peerExists((*i)->stringValue))
					{
						if(checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

						ids.push_back(central->getPeerId(clientInfo, (*i)->stringValue)->integerValue);
					}
				}
				if(ids.size() > 0 && ids.size() != parameters->at(0)->arrayValue->size()) return BaseLib::Variable::createError(-2, "Please provide only devices of one device family.");
				if(ids.size() > 0) return central->updateFirmware(clientInfo, ids, manual);
			}
			else if(((parameters->at(0)->type == BaseLib::VariableType::tInteger || parameters->at(0)->type == BaseLib::VariableType::tInteger64) && central->peerExists((uint64_t) parameters->at(0)->integerValue64)) ||
					(parameters->at(0)->type == BaseLib::VariableType::tString && central->peerExists(parameters->at(0)->stringValue)))
			{
				std::vector<uint64_t> ids;
				if(parameters->at(0)->type == BaseLib::VariableType::tString)
				{
					if(checkAcls) return BaseLib::Variable::createError(-32603, "Unauthorized. Device, category or room ACLs are set for this client. Usage of serial numbers to address devices is not allowed.");

					ids.push_back(central->getPeerId(clientInfo, parameters->at(0)->stringValue)->integerValue);
				}
				else
				{
					if(checkAcls)
					{
						auto peer = central->getPeer((uint64_t) parameters->at(0)->integerValue64);
						if(!peer || !clientInfo->acls->checkDeviceWriteAccess(peer)) return BaseLib::Variable::createError(-32603, "Unauthorized.");
					}

					ids.push_back((uint64_t) parameters->at(0)->integerValue64);
				}
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

BaseLib::PVariable RPCUpdateRoom::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAndRoomWriteAccess("updateRoom", (uint64_t) parameters->at(0)->integerValue64)) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->size() == 3) return GD::bl->db->updateRoom((uint64_t) parameters->at(0)->integerValue64, parameters->at(1), parameters->at(2));
		else return GD::bl->db->updateRoom((uint64_t) parameters->at(0)->integerValue64, parameters->at(1), std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
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

BaseLib::PVariable RPCUpdateStory::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct, BaseLib::VariableType::tStruct})
																												 }));
		if(error != ParameterError::Enum::noError) return getError(error);

		if(!clientInfo || !clientInfo->acls->checkMethodAccess("updateStory")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		if(parameters->size() == 3) return GD::bl->db->updateStory((uint64_t) parameters->at(0)->integerValue64, parameters->at(1), parameters->at(2));
		else return GD::bl->db->updateStory((uint64_t) parameters->at(0)->integerValue64, parameters->at(1), std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct));
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

BaseLib::PVariable RPCWriteLog::invoke(BaseLib::PRpcClientInfo clientInfo, BaseLib::PArray parameters)
{
	try
	{
		if(!clientInfo || !clientInfo->acls->checkMethodAccess("writeLog")) return BaseLib::Variable::createError(-32603, "Unauthorized.");

		ParameterError::Enum error = checkParameters(parameters, std::vector<std::vector<BaseLib::VariableType>>({
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}),
																														 std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tInteger})
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

}

}