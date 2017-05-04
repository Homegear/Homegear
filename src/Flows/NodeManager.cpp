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
		if
(		!getNodeName)
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
		GD::out.printInfo("Info: Disposing family module " + _name);
		GD::out.printDebug("Debug: Deleting factory pointer of module " + _name);
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

std::vector<std::shared_ptr<NodeManager::NodeInfo>> NodeManager::getNodeInfo()
{
	std::vector<std::shared_ptr<NodeManager::NodeInfo>> nodeInfoVector;
	try
	{
		std::vector<std::string> files = GD::bl->io.getFiles(GD::bl->settings.flowsPath() + "nodes/");
		if (files.empty()) return nodeInfoVector;
		for(std::vector<std::string>::iterator i = files.begin(); i != files.end(); ++i)
		{
			if(i->size() < 9) continue; //mod_?*.so
			std::string prefix = i->substr(0, 4);
			std::string extension = i->substr(i->size() - 3, 3);
			if (extension != ".so" || prefix != "node_") continue;
			std::shared_ptr<NodeManager::NodeInfo> nodeInfo(new NodeManager::NodeInfo());
			{
				std::lock_guard<std::mutex> nodeLoadersGuard(_nodeLoadersMutex);
				std::map<std::string, std::unique_ptr<NodeLoader>>::const_iterator nodeIterator = _nodeLoaders.find(*i);
				if (nodeIterator != _nodeLoaders.end() && nodeIterator->second)
				{
					nodeInfo->baselibVersion = nodeIterator->second->getVersion();
					nodeInfo->loaded = true;
					nodeInfo->filename = nodeIterator->first;
					nodeInfo->nodeName = nodeIterator->second->getNodeName();
					nodeInfoVector.push_back(nodeInfo);
					continue;
				}
			}
			std::string path(GD::bl->settings.flowsPath() + "nodes/" + *i);
			std::unique_ptr<NodeLoader> nodeLoader(new NodeLoader(*i, path));
			nodeInfo->baselibVersion = nodeLoader->getVersion();
			nodeInfo->loaded = false;
			nodeInfo->filename = *i;
			nodeInfo->nodeName = nodeLoader->getNodeName();
			nodeInfoVector.push_back(nodeInfo);
			nodeLoader->dispose();
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

void NodeManager::load()
{
	try
	{
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
