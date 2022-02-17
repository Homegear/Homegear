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

#include "NodeManager.h"
#include "SimplePhpNode.h"
#include "StatefulPhpNode.h"
#include "NodeRedNode.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

#include <memory>

namespace Homegear {

NodeLoader::NodeLoader(const std::string &soPath) {
  try {
    _soPath = soPath;

    void *nodeHandle = dlopen(soPath.c_str(), RTLD_NOW);
    if (!nodeHandle) {
      GD::out.printCritical("Critical: Could not open node \"" + soPath + "\": " + std::string(dlerror()));
      return;
    }

    Flows::NodeFactory *(*getFactory)();
    getFactory = (Flows::NodeFactory *(*)())dlsym(nodeHandle, "getFactory");
    if (!getFactory) {
      GD::out.printCritical("Critical: Could not open node \"" + soPath + R"(". Symbol "getFactory" not found.)");
      dlclose(nodeHandle);
      return;
    }

    _handle = nodeHandle;
    _factory = std::unique_ptr<Flows::NodeFactory>(getFactory());
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

NodeLoader::~NodeLoader() {
  try {
    GD::out.printInfo("Info: Disposing node " + _soPath);
    GD::out.printDebug("Debug: Deleting factory pointer of node " + _soPath);
    if (_factory) delete _factory.release();
    GD::out.printDebug("Debug: Closing dynamic library module " + _soPath);
    if (_handle) dlclose(_handle);
    _handle = nullptr;
    GD::out.printDebug("Debug: Dynamic library " + _soPath + " disposed");
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PINode NodeLoader::createNode(const std::atomic_bool *nodeEventsEnabled, const std::string &type, const std::string &nodePath) {
  if (!_factory) return Flows::PINode();
  return Flows::PINode(_factory->createNode(nodePath, type, nodeEventsEnabled));
}

NodeManager::NodeManager(const std::atomic_bool *nodeEventsEnabled) {
  _nodeEventsEnabled = nodeEventsEnabled;

  fillManagerModuleInfo();
}

NodeManager::~NodeManager() {
  std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
  _managerModuleInfo.clear();
  _managerModuleInfoByNodeType.clear();
  _nodeInfoByNodeType.clear();
  for (auto &node : _nodes) {
    if (!node.second) continue;
    node.second.reset();
  }
  _nodes.clear();
  std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
  _nodeLoaders.clear();
  _pythonNodeLoader.reset();
}

void NodeManager::clearManagerModuleInfo() {
  {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    _managerModuleInfo.clear();
    _managerModuleInfoByNodeType.clear();
    _nodeInfoByNodeType.clear();
  }
}

NodeManager::PManagerModuleInfo NodeManager::fillManagerModuleInfo(const std::string &directory) {
  auto packageJsonPath = GD::bl->settings.nodeBluePath().append("nodes/").append(directory).append("package.json");
  if (!BaseLib::Io::fileExists(packageJsonPath)) return {};

  auto managerModuleInfo = std::make_shared<ManagerModuleInfo>();
  managerModuleInfo->modulePath = GD::bl->settings.nodeBluePath().append("nodes/").append(directory);

  BaseLib::PVariable packageJson;
  try {
    packageJson = BaseLib::Rpc::JsonDecoder::decode(BaseLib::Io::getFileContent(packageJsonPath));
  } catch (const std::exception &ex) {
    GD::out.printError("Error decoding " + packageJsonPath + ". Please fix the error in the JSON file otherwise the nodes in that directory won't work.");
    return {};
  }

  managerModuleInfo->module = directory.substr(0, directory.size() - 1);

  auto jsonIterator = packageJson->structValue->find("name");
  if (jsonIterator == packageJson->structValue->end() || jsonIterator->second->stringValue.empty()) {
    GD::out.printError("Error decoding " + packageJsonPath + ": Property \"name\" is missing. Please fix the error in the JSON file otherwise the nodes in that directory won't work.");
    return {};
  }
  managerModuleInfo->name = jsonIterator->second->stringValue;

  jsonIterator = packageJson->structValue->find("version");
  if (jsonIterator == packageJson->structValue->end() || jsonIterator->second->stringValue.empty()) {
    GD::out.printError("Error decoding " + packageJsonPath + ": Property \"version\" is missing. Please fix the error in the JSON file otherwise the nodes in that directory won't work.");
    return {};
  }
  managerModuleInfo->version = jsonIterator->second->stringValue;

  jsonIterator = packageJson->structValue->find("node-blue");
  if (jsonIterator == packageJson->structValue->end()) {
    jsonIterator = packageJson->structValue->find("node-red");
    if (jsonIterator == packageJson->structValue->end()) {
      GD::out.printError("Error decoding " + packageJsonPath + ": Property \"node-blue\" is missing. Please fix the error in the JSON file otherwise the nodes in that directory won't work.");
      return {};
    }
  }

  auto nodesJsonIterator = jsonIterator->second->structValue->find("nodes");
  if (nodesJsonIterator == jsonIterator->second->structValue->end()) {
    GD::out.printError("Error decoding " + packageJsonPath + R"(: Property "node-blue\nodes" is missing. Please fix the error in the JSON file otherwise the nodes in that directory won't work.)");
    return {};
  }
  auto nodesJson = nodesJsonIterator->second;
  managerModuleInfo->nodesOrdered.reserve(nodesJson->structValue->size());

  auto localIterator = jsonIterator->second->structValue->find("local");
  if (localIterator != jsonIterator->second->structValue->end()) managerModuleInfo->local = localIterator->second->booleanValue;

  auto maxThreadCountsJsonIterator = jsonIterator->second->structValue->find("maxThreadCounts");
  auto maxThreadCountsJson = (maxThreadCountsJsonIterator == jsonIterator->second->structValue->end() ? std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct) : maxThreadCountsJsonIterator->second);

  for (auto &propertyName : *nodesJson->arrayValue) {
    try {
      auto node = nodesJson->structValue->find(propertyName->stringValue);
      if (node == nodesJson->structValue->end()) continue;
      auto nodeInfo = std::make_shared<NodeInfo>();
      nodeInfo->type = node->first; //Node type and node set are always the same in Homegear. For Node-RED they can differ (see .js section below)
      nodeInfo->nodeSet = node->first;
      nodeInfo->isUiNode = (directory == "ui/");
      auto filename = node->second->stringValue;
      //Possible file extensions are: ".so", ".s.hgn", ".s.php", ".hgn", ".php" and ".py"
      auto dotPosition = filename.find_first_of('.'); //Don't use find_last_of (because e.g. for .s.php)!
      if (dotPosition == std::string::npos) continue;
      auto extension = filename.substr(dotPosition);
      auto codefilePath = GD::bl->settings.nodeBluePath().append("nodes/").append(directory);

      {
        nodeInfo->filePrefix = filename;
        auto pos = nodeInfo->filePrefix.find_last_of('/');
        if (pos != std::string::npos) nodeInfo->filePrefix = nodeInfo->filePrefix.substr(pos + 1);
        pos = nodeInfo->filePrefix.find_first_of('.'); //Don't use find_last_of (because e.g. for .s.php)!
        if (pos != std::string::npos) nodeInfo->filePrefix = nodeInfo->filePrefix.substr(0, pos);
      }
      nodeInfo->fullCodefilePath = codefilePath + filename;
      auto slashPos = nodeInfo->fullCodefilePath.find_last_of('/');
      if (managerModuleInfo->codeDirectory.empty()) managerModuleInfo->codeDirectory = nodeInfo->fullCodefilePath.substr(0, slashPos + 1);
      if (!BaseLib::Io::fileExists(managerModuleInfo->modulePath + ".compiling") && (nodeInfo->fullCodefilePath.empty() || !BaseLib::Io::fileExists(nodeInfo->fullCodefilePath))) {
        GD::out.printError("Error: Node file \"" + filename + "\" defined in \"" + packageJsonPath + "\" does not exists.");
        continue;
      }

      if (BaseLib::Io::fileExists(codefilePath + nodeInfo->filePrefix + ".cred.json")) {
        auto credentialTypesString = BaseLib::Io::getFileContent(codefilePath + nodeInfo->filePrefix + ".cred.json");
        nodeInfo->credentialTypes = BaseLib::Rpc::JsonDecoder::decode(credentialTypesString);
      }

      if (extension == ".so") {
        nodeInfo->codeType = NodeCodeType::cpp;
        auto maxThreadCountsIterator = maxThreadCountsJson->structValue->find(node->first);
        if (maxThreadCountsIterator == maxThreadCountsJson->structValue->end()) {
          GD::out.printError(R"(Error: It is mandatory for C++ nodes to define "maxThreadCounts" for every node in package.json. No entry was found for ")" + node->first + "\" in " + packageJsonPath);
          continue;
        }
        nodeInfo->maxThreadCount = maxThreadCountsIterator->second->integerValue;
      } else if (extension == ".s.php") {
        nodeInfo->codeType = NodeCodeType::statefulPhp;
        auto maxThreadCountsIterator = maxThreadCountsJson->structValue->find(node->first);
        if (maxThreadCountsIterator == maxThreadCountsJson->structValue->end()) {
          GD::out.printError(R"(Error: It is mandatory for stateful PHP nodes to define "maxThreadCounts" for every node in package.json. No entry was found for ")" + node->first + "\" in " + packageJsonPath);
          continue;
        }
        nodeInfo->maxThreadCount = maxThreadCountsIterator->second->integerValue;
      } else if (extension == ".s.hgn") {
        nodeInfo->codeType = NodeCodeType::statefulPhpEncrypted;
        auto maxThreadCountsIterator = maxThreadCountsJson->structValue->find(node->first);
        if (maxThreadCountsIterator == maxThreadCountsJson->structValue->end()) {
          GD::out.printError(R"(Error: It is mandatory for stateful PHP nodes to define "maxThreadCounts" for every node in package.json. No entry was found for ")" + node->first + "\" in " + packageJsonPath);
          continue;
        }
        nodeInfo->maxThreadCount = maxThreadCountsIterator->second->integerValue;
      } else if (extension == ".php") {
        nodeInfo->codeType = NodeCodeType::simplePhp;
      } else if (extension == ".hgn") {
        nodeInfo->codeType = NodeCodeType::simplePhpEncrypted;
      } else if (extension == ".py") {
        nodeInfo->codeType = NodeCodeType::python;
        nodeInfo->maxThreadCount = 2;
      } else if (extension == ".hgnpy") {
        nodeInfo->codeType = NodeCodeType::pythonEncrypted;
        nodeInfo->maxThreadCount = 2;
      } else if (extension == ".js") {
        if (nodeInfo->type != "javascript") {
          if (!_nodeRedRequired) GD::out.printInfo("Info: Enabling Node-RED because node " + nodeInfo->type + " requires it. This message is not repeated for other nodes.");
          _nodeRedRequired = true;
        }
        nodeInfo->codeType = NodeCodeType::javascript;

        //{{{ Get type from HTML file - this is the only way that always works
        std::string htmlContent;
        if (BaseLib::Io::fileExists(managerModuleInfo->codeDirectory + nodeInfo->filePrefix + ".html")) {
          htmlContent = BaseLib::Io::getFileContent(managerModuleInfo->codeDirectory + nodeInfo->filePrefix + ".html");
        } else if (BaseLib::Io::fileExists(managerModuleInfo->codeDirectory + nodeInfo->filePrefix + ".hni")) {
          htmlContent = BaseLib::Io::getFileContent(managerModuleInfo->codeDirectory + nodeInfo->filePrefix + ".hni");
        }
        if (!htmlContent.empty()) {
          auto registerPos = htmlContent.find("RED.nodes.registerType(");
          while (registerPos != std::string::npos) {
            auto nodeInfoNode = std::make_shared<NodeInfo>();
            nodeInfoNode->isUiNode = nodeInfo->isUiNode;
            nodeInfoNode->filePrefix = nodeInfo->filePrefix;
            nodeInfoNode->codeType = nodeInfo->codeType;
            nodeInfoNode->credentialTypes = nodeInfo->credentialTypes;
            nodeInfoNode->nodeSet = nodeInfo->nodeSet;
            nodeInfoNode->fullCodefilePath = nodeInfo->fullCodefilePath;

            registerPos += sizeof("RED.nodes.registerType(") - 1;
            bool doubleQuote = false;
            auto startPos = htmlContent.find('\'', registerPos);
            auto startPos2 = htmlContent.find('"', registerPos);
            if (startPos == std::string::npos || (startPos2 != std::string::npos && startPos2 < startPos)) {
              doubleQuote = true;
              startPos = startPos2;
            }
            if (startPos != std::string::npos) {
              auto endPos = doubleQuote ? htmlContent.find('"', startPos + 1) : htmlContent.find('\'', startPos + 1);
              if (endPos != std::string::npos) {
                nodeInfoNode->type = htmlContent.substr(startPos + 1, endPos - startPos - 1);

                if (_managerModuleInfoByNodeType.find(nodeInfoNode->type) != _managerModuleInfoByNodeType.end()) {
                  GD::out.printError("Error: Node type \"" + nodeInfoNode->type + "\" is used more than once. That is not allowed.");
                } else {
                  managerModuleInfo->nodes.emplace(nodeInfoNode->type, nodeInfoNode);
                  managerModuleInfo->nodesOrdered.push_back(nodeInfoNode->type);
                  managerModuleInfo->nodesByFilePrefix.emplace(nodeInfoNode->filePrefix, nodeInfoNode);
                  _managerModuleInfoByNodeType.emplace(nodeInfoNode->type, managerModuleInfo);
                  _nodeInfoByNodeType.emplace(nodeInfoNode->type, nodeInfoNode);
                }
              }
            }

            registerPos = htmlContent.find("RED.nodes.registerType(", registerPos);
          }
        }
        //}}}

        continue;
      } else if (extension == ".hgnjs") {
        _nodeRedRequired = true;
        nodeInfo->codeType = NodeCodeType::javascriptEncrypted;
        //Unsupported at the moment, so continue
        continue;
      } else if (extension == ".hni" || extension == ".html") {
        nodeInfo->codeType = NodeCodeType::none;
      } else continue; //Unsupported type

      if (_managerModuleInfoByNodeType.find(node->first) != _managerModuleInfoByNodeType.end()) {
        GD::out.printError("Error: Node type \"" + node->first + "\" is used more than once. That is not allowed.");
        continue;
      }

      managerModuleInfo->nodes.emplace(nodeInfo->type, nodeInfo);
      managerModuleInfo->nodesOrdered.push_back(nodeInfo->type);
      managerModuleInfo->nodesByFilePrefix.emplace(nodeInfo->filePrefix, nodeInfo);
      _managerModuleInfoByNodeType.emplace(nodeInfo->type, managerModuleInfo);
      _nodeInfoByNodeType.emplace(nodeInfo->type, nodeInfo);
    } catch (std::exception &ex) {
      GD::out.printError("Error processing nodes entry in file \"" + packageJsonPath + "\": " + ex.what());
    }
  }
  return managerModuleInfo;
}

void NodeManager::fillManagerModuleInfo() {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);

    _managerModuleInfo.clear();
    _managerModuleInfoByNodeType.clear();
    _nodeInfoByNodeType.clear();

    std::vector<std::string> directories = GD::bl->io.getDirectories(GD::bl->settings.nodeBluePath() + "nodes/");
    for (auto &directory : directories) {
      PManagerModuleInfo managerModuleInfo;
      if (directory.at(0) == '@') {
        auto subdirectories = BaseLib::Io::getDirectories(GD::bl->settings.nodeBluePath() + "nodes/" + directory);
        for (auto &subdirectory : subdirectories) {
          managerModuleInfo = fillManagerModuleInfo(directory + subdirectory);
          if (!managerModuleInfo) continue;
          _managerModuleInfo.emplace(managerModuleInfo->module, managerModuleInfo);
        }
      } else {
        managerModuleInfo = fillManagerModuleInfo(directory);
        if (!managerModuleInfo) continue;
        _managerModuleInfo.emplace(managerModuleInfo->module, managerModuleInfo);
      }
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

Flows::PINode NodeManager::getNode(const std::string &id) {
  try {
    Flows::PINode node;
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    auto nodeIterator = _nodes.find(id);
    bool locked = false;
    if (nodeIterator != _nodes.end() && nodeIterator->second) {
      node = nodeIterator->second;
      auto nodeInfoIterator = _nodeInfoByNodeType.find(node->getType());
      if (nodeInfoIterator != _nodeInfoByNodeType.end()) locked = nodeInfoIterator->second->locked;
    }
    if (node && !locked) return node;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return Flows::PINode();
}

BaseLib::PVariable NodeManager::getModuleInfo() {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    auto moduleInfoArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    moduleInfoArray->arrayValue->reserve(_managerModuleInfo.size());
    for (auto &moduleInfo : _managerModuleInfo) {
      for (auto &node : moduleInfo.second->nodes) {
        auto nodeListEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
        nodeListEntry->structValue->emplace("id", std::make_shared<BaseLib::Variable>(moduleInfo.second->module + "/" + node.second->nodeSet));
        nodeListEntry->structValue->emplace("name", std::make_shared<BaseLib::Variable>(node.second->nodeSet));
        auto typesEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        typesEntry->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(node.second->type));
        nodeListEntry->structValue->emplace("types", typesEntry);
        nodeListEntry->structValue->emplace("enabled", std::make_shared<BaseLib::Variable>(true));
        nodeListEntry->structValue->emplace("local", std::make_shared<BaseLib::Variable>(moduleInfo.second->local));
        nodeListEntry->structValue->emplace("module", std::make_shared<BaseLib::Variable>(moduleInfo.second->module));
        nodeListEntry->structValue->emplace("version", std::make_shared<BaseLib::Variable>(moduleInfo.second->version));
        moduleInfoArray->arrayValue->emplace_back(nodeListEntry);
      }
    }
    return moduleInfoArray;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
}

BaseLib::PVariable NodeManager::getNodesAddedInfo(const std::string &module) {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);

    auto moduleInfoIterator = _managerModuleInfo.find(module);
    if (moduleInfoIterator == _managerModuleInfo.end()) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    auto moduleInfo = moduleInfoIterator->second;

    auto moduleInfoArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    moduleInfoArray->arrayValue->reserve(moduleInfo->nodes.size());
    for (auto &nodeType : moduleInfo->nodesOrdered) {
      auto nodeIterator = moduleInfo->nodes.find(nodeType);
      if (nodeIterator == moduleInfo->nodes.end()) continue;
      auto &node = nodeIterator->second;
      auto nodeListEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      nodeListEntry->structValue->emplace("id", std::make_shared<BaseLib::Variable>(moduleInfo->module + "/" + node->nodeSet));
      nodeListEntry->structValue->emplace("name", std::make_shared<BaseLib::Variable>(node->type));
      auto typesEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      typesEntry->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(nodeIterator->first));
      nodeListEntry->structValue->emplace("types", typesEntry);
      nodeListEntry->structValue->emplace("added", std::make_shared<BaseLib::Variable>(true));
      nodeListEntry->structValue->emplace("enabled", std::make_shared<BaseLib::Variable>(true));
      nodeListEntry->structValue->emplace("local", std::make_shared<BaseLib::Variable>(moduleInfo->local));
      nodeListEntry->structValue->emplace("module", std::make_shared<BaseLib::Variable>(moduleInfo->module));
      nodeListEntry->structValue->emplace("version", std::make_shared<BaseLib::Variable>(moduleInfo->version));
      moduleInfoArray->arrayValue->emplace_back(nodeListEntry);
    }
    return moduleInfoArray;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
}

BaseLib::PVariable NodeManager::getNodesUpdatedInfo(const std::string &module) {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);

    auto moduleInfoIterator = _managerModuleInfo.find(module);
    if (moduleInfoIterator == _managerModuleInfo.end()) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    auto moduleInfo = moduleInfoIterator->second;

    auto updateInfo = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    updateInfo->structValue->emplace("module", std::make_shared<BaseLib::Variable>(moduleInfo->module));
    updateInfo->structValue->emplace("version", std::make_shared<BaseLib::Variable>(moduleInfo->version));
    return updateInfo;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
}

BaseLib::PVariable NodeManager::getNodesRemovedInfo(const std::string &module) {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);

    auto moduleInfoIterator = _managerModuleInfo.find(module);
    if (moduleInfoIterator == _managerModuleInfo.end()) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    auto moduleInfo = moduleInfoIterator->second;

    auto moduleInfoArray = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
    moduleInfoArray->arrayValue->reserve(moduleInfo->nodes.size());
    for (auto &node : moduleInfo->nodes) {
      auto nodeListEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      nodeListEntry->structValue->emplace("id", std::make_shared<BaseLib::Variable>(moduleInfo->module + "/" + node.second->nodeSet));
      nodeListEntry->structValue->emplace("name", std::make_shared<BaseLib::Variable>(node.second->type));
      auto typesEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      typesEntry->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(node.first));
      nodeListEntry->structValue->emplace("types", typesEntry);
      nodeListEntry->structValue->emplace("enabled", std::make_shared<BaseLib::Variable>(false));
      nodeListEntry->structValue->emplace("local", std::make_shared<BaseLib::Variable>(moduleInfo->local));
      nodeListEntry->structValue->emplace("module", std::make_shared<BaseLib::Variable>(moduleInfo->module));
      moduleInfoArray->arrayValue->emplace_back(nodeListEntry);
    }
    return moduleInfoArray;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
}

std::string NodeManager::getModuleIconsDirectory(const std::string &module) {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);

    auto moduleInfoIterator = _managerModuleInfo.find(module);
    if (moduleInfoIterator == _managerModuleInfo.end()) return "";
    return moduleInfoIterator->second->codeDirectory + "icons/";
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "";
}

std::unordered_map<NodeManager::NodeType, uint32_t> NodeManager::getMaxThreadCounts() {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    std::unordered_map<NodeType, uint32_t> maxThreadCounts;
    for (auto &module : _managerModuleInfo) {
      for (auto &node : module.second->nodes) {
        if (node.second->maxThreadCount > 0) maxThreadCounts.emplace(node.first, node.second->maxThreadCount);
      }
    }
    return maxThreadCounts;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::unordered_map<NodeType, uint32_t>();
}

bool NodeManager::nodeRedRequired() {
  return _nodeRedRequired;
}

void NodeManager::setNodeRedRequired() {
  _nodeRedRequired = true;
}

std::string NodeManager::getFrontendCode() {
  try {
    std::string code;
    code.reserve(1 * 1024 * 1024);
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    for (auto &module : _managerModuleInfo) {
      std::string content;
      if (BaseLib::Io::fileExists(module.second->codeDirectory + module.second->module + ".hni")) {
        content = BaseLib::Io::getFileContent(module.second->codeDirectory + module.second->module + ".hni");
        if (code.size() + content.size() > code.capacity()) code.reserve(code.size() + content.size() + (1 * 1024 * 1024));
        code.append(content);
      } else if (BaseLib::Io::fileExists(module.second->codeDirectory + module.second->module + ".html")) {
        content = BaseLib::Io::getFileContent(module.second->codeDirectory + module.second->module + ".html");
        if (code.size() + content.size() > code.capacity()) code.reserve(code.size() + content.size() + (1 * 1024 * 1024));
        code.append(content);
      } else {
        for (auto &nodeType : module.second->nodesOrdered) {
          auto nodeIterator = module.second->nodes.find(nodeType);
          if (nodeIterator == module.second->nodes.end()) continue;
          auto &node = nodeIterator->second;
          content.clear();
          if (BaseLib::Io::fileExists(module.second->codeDirectory + node->filePrefix + ".hni")) {
            content = BaseLib::Io::getFileContent(module.second->codeDirectory + node->filePrefix + ".hni");
          } else if (BaseLib::Io::fileExists(module.second->codeDirectory + node->filePrefix + ".html")) {
            content = BaseLib::Io::getFileContent(module.second->codeDirectory + node->filePrefix + ".html");
          }

          if (code.size() + content.size() + 1024 > code.capacity()) code.reserve(code.size() + content.size() + (1 * 1024 * 1024));
          //The following line helps the frontend to split module code. This is essential on HTML errors (e. g. missing closing tags).
          code.append("<!-- --- [red-module:" + module.second->module + "/" + node->nodeSet + "] --- -->\r\n");
          code.append(content);
        }
      }
    }
    return code;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

std::string NodeManager::getFrontendCode(const std::string &module, const std::string &filePrefix) {
  try {
    std::string code;
    code.reserve(1 * 1024 * 1024);
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    auto moduleInfoIterator = _managerModuleInfo.find(module);
    if (moduleInfoIterator == _managerModuleInfo.end()) return "";

    auto nodeInfoIterator = moduleInfoIterator->second->nodesByFilePrefix.find(filePrefix);
    if (nodeInfoIterator == moduleInfoIterator->second->nodesByFilePrefix.end()) return "";
    auto node = nodeInfoIterator->second;

    std::string content;
    if (BaseLib::Io::fileExists(moduleInfoIterator->second->codeDirectory + node->filePrefix + ".hni")) {
      content = BaseLib::Io::getFileContent(moduleInfoIterator->second->codeDirectory + node->filePrefix + ".hni");
    } else if (BaseLib::Io::fileExists(moduleInfoIterator->second->codeDirectory + node->filePrefix + ".html")) {
      content = BaseLib::Io::getFileContent(moduleInfoIterator->second->codeDirectory + node->filePrefix + ".html");
    }

    if (code.size() + content.size() + 1024 > code.capacity()) code.reserve(code.size() + content.size() + (1 * 1024 * 1024));
    code.append("<!-- --- [red-module:" + moduleInfoIterator->second->module + "/" + node->nodeSet + "] --- -->\r\n");
    code.append(content);

    return code;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return {};
}

NodeManager::NodeCodeType NodeManager::getNodeCodeType(const std::string &type) {
  try {
    auto nodeInfoIterator = _nodeInfoByNodeType.find(type);
    if (nodeInfoIterator == _nodeInfoByNodeType.end()) return NodeCodeType::undefined;

    return nodeInfoIterator->second->codeType;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return NodeCodeType::undefined;
}

bool NodeManager::isNodeRedNode(const std::string &type) {
  try {
    auto nodeInfoIterator = _nodeInfoByNodeType.find(type);
    if (nodeInfoIterator == _nodeInfoByNodeType.end()) return false;

    return (nodeInfoIterator->second->codeType == NodeCodeType::javascript || nodeInfoIterator->second->codeType == NodeCodeType::javascriptEncrypted);
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

bool NodeManager::isUiNode(const std::string &type) {
  try {
    auto nodeInfoIterator = _nodeInfoByNodeType.find(type);
    if (nodeInfoIterator == _nodeInfoByNodeType.end()) return false;

    return nodeInfoIterator->second->isUiNode;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return false;
}

BaseLib::PVariable NodeManager::getNodeCredentialTypes(const std::string &type) {
  try {
    auto nodeInfoIterator = _nodeInfoByNodeType.find(type);
    if (nodeInfoIterator == _nodeInfoByNodeType.end()) return std::make_shared<BaseLib::Variable>();

    if (nodeInfoIterator->second->credentialTypes) return nodeInfoIterator->second->credentialTypes;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>();
}

void NodeManager::registerCredentialTypes(const std::string &type, const BaseLib::PVariable &credentialTypes, bool fromIpcServer) {
  try {
    auto nodeInfoIterator = _nodeInfoByNodeType.find(type);
    if (nodeInfoIterator == _nodeInfoByNodeType.end()) {
      if (fromIpcServer && (type == "http proxy" || type == "http request")) return; //Ignore
      GD::out.printWarning("Warning: Could not register credential types. Node type " + type + " is unknown.");
      return;
    }

    if (fromIpcServer && nodeInfoIterator->second->codeType != NodeCodeType::javascript) {
      //Ignore registrations from Node-PINK core nodes as core node registrations are handled by Node-BLUE.
      GD::out.printDebug("Debug: Not registering credential types for Node-PINK type " + type + " as this type already exists in Node-BLUE.");
      return;
    }

    if (!nodeInfoIterator->second->credentialTypes) nodeInfoIterator->second->credentialTypes = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    for (auto &element : *credentialTypes->structValue) {
      //Only inserts elements when non-existant
      nodeInfoIterator->second->credentialTypes->structValue->emplace(element.first, element.second);
    }
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable NodeManager::getIcons() {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    BaseLib::PVariable iconStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    std::string iconsPath = GD::bl->settings.nodeBluePath() + "www/icons/";
    if (BaseLib::Io::directoryExists(iconsPath)) {
      std::vector<std::string> files = BaseLib::Io::getFiles(iconsPath);
      if (!files.empty()) {
        auto moduleIcons = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
        moduleIcons->arrayValue->reserve(files.size());
        for (auto &file: files) {
          moduleIcons->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(file));
        }
        iconStruct->structValue->emplace("node-red", moduleIcons);
      }
    }

    for (auto &module : _managerModuleInfo) {
      iconsPath = module.second->codeDirectory + "icons/";
      if (!BaseLib::Io::directoryExists(iconsPath)) continue;
      std::vector<std::string> files = BaseLib::Io::getFiles(iconsPath);
      if (files.empty()) continue;
      auto moduleIcons = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      moduleIcons->arrayValue->reserve(files.size());
      for (auto &file: files) {
        moduleIcons->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(file));
      }
      iconStruct->structValue->emplace(module.first, moduleIcons);
    }
    return iconStruct;
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
}

BaseLib::PVariable NodeManager::getExamples(const std::string &relativeExamplesPath) {
  try {
    if (relativeExamplesPath == "") {
      std::lock_guard<std::mutex> nodesGuard(_nodesMutex);

      auto responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);
      responseJson->arrayValue->reserve(_managerModuleInfo.size() + 1);

      responseJson->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>("node-blue"));
      for (auto &module : _managerModuleInfo) {
        auto examplesPath = module.second->modulePath + "examples/";
        if (!BaseLib::Io::directoryExists(examplesPath)) {
          examplesPath = module.second->codeDirectory + "examples/";
          if (!BaseLib::Io::directoryExists(examplesPath)) continue;
        }
        responseJson->arrayValue->emplace_back(std::make_shared<BaseLib::Variable>(module.first));
      }

      return responseJson;
    }
    else {
      auto responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tArray);

      std::string examplesPath;
      if (relativeExamplesPath.compare(0, 10, "node-blue/") == 0) {
        examplesPath = GD::bl->settings.nodeBlueDataPath() + "examples/" + relativeExamplesPath;
      } else {
        auto modulePair = BaseLib::HelperFunctions::splitFirst(relativeExamplesPath, '/');

        std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
        auto moduleIterator = _managerModuleInfo.find(modulePair.first);
        if (moduleIterator == _managerModuleInfo.end()) return responseJson;

        examplesPath = moduleIterator->second->modulePath + "examples/" + modulePair.second;
        if (!BaseLib::Io::fileExists(examplesPath) && !BaseLib::Io::directoryExists(examplesPath)) {
          examplesPath = moduleIterator->second->codeDirectory + "examples/" + modulePair.second;
        }
      }

      bool isDirectory = false;
      if (BaseLib::Io::isDirectory(examplesPath, isDirectory) == -1) return responseJson;

      if (!isDirectory) {
        responseJson->type = BaseLib::VariableType::tString;
        responseJson->stringValue = BaseLib::Io::getFileContent(examplesPath);

        BaseLib::HelperFunctions::stringReplace(responseJson->stringValue, "\"type\": \"inject\"", "\"type\": \"constant\"");
        BaseLib::HelperFunctions::stringReplace(responseJson->stringValue, "\"type\":\"inject\"", "\"type\":\"constant\"");
      } else {
        auto directories = BaseLib::Io::getDirectories(examplesPath);
        auto files = BaseLib::Io::getFiles(examplesPath, false);

        responseJson->arrayValue->reserve(directories.size() + files.size());

        for (auto &directory : directories) {
          responseJson->arrayValue->push_back(std::make_shared<BaseLib::Variable>(directory.substr(0, directory.size() - 1)));
        }

        for (auto &file : files) {
          auto fileEntry = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
          fileEntry->structValue->emplace("fn", std::make_shared<BaseLib::Variable>(file));
          responseJson->arrayValue->push_back(fileEntry);
        }
      }

      return responseJson;
    }
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
}

std::string NodeManager::getNodeLocales(std::string &language) {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    BaseLib::PVariable localeStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    for (auto &module : _managerModuleInfo) {
      std::string localePath = module.second->codeDirectory + "/locales/" + language + "/";
      if (!BaseLib::Io::directoryExists(localePath)) continue;
      std::vector<std::string> files = BaseLib::Io::getFiles(localePath);
      if (files.empty()) continue;
      for (auto &file : files) {
        std::string path = localePath + file;
        try {
          if (file.size() < 6) continue;
          if (file.compare(file.size() - 5, 5, ".json") != 0) continue;

          std::string content = BaseLib::Io::getFileContent(path);
          BaseLib::PVariable json;
          try {
            json = BaseLib::Rpc::JsonDecoder::decode(content);
          } catch (const std::exception &ex) {
            GD::out.printError("Error in locale file \"" + path + "\": " + ex.what());
            continue;
          }
          if (json->structValue->empty()) continue;

          for (auto &node : *json->structValue) {
            auto nodeInfoIterator = module.second->nodesByFilePrefix.find(node.first);
            if (nodeInfoIterator == module.second->nodesByFilePrefix.end()) {
              //Fallback - use info from locale JSON.
              auto id = module.second->module + "/" + node.first;
              auto nodeJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
              nodeJson->structValue->emplace(node);
              auto localeStructIterator = localeStruct->structValue->find(id);
              if (localeStructIterator == localeStruct->structValue->end()) localeStruct->structValue->emplace(id, nodeJson);
              else localeStructIterator->second->structValue->emplace(node.first, node.second);
            } else {
              auto &nodeInfo = nodeInfoIterator->second;
              auto id = module.second->module + "/" + nodeInfo->nodeSet;
              auto nodeJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
              nodeJson->structValue->emplace(node);
              auto localeStructIterator = localeStruct->structValue->find(id);
              if (localeStructIterator == localeStruct->structValue->end()) localeStruct->structValue->emplace(id, nodeJson);
              else localeStructIterator->second->structValue->emplace(nodeInfo->type, node.second);
            }
          }
        }
        catch (const std::exception &ex) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error opening file " + path + ": " + ex.what());
        }
      }
      //{{{ Read HTML help files
      for (auto &file : files) {
        std::string path = localePath + file;
        try {
          if (file.size() < 11) continue;
          if (file.compare(file.size() - 10, 10, ".html") != 0 && file.compare(file.size() - 10, 10, ".help.html") != 0) continue;
          auto type = file.substr(0, file.size() - 10);
          std::string help = BaseLib::Io::getFileContent(path);

          auto id = module.second->module + "/" + type;
          auto localeStructIterator = localeStruct->structValue->find(id);
          if (localeStructIterator != localeStruct->structValue->end()) {
            auto typeIterator = localeStructIterator->second->structValue->find(type);
            if (typeIterator != localeStructIterator->second->structValue->end()) {
              typeIterator->second->structValue->emplace("help", std::make_shared<BaseLib::Variable>(help));
            }
          }
        }
        catch (const std::exception &ex) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error opening file " + path + ": " + ex.what());
        }
      }
      //}}}
    }

    std::string localeString;
    BaseLib::Rpc::JsonEncoder::encode(localeStruct, localeString);
    return localeString;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "";
}

int32_t NodeManager::loadNode(std::string type, const std::string &id, Flows::PINode &node) {
  try {
    std::unique_lock<std::mutex> nodesGuard(_nodesMutex);
    if (_managerModuleInfo.empty()) {
      nodesGuard.unlock();
      fillManagerModuleInfo();
      nodesGuard.lock();
    }

    {
      auto nodesIterator = _nodes.find(id);
      if (nodesIterator != _nodes.end()) {
        node = nodesIterator->second;
        return 1;
      }
    }

    PNodeInfo nodeInfo;
    {
      auto nodeInfoIterator = _nodeInfoByNodeType.find(type);
      if (nodeInfoIterator == _nodeInfoByNodeType.end()) {
        BaseLib::HelperFunctions::stringReplace(type, " ", "-");
        nodeInfoIterator = _nodeInfoByNodeType.find(type);
        if (nodeInfoIterator == _nodeInfoByNodeType.end()) {
          GD::out.printWarning("Warning: No node information was found for type \"" + type + "\". Is this node installed?");
          return -3;
        }
      }
      nodeInfo = nodeInfoIterator->second;
    }

    if (!BaseLib::Io::fileExists(nodeInfo->fullCodefilePath)) {
      GD::out.printWarning("Warning: Node file " + nodeInfo->fullCodefilePath + " does not exist anymore.");
      return -4;
    }

    if (nodeInfo->codeType == NodeCodeType::cpp) //C++ module
    {
      GD::out.printInfo("Info: Loading " + type + ".so for node " + id);

      bool loaderCreated = false;
      std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
      if (_nodeLoaders.find(type) == _nodeLoaders.end()) {
        loaderCreated = true;
        _nodeLoaders.emplace(type, std::make_unique<NodeLoader>(nodeInfo->fullCodefilePath));
      }

      node = _nodeLoaders.at(type)->createNode(_nodeEventsEnabled, type, nodeInfo->fullCodefilePath);

      if (node) {
        nodeInfo->referenceCounter++;
        _nodes.emplace(id, node);
      } else {
        if (loaderCreated) _nodeLoaders.erase(type);
        GD::out.printError("Error: Could not load node file " + nodeInfo->fullCodefilePath + ".");
        return -3;
      }
      return 0;
    }
#ifndef NO_SCRIPTENGINE
    else if (nodeInfo->codeType == NodeCodeType::statefulPhpEncrypted) //Encrypted PHP
    {
      GD::out.printInfo("Info: Loading node " + type + ".s.hgn");
      node = std::make_shared<StatefulPhpNode>(nodeInfo->fullCodefilePath, type, _nodeEventsEnabled);
      _nodes.emplace(id, node);
      return 0;
    } else if (nodeInfo->codeType == NodeCodeType::statefulPhp) //Unencrypted PHP
    {
      GD::out.printInfo("Info: Loading node " + type + ".s.php");
      node = std::make_shared<StatefulPhpNode>(nodeInfo->fullCodefilePath, type, _nodeEventsEnabled);
      _nodes.emplace(id, node);
      return 0;
    } else if (nodeInfo->codeType == NodeCodeType::simplePhpEncrypted) //Encrypted PHP
    {
      GD::out.printInfo("Info: Loading node " + type + ".hgn");
      node = std::make_shared<SimplePhpNode>(nodeInfo->fullCodefilePath, type, _nodeEventsEnabled);
      _nodes.emplace(id, node);
      return 0;
    } else if (nodeInfo->codeType == NodeCodeType::simplePhp) //Unencrypted PHP
    {
      GD::out.printInfo("Info: Loading node " + type + ".php");
      node = std::make_shared<SimplePhpNode>(nodeInfo->fullCodefilePath, type, _nodeEventsEnabled);
      _nodes.emplace(id, node);
      return 0;
    }
#endif
    else if (nodeInfo->codeType == NodeCodeType::python) //Python
    {
      GD::out.printInfo("Info: Loading python-wrapper.so for node " + id);
      auto soPath = GD::bl->settings.nodeBluePath() + "nodes/python-wrapper/python-wrapper.so";

      std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
      if (!_pythonNodeLoader) _pythonNodeLoader = std::make_unique<NodeLoader>(soPath);

      node = _pythonNodeLoader->createNode(_nodeEventsEnabled, type, nodeInfo->fullCodefilePath);

      if (node) _nodes.emplace(id, node);
      else {
        GD::out.printError("Error: Could not load node file " + nodeInfo->fullCodefilePath + ".");
        return -3;
      }
      return 0;
    } else if (nodeInfo->codeType == NodeCodeType::javascript) {
      GD::out.printInfo("Info: Loading node " + type + ".js");
      node = std::make_shared<NodeRedNode>(nodeInfo->fullCodefilePath, type, _nodeEventsEnabled);
      _nodes.emplace(id, node);
      return 0;
    } else if (nodeInfo->codeType == NodeCodeType::none) {
      //Do nothing
      return 0;
    } else {
      GD::out.printError("Error: Could not load node file " + nodeInfo->fullCodefilePath + ". Code type is not supported at the moment.");
      return -2;
    }
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return -1;
}

int32_t NodeManager::unloadNode(const std::string &id) {
  try {
    std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
    std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);

    auto nodesIterator = _nodes.find(id);
    if (nodesIterator == _nodes.end()) return 1;
    auto nodesUsageIterator = _nodeInfoByNodeType.find(nodesIterator->second->getType());
    if (nodesUsageIterator != _nodeInfoByNodeType.end()) {
      nodesUsageIterator->second->referenceCounter--;
      if (nodesUsageIterator->second->referenceCounter > 0) {
        _nodes.erase(nodesIterator);
        return 0;
      }
    }

    GD::out.printInfo("Info: Unloading node " + id);

    Flows::PINode node = nodesIterator->second;
    std::string nodeType = node->getType();

    if (nodesUsageIterator != _nodeInfoByNodeType.end()) {
      nodesUsageIterator->second->locked = true;
      while (node.use_count() > 2) //This is the last node, that's why we can do this; At this point, the node is used by "_nodes" and "node".
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
      }
    }
    _nodes.erase(nodesIterator);
    node.reset();

    auto nodeLoaderIterator = _nodeLoaders.find(nodeType);
    if (nodeLoaderIterator == _nodeLoaders.end()) {
      if (nodesUsageIterator != _nodeInfoByNodeType.end()) nodesUsageIterator->second->locked = false;
      return 1;
    }

    nodeLoaderIterator->second.reset();
    _nodeLoaders.erase(nodeLoaderIterator);

    if (nodesUsageIterator != _nodeInfoByNodeType.end()) nodesUsageIterator->second->locked = false;
    return 0;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return -1;
}

}