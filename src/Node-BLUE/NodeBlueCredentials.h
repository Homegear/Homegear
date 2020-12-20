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

#ifndef HOMEGEAR_SRC_NODE_BLUE_NODEBLUECREDENTIALS_H_
#define HOMEGEAR_SRC_NODE_BLUE_NODEBLUECREDENTIALS_H_

#include <homegear-base/BaseLib.h>

namespace Homegear {
namespace NodeBlue {

class NodeBlueCredentialsException : public BaseLib::Exception {
 public:
  explicit NodeBlueCredentialsException(const std::string &message) : BaseLib::Exception(message) {}
};

class NodeBlueCredentials {
 private:
  std::vector<uint8_t> _credentialKey;
 public:
  NodeBlueCredentials();
  ~NodeBlueCredentials() = default;

  /**
   * Removes credentials from nodes, encrypts them and places them in Homegear's database.
   *
   * @param flowsJson The flows to process.
   */
  void processFlowCredentials(BaseLib::PVariable &flowsJson);

  /**
   * Reads credentials from database, decrypts it and returns it as a Struct.
   *
   * @param nodeId The node to get credentials for.
   * @return The decrypted credentials as a Struct. On error an empty Struct is returned so there is no need to check for a nullptr.
   */
  BaseLib::PVariable getCredentials(const std::string &nodeId);

  /**
   * Encrypts credentials and writes them to database database.
   *
   * @param nodeId The node to set credentials for.
   * @return Returns void on success.
   */
  BaseLib::PVariable setCredentials(const std::string &nodeId, const BaseLib::PVariable &credentials);
};

}
}

#endif //HOMEGEAR_SRC_NODE_BLUE_NODEBLUECREDENTIALS_H_
