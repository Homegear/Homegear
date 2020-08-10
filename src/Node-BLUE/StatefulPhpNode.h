/* Copyright 2013-2020 Homegear GmbH
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

#ifndef NO_SCRIPTENGINE

#include <homegear-node/INode.h>

namespace Homegear {

class StatefulPhpNode : public Flows::INode {
 private:
  Flows::PVariable _nodeInfo;
 public:
  StatefulPhpNode(const std::string &path, const std::string &nodeNamespace, const std::string &type, const std::atomic_bool *frontendConnected);

  ~StatefulPhpNode() override;

  bool init(const Flows::PNodeInfo &nodeInfo) override;

  bool start() override;

  void stop() override;

  void waitForStop() override;

  void configNodesStarted() override;

  void startUpComplete() override;

  void variableEvent(const std::string &source, uint64_t peerId, int32_t channel, const std::string &variable, const Flows::PVariable &value, const Flows::PVariable &metadata) override;

  void setNodeVariable(const std::string &variable, const Flows::PVariable &value) override;

  Flows::PVariable getConfigParameterIncoming(const std::string &name) override;

  void input(const Flows::PNodeInfo &nodeInfo, uint32_t index, const Flows::PVariable &message) override;

  Flows::PVariable invokeLocal(const std::string &methodName, const Flows::PArray &parameters) override;
};

}

#endif
#endif
