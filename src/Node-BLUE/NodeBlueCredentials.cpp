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

#include "NodeBlueCredentials.h"
#include "../GD/GD.h"

namespace Homegear {
namespace NodeBlue {

NodeBlueCredentials::NodeBlueCredentials() {
  { //Load credential key
    if (!BaseLib::Io::fileExists(GD::configPath + "nodeBlueCredentialKey.txt")) {
      GD::bl->out.printError("Error: File \"" + GD::configPath + "nodeBlueCredentialKey.txt\" does not exist. Please create it and place a long random passphrase in it.");
      throw NodeBlueCredentialsException("Credentials key file does not exist.");
    }
    auto content = BaseLib::Io::getFileContent(GD::configPath + "nodeBlueCredentialKey.txt");
    std::fill(content.begin(), content.end(), 0);
    BaseLib::HelperFunctions::trim(content);
    std::vector<uint8_t> dataIn(content.begin(), content.end());
    if (dataIn.size() < 14) {
      GD::bl->out.printError("Error: The passphrase in \"" + GD::configPath + "nodeBlueCredentialKey.txt\" is too short. The minimum length is 14 characters. Recommended are 43 characters.");
      throw NodeBlueCredentialsException("Passphrase is too short.");
    } else if (dataIn.size() < 43) {
      GD::bl->out.printWarning("Warning: The passphrase in \"" + GD::configPath + "nodeBlueCredentialKey.txt\" is short. We recommend using 43 characters.");
    }

    BaseLib::Security::Hash::sha256(dataIn, _credentialKey);
    std::fill(dataIn.begin(), dataIn.end(), 0);
    if (_credentialKey.empty()) {
      GD::bl->out.printError("Error hashing credential key.");
      throw NodeBlueCredentialsException("Error hashing credential key.");
    }
  }
}

void NodeBlueCredentials::processFlowCredentials(BaseLib::PVariable &flowsJson) {
  try {
    for (auto &node : *flowsJson->arrayValue) {
      auto credentialsIterator = node->structValue->find("credentials");
      if (credentialsIterator != node->structValue->end()) {
        auto credentials = credentialsIterator->second;
        node->structValue->erase(credentialsIterator);
        auto nodeId = node->structValue->find("id");
        if (nodeId == node->structValue->end() || nodeId->second->stringValue.empty()) continue;

        setCredentials(nodeId->second->stringValue, credentials);
      }
    }
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

BaseLib::PVariable NodeBlueCredentials::getCredentials(const std::string &nodeId) {
  try {
    auto nodeData = GD::bl->db->getNodeData(nodeId, "credentials");
    if (!nodeData || nodeData->stringValue.empty()) return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

    auto dataPair = BaseLib::HelperFunctions::splitFirst(nodeData->stringValue, '.');

    BaseLib::Security::Gcrypt aes(GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
    aes.setKey(_credentialKey);
    aes.setIv(BaseLib::HelperFunctions::getUBinary(dataPair.first));
    std::vector<char> plaintext;
    aes.decrypt(plaintext, BaseLib::HelperFunctions::getBinary(dataPair.second));

    return BaseLib::Rpc::JsonDecoder::decode(plaintext);
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
}

BaseLib::PVariable NodeBlueCredentials::setCredentials(const std::string &nodeId, const BaseLib::PVariable &credentials) {
  try {
    std::string credentialString;
    BaseLib::Rpc::JsonEncoder::encode(credentials, credentialString);
    std::vector<uint8_t> plaintext(credentialString.begin(), credentialString.end());

    BaseLib::Security::Gcrypt aes(GCRY_CIPHER_AES256, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
    std::vector<uint8_t> iv;
    iv.resize(12);
    BaseLib::HelperFunctions::memcpyBigEndian(iv, BaseLib::HelperFunctions::getTimeMicroseconds());
    auto randomBytes = BaseLib::HelperFunctions::getRandomBytes(4);
    std::copy(randomBytes.begin(), randomBytes.end(), iv.begin() + 8);

    aes.setKey(_credentialKey);
    aes.setIv(iv);
    std::vector<uint8_t> ciphertext;
    aes.encrypt(ciphertext, plaintext);

    auto nodeData = std::make_shared<BaseLib::Variable>(BaseLib::HelperFunctions::getHexString(iv) + "." + BaseLib::HelperFunctions::getHexString(ciphertext));
    GD::bl->db->setNodeData(nodeId, "credentials", nodeData);

    return std::make_shared<BaseLib::Variable>();
  } catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

}
}