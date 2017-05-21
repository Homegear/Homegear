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

#include "NodeManager.h"
#include "PhpNode.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

NodeLoader::NodeLoader(std::string name, std::string path)
{
	try
	{
		_filename = name + ".so";
		_path = path;
		_name = name;

		void* nodeHandle = dlopen(_path.c_str(), RTLD_NOW);
		if (!nodeHandle)
		{
			GD::out.printCritical("Critical: Could not open node \"" + _filename + "\": " + std::string(dlerror()));
			return;
		}

		Flows::NodeFactory* (*getFactory)();
		getFactory = (Flows::NodeFactory* (*)())dlsym(nodeHandle, "getFactory");
		if(!getFactory)
		{
			GD::out.printCritical("Critical: Could not open node \"" + _filename + "\". Symbol \"getFactory\" not found.");
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
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

NodeLoader::~NodeLoader()
{
	try
	{
		GD::out.printInfo("Info: Disposing node " + _name);
		GD::out.printDebug("Debug: Deleting factory pointer of node " + _name);
		if(_factory) delete _factory.release();
		GD::out.printDebug("Debug: Closing dynamic library module " + _name);
		if(_handle) dlclose(_handle);
		_handle = nullptr;
		GD::out.printDebug("Debug: Dynamic library " + _name + " disposed");
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

Flows::PINode NodeLoader::createNode()
{
	if (!_factory) return Flows::PINode();
	return Flows::PINode(_factory->createNode(_path, _name));
}

NodeManager::NodeManager()
{
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

Flows::PINode NodeManager::getNode(std::string id)
{
	try
	{
		Flows::PINode node;
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		auto nodeIterator = _nodes.find(id);
		bool locked = false;
		if (nodeIterator != _nodes.end() && nodeIterator->second)
		{
			node = nodeIterator->second;
			auto nodeInfoIterator = _nodesUsage.find(node->getName());
			if(nodeInfoIterator != _nodesUsage.end()) locked = nodeInfoIterator->second->locked;
		}
		if (node && !locked) return node;
	}
	catch (const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch (...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return Flows::PINode();
}

std::vector<NodeManager::PNodeInfo> NodeManager::getNodeInfo()
{
	std::vector<NodeManager::PNodeInfo> nodeInfoVector;
	try
	{
		std::unique_ptr<BaseLib::Rpc::JsonDecoder> jsonDecoder(new BaseLib::Rpc::JsonDecoder(GD::bl.get()));
		std::vector<std::string> directories = GD::bl->io.getDirectories(GD::bl->settings.flowsPath() + "nodes/");
		for(auto& directory : directories)
		{
			std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.flowsPath() + "nodes/" + directory);
			if (files.empty()) continue;
			for(auto& file : files)
			{
				std::string path = GD::bl->settings.flowsPath() + "nodes/" + directory + "/" + file;
				try
				{
					if(file.size() < 6) continue; //?*.hni
					std::string extension = file.substr(file.size() - 4, 4);
					if (extension != ".hni") continue;
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
				catch(BaseLib::Exception& ex)
				{
					GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error opening file " + path + ": " + ex.what());
				}
				catch(...)
				{
					GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, "Error opening file " + path);
				}
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
	return nodeInfoVector;
}

int32_t NodeManager::loadNode(std::string name, std::string id, Flows::PINode& node)
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

		std::string path(GD::bl->settings.flowsPath() + "nodes/" + name + "/");
		if(BaseLib::Io::fileExists(path + name + ".so")) //C++ module
		{
			GD::out.printInfo("Info: Loading node " + name + ".so");
			path = path + name + ".so";

			std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
			if(_nodeLoaders.find(name) == _nodeLoaders.end()) _nodeLoaders.emplace(name, std::unique_ptr<NodeLoader>(new NodeLoader(name, path)));

			node = _nodeLoaders.at(name)->createNode();

			if(node)
			{
				auto nodeUsageIterator = _nodesUsage.find(name);
				if(nodeUsageIterator == _nodesUsage.end())
				{
					auto result = _nodesUsage.emplace(name, std::make_shared<NodeUsageInfo>());
					if(!result.second) return -1;
					nodeUsageIterator = result.first;
				}

				nodeUsageIterator->second->referenceCounter++;
				_nodes.emplace(id, node);
			}
			else
			{
				_nodeLoaders.erase(name);
				return -3;
			}
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + name + ".hgn")) //Encrypted PHP
		{
			GD::out.printInfo("Info: Loading node " + name + ".hgn");
			node = std::make_shared<PhpNode>(path + name + ".hgn", name);
			_nodes.emplace(id, node);
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + name + ".php")) //Unencrypted PHP
		{
			GD::out.printInfo("Info: Loading node " + name + ".php");
			node = std::make_shared<PhpNode>(path + name + ".php", name);
			_nodes.emplace(id, node);
			return 0;
		}
		else return -2;
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
    return -1;
}

int32_t NodeManager::unloadNode(std::string id)
{
	try
	{
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);

		auto nodesIterator = _nodes.find(id);
		if(nodesIterator == _nodes.end()) return 1;
		nodesIterator->second->stop();
		auto nodesUsageIterator = _nodesUsage.find(nodesIterator->second->getName());
		if(nodesUsageIterator != _nodesUsage.end())
		{
			nodesUsageIterator->second->referenceCounter--;
			if(nodesUsageIterator->second->referenceCounter > 0) return 0;
		}

		std::string path = nodesIterator->second->getPath();

		if(!BaseLib::Io::fileExists(path)) return -2;

		Flows::PINode node = nodesIterator->second;

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

		auto nodeLoaderIterator = _nodeLoaders.find(nodesIterator->second->getName());
		if(nodeLoaderIterator == _nodeLoaders.end()) return 1;

		nodeLoaderIterator->second.reset();
		_nodeLoaders.erase(nodeLoaderIterator);
		return 0;
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
    return -1;
}
