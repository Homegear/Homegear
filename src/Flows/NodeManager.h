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
#include <homegear-node/INode.h>
#include <homegear-node/NodeFactory.h>

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include <dlfcn.h>

class NodeLoader
{
public:
	NodeLoader(std::string filename, std::string path);
	virtual ~NodeLoader();

	Flows::PINode createNode(const std::atomic_bool* nodeEventsEnabled);
private:
	std::string _filename;
	std::string _path;
	std::string _name;
	void* _handle = nullptr;
	std::unique_ptr<Flows::NodeFactory> _factory;

	NodeLoader(const NodeLoader&) = delete;
	NodeLoader& operator=(const NodeLoader&) = delete;
};

class NodeManager
{
public:
	struct NodeInfo
	{
		std::string filename;
		std::string fullPath;
		std::string nodeId;
		std::string nodeName;
		std::string readableName;
		std::string version;
		int32_t maxThreadCount;
		std::string frontendListEntry;
		std::string frontendCode;
	};
	typedef std::shared_ptr<NodeInfo> PNodeInfo;

	struct NodeUsageInfo
	{
		std::atomic_bool locked;
		std::atomic_int referenceCounter;
	};
	typedef std::shared_ptr<NodeUsageInfo> PNodeUsageInfo;

	NodeManager(const std::atomic_bool* nodeEventsEnabled);
	virtual ~NodeManager();

	/**
	 * Returns a vector of type NodeInfo with information about all nodes.
	 * @return Returns a vector of type NodeInfo.
	 */
	static std::vector<PNodeInfo> getNodeInfo();

	static std::string getNodeLocales(std::string& language);

	/**
	 * Loads a node. The node needs to be in Homegear's node path.
	 * @param name The name of the node (e. g. variable).
	 * @param id The id of the node (e. g. 142947a.387ef34ad)
	 * @param[out] node If loading was successful, this variable contains the loaded node.
	 * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, 1: Node already loaded, -1: System error, -2: Node does not exists, -4: Node initialization failed
	 */
	int32_t loadNode(std::string nodeNamespace, std::string name, std::string id, Flows::PINode& node);

	/**
	 * Unloads a previously loaded node.
	 * @param id The id of the node (e. g. 142947a.387ef34ad).
	 * @return Returns positive values or 0 on success and negative values on error. 0: Node successfully loaded, 1: Node not loaded, -1: System error, -2: Node does not exists
	 */
	int32_t unloadNode(std::string id);

	/*
	 * Returns the node specified by id.
	 */
	Flows::PINode getNode(std::string& id);
private:
	std::mutex _nodeLoadersMutex;
	std::map<std::string, std::unique_ptr<NodeLoader>> _nodeLoaders;

	typedef std::string NodeId; //Node ID from Homegear
	typedef std::string NodeName; //Node name from Homegear

	std::mutex _nodesMutex;
	std::unordered_map<NodeId, Flows::PINode> _nodes;
	std::unordered_map<NodeName, PNodeUsageInfo> _nodesUsage;

	const std::atomic_bool* _nodeEventsEnabled;

	NodeManager(const NodeManager&) = delete;
	NodeManager& operator=(const NodeManager&) = delete;
};

#endif
