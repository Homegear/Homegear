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

NodeLoader::NodeLoader(std::string filename, std::string path)
{
	try
	{
		_filename = filename;
		_path = path;
		GD::out.printInfo("Info: Loading node " + _filename);

		void* nodeHandle = dlopen(_path.c_str(), RTLD_NOW);
		if (!nodeHandle)
		{
			GD::out.printCritical("Critical: Could not open node \"" + _filename + "\": " + std::string(dlerror()));
			return;
		}

		BaseLib::Flows::NodeFactory* (*getFactory)();
		getFactory = (BaseLib::Flows::NodeFactory* (*)())dlsym(nodeHandle, "getFactory");
		if(!getFactory)
		{
			GD::out.printCritical("Critical: Could not open node \"" + _filename + "\". Symbol \"getFactory\" not found.");
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
	try
	{
		GD::out.printInfo("Info: Disposing node " + _filename);
		GD::out.printDebug("Debug: Deleting factory pointer of node " + _filename);
		if(_factory) delete _factory.release();
		GD::out.printDebug("Debug: Closing dynamic library module " + _filename);
		if(_handle) dlclose(_handle);
		_handle = nullptr;
		GD::out.printDebug("Debug: Dynamic library " + _filename + " disposed");
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

std::unique_ptr<BaseLib::Flows::INode> NodeLoader::createNode()
{
	if (!_factory) return std::unique_ptr<BaseLib::Flows::INode>();
	return std::unique_ptr<BaseLib::Flows::INode>(_factory->createNode(GD::bl.get(), _filename));
}

NodeManager::NodeManager()
{
}

NodeManager::~NodeManager()
{
	std::lock_guard<std::mutex> nodesGuard(_nodesNameNodeMapMutex);
	for(auto& node : _nodesNameNodeMap)
	{
		if(!node.second) continue;
		node.second.reset();
	}
	_nodesNameNodeMap.clear();
	_nodeLoaders.clear();
}

std::shared_ptr<BaseLib::Flows::INode> NodeManager::getNode(std::string name)
{
	try
	{
		BaseLib::Flows::PINode node;
		std::lock_guard<std::mutex> nodesGuard(_nodesNameNodeMapMutex);
		auto nodeIterator = _nodesNameNodeMap.find(name);
		if (nodeIterator != _nodesNameNodeMap.end()) node = nodeIterator->second;
		if (node && !node->locked()) return node;
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

int32_t NodeManager::loadNode(std::string name, BaseLib::Flows::PINode& node)
{
	try
	{
		std::lock_guard<std::mutex> nodesGuard(_nodesNameNodeMapMutex);
		{
			auto nodesIterator = _nodesNameNodeMap.find(name);
			if(nodesIterator != _nodesNameNodeMap.end())
			{
				nodesIterator->second->incrementReferenceCounter();
				node = nodesIterator->second;
				return 1;
			}
		}

		std::string path(GD::bl->settings.flowsPath() + "nodes/" + name + "/");
		if(BaseLib::Io::fileExists(path + name + ".so")) //C++ module
		{
			std::string filename = name + ".so";
			path = path + name + ".so";

			std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
			if(_nodeLoaders.find(filename) != _nodeLoaders.end()) return 1;
			_nodeLoaders.emplace(filename, std::unique_ptr<NodeLoader>(new NodeLoader(filename, path)));

			node = _nodeLoaders.at(filename)->createNode();

			if(node)
			{
				node->incrementReferenceCounter();
				_nodesNameNodeMap.emplace(name, node);
			}
			else
			{
				_nodeLoaders.erase(filename);
				return -3;
			}
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + name + ".hgn")) //Encrypted PHP
		{
			node = std::make_shared<PhpNode>(path + name + ".hgn");
			node->incrementReferenceCounter();
			_nodesNameNodeMap.emplace(name, node);
			return 0;
		}
		else if(BaseLib::Io::fileExists(path + name + ".php")) //Unencrypted PHP
		{
			node = std::make_shared<PhpNode>(path + name + ".php");
			node->incrementReferenceCounter();
			_nodesNameNodeMap.emplace(name, node);
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

int32_t NodeManager::unloadNode(std::string name)
{
	try
	{
		std::lock_guard<std::mutex> nodesGuard(_nodesNameNodeMapMutex);
		std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);

		auto nodesIterator = _nodesNameNodeMap.find(name);
		if(nodesIterator == _nodesNameNodeMap.end()) return 1;
		nodesIterator->second->decrementReferenceCounter();
		if(nodesIterator->second->getReferenceCounter() > 0) return 0;

		std::string path = nodesIterator->second->getFilename();

		if(!BaseLib::Io::fileExists(path)) return -2;

		auto nodeLoaderIterator = _nodeLoaders.find(nodesIterator->second->getFilename());
		if(nodeLoaderIterator != _nodeLoaders.end()) return 1;

		BaseLib::Flows::PINode node = nodesIterator->second;

		node->lock();
		while(node.use_count() > 1)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		_nodesNameNodeMap.erase(nodesIterator);
		node.reset();

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
