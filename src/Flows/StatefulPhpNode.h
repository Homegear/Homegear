/* Copyright 2013-2017 Sathya Laufer
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

#ifndef STATEFULPHPNODE_H_
#define STATEFULPHPNODE_H_

#include <homegear-node/INode.h>

class StatefulPhpNode : public Flows::INode
{
private:
	Flows::PVariable _nodeInfo;
public:
	StatefulPhpNode(std::string path, std::string nodeNamespace, std::string type, const std::atomic_bool* frontendConnected);
	virtual ~StatefulPhpNode();

	virtual bool init(Flows::PNodeInfo nodeInfo);
	virtual bool start();
	virtual void stop();
	virtual void waitForStop();

	virtual void configNodesStarted();

	virtual void variableEvent(uint64_t peerId, int32_t channel, std::string variable, Flows::PVariable value);
	virtual void setNodeVariable(std::string& variable, Flows::PVariable& value);

	virtual Flows::PVariable getConfigParameterIncoming(std::string name);

	virtual void input(Flows::PNodeInfo nodeInfo, uint32_t index, Flows::PVariable message);

	virtual Flows::PVariable invokeLocal(std::string methodName, Flows::PArray& parameters);
};

#endif
