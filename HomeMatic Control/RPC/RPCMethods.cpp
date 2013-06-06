#include "RPCMethods.h"
#include "Devices.h"
#include "../GD.h"

namespace RPC
{

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
