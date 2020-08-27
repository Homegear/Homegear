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

#ifndef HOMEGEAR_FLOWPARSER_H
#define HOMEGEAR_FLOWPARSER_H

#include <homegear-base/BaseLib.h>

namespace Homegear {
namespace NodeBlue {

class FlowParser {
 public:
  static const size_t nodeHeight;
  static const size_t nodeConnectorHeight;

  FlowParser() = delete;

  /**
   * Generates a random ID as used in nodes or flows.
   * @param existingIds Optional set with existing IDs. The method makes sure the new ID is not one of the existing ones.
   *
   * @return The generated random ID.
   */
  static std::string generateRandomId(const std::unordered_set<std::string> &existingIds = std::unordered_set<std::string>());

  /**
   * Adds nodes to a flow.
   *
   * @param flow The flow to add the nodes to.
   * @param tab The tab to add the nodes to. If the tab doesn't exist, it is created.
   * @param tag A tag that is added to the nodes added by this method. If specified, all nodes with this tag are deleted before the new nodes are added.
   * @param nodes The nodes to add.
   * @return Returns the flow with the new nodes on success or a null pointer when there is no change.
   */
  static BaseLib::PVariable addNodesToFlow(const BaseLib::PVariable &flow, const std::string &tab, const std::string &tag, const BaseLib::PVariable &nodes);

  /**
   * Removes tagged nodes from a flow
   *
   * @param flow The flow to remove the nodes from.
   * @param tab The tab where to search for "tag".
   * @param tag The tag to remove.
   * @return Returns the flow with the nodes removed on success or a null pointer when there is no change.
   */
  static BaseLib::PVariable removeNodesFromFlow(const BaseLib::PVariable &flow, const std::string &tab, const std::string &tag);

  /**
   * Checks if a flow contains nodes with the specified tag.
   *
   * @param flow The flow to search for nodes with "tag"
   * @param tab The tab to seach the nodes in.
   * @param tag The tag to search for.
   * @return Returns true when at least one node with tag was found and false otherwise.
   */
  static bool flowHasTag(const BaseLib::PVariable &flow, const std::string &tab, const std::string &tag);
};

}
}

#endif //HOMEGEAR_FLOWPARSER_H
