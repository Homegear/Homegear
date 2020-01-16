/* Copyright 2013-2019 Homegear GmbH
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
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

namespace Homegear
{

NodeLoader::NodeLoader(std::string nodeNamespace, std::string type, std::string path)
{
	try
	{
		_filename = type + ".so";
		_path = path;
		_namespace = nodeNamespace;
		_type = type;

		void* nodeHandle = dlopen(_path.c_str(), RTLD_NOW);
		if(!nodeHandle)
		{
			GD::out.printCritical("Critical: Could not open node \"" + _path + "\": " + std::string(dlerror()));
			return;
		}

		Flows::NodeFactory* (* getFactory)();
		getFactory = (Flows::NodeFactory* (*)()) dlsym(nodeHandle, "getFactory");
		if(!getFactory)
		{
			GD::out.printCritical("Critical: Could not open node \"" + _path + "\". Symbol \"getFactory\" not found.");
			dlclose(nodeHandle);
			return;
		}

		_handle = nodeHandle;
		_factory = std::unique_ptr<Flows::NodeFactory>(getFactory());
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

NodeLoader::~NodeLoader()
{
	try
	{
		GD::out.printInfo("Info: Disposing node " + _type);
		GD::out.printDebug("Debug: Deleting factory pointer of node " + _type);
		if(_factory) delete _factory.release();
		GD::out.printDebug("Debug: Closing dynamic library module " + _type);
		if(_handle) dlclose(_handle);
		_handle = nullptr;
		GD::out.printDebug("Debug: Dynamic library " + _type + " disposed");
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
}

Flows::PINode NodeLoader::createNode(const std::atomic_bool* nodeEventsEnabled)
{
	if(!_factory) return Flows::PINode();
	return Flows::PINode(_factory->createNode(_path, _namespace, _type, nodeEventsEnabled));
}

NodeManager::NodeManager(const std::atomic_bool* nodeEventsEnabled)
{
	_nodeEventsEnabled = nodeEventsEnabled;
}

NodeManager::~NodeManager()
{
	std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
	_nodesUsage.clear();
	for(auto& node : _nodes)
	{
		if(!node.second) continue;
		node.second.reset();
	}
	_nodes.clear();
	std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
	_nodeLoaders.clear();
}

Flows::PINode NodeManager::getNode(const std::string& id)
{
	try
	{
		Flows::PINode node;
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		auto nodeIterator = _nodes.find(id);
		bool locked = false;
		if(nodeIterator != _nodes.end() && nodeIterator->second)
		{
			node = nodeIterator->second;
			auto nodeInfoIterator = _nodesUsage.find(node->getNamespace() + "." + node->getType());
			if(nodeInfoIterator != _nodesUsage.end()) locked = nodeInfoIterator->second->locked;
		}
		if(node && !locked) return node;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return Flows::PINode();
}

std::vector<NodeManager::PNodeInfo> NodeManager::getNodeInfo()
{
	std::vector<NodeManager::PNodeInfo> nodeInfoVector;
	try
	{
		std::unique_ptr<BaseLib::Rpc::JsonDecoder> jsonDecoder(new BaseLib::Rpc::JsonDecoder(GD::bl.get()));
		std::vector<std::string> directories = GD::bl->io.getDirectories(GD::bl->settings.nodeBluePath() + "nodes/");
		for(auto& directory : directories)
		{
			std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.nodeBluePath() + "nodes/" + directory);
			if(files.empty()) continue;
			if(nodeInfoVector.size() + files.size() > nodeInfoVector.capacity()) nodeInfoVector.reserve(nodeInfoVector.capacity() + 100);
			for(auto& file : files)
			{
				std::string path = GD::bl->settings.nodeBluePath() + "nodes/" + directory + "/" + file;
				try
				{
					if(file.size() < 6) continue; //?*.hni
					std::string extension = file.substr(file.size() - 4, 4);
					if(extension != ".hni") continue;
					std::string content = GD::bl->io.getFileContent(path);
					BaseLib::HelperFunctions::trim(content);
					if(content.empty() || content.compare(0, 31, "<script type=\"text/x-homegear\">") != 0)
					{
						GD::out.printError("Error: Could not parse file " + path + ". Node header is missing.");
						continue;
					}
					auto endPos = content.find("</script>");
					if(endPos == std::string::npos)
					{
						GD::out.printError("Error: Could not parse file " + path + ". Node header is malformed.");
						continue;
					}

					PNodeInfo nodeInfo = std::make_shared<NodeManager::NodeInfo>();
					nodeInfo->filename = file;
					nodeInfo->fullPath = path;

					std::string headerJson = content.substr(32, endPos - 32);
					BaseLib::PVariable header = jsonDecoder->decode(headerJson);
					auto headerIterator = header->structValue->find("name");
					if(headerIterator == header->structValue->end() || headerIterator->second->stringValue.empty())
					{
						GD::out.printError("Error: Could not parse file " + path + ". \"name\" is not defined in header.");
						continue;
					}
					nodeInfo->nodeName = headerIterator->second->stringValue;

					headerIterator = header->structValue->find("readableName");
					if(headerIterator == header->structValue->end() || headerIterator->second->stringValue.empty())
					{
						GD::out.printError("Error: Could not parse file " + path + ". \"readableName\" is not defined in header.");
						continue;
					}
					nodeInfo->readableName = headerIterator->second->stringValue;

					headerIterator = header->structValue->find("version");
					if(headerIterator == header->structValue->end() || headerIterator->second->stringValue.empty())
					{
						GD::out.printError("Error: Could not parse file " + path + ". \"version\" is not defined in header.");
						continue;
					}
					nodeInfo->version = headerIterator->second->stringValue;

                    headerIterator = header->structValue->find("coreNode");
                    if(headerIterator != header->structValue->end()) nodeInfo->coreNode = headerIterator->second->booleanValue;

					headerIterator = header->structValue->find("maxThreadCount");
					if(headerIterator == header->structValue->end())
					{
						GD::out.printError("Error: Could not parse file " + path + ". \"maxThreadCount\" is not defined in header.");
						continue;
					}
					nodeInfo->maxThreadCount = headerIterator->second->integerValue;

					nodeInfo->nodeId = directory + file;
					content = content.substr(endPos + 9);
					nodeInfo->frontendCode.swap(content);
					nodeInfoVector.push_back(nodeInfo);
				}
				catch(const std::exception& ex)
				{
					GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error opening file " + path + ": " + ex.what());
				}
			}
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return nodeInfoVector;
}

std::string NodeManager::getNodeLocales(std::string& language)
{
	try
	{
		std::unique_ptr<BaseLib::Rpc::JsonDecoder> jsonDecoder(new BaseLib::Rpc::JsonDecoder(GD::bl.get()));
		std::string locales = "{";
		locales.reserve(8192);
		std::vector<std::string> directories = GD::bl->io.getDirectories(GD::bl->settings.nodeBluePath() + "nodes/");
		bool firstFile = true;
		for(auto& directory : directories)
		{
			std::string localePath = GD::bl->settings.nodeBluePath() + "nodes/" + directory + "/locales/" + language + "/";
			if(!BaseLib::Io::directoryExists(localePath)) continue;
			std::vector<std::string> files = GD::bl->io.getFiles(localePath);
			if(files.empty()) continue;
			for(auto& file : files)
			{
				std::string path = localePath + file;
				try
				{
				    if(file.find('.') != std::string::npos) continue;

					std::string content = BaseLib::Io::getFileContent(path);
					BaseLib::HelperFunctions::trim(content);
					BaseLib::PVariable json = jsonDecoder->decode(content); //Check for JSON errors
					if(json->structValue->empty() || json->structValue->begin()->second->structValue->empty()) continue;

					if(BaseLib::Io::fileExists(path + ".help"))
                    {
					    std::string help = BaseLib::Io::getFileContent(path + ".help");
					    json->structValue->begin()->second->structValue->begin()->second->structValue->emplace("help", std::make_shared<BaseLib::Variable>(help));
					    BaseLib::Rpc::JsonEncoder::encode(json, content);
                    }

					if(locales.size() + content.size() > locales.capacity()) locales.reserve(locales.capacity() + content.size() + 8192);
					if(!firstFile) locales += ",";
					else firstFile = false;
					locales.append(content.begin() + 1, content.end() - 1);
				}
				catch(const std::exception& ex)
				{
					GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error opening file " + path + ": " + ex.what());
				}
			}
		}
		locales += "}";
		return locales;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return "";
}

int32_t NodeManager::loadNode(const std::string& nodeNamespace, const std::string& type, const std::string& id, Flows::PINode& node)
{
	try
	{
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		{
			auto nodesIterator = _nodes.find(id);
			if(nodesIterator != _nodes.end())
			{
				node = nodesIterator->second;
				return 1;
			}
		}

		std::string path(GD::bl->settings.nodeBluePath() + "nodes/" + nodeNamespace + "/");
		if(BaseLib::Io::fileExists(path + type + ".so")) //C++ module
		{
			GD::out.printInfo("Info: Loading " + type + ".so for node " + id);
			path = path + type + ".so";

			bool loaderCreated = false;
			std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
			if(_nodeLoaders.find(nodeNamespace + "." + type) == _nodeLoaders.end())
			{
				loaderCreated = true;
				_nodeLoaders.emplace(nodeNamespace + "." + type, std::unique_ptr<NodeLoader>(new NodeLoader(nodeNamespace, type, path)));
			}

			node = _nodeLoaders.at(nodeNamespace + "." + type)->createNode(_nodeEventsEnabled);

			if(node)
			{
				auto nodeUsageIterator = _nodesUsage.find(nodeNamespace + "." + type);
				if(nodeUsageIterator == _nodesUsage.end())
				{
					auto result = _nodesUsage.emplace(nodeNamespace + "." + type, std::make_shared<NodeUsageInfo>());
					if(!result.second)
					{
						if(loaderCreated) _nodeLoaders.erase(nodeNamespace + "." + type);
						return -1;
					}
					nodeUsageIterator = result.first;
				}

				nodeUsageIterator->second->referenceCounter++;
				_nodes.emplace(id, node);
			}
			else
			{
				if(loaderCreated) _nodeLoaders.erase(nodeNamespace + "." + type);
				GD::out.printError("Error: Could not load node file " + path + ".");
				return -3;
			}
			return 0;
		}
#ifndef NO_SCRIPTENGINE
		else if(BaseLib::Io::fileExists(path + type + ".s.hgn")) //Encrypted PHP
		{
			GD::out.printInfo("Info: Loading node " + type + ".s.hgn");
			node = std::make_shared<StatefulPhpNode>(path + type + ".s.hgn", nodeNamespace, type, _nodeEventsEnabled);
			_nodes.emplace(id, node);
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + type + ".s.php")) //Unencrypted PHP
		{
			GD::out.printInfo("Info: Loading node " + type + ".s.php");
			node = std::make_shared<StatefulPhpNode>(path + type + ".s.php", nodeNamespace, type, _nodeEventsEnabled);
			_nodes.emplace(id, node);
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + type + ".hgn")) //Encrypted PHP
		{
			GD::out.printInfo("Info: Loading node " + type + ".hgn");
			node = std::make_shared<SimplePhpNode>(path + type + ".hgn", nodeNamespace, type, _nodeEventsEnabled);
			_nodes.emplace(id, node);
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + type + ".php")) //Unencrypted PHP
		{
			GD::out.printInfo("Info: Loading node " + type + ".php");
			node = std::make_shared<SimplePhpNode>(path + type + ".php", nodeNamespace, type, _nodeEventsEnabled);
			_nodes.emplace(id, node);
			return 0;
		}
#endif
		else
		{
			GD::out.printError("Error: No node file starting with \"" + path + type + "\" found (possible endings: \".so\", \".s.hgn\", \".s.php\", \".hgn\" and \".php\".");
			return -2;
		}
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return -1;
}

int32_t NodeManager::unloadNode(const std::string& id)
{
	try
	{
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);

		auto nodesIterator = _nodes.find(id);
		if(nodesIterator == _nodes.end()) return 1;
		auto nodesUsageIterator = _nodesUsage.find(nodesIterator->second->getNamespace() + "." + nodesIterator->second->getType());
		if(nodesUsageIterator != _nodesUsage.end())
		{
			nodesUsageIterator->second->referenceCounter--;
			if(nodesUsageIterator->second->referenceCounter > 0)
			{
				_nodes.erase(nodesIterator);
				return 0;
			}
		}

		GD::out.printInfo("Info: Unloading node " + id);

		Flows::PINode node = nodesIterator->second;
		std::string nodeNamespace = node->getNamespace();
		std::string nodeType = node->getType();

		if(nodesUsageIterator != _nodesUsage.end())
		{
			nodesUsageIterator->second->locked = true;
			while(node.use_count() > 2) //This is the last node, that's why we can do this; At this point, the node is used by "_nodes" and "node".
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
		_nodes.erase(nodesIterator);
		node.reset();

		if(nodesUsageIterator != _nodesUsage.end()) _nodesUsage.erase(nodesUsageIterator);

		auto nodeLoaderIterator = _nodeLoaders.find(nodeNamespace + "." + nodeType);
		if(nodeLoaderIterator == _nodeLoaders.end()) return 1;

		nodeLoaderIterator->second.reset();
		_nodeLoaders.erase(nodeLoaderIterator);
		return 0;
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	return -1;
}

}