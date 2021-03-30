/* Copyright 2013-2020 Homegear GmbH
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

#include "FlowParser.h"

namespace Homegear {
namespace NodeBlue {

const size_t FlowParser::nodeHeight = 30;
const size_t FlowParser::nodeConnectorHeight = 20;

std::string FlowParser::generateRandomId(const std::unordered_set<std::string> &existingIds) {
  std::string randomString;
  do {
    std::string randomString1 = BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(-2147483648, 2147483647), 8);
    BaseLib::HelperFunctions::toLower(randomString1);
    std::string randomString2 = BaseLib::HelperFunctions::getHexString(BaseLib::HelperFunctions::getRandomNumber(0, 1048575), 5);
    BaseLib::HelperFunctions::toLower(randomString2);
    randomString = randomString1 + "." + randomString2;
  } while (existingIds.find(randomString) != existingIds.end());

  return randomString;
}

BaseLib::PVariable FlowParser::addNodesToFlow(const BaseLib::PVariable &flow, const std::string &tab, const std::string &tag, const BaseLib::PVariable &nodes, std::unordered_map<std::string, std::string> &nodeIdMap) {
  if (!flow || flow->arrayValue->empty() || tab.empty() || !nodes || nodes->arrayValue->empty()) return BaseLib::PVariable();

  //A set to store all existing IDs to avoid duplicates.
  std::unordered_set<std::string> allIds;

  std::string tabId;
  int32_t yOffset = 0;
  int32_t yOffsetNewNodes = -1;
  nodeIdMap.clear();
  BaseLib::PVariable returnedFlow = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  returnedFlow->arrayValue->reserve(flow->arrayValue->size() + nodes->arrayValue->size());

  //{{{ Find tab
  for (auto &flowElement : *flow->arrayValue) {
    auto typeIterator = flowElement->structValue->find("type");
    if (typeIterator != flowElement->structValue->end() && typeIterator->second->stringValue == "tab") {
      auto labelIterator = flowElement->structValue->find("label");
      if (labelIterator != flowElement->structValue->end() && labelIterator->second->stringValue == tab) {
        auto idIterator = flowElement->structValue->find("id");
        if (idIterator == flowElement->structValue->end()) continue;

        tabId = idIterator->second->stringValue;
        continue;
      }
    }
  }
  //}}}

  //{{{ Remove all nodes with tag in tab
  if (!tag.empty()) {
    for (auto &flowElement : *flow->arrayValue) {
      auto zIterator = flowElement->structValue->find("z");
      if (zIterator != flowElement->structValue->end() && !tabId.empty() && zIterator->second->stringValue == tabId) {
        auto tagIterator = flowElement->structValue->find("homegearTag");
        if (tagIterator != flowElement->structValue->end() && tagIterator->second->stringValue == tag) {
          continue;
        }
      }

      auto idIterator = flowElement->structValue->find("id");
      if (idIterator == flowElement->structValue->end()) continue;

      allIds.emplace(idIterator->second->stringValue);

      returnedFlow->arrayValue->emplace_back(flowElement);
    }
  } else {
    *returnedFlow = *flow;
  }
  //}}}

  //{{{ Find lowest node in flow
  if (!tabId.empty()) {
    for (auto &flowElement : *returnedFlow->arrayValue) {
      auto zIterator = flowElement->structValue->find("z");
      if (zIterator == flowElement->structValue->end() || zIterator->second->stringValue != tabId) continue; //No node or node is not in our tab
      auto yIterator = flowElement->structValue->find("y");
      if (yIterator == flowElement->structValue->end()) continue;

      int32_t inputs = 0;
      int32_t outputs = 0;

      auto inputsIterator = flowElement->structValue->find("inputs");
      if (inputsIterator != flowElement->structValue->end()) inputs = inputsIterator->second->integerValue;
      auto outputsIterator = flowElement->structValue->find("outputs");
      if (outputsIterator != flowElement->structValue->end()) outputs = outputsIterator->second->integerValue;

      int32_t currentOffset = yIterator->second->integerValue + std::max(nodeHeight, std::max(inputs * nodeConnectorHeight, outputs * nodeConnectorHeight));
      if (currentOffset > yOffset) yOffset = currentOffset;
    }
  }
  //}}}

  //{{{ Find highest node in new nodes and create replacement map
  for (auto &flowElement : *nodes->arrayValue) {
    auto idIterator = flowElement->structValue->find("id");
    if (idIterator == flowElement->structValue->end()) continue;

    auto newId = generateRandomId(allIds);
    allIds.emplace(newId);
    nodeIdMap.emplace(idIterator->second->stringValue, newId);

    auto yIterator = flowElement->structValue->find("y");
    if (yIterator == flowElement->structValue->end()) continue;

    if (yOffsetNewNodes == -1 || yIterator->second->integerValue < yOffsetNewNodes) yOffsetNewNodes = yIterator->second->integerValue;
  }
  //}}}

  yOffsetNewNodes -= nodeHeight;

  //{{{ Create tab, if it doesn't exist.
  if (tabId.empty()) {
    tabId = generateRandomId(allIds);
    allIds.emplace(tabId);

    auto tabStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    tabStruct->structValue->emplace("id", std::make_shared<BaseLib::Variable>(tabId));
    tabStruct->structValue->emplace("label", std::make_shared<BaseLib::Variable>(tab));
    tabStruct->structValue->emplace("namespace", std::make_shared<BaseLib::Variable>("tab"));
    tabStruct->structValue->emplace("type", std::make_shared<BaseLib::Variable>("tab"));

    returnedFlow->arrayValue->emplace_back(tabStruct);
  }
  //}}}

  //{{{ Add nodes to tab
  for (auto &flowElement : *nodes->arrayValue) {
    auto idIterator = flowElement->structValue->find("id");
    if (idIterator == flowElement->structValue->end()) continue;

    auto newIdIterator = nodeIdMap.find(idIterator->second->stringValue);
    if (newIdIterator == nodeIdMap.end()) continue;
    auto &newId = newIdIterator->second;

    BaseLib::PVariable newNode = std::make_shared<BaseLib::Variable>();
    *newNode = *flowElement;

    int32_t xPos = 0;
    int32_t yPos = 0;
    bool global = false;

    auto xIterator = flowElement->structValue->find("x");
    auto yIterator = flowElement->structValue->find("y");
    auto zIterator = flowElement->structValue->find("z");

    if (xIterator != flowElement->structValue->end()) {
      xPos = xIterator->second->integerValue;
    }

    if (yIterator != flowElement->structValue->end()) {
      yPos = yOffset + (yIterator->second->integerValue - yOffsetNewNodes);
    }

    if (zIterator != flowElement->structValue->end() && zIterator->second->stringValue == "g") {
      global = true;
    }

    newNode->structValue->erase("id");
    newNode->structValue->erase("homegearTag");
    newNode->structValue->erase("x");
    newNode->structValue->erase("y");
    newNode->structValue->erase("z");

    for (auto &parameter : *newNode->structValue) {
      if (parameter.second->type == BaseLib::VariableType::tString) {
        auto parameterIdIterator = nodeIdMap.find(parameter.second->stringValue);
        if (parameterIdIterator != nodeIdMap.end()) parameter.second->stringValue = parameterIdIterator->second;
      }
    }

    newNode->structValue->emplace("id", std::make_shared<BaseLib::Variable>(newId));
    newNode->structValue->emplace("homegearTag", std::make_shared<BaseLib::Variable>(tag));
    newNode->structValue->emplace("x", std::make_shared<BaseLib::Variable>(xPos));
    newNode->structValue->emplace("y", std::make_shared<BaseLib::Variable>(yPos));
    if (!global) newNode->structValue->emplace("z", std::make_shared<BaseLib::Variable>(tabId));

    auto wiresIterator = newNode->structValue->find("wires");
    if (wiresIterator != newNode->structValue->end()) {
      for (auto &outputWireGroup : *wiresIterator->second->arrayValue) {
        for (auto &outputWire : *outputWireGroup->arrayValue) {
          auto outputIdIterator = outputWire->structValue->find("id");
          if (outputIdIterator == outputWire->structValue->end()) continue;

          auto outputNewIdIterator = nodeIdMap.find(outputIdIterator->second->stringValue);
          if (outputNewIdIterator == nodeIdMap.end()) continue;
          outputIdIterator->second->stringValue = outputNewIdIterator->second;
        }
      }
    }

    returnedFlow->arrayValue->emplace_back(std::move(newNode));
  }
  //}}}

  return returnedFlow;
}

BaseLib::PVariable FlowParser::removeNodesFromFlow(const BaseLib::PVariable &flow, const std::string &tab, const std::string &tag) {
  if (!flow || flow->arrayValue->empty() || tag.empty() || tab.empty()) return BaseLib::PVariable();

  BaseLib::PVariable tabStruct;
  std::string tabId;
  BaseLib::PVariable returnedFlow = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
  returnedFlow->arrayValue->reserve(flow->arrayValue->size());

  bool foundTag = false;

  //{{{ Find tab
  for (auto &flowElement : *flow->arrayValue) {
    auto typeIterator = flowElement->structValue->find("type");
    if (typeIterator != flowElement->structValue->end() && typeIterator->second->stringValue == "tab") {
      auto labelIterator = flowElement->structValue->find("label");
      if (labelIterator != flowElement->structValue->end() && labelIterator->second->stringValue == tab) {
        auto idIterator = flowElement->structValue->find("id");
        if (idIterator == flowElement->structValue->end()) continue;

        tabStruct = flowElement;
        tabId = idIterator->second->stringValue;
        continue;
      }
    }
  }
  if (tabId.empty()) return BaseLib::PVariable();
  //}}}

  //{{{ Remove all nodes with tag in tab
  for (auto &flowElement : *flow->arrayValue) {
    auto zIterator = flowElement->structValue->find("z");
    if (zIterator != flowElement->structValue->end() && zIterator->second->stringValue == tabId) {
      auto tagIterator = flowElement->structValue->find("homegearTag");
      if (tagIterator != flowElement->structValue->end() && tagIterator->second->stringValue == tag) {
        foundTag = true;
        continue;
      }
    }

    auto idIterator = flowElement->structValue->find("id");
    if (idIterator == flowElement->structValue->end() || idIterator->second->stringValue == tabId) continue;

    returnedFlow->arrayValue->emplace_back(flowElement);
  }
  if (!foundTag) return BaseLib::PVariable();
  //}}}

  //{{{ Check if there are any remaining nodes in flow. If yes, readd tab so it is not deleted.
  for (auto &flowElement : *returnedFlow->arrayValue) {
    auto zIterator = flowElement->structValue->find("z");
    if (zIterator != flowElement->structValue->end() && zIterator->second->stringValue == tabId) {
      returnedFlow->arrayValue->emplace_back(tabStruct);
    }
  }
  //}}}

  return returnedFlow;
}

bool FlowParser::flowHasTag(const BaseLib::PVariable &flow, const std::string &tab, const std::string &tag) {
  std::string tabId;

  //{{{ Find tab
  for (auto &flowElement : *flow->arrayValue) {
    auto typeIterator = flowElement->structValue->find("type");
    if (typeIterator != flowElement->structValue->end() && typeIterator->second->stringValue == "tab") {
      auto labelIterator = flowElement->structValue->find("label");
      if (labelIterator != flowElement->structValue->end() && labelIterator->second->stringValue == tab) {
        auto idIterator = flowElement->structValue->find("id");
        if (idIterator == flowElement->structValue->end()) continue;

        tabId = idIterator->second->stringValue;
        continue;
      }
    }
  }
  if (tabId.empty()) return false;
  //}}}

  for (auto &flowElement : *flow->arrayValue) {
    auto zIterator = flowElement->structValue->find("z");
    if (zIterator != flowElement->structValue->end() && zIterator->second->stringValue == tabId) {
      auto tagIterator = flowElement->structValue->find("homegearTag");
      if (tagIterator != flowElement->structValue->end() && tagIterator->second->stringValue == tag) {
        return true;
      }
    }
  }

  return false;
}

}
}