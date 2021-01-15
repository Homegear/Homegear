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

#ifndef UICONTROLLER_H_
#define UICONTROLLER_H_

#include <homegear-base/BaseLib.h>
#include <homegear-base/DeviceDescription/UI/UiElements.h>

namespace Homegear {

class UiController {
 public:
  struct UiElement {
    uint64_t databaseId = 0;
    std::string elementId;
    std::string label;
    BaseLib::PVariable data;
    BaseLib::PVariable metadata;
    uint64_t roomId = 0;
    std::unordered_set<uint64_t> categoryIds;
    std::string nodeId;
    BaseLib::DeviceDescription::UiElements::PUiPeerInfo peerInfo = std::make_shared<BaseLib::DeviceDescription::UiElements::UiPeerInfo>();
    std::unordered_map<std::string, BaseLib::DeviceDescription::PHomegearUiElement> rpcElement;
  };
  typedef std::shared_ptr<UiElement> PUiElement;

  UiController();

  virtual ~UiController();

  void load();

  std::unordered_map<std::string, std::unordered_set<PUiElement>> getAllNodeUiElements();

  std::unordered_set<uint64_t> getUiElementsWithVariable(uint64_t peerId, int32_t channel, const std::string &variableName);

  void removeNodeUiElements(const std::string &nodeId);

  BaseLib::PVariable addUiElement(BaseLib::PRpcClientInfo clientInfo, const std::string &elementId, const BaseLib::PVariable &data, const BaseLib::PVariable &metadata);

  BaseLib::PVariable addUiElementSimple(const BaseLib::PRpcClientInfo &clientInfo, const std::string &label, const BaseLib::PVariable &variable, bool dryRun);

  BaseLib::PVariable getAllUiElements(const BaseLib::PRpcClientInfo &clientInfo, const std::string &language);

  BaseLib::PVariable getAvailableUiElements(const BaseLib::PRpcClientInfo &clientInfo, const std::string &language);

  BaseLib::PVariable getUiElement(const BaseLib::PRpcClientInfo &clientInfo, uint64_t databaseId, const std::string &language);

  BaseLib::PVariable getUiElementMetadata(const BaseLib::PRpcClientInfo &clientInfo, uint64_t databaseId);

  BaseLib::PVariable getUiElementsInRoom(const BaseLib::PRpcClientInfo &clientInfo, uint64_t roomId, const std::string &language);

  BaseLib::PVariable getUiElementsInCategory(const BaseLib::PRpcClientInfo &clientInfo, uint64_t categoryId, const std::string &language);

  BaseLib::PVariable getUiElementTemplate(const BaseLib::PRpcClientInfo &clientInfo, const std::string &elementId, const std::string &language);

  static BaseLib::PVariable requestUiRefresh(const std::string &id);

  BaseLib::PVariable removeUiElement(uint64_t databaseId);

  BaseLib::PVariable setUiElementMetadata(const BaseLib::PRpcClientInfo &clientInfo, uint64_t databaseId, const BaseLib::PVariable &metadata);
 protected:
  std::unique_ptr<BaseLib::Rpc::RpcDecoder> _rpcDecoder;
  std::unique_ptr<BaseLib::DeviceDescription::UiElements> _descriptions;

  std::mutex _uiElementsMutex;
  std::unordered_map<uint64_t, PUiElement> _uiElements;
  std::unordered_map<uint64_t, std::unordered_set<PUiElement>> _uiElementsByRoom;
  std::unordered_map<uint64_t, std::unordered_set<PUiElement>> _uiElementsByCategory;
  std::unordered_map<std::string, std::unordered_set<PUiElement>> _uiElementsByNode;

  static void addDataInfo(PUiElement &uiElement, const BaseLib::PVariable &data);

  static void addVariableInfo(const BaseLib::PRpcClientInfo &clientInfo, const PUiElement &uiElement, BaseLib::PArray &variables, bool addValue);

  BaseLib::PVariable findRoleVariables(const BaseLib::PRpcClientInfo &clientInfo, const BaseLib::PVariable &uiInfo, const BaseLib::PVariable &variable, uint64_t roomId, BaseLib::PVariable &inputPeers, BaseLib::PVariable &outputPeers);

  static bool checkElementAccess(const BaseLib::PRpcClientInfo &clientInfo, const PUiElement &uiElement, const BaseLib::DeviceDescription::PHomegearUiElement &rpcElement);
};

}

#endif
