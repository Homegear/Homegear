#include "RPCMethods.h"
#include "Devices.h"
#include "../GD.h"
#include "../HelperFunctions.h"

namespace RPC
{

std::shared_ptr<RPCVariable> RPCSystemGetCapabilities::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(!parameters->empty()) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<RPCVariable> capabilities(new RPCVariable(RPCVariableType::rpcStruct));

		std::shared_ptr<RPCVariable> element(new RPCVariable(RPCVariableType::rpcStruct));
		element->name = "xmlrpc";
		element->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("specUrl", "http://www.xmlrpc.com/spec")));
		element->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("specVersion", 1)));
		capabilities->structValue->push_back(element);

		element.reset(new RPCVariable(RPCVariableType::rpcStruct));
		element->name = "faults_interop";
		element->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("specUrl", "http://xmlrpc-epi.sourceforge.net/specs/rfc.fault_codes.php")));
		element->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("specVersion", 20010516)));
		capabilities->structValue->push_back(element);

		element.reset(new RPCVariable(RPCVariableType::rpcStruct));
		element->name = "introspection";
		element->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("specUrl", "http://scripts.incutio.com/xmlrpc/introspection.html")));
		element->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("specVersion", 1)));
		capabilities->structValue->push_back(element);

		return capabilities;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
			if((*i)->structValue->at(0)->name != "methodName" || (*i)->structValue->at(0)->type != RPCVariableType::rpcString)
			{
				returns->arrayValue->push_back(RPCVariable::createError(-32602, "No method name provided."));
				continue;
			}
			if((*i)->structValue->at(1)->name != "params" || (*i)->structValue->at(1)->type != RPCVariableType::rpcArray)
			{
				returns->arrayValue->push_back(RPCVariable::createError(-32602, "No parameters provided."));
				continue;
			}
			std::string methodName = (*i)->structValue->at(0)->stringValue;
			std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters = (*i)->structValue->at(1)->arrayValue;

			if(methodName == "system.multicall") returns->arrayValue->push_back(RPCVariable::createError(-32602, "Recursive calls to system.multicall are not allowed."));
			else if(methods->find(methodName) == methods->end()) returns->arrayValue->push_back(RPCVariable::createError(-32601, "Requested method not found."));
			else returns->arrayValue->push_back(methods->at(methodName)->invoke(parameters));
		}

		return returns;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCDeleteDevice::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcInteger }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getDeviceDescription. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getDeviceDescription. Please add a central device.");
		}
		return GD::devices.getCentral()->deleteDevice(parameters->at(0)->stringValue, parameters->at(1)->integerValue);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCDeleteMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		ParameterError::Enum error2 = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError && error2 != ParameterError::Enum::noError) return getError((error != ParameterError::Enum::noError) ? error : error2);
		if(parameters->at(0)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(parameters->size() > 1 && parameters->at(1)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(0)->stringValue)));
		std::string command("DELETE FROM metadata WHERE objectID=?");
		if(parameters->size() > 1)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(1)->stringValue)));
			command.append(" AND dataID=?");
		}
		GD::db.executeCommand(command, data);

		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}


std::shared_ptr<RPCVariable> RPCGetAllMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		if(parameters->at(0)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(0)->stringValue)));

		DataTable rows = GD::db.executeCommand("SELECT dataID, serializedObject FROM metadata WHERE objectID=?", data);
		if(rows.empty()) return RPCVariable::createError(-1, "No metadata found.");

		std::shared_ptr<RPCVariable> metadataStruct(new RPCVariable(RPCVariableType::rpcStruct));
		for(DataTable::iterator i = rows.begin(); i != rows.end(); ++i)
		{
			if(i->second.size() < 2) continue;
			std::shared_ptr<RPCVariable> metadata = _rpcDecoder.decodeResponse(i->second.at(1)->binaryValue, 8);
			metadata->name = i->second.at(0)->textValue;
			metadataStruct->structValue->push_back(metadata);
		}

		return metadataStruct;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetDeviceDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		int32_t channel = -1;
		std::string serialNumber;
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getDeviceDescription. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getDeviceDescription. Please add a central device.");
		}
		return GD::devices.getCentral()->getDeviceDescription(serialNumber, channel);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetInstallMode::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getInstallMode. Please add a central device." << std::endl;
			return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcArray));
		}
		return central->getInstallMode();
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetLinks::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcInteger }));
		ParameterError::Enum error2 = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		ParameterError::Enum error3 = checkParameters(parameters, std::vector<RPCVariableType>());
		if(error != ParameterError::Enum::noError && error2 != ParameterError::Enum::noError && error3 != ParameterError::Enum::noError) return getError((error != ParameterError::Enum::noError) ? error : ((error2 != ParameterError::Enum::noError) ? error2 : error3));

		int32_t channel = -1; //If no channel is defined take the base device's channel
		std::string serialNumber;
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		int32_t flags = 0;
		if(parameters->size() == 3) flags = parameters->at(0)->integerValue;

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getLinks. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getLinks. Please add a central device.");
		}

		return central->getLinks(serialNumber, channel, flags);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		if(parameters->at(0)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(parameters->at(1)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(0)->stringValue)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(1)->stringValue)));

		DataTable rows = GD::db.executeCommand("SELECT serializedObject FROM metadata WHERE objectID=? AND dataID=?", data);
		if(rows.empty() || rows.at(0).empty()) return RPCVariable::createError(-1, "No metadata found.");

		std::shared_ptr<RPCVariable> metadata = _rpcDecoder.decodeResponse(rows.at(0).at(0)->binaryValue, 8);
		return metadata;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetParamsetDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		uint32_t channel = 0; //If no channel is defined take the base device's channel
		std::string serialNumber;
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getParamsetDescription. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getParamsetDescription. Please add a central device.");
		}
		return central->getParamsetDescription(serialNumber, channel, ParameterSet::typeFromString(parameters->at(1)->stringValue));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getParamsetId. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getParamsetId. Please add a central device.");
		}

		return central->getParamsetId(serialNumber, channel, ParameterSet::typeFromString(parameters->at(1)->stringValue));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetParamset::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t channel = 0;
		std::string serialNumber;
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getParamsetId. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getParamsetId. Please add a central device.");
		}

		return central->getParamset(serialNumber, channel, ParameterSet::typeFromString(parameters->at(1)->stringValue));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCGetServiceMessages::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getValue. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getValue. Please add a central device.");
		}
		return central->getServiceMessages();
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCGetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos != 10 || parameters->at(0)->stringValue.size() < 12)
		{
			if(GD::debugLevel >= 3) std::cout << "Warning: Wrong serial number format in getValue." << std::endl;
			return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
		}

		std::string serialNumber = parameters->at(0)->stringValue.substr(0, 10);
		uint32_t channel = std::stol(parameters->at(0)->stringValue.substr(11));

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getValue. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getValue. Please add a central device.");
		}
		return central->getValue(serialNumber, channel, parameters->at(1)->stringValue);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCInit::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::pair<std::string, std::string> server = HelperFunctions::split(parameters->at(0)->stringValue, ':');
		if(server.first.empty() || server.second.empty()) return RPCVariable::createError(-32602, "Server address or port is empty.");
		if(server.first.size() < 5) return RPCVariable::createError(-32602, "Server address too short.");
		if(server.first.substr(0, 5) == "https") return RPCVariable::createError(-32602, "https is currently not supported.");
		server.second = std::to_string(HelperFunctions::getNumber(server.second));
		if(server.second == "0" || server.second.empty()) return RPCVariable::createError(-32602, "Port number is invalid.");

		if(!GD::devices.getCentral())
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method init. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method init. Please add a central device.");
		}

		if(parameters->at(1)->stringValue.empty())
		{
			GD::rpcClient.removeServer(server);
		}
		else
		{
			GD::rpcClient.addServer(server, parameters->at(1)->stringValue);
			GD::rpcClient.listDevices(server);
			GD::rpcClient.sendUnknownDevices(server);
		}

		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListBidcosInterfaces::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0) return getError(ParameterError::Enum::wrongCount);

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central) return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcArray));
		std::shared_ptr<RPCVariable> array(new RPCVariable(RPCVariableType::rpcArray));
		std::shared_ptr<RPCVariable> interface(new RPCVariable(RPCVariableType::rpcStruct));
		array->arrayValue->push_back(interface);
		interface->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("ADDRESS", central->serialNumber())));
		interface->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("DESCRIPTION", std::string("Homegear default interface"))));
		interface->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("CONNECTED", true)));
		interface->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("DEFAULT", true)));
		return array;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCListDevices::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		if(parameters->size() > 0)
		{
			ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcBoolean }));
			if(error != ParameterError::Enum::noError) return getError(error);
		}
		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method listDevices. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method listDevices. Please add a central device.");
		}
		return central->listDevices();
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
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
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCPutParamset::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcStruct }));
		if(error != ParameterError::Enum::noError) return getError(error);

		uint32_t channel = 0;
		std::string serialNumber;
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos > -1)
		{
			serialNumber = parameters->at(0)->stringValue.substr(0, pos);
			if(parameters->at(0)->stringValue.size() > (unsigned)pos + 1) channel = std::stoll(parameters->at(0)->stringValue.substr(pos + 1));
		}
		else serialNumber = parameters->at(0)->stringValue;

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method getParamsetId. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method getParamsetId. Please add a central device.");
		}

		return central->putParamset(serialNumber, channel, ParameterSet::typeFromString(parameters->at(1)->stringValue), parameters->at(2));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

std::shared_ptr<RPCVariable> RPCSetInstallMode::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcBoolean }));
		if(error != ParameterError::Enum::noError) return getError(error);

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method setInstallMode. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method setInstallMode. Please add a central device.");
		}
		return central->setInstallMode(parameters->at(0)->booleanValue);
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetMetadata::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcVariant }));
		if(error != ParameterError::Enum::noError) return getError(error);
		if(parameters->at(0)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(parameters->at(1)->stringValue.size() > 250) return RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(parameters->at(2)->stringValue.size() > 1000) return RPC::RPCVariable::createError(-32602, "Data has more than 1000 characters.");
		if(parameters->at(2)->type != RPC::RPCVariableType::rpcBase64 && parameters->at(2)->type != RPC::RPCVariableType::rpcString && parameters->at(2)->type != RPC::RPCVariableType::rpcInteger && parameters->at(2)->type != RPC::RPCVariableType::rpcFloat && parameters->at(2)->type != RPC::RPCVariableType::rpcBoolean) return RPC::RPCVariable::createError(-32602, "Type " + RPC::RPCVariable::getTypeString(parameters->at(2)->type) + " is currently not supported.");

		DataTable rows = GD::db.executeCommand("SELECT COUNT(*) FROM metadata");
		if(rows.size() == 0 || rows.at(0).size() == 0) return RPCVariable::createError(-32500, "Error counting metadata in database.");
		std::cout << "Metadata rows: " << rows.at(0).at(0)->intValue << std::endl;
		if(rows.at(0).at(0)->intValue > 1000000) return RPCVariable::createError(-32500, "Reached limit of 1000000 metadata entries. Please delete metadata before adding new entries.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(0)->stringValue)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameters->at(1)->stringValue)));
		GD::db.executeCommand("DELETE FROM metadata WHERE objectID=? AND dataID=?", data);

		std::shared_ptr<std::vector<char>> value = _rpcEncoder.encodeResponse(parameters->at(2));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));

		GD::db.executeCommand("INSERT INTO metadata VALUES(?, ?, ?)", data);

		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPCVariable> RPCSetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	try
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcVariant }));
		if(error != ParameterError::Enum::noError) return getError(error);
		int32_t pos = parameters->at(0)->stringValue.find(':');
		if(pos != 10 || parameters->at(0)->stringValue.size() < 12)
		{
			if(GD::debugLevel >= 3) std::cout << "Warning: Wrong serial number format in setValue." << std::endl;
			return RPC::RPCVariable::createError(-2, "Wrong serial number format.");
		}

		std::string serialNumber = parameters->at(0)->stringValue.substr(0, 10);
		uint32_t channel = std::stol(parameters->at(0)->stringValue.substr(11));

		std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
		if(!central)
		{
			if(GD::debugLevel >= 2) std::cout << "Error: Could not execute RPC method setValue. Please add a central device." << std::endl;
			return RPCVariable::createError(-32500, ": Could not execute RPC method setValue. Please add a central device.");
		}
		return central->setValue(serialNumber, channel, parameters->at(1)->stringValue, parameters->at(2));
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error. Check the address format.");
}

} /* namespace RPC */
