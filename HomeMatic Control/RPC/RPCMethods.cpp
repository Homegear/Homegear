#include "RPCMethods.h"
#include "Devices.h"
#include "../GD.h"

namespace RPC
{

std::shared_ptr<RPCVariable> RPCGetParamsetDescription::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
	if(error != ParameterError::Enum::noError) return getError(error);
	uint32_t pos = parameters->at(0)->stringValue.find(':');
	if(pos != 10 || parameters->at(0)->stringValue.size() < 12)
	{
		if(GD::debugLevel >= 3) cout << "Warning: Wrong serial number format in getParamsetDescription." << endl;
		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
	}

	string serialNumber = parameters->at(0)->stringValue.substr(0, 10);
	uint32_t channel = std::stol(parameters->at(0)->stringValue.substr(11));
	ParameterSet::Type::Enum type;
	if(parameters->at(1)->stringValue == "MASTER") type = ParameterSet::Type::Enum::master;
	else if(parameters->at(1)->stringValue == "VALUES") type = ParameterSet::Type::Enum::values;
	else if(parameters->at(1)->stringValue == "LINK") type = ParameterSet::Type::Enum::link;

	std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
	if(!central)
	{
		if(GD::debugLevel >= 2) cout << "Error: Could not execute RPC method list devices. Please add a central device." << endl;
		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcArray));
	}
	return GD::devices.getCentral()->getParamsetDescription(serialNumber, channel, type);
}

std::shared_ptr<RPCVariable> RPCGetValue::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
	if(error != ParameterError::Enum::noError) return getError(error);
	uint32_t pos = parameters->at(0)->stringValue.find(':');
	if(pos != 10 || parameters->at(0)->stringValue.size() < 12)
	{
		if(GD::debugLevel >= 3) cout << "Warning: Wrong serial number format in getParamsetDescription." << endl;
		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
	}

	string serialNumber = parameters->at(0)->stringValue.substr(0, 10);
	uint32_t channel = std::stol(parameters->at(0)->stringValue.substr(11));

	std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
	if(!central)
	{
		if(GD::debugLevel >= 2) cout << "Error: Could not execute RPC method list devices. Please add a central device." << endl;
		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcArray));
	}
	return GD::devices.getCentral()->getValue(serialNumber, channel, parameters->at(1)->stringValue);
}

std::shared_ptr<RPCVariable> RPCInit::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString, RPCVariableType::rpcString }));
	if(error == ParameterError::Enum::noError)
	{
		if(parameters->at(2)->stringValue.size() == 0)
		{
			//Remove client
		}
		else
		{
			//Add client
		}
	}
	else if(error == ParameterError::Enum::wrongCount)
	{
		error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		//Remove client
	}
	else return getError(error);
	return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcVoid));
}

std::shared_ptr<RPCVariable> RPCListDevices::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	std::string interfaceID;
	if(parameters->size() > 0)
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcString }));
		if(error != ParameterError::Enum::noError) return getError(error);
		interfaceID = parameters->at(0)->stringValue;
	}
	std::shared_ptr<HomeMaticCentral> central = GD::devices.getCentral();
	if(!central)
	{
		if(GD::debugLevel >= 2) cout << "Error: Could not execute RPC method list devices. Please add a central device." << endl;
		return std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariableType::rpcArray));
	}
	return GD::devices.getCentral()->listDevices();
}

std::shared_ptr<RPCVariable> RPCLogLevel::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	if(parameters->size() > 0)
	{
		ParameterError::Enum error = checkParameters(parameters, std::vector<RPCVariableType>({ RPCVariableType::rpcInteger }));
		if(error != ParameterError::Enum::noError) return getError(error);
		GD::rpcLogLevel = parameters->at(0)->integerValue;
	}
	return std::shared_ptr<RPCVariable>(new RPCVariable(GD::rpcLogLevel));
}

} /* namespace RPC */
