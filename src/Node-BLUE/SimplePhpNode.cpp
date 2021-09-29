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

#include "SimplePhpNode.h"

#ifndef NO_SCRIPTENGINE

#include "../GD/GD.h"

namespace Homegear {

SimplePhpNode::SimplePhpNode(const std::string &path, const std::string &type, const std::atomic_bool *frontendConnected)
    : INode(path, type, frontendConnected) {
}

SimplePhpNode::~SimplePhpNode() = default;

void SimplePhpNode::input(const Flows::PNodeInfo &nodeInfo, uint32_t index, const Flows::PVariable &message) {
  try {
    if (!_nodeInfo) _nodeInfo = nodeInfo->serialize();

    Flows::PArray parameters = std::make_shared<Flows::Array>();
    parameters->reserve(4);
    parameters->push_back(_nodeInfo);
    parameters->push_back(std::make_shared<Flows::Variable>(_path));
    parameters->push_back(std::make_shared<Flows::Variable>(index));
    parameters->push_back(message);

    Flows::PVariable result = invoke("executePhpNode", parameters);
    if (result->errorStruct) _out->printError("Error calling executePhpNode: " + result->structValue->at("faultString")->stringValue);
  }
  catch (const std::exception &ex) {
    _out->printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

}

#endif
