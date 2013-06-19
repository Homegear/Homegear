#include "RPCMethod.h"

namespace RPC
{
std::shared_ptr<RPCVariable> RPCMethod::invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters)
{
	return std::shared_ptr<RPCVariable>(new RPCVariable());
}

RPCMethod::ParameterError::Enum RPCMethod::checkParameters(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters, std::vector<RPCVariableType> types)
{
	if(types.size() != parameters->size())
	{
		return RPCMethod::ParameterError::Enum::wrongCount;
	}
	for(uint32_t i = 0; i < types.size(); i++)
	{
		if(types.at(i) == RPCVariableType::rpcVariant && parameters->at(i)->type != RPCVariableType::rpcVoid) continue;
		if(types.at(i) != parameters->at(i)->type) return RPCMethod::ParameterError::Enum::wrongType;
	}
	return RPCMethod::ParameterError::Enum::noError;
}

std::shared_ptr<RPCVariable> RPCMethod::getError(RPCMethod::ParameterError::Enum error)
{
	if(error == ParameterError::Enum::wrongCount) return RPCVariable::createError(-1, "wrong parameter count");
	else if(error == ParameterError::Enum::wrongType) return RPCVariable::createError(-1, "type error");
	return RPCVariable::createError(-1, "unknown error");
}

void RPCMethod::setHelp(std::string help)
{
	_help.reset(new RPCVariable(help));
}

void RPCMethod::addSignature(RPCVariableType returnType, std::vector<RPCVariableType> parameterTypes)
{
	if(!_signatures) _signatures.reset(new RPCVariable(RPCVariableType::rpcArray));

	std::shared_ptr<RPCVariable> element(new RPCVariable(RPCVariableType::rpcArray));

	element->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariable::getTypeString(returnType))));

	for(std::vector<RPCVariableType>::iterator i = parameterTypes.begin(); i != parameterTypes.end(); ++i)
	{
		element->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(RPCVariable::getTypeString(*i))));
	}
	_signatures->arrayValue->push_back(element);
}

} /* namespace RPC */
