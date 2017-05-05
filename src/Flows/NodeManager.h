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

#ifndef NODEMANAGER_H_
#define NODEMANAGER_H_

#include <homegear-base/BaseLib.h>

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include <dlfcn.h>

class NodeLoader
{
public:
	NodeLoader(std::string name, std::string path);
	virtual ~NodeLoader();
	void dispose();
	std::string getNodeName();
	std::string getVersion();

	std::unique_ptr<BaseLib::Flows::INode> createNode();
private:
	bool _disposing = false;
	std::string _name;
	std::string _nodeName;
	std::string _version;
	void* _handle = nullptr;
	std::unique_ptr<BaseLib::Flows::NodeFactory> _factory;

	NodeLoader(const NodeLoader&) = delete;
	NodeLoader& operator=(const NodeLoader&) = delete;
};

class NodeManager
{
public:
	struct NodeInfo
	{
		std::string filename;
		std::string baselibVersion;
		std::string nodeName;
		bool loaded;
	};

	NodeManager();
	virtual ~NodeManager();
	void disposeNodes();
	void dispose();

	/**
	 * Returns a vector of type NodeInfo with information about all loaded and not loaded nodes.
	 * @return Returns a vector of type NodeInfo.
	 */
	std::vector<std::shared_ptr<NodeInfo>> getNodeInfo();

	/**
	 * Loads a node. The node needs to be in Homegear's node path.
	 * @param filename The filename of the node (e. g. node_variable.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, 1: Node already loaded, -1: System error, -2: Node does not exists, -4: Node initialization failed
	 */
	int32_t loadNode(std::string filename);

	/**
	 * Unloads a previously loaded node.
	 * @param filename The filename of the node (e. g. node_variable.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, 1: Node not loaded, -1: System error, -2: Node does not exists
	 */
	int32_t unloadNode(std::string filename);

	/**
	 * Unloads and loads a node again. The node needs to be in Homegear's node path.
	 *
	 * @param filename The filename of the node (e. g. node_variable.so).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, -1: System error, -2: Node does not exists, -4: Node initialization failed
	 */
	int32_t reloadNode(std::string filename);

	/*
	 * Returns the node specified by name.
	 */
	std::shared_ptr<BaseLib::Flows::INode> getNode(std::string name);

	/*
	 * Returns the node map.
	 */
	std::map<std::string, std::shared_ptr<BaseLib::Flows::INode>> getNodes();

	void loadNodes();
private:
	std::mutex _nodeLoadersMutex;
	std::map<std::string, std::unique_ptr<NodeLoader>> _nodeLoaders;

	std::mutex _nodesIdNodeMapMutex;
	typedef std::string NodeId; //Node ID from Node-BLUE
	std::unordered_map<NodeId, std::shared_ptr<BaseLib::Flows::INode>> _nodesIdNodeMap;

	std::mutex _nodesNameIdMapMutex;
	typedef std::string NodeName; //Node name from Homegear
	std::unordered_map<NodeName, std::set<NodeId>> _nodesNameIdMap;

	std::mutex _nodesNameFilenameMapMutex;
	typedef std::string NodeFilename;
	std::unordered_map<NodeName, NodeFilename> _nodesNameFilenameMap;

	NodeManager(const NodeManager&) = delete;
	NodeManager& operator=(const NodeManager&) = delete;
};

#endif
