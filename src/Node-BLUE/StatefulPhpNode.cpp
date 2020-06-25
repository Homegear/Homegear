/* Copyright 2013-2020 Homegear GmbH
 *
 * libhomegear-base is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * libhomegear-base is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libhomegear-base.  If not, see
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

#include "StatefulPhpNode.h"

#ifndef NO_SCRIPTENGINE

#include "../GD/GD.h"

namespace Homegear
{

StatefulPhpNode::StatefulPhpNode(std::string path, std::string nodeNamespace, std::string type, const std::atomic_bool* frontendConnected)
		: INode(path, nodeNamespace, type, frontendConnected)
{
}

StatefulPhpNode::~StatefulPhpNode()
{
}

bool StatefulPhpNode::init(Flows::PNodeInfo nodeInfo)
{
	_nodeInfo = nodeInfo->serialize();

	Flows::PArray parameters = std::make_shared<Flows::Array>();
	parameters->reserve(3);
	parameters->push_back(_nodeInfo->structValue->at("type"));
	parameters->push_back(_nodeInfo);
	parameters->push_back(std::make_shared<Flows::Variable>(_path));

	Flows::PVariable result = invoke("executePhpNode", parameters);
	if(result->errorStruct)
	{
		GD::out.printError("Error calling executePhpNode: " + result->structValue->at("faultString")->stringValue);
		return false;
	}

	parameters->clear();
	parameters->reserve(3);
	parameters->push_back(std::make_shared<Flows::Variable>(_id));
	parameters->push_back(std::make_shared<Flows::Variable>("init"));
	Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
	innerParameters->arrayValue->push_back(_nodeInfo);
	parameters->push_back(innerParameters);

	result = invoke("executePhpNodeMethod", parameters);
	if(result->errorStruct)
	{
		GD::out.printError("Error calling init: " + result->structValue->at("faultString")->stringValue);
		return false;
	}

	return result->booleanValue;
}

bool StatefulPhpNode::start()
{
	Flows::PArray parameters = std::make_shared<Flows::Array>();
	parameters->reserve(3);
	parameters->push_back(std::make_shared<Flows::Variable>(_id));
	parameters->push_back(std::make_shared<Flows::Variable>("start"));
	Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
	parameters->push_back(innerParameters);

	Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
	if(result->errorStruct)
	{
		GD::out.printError("Error calling start: " + result->structValue->at("faultString")->stringValue);
		return false;
	}

	return result->booleanValue;
}

void StatefulPhpNode::stop()
{
	Flows::PArray parameters = std::make_shared<Flows::Array>();
	parameters->reserve(3);
	parameters->push_back(std::make_shared<Flows::Variable>(_id));
	parameters->push_back(std::make_shared<Flows::Variable>("stop"));
	Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
	parameters->push_back(innerParameters);

	Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
	if(result->errorStruct) GD::out.printError("Error calling stop: " + result->structValue->at("faultString")->stringValue);
}

void StatefulPhpNode::waitForStop()
{
	Flows::PArray parameters = std::make_shared<Flows::Array>();
	parameters->reserve(3);
	parameters->push_back(std::make_shared<Flows::Variable>(_id));
	parameters->push_back(std::make_shared<Flows::Variable>("waitForStop"));
	Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
	parameters->push_back(innerParameters);

	Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
	if(result->errorStruct) GD::out.printError("Error calling waitForStop: " + result->structValue->at("faultString")->stringValue);
}

void StatefulPhpNode::configNodesStarted()
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>("configNodesStarted"));
		Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
		parameters->push_back(innerParameters);

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling configNodesStarted: " + result->structValue->at("faultString")->stringValue);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void StatefulPhpNode::startUpComplete()
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>("startUpComplete"));
		Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
		parameters->push_back(innerParameters);

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling startUpComplete: " + result->structValue->at("faultString")->stringValue);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void StatefulPhpNode::variableEvent(std::string source, uint64_t peerId, int32_t channel, std::string variable, Flows::PVariable value)
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>("variableEvent"));
		Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
		innerParameters->arrayValue->reserve(4);
		innerParameters->arrayValue->push_back(std::make_shared<Flows::Variable>(peerId));
		innerParameters->arrayValue->push_back(std::make_shared<Flows::Variable>(channel));
		innerParameters->arrayValue->push_back(std::make_shared<Flows::Variable>(variable));
		innerParameters->arrayValue->push_back(value);
		parameters->push_back(innerParameters);

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling variableEvent: " + result->structValue->at("faultString")->stringValue);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

void StatefulPhpNode::setNodeVariable(const std::string& variable, Flows::PVariable value)
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>("setNodeVariable"));
		Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
		innerParameters->arrayValue->reserve(2);
		innerParameters->arrayValue->push_back(std::make_shared<Flows::Variable>(variable));
		innerParameters->arrayValue->push_back(value);
		parameters->push_back(innerParameters);

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling setNodeVariable: " + result->structValue->at("faultString")->stringValue);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

Flows::PVariable StatefulPhpNode::getConfigParameterIncoming(std::string name)
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>("getConfigParameterIncoming"));
		Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
		innerParameters->arrayValue->push_back(std::make_shared<Flows::Variable>(name));
		parameters->push_back(innerParameters);

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling getConfigParameterIncoming: " + result->structValue->at("faultString")->stringValue);
		return result;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return std::make_shared<Flows::Variable>();
}

void StatefulPhpNode::input(Flows::PNodeInfo nodeInfo, uint32_t index, Flows::PVariable message)
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>("input"));
		Flows::PVariable innerParameters = std::make_shared<Flows::Variable>(Flows::VariableType::tArray);
		innerParameters->arrayValue->reserve(3);
		innerParameters->arrayValue->push_back(_nodeInfo);
		innerParameters->arrayValue->push_back(std::make_shared<Flows::Variable>(index));
		innerParameters->arrayValue->push_back(message);
		parameters->push_back(innerParameters);

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling input: " + result->structValue->at("faultString")->stringValue);
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

Flows::PVariable StatefulPhpNode::invokeLocal(const std::string& methodName, Flows::PArray innerParameters)
{
	try
	{
		Flows::PArray parameters = std::make_shared<Flows::Array>();
		parameters->reserve(3);
		parameters->push_back(std::make_shared<Flows::Variable>(_id));
		parameters->push_back(std::make_shared<Flows::Variable>(methodName));
		parameters->push_back(std::make_shared<Flows::Variable>(innerParameters));

		Flows::PVariable result = invoke("executePhpNodeMethod", parameters);
		if(result->errorStruct) GD::out.printError("Error calling " + methodName + ": " + result->structValue->at("faultString")->stringValue);
		return result;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return Flows::Variable::createError(-32601, "Requested method not found.");
}

}

#endif
