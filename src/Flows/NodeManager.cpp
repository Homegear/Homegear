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
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

NodeLoader::NodeLoader(std::string name, std::string path)
{
	try
	{
		_name = name;
		GD::out.printInfo("Info: Loading node " + _name);

		void* nodeHandle = dlopen(path.c_str(), RTLD_NOW);
		if (!nodeHandle)
		{
			GD::out.printCritical("Critical: Could not open node \"" + path + "\": " + std::string(dlerror()));
			return;
		}

		std::string(*getNodeName)();
		getNodeName = (std::string (*)())dlsym(nodeHandle, "getNodeName");
		if (!getNodeName)
		{
			GD::out.printCritical("Critical: Could not open node \"" + path + "\". Symbol \"getNodeName\" not found.");
			dlclose(nodeHandle);
			return;
		}
		_nodeName = getNodeName();
		if (_nodeName.empty())
		{
			GD::out.printCritical("Critical: Could not open node \"" + path + "\". Got empty node name.");
			dlclose(nodeHandle);
			return;
		}

		std::string (*getVersion)();
		getVersion = (std::string (*)())dlsym(nodeHandle, "getVersion");
		if(!getVersion)
		{
			GD::out.printCritical("Critical: Could not open node \"" + path + "\". Symbol \"getVersion\" not found.");
			dlclose(nodeHandle);
			return;
		}
		_version = getVersion();
		if(GD::bl->version() != _version)
		{
			GD::out.printCritical("Critical: Could not open node \"" + path + "\". Module is compiled for Homegear version " + _version);
			dlclose(nodeHandle);
			return;
		}

		BaseLib::Flows::NodeFactory* (*getFactory)();
		getFactory = (BaseLib::Flows::NodeFactory* (*)())dlsym(nodeHandle, "getFactory");
		if(!getFactory)
		{
			GD::out.printCritical("Critical: Could not open node \"" + path + "\". Symbol \"getFactory\" not found.");
			dlclose(nodeHandle);
			return;
		}

		_handle = nodeHandle;
		_factory = std::unique_ptr<BaseLib::Flows::NodeFactory>(getFactory());
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
	dispose();
}

void NodeLoader::dispose()
{
	try
	{
		if(_disposing) return;
		_disposing = true;
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

std::string NodeLoader::getNodeName()
{
	return _nodeName;
}

std::string NodeLoader::getVersion()
{
	return _version;
}

std::unique_ptr<BaseLib::Flows::INode> NodeLoader::createNode()
{
	if (!_factory) return std::unique_ptr<BaseLib::Flows::INode>();
	return std::unique_ptr<BaseLib::Flows::INode>(_factory->createNode(GD::bl.get()));
}

NodeManager::NodeManager()
{
}

NodeManager::~NodeManager()
{
	dispose();
}

std::map<std::string, std::shared_ptr<BaseLib::Flows::INode>> NodeManager::getNodes()
{
	std::map<std::string, std::shared_ptr<BaseLib::Flows::INode>> nodes;
	try
	{
		/*std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		for (auto& node : nodes)
		{
			if (!node.second || node.second->locked()) continue;
			nodes.emplace(node);
		}*/
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
	return nodes;
}

std::shared_ptr<BaseLib::Flows::INode> NodeManager::getNode(std::string name)
{
	try
	{
		/*std::shared_ptr<BaseLib::Flows::INode> node;
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		auto nodeIterator = _nodes.find(name);
		if (nodeIterator != _nodes.end()) node = nodeIterator->second;
		if (node && !node->locked()) return node;*/
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
	return std::shared_ptr<BaseLib::Flows::INode>();
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

int32_t NodeManager::loadNode(std::string filename)
{
	try
	{
		/*std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
		std::string path(GD::bl->settings.flowsPath() + "nodes/" + filename);
		if(!BaseLib::Io::fileExists(path)) return -2;
		if(_nodeLoaders.find(filename) != _nodeLoaders.end()) return 1;
		_nodeLoaders.emplace(filename, std::unique_ptr<NodeLoader>(new NodeLoader(filename, path)));

		std::shared_ptr<BaseLib::Flows::INode> node = _nodeLoaders.at(filename)->createNode();

		if(node)
		{
			std::string name = node->getName();
			BaseLib::HelperFunctions::toLower(name);
			BaseLib::HelperFunctions::stringReplace(name, " ", "");
			{
				std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
				_nodes[node->getName()] = node;
			}
		}
		else
		{
			_nodeLoaders.at(filename)->dispose();
			_nodeLoaders.erase(filename);
			return -3;
		}
		return 0;*/
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

int32_t NodeManager::unloadNode(std::string filename)
{
	try
	{
		/*std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
		std::string path(GD::bl->settings.flowsPath() + "nodes/" + filename);
		if(!BaseLib::Io::fileExists(path)) return -2;

		auto nodeLoaderIterator = _nodeLoaders.find(filename);
		if(nodeLoaderIterator != _nodeLoaders.end()) return 1;

		std::shared_ptr<BaseLib::Flows::INode> node = getNode(nodeLoaderIterator->second->getNodeName());
		if(node)
		{
			{
				std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
				node->lock();
			}

			while(node.use_count() > 1)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			node->dispose();
			std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
			_nodes[node->getName()].reset();
			node.reset();
		}


		nodeLoaderIterator->second->dispose();
		nodeLoaderIterator->second.reset();
		_nodeLoaders.erase(nodeLoaderIterator);
		return 0;*/
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

int32_t NodeManager::reloadNode(std::string filename)
{
	try
	{
		int32_t result = unloadNode(filename);
		if(result < 0) return result;
		return loadNode(filename);
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

void NodeManager::loadNodes()
{
	try
	{
		/*GD::out.printDebug("Debug: Loading nodes...");
		std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);

		std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.flowsPath() + "nodes/");
		if(files.empty())
		{
			GD::out.printError("Error: No nodes found in \"" + GD::bl->settings.flowsPath() + "nodes/\".");
			return;
		}
		for(auto& file : files)
		{
			if(file.size() < 10) continue; //node_?*.so
			std::string prefix = file.substr(0, 5);
			std::string extension = file.substr(file.size() - 3, 3);
			if(extension != ".so" || prefix != "node_") continue;
			std::string path(GD::bl->settings.flowsPath() + "nodes/" + file);

			_nodeLoaders.emplace(file, std::unique_ptr<NodeLoader>(new NodeLoader(file, path)));
		}
		if(_nodes.empty())
		{
			GD::out.printError("Error: Could not load any nodes from \"" + GD::bl->settings.flowsPath() + "nodes/\".");
		}*/
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

void NodeManager::disposeNodes()
{
	try
	{
		/*auto nodes = getNodes();
		for(auto node : nodes)
		{
			if(!node.second) continue;
			node.second->dispose();
			node.second.reset();
			std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
			_nodes[node.first].reset();
		}
		nodes.clear();
		std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
		_nodes.clear();*/
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

void NodeManager::dispose()
{
	try
	{
		/*{
			std::lock_guard<std::mutex> nodesGuard(_nodesMutex);
			_nodes.clear();
		}
		std::lock_guard<std::mutex> nodesLoadersGuard(_nodeLoadersMutex);
		_nodeLoaders.clear();*/
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
