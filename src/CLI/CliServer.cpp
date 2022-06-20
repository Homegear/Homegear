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

#include "CliServer.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>
#include <homegear-base/Security/Acl.h>

namespace Homegear {

CliServer::CliServer(int32_t clientId) {
  _clientId = clientId;
  _dummyClientInfo = std::make_shared<BaseLib::RpcClientInfo>();
  _dummyClientInfo->initInterfaceId = "cliServer";
  _dummyClientInfo->ipcServer = true;
  _dummyClientInfo->acls = std::make_shared<BaseLib::Security::Acls>(GD::bl.get(), -1);
  std::vector<uint64_t> groups{3};
  _dummyClientInfo->acls->fromGroups(groups);
  _dummyClientInfo->user = "SYSTEM (3)";
}

CliServer::~CliServer() {
}

std::string CliServer::userCommand(std::string &command) {
  try {
    std::ostringstream stringStream;
    std::vector<std::string> arguments;
    bool showHelp = false;
    if (BaseLib::HelperFunctions::checkCliCommand(command, "users help", "uh", "", 0, arguments, showHelp)) {
      stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
      stringStream << "For more information about the individual command type: COMMAND help" << std::endl
                   << std::endl;
      stringStream << "groups list (gl)     Lists all groups." << std::endl;
      stringStream << "groups restore       Factory resets all system groups." << std::endl;
      stringStream << "users list (ul)      Lists all users." << std::endl;
      stringStream << "users create (uc)    Create a new user." << std::endl;
      stringStream << "users update (uu)    Change the password of an existing user." << std::endl;
      stringStream << "users delete (ud)    Delete an existing user." << std::endl;
      return stringStream.str();
    }
    if (BaseLib::HelperFunctions::checkCliCommand(command, "groups list", "gl", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command lists all known groups." << std::endl;
        stringStream << "Usage: groups list" << std::endl << std::endl;
        return stringStream.str();
      }

      auto groups = GD::bl->db->getGroups("en");
      if (groups->arrayValue->empty()) groups = GD::bl->db->getGroups("en-US"); //Backwards compatibility
      for (auto &group: *groups->arrayValue) {
        auto nameIterator = group->structValue->find("NAME");
        if (nameIterator == group->structValue->end()) continue;

        auto idIterator = group->structValue->find("ID");
        if (idIterator == group->structValue->end()) continue;

        auto aclIterator = group->structValue->find("ACL");
        if (aclIterator == group->structValue->end()) continue;

        BaseLib::Security::Acl acl;
        acl.fromVariable(aclIterator->second);

        stringStream << nameIterator->second->stringValue << ":" << std::endl;
        stringStream << "  ID: " << (uint64_t)idIterator->second->integerValue64 << std::endl;
        stringStream << "  ACL: " << std::endl;
        stringStream << acl.toString(4) << std::endl;
      }

      return stringStream.str();
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "groups restore", "", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command factory resets all system groups." << std::endl;
        stringStream << "Usage: groups restore" << std::endl << std::endl;
        return stringStream.str();
      }

      BaseLib::PVariable denyAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      denyAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(false));

      BaseLib::PVariable grantAll = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      grantAll->structValue->emplace("*", std::make_shared<BaseLib::Variable>(true));

      auto grantStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      grantStruct->structValue->emplace("methods", grantAll);
      grantStruct->structValue->emplace("eventServerMethods", grantAll);
      grantStruct->structValue->emplace("services", grantAll);

      auto denyStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      denyStruct->structValue->emplace("methods", denyAll);
      denyStruct->structValue->emplace("eventServerMethods", denyAll);
      denyStruct->structValue->emplace("services", denyAll);

      bool error = false;
      for (int32_t i = 1; i <= 8; i++) {
        auto aclStruct = GD::bl->db->getAcl(i);
        if (!aclStruct || aclStruct->errorStruct) {
          error = true;
          GD::out.printError("Error getting system group " + std::to_string(i) + ". It seems, your database is broken.");
        } else {
          GD::bl->db->updateGroup(i, BaseLib::PVariable(), grantStruct);
        }
      }

      {
        auto aclStruct = GD::bl->db->getAcl(9);
        if (aclStruct && !aclStruct->errorStruct) {
          GD::bl->db->updateGroup(9, BaseLib::PVariable(), denyStruct);
        } else {
          error = true;
          GD::out.printError("Error getting system group 9. It seems, your database is broken.");
        }
      }

      if (error)
        stringStream
            << "Error reading one or more system groups from database. It seems, your database is broken."
            << std::endl;
      else stringStream << "System groups successfully restored." << std::endl;

      return stringStream.str();
    } else if (command.compare(0, 10, "users list") == 0 || command.compare(0, 2, "ul") == 0 || command.compare(0, 2, "ls") == 0) {
      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'l') ? 0 : 1;
      int32_t index = 0;
      bool printHelp = false;
      while (std::getline(stream, element, ' ')) {
        if (index < 1 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset && element == "help") printHelp = true;
        else {
          printHelp = true;
          break;
        }
        index++;
      }
      if (printHelp) {
        stringStream << "Description: This command lists all known users." << std::endl;
        stringStream << "Usage: users list" << std::endl << std::endl;
        return stringStream.str();
      }

      std::map<uint64_t, User::UserInfo> users;
      User::getAll(users);
      if (users.size() == 0) return "No users exist.\n";

      stringStream << std::left << std::setfill(' ') << std::setw(6) << "ID" << std::setw(100) << "Name"
                   << "Groups" << std::endl;
      for (auto &user: users) {
        stringStream << std::setw(6) << user.first << std::setw(100) << user.second.name;
        for (auto &group: user.second.groups) {
          stringStream << group << " ";
        }
        stringStream << std::endl;
      }

      return stringStream.str();
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "users create", "uc", "", 3, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command creates a new user." << std::endl;
        stringStream << "Usage: users create USERNAME \"PASSWORD\" \"GROUPS\"" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream
            << "  USERNAME: The user name of the new user to create. It may contain alphanumeric characters, \"_\""
            << std::endl;
        stringStream << "            and \"-\". Example: foo" << std::endl;
        stringStream
            << "  PASSWORD: The password for the new user. All characters are allowed. If the password contains spaces,"
            << std::endl;
        stringStream << "            wrap it in double quotes." << std::endl;
        stringStream << "            Example: MyPassword" << std::endl;
        stringStream
            << "            Example with spaces and escape characters: \"My_\\\\\\\"Password\" (Translates to: My_\\\"Password)"
            << std::endl;
        stringStream
            << "  GROUPS:   Comma seperated list of group IDs that should be assigned to the user. The list needs to be wrapped in double quotes."
            << std::endl;
        stringStream << "            Example: \"1,7\"" << std::endl;
        return stringStream.str();
      }

      std::string userName = BaseLib::HelperFunctions::toLower(BaseLib::HelperFunctions::trim(arguments.at(0)));
      if (userName.empty() || userName == "\"\"") {
        stringStream << "No user name supplied." << std::endl;
        return stringStream.str();
      }

      if (userName.size() > 2 && userName.front() == '"' && userName.back() == '"') {
        userName = userName.substr(1, userName.size() - 2);
        BaseLib::HelperFunctions::stringReplace(userName, "\\\"", "\"");
        BaseLib::HelperFunctions::stringReplace(userName, "\\\\", "\\");
      }

      std::string password = BaseLib::HelperFunctions::trim(arguments.at(1));
      if (password.size() > 2 && password.front() == '"' && password.back() == '"') {
        password = password.substr(1, password.size() - 2);
        BaseLib::HelperFunctions::stringReplace(password, "\\\"", "\"");
        BaseLib::HelperFunctions::stringReplace(password, "\\\\", "\\");
      }
      if (!password.empty() && password != "\"\"" && password.size() < 8) {
        stringStream << "The password is too short. Please choose a password with at least 8 characters."
                     << std::endl;
        return stringStream.str();
      }

      std::vector<uint64_t> groups;
      std::string groupString = arguments.at(2);
      if (groupString.front() == '"' && groupString.back() == '"') {
        groupString = groupString.substr(1, groupString.size() - 2);
        std::vector<std::string> splitGroupString = BaseLib::HelperFunctions::splitAll(groupString, ',');
        groups.reserve(splitGroupString.size());
        for (auto &element: splitGroupString) {
          BaseLib::HelperFunctions::trim(element);
          uint64_t id = BaseLib::Math::getNumber64(element, false);
          if (id != 0) groups.push_back(id);
        }
      }

      if (groups.empty()) return "No groups specified.\n";

      if (User::exists(userName)) return "A user with that name already exists.\n";

      if (User::create(userName, password, groups)) stringStream << "User successfully created." << std::endl;
      else stringStream << "Error creating user. See log for more details." << std::endl;

      return stringStream.str();
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "users update", "uu", "", 2, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command sets a new password for an existing user." << std::endl;
        stringStream << "Usage: users update USERNAME \"PASSWORD\" [\"GROUPS\"]" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  USERNAME: The user name of an existing user. Example: foo" << std::endl;
        stringStream
            << "  PASSWORD: The new password for the user. All characters are allowed. If the password contains spaces,"
            << std::endl;
        stringStream << "            wrap it in double quotes." << std::endl;
        stringStream << "            Example: MyPassword" << std::endl;
        stringStream
            << "            Example with spaces and escape characters: \"My_\\\\\\\"Password\" (Translates to: My_\\\"Password)"
            << std::endl;
        stringStream
            << "  GROUPS:   Comma seperated list of group IDs that should be assigned to the user. The list needs to be wrapped in double quotes. If not specified, the groups stay unchanged."
            << std::endl;
        stringStream << "            Example: \"1,7\"" << std::endl;
        return stringStream.str();
      }

      std::string userName = BaseLib::HelperFunctions::toLower(BaseLib::HelperFunctions::trim(arguments.at(0)));
      if (userName.empty() || !BaseLib::HelperFunctions::isAlphaNumeric(userName)) {
        stringStream
            << "The user name contains invalid characters. Only alphanumeric characters, \"_\" and \"-\" are allowed."
            << std::endl;
        return stringStream.str();
      }

      std::string password = BaseLib::HelperFunctions::trim(arguments.at(1));
      if (password.size() > 2 && password.front() == '"' && password.back() == '"') {
        password = password.substr(1, password.size() - 2);
        BaseLib::HelperFunctions::stringReplace(password, "\\\"", "\"");
        BaseLib::HelperFunctions::stringReplace(password, "\\\\", "\\");
      }
      if (!password.empty() && password.size() < 8) {
        stringStream << "The password is too short. Please choose a password with at least 8 characters."
                     << std::endl;
        return stringStream.str();
      }

      std::vector<uint64_t> groups;
      if (arguments.size() > 2) {
        std::string groupString = arguments.at(2);
        if (groupString.front() == '"' && groupString.back() == '"') {
          groupString = groupString.substr(1, groupString.size() - 2);
          std::vector<std::string> splitGroupString = BaseLib::HelperFunctions::splitAll(groupString, ',');
          groups.reserve(splitGroupString.size());
          for (auto &element: splitGroupString) {
            BaseLib::HelperFunctions::trim(element);
            uint64_t id = BaseLib::Math::getNumber64(element, false);
            if (id != 0) groups.push_back(id);
          }
        }
      }

      if (!User::exists(userName)) return "The user doesn't exist.\n";

      if (User::update(userName, password, groups)) stringStream << "User successfully updated." << std::endl;
      else stringStream << "Error updating user. See log for more details." << std::endl;

      return stringStream.str();
    } else if (command.compare(0, 12, "users delete") == 0 || command.compare(0, 2, "ud") == 0) {
      std::string userName;

      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'd') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index < 1 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset) {
          if (element == "help") break;
          else {
            userName = BaseLib::HelperFunctions::trim(element);
            if (userName.empty() || !BaseLib::HelperFunctions::isAlphaNumeric(userName)) {
              stringStream
                  << "The user name contains invalid characters. Only alphanumeric characters, \"_\" and \"-\" are allowed."
                  << std::endl;
              return stringStream.str();
            }
          }
        }
        index++;
      }
      if (index == 1 + offset) {
        stringStream << "Description: This command deletes an existing user." << std::endl;
        stringStream << "Usage: users delete USERNAME" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  USERNAME:\tThe user name of the user to delete. Example: foo" << std::endl;
        return stringStream.str();
      }

      if (!User::exists(userName)) return "The user doesn't exist.\n";

      if (User::remove(userName)) stringStream << "User successfully deleted." << std::endl;
      else stringStream << "Error deleting user. See log for more details." << std::endl;

      return stringStream.str();
    } else return "Unknown command.\n";
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "Error executing command. See log file for more details.\n";
}

std::string CliServer::moduleCommand(std::string &command) {
  try {
    std::ostringstream stringStream;
    if (command.compare(0, 12, "modules help") == 0 || command.compare(0, 2, "mh") == 0) {
      stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
      stringStream << "For more information about the individual command type: COMMAND help" << std::endl
                   << std::endl;
      stringStream << "modules list (mls)\tLists all family modules" << std::endl;
      stringStream << "modules load (mld)\tLoads a family module" << std::endl;
      stringStream << "modules unload (mul)\tUnloads a family module" << std::endl;
      stringStream << "modules reload (mrl)\tReloads a family module" << std::endl;
      return stringStream.str();
    }
    if (command.compare(0, 12, "modules list") == 0 || command.compare(0, 3, "mls") == 0) {
      std::stringstream stream(command);
      std::string element;
      bool printHelp = false;
      int32_t offset = (command.at(1) == 'l') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index < 1 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset && element == "help") printHelp = true;
        else {
          printHelp = true;
          break;
        }
        index++;
      }
      if (printHelp) {
        stringStream
            << "Description: This command lists all loaded family modules and shows the corresponding family ids."
            << std::endl;
        stringStream << "Usage: modules list" << std::endl << std::endl;
        return stringStream.str();
      }

      auto modules = GD::familyController->getModuleInfo();
      if (modules.size() == 0) return "No modules loaded.\n";

      stringStream << std::left << std::setfill(' ') << std::setw(6) << "ID" << std::setw(30) << "Family Name"
                   << std::setw(30) << "Filename" << std::setw(14) << "Compiled For" << std::setw(7) << "Loaded"
                   << std::endl;
      for (auto &module: modules) {
        stringStream << std::setw(6) << module->familyId << std::setw(30) << module->familyName << std::setw(30)
                     << module->filename << std::setw(14) << module->baselibVersion << std::setw(7)
                     << (module->loaded ? "true" : "false") << std::endl;
      }

      return stringStream.str();
    } else if (command.compare(0, 12, "modules load") == 0 || command.compare(0, 3, "mld") == 0) {
      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'l') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index == 0 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset) {
          if (element == "help" || element.empty()) break;
        }
        index++;
      }
      if (index == 1 + offset) {
        stringStream
            << "Description: This command loads a family module from \"" + GD::bl->settings.modulePath() + "\"."
            << std::endl;
        stringStream << "Usage: modules load FILENAME" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  FILENAME:\t\tThe file name of the module. E. g. \"mod_miscellaneous.so\""
                     << std::endl;
        return stringStream.str();
      }

      int32_t result = GD::familyController->loadModule(element);
      switch (result) {
        case 0: stringStream << "Module successfully loaded." << std::endl;
          break;
        case 1: stringStream << "Module already loaded." << std::endl;
          break;
        case -1: stringStream << "System error. See log for more details." << std::endl;
          break;
        case -2: stringStream << "Module does not exist." << std::endl;
          break;
        case -4: stringStream << "Family initialization failed. See log for more details." << std::endl;
          break;
        default: stringStream << "Unknown result code: " << result << std::endl;
      }

      stringStream << "Exit code: " << std::dec << result << std::endl;
      return stringStream.str();
    } else if (command.compare(0, 14, "modules unload") == 0 || command.compare(0, 3, "mul") == 0) {
      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'u') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index == 0 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset) {
          if (element == "help" || element.empty()) break;
        }
        index++;
      }
      if (index == 1 + offset) {
        stringStream << "Description: This command unloads a family module." << std::endl;
        stringStream << "Usage: modules unload FILENAME" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  FILENAME:\t\tThe file name of the module. E. g. \"mod_miscellaneous.so\""
                     << std::endl;
        return stringStream.str();
      }

      int32_t result = GD::familyController->unloadModule(element);
      switch (result) {
        case 0: stringStream << "Module successfully unloaded." << std::endl;
          break;
        case 1: stringStream << "Module not loaded." << std::endl;
          break;
        case -1: stringStream << "System error. See log for more details." << std::endl;
          break;
        case -2: stringStream << "Module does not exist." << std::endl;
          break;
        default: stringStream << "Unknown result code: " << result << std::endl;
      }

      stringStream << "Exit code: " << std::dec << result << std::endl;
      return stringStream.str();
    } else if (command.compare(0, 14, "modules reload") == 0 || command.compare(0, 3, "mrl") == 0) {
      std::stringstream stream(command);
      std::string element;
      int32_t offset = (command.at(1) == 'r') ? 0 : 1;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index == 0 + offset) {
          index++;
          continue;
        } else if (index == 1 + offset) {
          if (element == "help" || element.empty()) break;
        }
        index++;
      }
      if (index == 1 + offset) {
        stringStream << "Description: This command reloads a family module." << std::endl;
        stringStream << "Usage: modules reload FILENAME" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  FILENAME:\t\tThe file name of the module. E. g. \"mod_miscellaneous.so\""
                     << std::endl;
        return stringStream.str();
      }

      int32_t result = GD::familyController->reloadModule(element);
      switch (result) {
        case 0: stringStream << "Module successfully reloaded." << std::endl;
          break;
        case 1: stringStream << "Module already loaded." << std::endl;
          break;
        case -1: stringStream << "System error. See log for more details." << std::endl;
          break;
        case -2: stringStream << "Module does not exist." << std::endl;
          break;
        case -4: stringStream << "Family initialization failed. See log for more details." << std::endl;
          break;
        default: stringStream << "Unknown result code: " << result << std::endl;
      }

      stringStream << "Exit code: " << std::dec << result << std::endl;
      return stringStream.str();
    } else return "Unknown command.\n";
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "Error executing command. See log file for more details.\n";
}

BaseLib::PVariable CliServer::generalCommand(std::string &command) {
  try {
    if (command.empty()) return std::make_shared<BaseLib::Variable>(std::string());
    std::ostringstream stringStream;
    std::vector<std::string> arguments;
    bool showHelp = false;
    if (BaseLib::HelperFunctions::checkCliCommand(command, "help", "h", "", 0, arguments, showHelp)) {
      stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
      stringStream << "For more information about the individual command type: COMMAND help" << std::endl
                   << std::endl;
      stringStream << "debuglevel (dl)      Changes the debug level" << std::endl;
      stringStream << "events (ev)          Prints variable updates to the standard output" << std::endl;
      stringStream << "lifetick (lt)        Checks the lifeticks of all components." << std::endl;
      stringStream << "rpcservers (rpc)     Lists all active RPC servers" << std::endl;
      stringStream << "rpcclients (rcl)     Lists all active RPC clients" << std::endl;
      stringStream << "reloadroles (rrl)    Delete all roles and recreate them from \"defaultRoles.json\"." << std::endl;
      stringStream << "threads              Prints the current thread count" << std::endl;
      stringStream << "load (ld)            Prints the current load" << std::endl;
      stringStream << "nodeproctimes (npt)  Prints the time required for every node input to process incoming data" << std::endl;
      stringStream << "slavemode (sm)       Enables slave mode when in a master/slave installation" << std::endl;
#ifndef NO_SCRIPTENGINE
      stringStream << "runscript (rs)       Executes a script with the internal PHP engine" << std::endl;
      stringStream << "runcommand (rc)      Executes a PHP command" << std::endl;
      stringStream << "scriptcount (sc)     Returns the number of currently running scripts" << std::endl;
      stringStream << "scriptsrunning (sr)  Returns the ID and filename of all running scripts" << std::endl;
#endif
      if (GD::bl->settings.enableNodeBlue()) {
        stringStream << "flowcount (fc)       Returns the number of currently running flows" << std::endl;
        stringStream << "flowsrestart (fr)    Restarts all flows" << std::endl;
        stringStream << "flowsstop (ft)       Stops all flows" << std::endl;
      }
      stringStream << "users [COMMAND]      Execute user commands. Type \"users help\" for more information."
                   << std::endl;
      stringStream
          << "families [COMMAND]   Execute device family commands. Type \"families help\" for more information."
          << std::endl;
      stringStream << "modules [COMMAND]    Execute module commands. Type \"modules help\" for more information."
                   << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "test", "tst", "", 0, arguments, showHelp)) {
#ifndef NO_SCRIPTENGINE
      GD::scriptEngineServer->devTestClient();
#endif
      stringStream << "Done." << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "debuglevel", "dl", "", 1, arguments, showHelp)) {
      int32_t debugLevel = 3;

      if (showHelp) {
        stringStream
            << "Description: This command changes the current debug level temporarily until Homegear is restarted."
            << std::endl;
        stringStream << "Usage: debuglevel DEBUGLEVEL" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  DEBUGLEVEL:\tThe debug level between 0 and 10." << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      debugLevel = BaseLib::Math::getNumber(arguments.at(0), false);
      if (debugLevel < 0 || debugLevel > 10) return std::make_shared<BaseLib::Variable>(std::string("Invalid debug level. Please provide a debug level between 0 and 10.\n"));

      GD::bl->debugLevel = debugLevel;
      stringStream << "Debug level set to " << debugLevel << "." << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    }
#ifndef NO_SCRIPTENGINE
    else if (command.compare(0, 9, "runscript") == 0 || command.compare(0, 2, "rs") == 0) {
      std::string fullPath;
      std::string relativePath;

      std::stringstream stream(command);
      std::string element;
      std::stringstream scriptArguments;
      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index == 0) {
          index++;
          continue;
        } else if (index == 1) {
          if (element == "help" || element.empty()) break;
          relativePath = element;
          if (!BaseLib::Io::fileExists(relativePath)) fullPath = GD::bl->settings.scriptPath() + element;
          else fullPath = element;
          if (!BaseLib::Io::fileExists(fullPath)) {
            auto output = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
            output->structValue->emplace("output", std::make_shared<BaseLib::Variable>("File not found.\n"));
            output->structValue->emplace("exitCode", std::make_shared<BaseLib::Variable>(1));
            return output;
          }
        } else {
          scriptArguments << element << " ";
        }
        index++;
      }
      if (index < 2) {
        stringStream
            << "Description: This command executes a script in the Homegear script folder using the internal PHP engine."
            << std::endl;
        stringStream << "Usage: runscript PATH [ARGUMENTS]" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  PATH:\t\tPath relative to the Homegear scripts folder." << std::endl;
        stringStream << "  ARGUMENTS:\tParameters passed to the script." << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      std::string argumentsString = scriptArguments.str();
      BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli, fullPath, relativePath, argumentsString));
      scriptInfo->scriptFinishedCallback = std::bind(&CliServer::scriptFinished, this, std::placeholders::_1, std::placeholders::_2);
      scriptInfo->scriptOutputCallback = std::bind(&CliServer::scriptOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      GD::scriptEngineServer->executeScript(scriptInfo, false);

      std::unique_lock<std::mutex> waitLock(_waitMutex);
      int64_t startTime = BaseLib::HelperFunctions::getTime();
      while (!_waitConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
        if (BaseLib::HelperFunctions::getTime() - startTime > 300000) return true;
        else return _scriptFinished;
      }));

      auto output = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      output->structValue->emplace("exitCode", std::make_shared<BaseLib::Variable>(scriptInfo->exitCode));
      return output;
    } else if (command.compare(0, 10, "runcommand") == 0 || command.compare(0, 3, "rc ") == 0 || command.compare(0, 1, "$") == 0) {
      int32_t commandSize = 11;
      if (command.compare(0, 2, "rc") == 0) commandSize = 3;
      else if (command.compare(0, 1, "$") == 0) commandSize = 0;
      if (((int32_t)command.size()) - commandSize < 4 || command.compare(commandSize, 4, "help") == 0) {
        stringStream << "Description: Executes a PHP command. The Homegear object ($hg) is defined implicitly."
                     << std::endl;
        stringStream << "Usage: runcommand COMMAND" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  COMMAND:\t\tThe command to execute. E. g.: $hg->setValue(12, 2, \"STATE\", true);"
                     << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      std::string script;
      script.reserve(command.size() - commandSize + 50);
      script.append("<?php\nuse Homegear\\Homegear as Homegear;\nuse Homegear\\HomegearGpio as HomegearGpio;\nuse Homegear\\HomegearSerial as HomegearSerial;\nuse Homegear\\HomegearI2c as HomegearI2c;\n$hg = new Homegear();\n");
      if (commandSize > 0) command = command.substr(commandSize);
      BaseLib::HelperFunctions::trim(command);
      if (command.size() < 4) return std::make_shared<BaseLib::Variable>(std::string("Invalid code.\n"));
      if ((command.front() == '\"' && command.back() == '\"') || (command.front() == '\'' && command.back() == '\'')) command = command.substr(1, command.size() - 2);
      command.push_back(';');

      script.append(command);

      std::string fullPath = GD::bl->settings.scriptPath() + "inline.php";
      std::string relativePath = "/inline.php";
      std::string arguments;
      BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::cli, fullPath, relativePath, script, arguments));
      scriptInfo->scriptFinishedCallback = std::bind(&CliServer::scriptFinished, this, std::placeholders::_1, std::placeholders::_2);
      scriptInfo->scriptOutputCallback = std::bind(&CliServer::scriptOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      GD::scriptEngineServer->executeScript(scriptInfo, false);

      std::unique_lock<std::mutex> waitLock(_waitMutex);
      int64_t startTime = BaseLib::HelperFunctions::getTime();
      while (!_waitConditionVariable.wait_for(waitLock, std::chrono::milliseconds(1000), [&] {
        if (BaseLib::HelperFunctions::getTime() - startTime > 300000) return true;
        else return _scriptFinished;
      }));

      auto output = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      output->structValue->emplace("exitCode", std::make_shared<BaseLib::Variable>(scriptInfo->exitCode));
      return output;
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "scriptcount", "sc", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command returns the total number of currently running scripts."
                     << std::endl;
        stringStream << "Usage: scriptcount" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      stringStream << std::dec << GD::scriptEngineServer->scriptCount() << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "scriptsrunning", "sr", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command returns the script IDs and filenames of all running scripts."
                     << std::endl;
        stringStream << "Usage: scriptsrunning" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      std::vector<std::tuple<int32_t, uint64_t, std::string, int32_t, std::string>> runningScripts = GD::scriptEngineServer->getRunningScripts();
      if (runningScripts.empty()) return std::make_shared<BaseLib::Variable>(std::string("No scripts are being executed.\n"));

      stringStream << std::left << std::setfill(' ') << std::setw(10) << "PID" << std::setw(10) << "Peer ID"
                   << std::setw(17) << "Node ID" << std::setw(10) << "Script ID" << std::setw(80) << "Filename" << std::endl;
      for (auto &script: runningScripts) {
        stringStream << std::setw(10) << std::get<0>(script) << std::setw(10)
                     << (std::get<1>(script) > 0 ? std::to_string(std::get<1>(script)) : "") << std::setw(17)
                     << std::get<2>(script) << std::setw(10) << std::get<3>(script) << std::setw(80) << std::get<4>(script) << std::endl;
      }

      return std::make_shared<BaseLib::Variable>(stringStream.str());
    }
#endif
    else if (BaseLib::HelperFunctions::checkCliCommand(command, "flowcount", "fc", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command returns the total number of currently running flows."
                     << std::endl;
        stringStream << "Usage: flowcount" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      if (GD::nodeBlueServer) stringStream << std::dec << GD::nodeBlueServer->flowCount() << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "flowsrestart", "fr", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command restarts all flows." << std::endl;
        stringStream << "Usage: flowsrestart" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      if (GD::nodeBlueServer) GD::nodeBlueServer->restartFlows();

      stringStream << "Flows restarted." << std::endl;

      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "flowsstop", "ft", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command stops all flows." << std::endl;
        stringStream << "Usage: flowsstop" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      if (GD::nodeBlueServer) GD::nodeBlueServer->stopFlows();

      stringStream << "Flows stopped." << std::endl;

      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "reloadroles", "rrl", "", 0, arguments, showHelp)) {
      if (showHelp) {
        stringStream
            << "Description: Deletes all existing roles and recreates them from \"defaultRoles.json\"."
            << std::endl;
        stringStream << "Usage: reloadroles" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      GD::bl->db->deleteAllRoles();
      GD::bl->db->createDefaultRoles();

      stringStream << "Recreating roles... Please check the Homegear log for errors." << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (command.compare(0, 10, "rpcclients") == 0 || command.compare(0, 3, "rcl") == 0) {
      std::stringstream stream(command);
      std::string element;

      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index == 0) {
          index++;
          continue;
        } else {
          index++;
          break;
        }
      }
      if (index > 1) {
        stringStream << "Description: This command lists all connected RPC clients." << std::endl;
        stringStream << "Usage: rpcclients" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      int32_t idWidth = 10;
      int32_t addressWidth = 32;
      int32_t urlWidth = 30;
      int32_t interfaceIdWidth = 30;
      int32_t xmlWidth = 10;
      int32_t binaryWidth = 10;
      int32_t jsonWidth = 10;
      int32_t websocketWidth = 10;

      //Safe to use without mutex
      for (auto &server: GD::rpcServers) {
        if (!server.second->isRunning()) continue;
        const BaseLib::Rpc::PServerInfo settings = server.second->getInfo();
        const std::vector<BaseLib::PRpcClientInfo> clients = server.second->getClientInfo();

        stringStream << "Server " << settings->name << " (Port: " << std::to_string(settings->port) << "):"
                     << std::endl;

        std::string idCaption("Client ID");
        std::string addressCaption("Address");
        std::string urlCaption("Init URL");
        std::string interfaceIdCaption("Init ID");
        idCaption.resize(idWidth, ' ');
        addressCaption.resize(addressWidth, ' ');
        urlCaption.resize(urlWidth, ' ');
        interfaceIdCaption.resize(interfaceIdWidth, ' ');
        stringStream << std::setfill(' ')
                     << "    "
                     << idCaption << "  "
                     << addressCaption << "  "
                     << urlCaption << "  "
                     << interfaceIdCaption << "  "
                     << std::setw(xmlWidth) << "XML-RPC" << "  "
                     << std::setw(binaryWidth) << "Binary RPC" << "  "
                     << std::setw(jsonWidth) << "JSON-RPC" << "  "
                     << std::setw(websocketWidth) << "Websocket" << "  "
                     << std::endl;

        for (std::vector<BaseLib::PRpcClientInfo>::const_iterator j = clients.begin(); j != clients.end(); ++j) {
          std::string id = std::to_string((*j)->id);
          id.resize(idWidth, ' ');

          std::string address = (*j)->address;
          if (address.size() > (unsigned)addressWidth) {
            address.resize(addressWidth - 3);
            address += "...";
          } else address.resize(addressWidth, ' ');

          std::string url = (*j)->sendEventsToRpcServer ? "Single connection" : (*j)->initUrl;
          if (url.size() > (unsigned)urlWidth) {
            url.resize(urlWidth - 3);
            url += "...";
          } else url.resize(urlWidth, ' ');

          std::string interfaceId = (*j)->initInterfaceId;
          if (interfaceId.size() > (unsigned)interfaceIdWidth) {
            interfaceId.resize(interfaceIdWidth - 3);
            interfaceId += "...";
          } else interfaceId.resize(interfaceIdWidth, ' ');

          stringStream
              << "    "
              << id << "  "
              << address << "  "
              << url << "  "
              << interfaceId << "  "
              << std::setw(xmlWidth) << ((*j)->rpcType == BaseLib::RpcType::xml ? "true" : "false")
              << "  "
              << std::setw(binaryWidth) << ((*j)->rpcType == BaseLib::RpcType::binary ? "true" : "false")
              << "  "
              << std::setw(jsonWidth) << ((*j)->rpcType == BaseLib::RpcType::json ? "true" : "false")
              << "  "
              << std::setw(websocketWidth)
              << ((*j)->rpcType == BaseLib::RpcType::websocket ? "true" : "false") << "  "
              << std::endl;
        }

        stringStream << std::endl;
      }
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (command.compare(0, 10, "rpcservers") == 0 || command.compare(0, 3, "rpc") == 0) {
      std::stringstream stream(command);
      std::string element;

      int32_t index = 0;
      while (std::getline(stream, element, ' ')) {
        if (index == 0) {
          index++;
          continue;
        } else {
          index++;
          break;
        }
      }
      if (index > 1) {
        stringStream << "Description: This command lists all active RPC servers." << std::endl;
        stringStream << "Usage: rpcservers" << std::endl << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      int32_t nameWidth = 20;
      int32_t interfaceWidth = 20;
      int32_t portWidth = 6;
      int32_t sslWidth = 5;
      int32_t authWidth = 5;
      std::string nameCaption("Name");
      std::string interfaceCaption("Interface");
      nameCaption.resize(nameWidth, ' ');
      interfaceCaption.resize(interfaceWidth, ' ');
      stringStream << std::setfill(' ')
                   << nameCaption << "  "
                   << interfaceCaption << "  "
                   << std::setw(portWidth) << "Port" << "  "
                   << std::setw(sslWidth) << "SSL" << "  "
                   << std::setw(authWidth) << "Auth" << "  "
                   << std::endl;

      //Safe to use without mutex
      for (auto &server: GD::rpcServers) {
        if (!server.second->isRunning()) continue;
        const BaseLib::Rpc::PServerInfo settings = server.second->getInfo();
        std::string name = settings->name;
        if (name.size() > (unsigned)nameWidth) {
          name.resize(nameWidth - 3);
          name += "...";
        } else name.resize(nameWidth, ' ');
        std::string interface = settings->interface;
        if (interface.size() > (unsigned)interfaceWidth) {
          interface.resize(interfaceWidth - 3);
          interface += "...";
        } else interface.resize(interfaceWidth, ' ');
        stringStream
            << name << "  "
            << interface << "  "
            << std::setw(portWidth) << settings->port << "  "
            << std::setw(sslWidth) << (settings->ssl ? "true" : "false") << "  "
            << std::setw(authWidth)
            << (settings->authType == BaseLib::Rpc::ServerInfo::Info::AuthType::basic ? "basic" : "none")
            << "  "
            << std::endl;

      }
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (command.compare(0, 7, "threads") == 0) {
      stringStream << GD::bl->threadManager.getCurrentThreadCount() << " of "
                   << GD::bl->threadManager.getMaxThreadCount() << std::endl
                   << "Maximum thread count since start: " << GD::bl->threadManager.getMaxRegisteredThreadCount()
                   << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "load", "ld", "", 0, arguments, showHelp)) {
      auto load1 = GD::scriptEngineServer->getLoad();
      auto load2 = GD::nodeBlueServer->getLoad();
      load1->structValue->insert(load2->structValue->begin(), load2->structValue->end());
      stringStream << BaseLib::Rpc::JsonEncoder::encode(GD::nodeBlueServer->getLoad()) << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "nodeproctimes", "npt", "", 0, arguments, showHelp)) {
      stringStream << BaseLib::Rpc::JsonEncoder::encode(GD::nodeBlueServer->getNodeProcessingTimes()) << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "slavemode", "sm", "", 1, arguments, showHelp)) {
      if (showHelp) {
        stringStream << "Description: This command enables or disables slave mode when running in a master/slave installation." << std::endl;
        stringStream << "             Executing this command stops all Node-BLUE flows and sets a flag, that slave mode is enabled." << std::endl;
        stringStream << "             This flag can be used within device families to disable device communication." << std::endl;
        stringStream << "Usage: slavemode ENABLE" << std::endl << std::endl;
        stringStream << "Parameters:" << std::endl;
        stringStream << "  ENABLE: \"true\" to enable and \"false\" to disable slave mode." << std::endl;
        return std::make_shared<BaseLib::Variable>(stringStream.str());
      }

      bool enable = BaseLib::HelperFunctions::toLower(BaseLib::HelperFunctions::trim(arguments.at(0))) == "true";
      if (enable && !GD::bl->slaveMode) {
        GD::bl->slaveMode = true;
        if (GD::nodeBlueServer) GD::nodeBlueServer->stopFlows();
        stringStream << "Slave mode enabled." << std::endl;
      } else if (!enable && GD::bl->slaveMode) {
        GD::bl->slaveMode = false;
        if (GD::nodeBlueServer) GD::nodeBlueServer->restartFlows();
        stringStream << "Slave mode disabled." << std::endl;
      } else {
        stringStream << "Slave mode unchanged." << std::endl;
      }
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "lifetick", "lt", "", 0, arguments, showHelp)) {
      int32_t exitCode = 0;
      try {
        if (!GD::rpcClient->lifetick()) {
          stringStream << "RPC Client: Failed" << std::endl;
          exitCode = 1;
        } else stringStream << "RPC Client: OK" << std::endl;
        for (auto &server: GD::rpcServers) {
          if (!server.second->lifetick()) {
            stringStream << "RPC Server (Port " << server.second->getInfo()->port << "): Failed"
                         << std::endl;
            exitCode = 2;
          } else stringStream << "RPC Server (Port " << server.second->getInfo()->port << "): OK" << std::endl;
        }
        if (!GD::familyController->lifetick()) {
          stringStream << "Device families: Failed" << std::endl;
          exitCode = 3;
        } else stringStream << "Device families: OK" << std::endl;
      }
      catch (const std::exception &ex) {
        exitCode = 127;
        GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      auto output = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
      output->structValue->emplace("exitCode", std::make_shared<BaseLib::Variable>(exitCode));
      output->structValue->emplace("output", std::make_shared<BaseLib::Variable>(stringStream.str()));
      return output;
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "families help", "fh", "", 0, arguments, showHelp)) {
      stringStream << "List of commands (shortcut in brackets):" << std::endl << std::endl;
      stringStream << "For more information about the individual command type: COMMAND help" << std::endl
                   << std::endl;
      stringStream << "families list (ls)\tList all available device families" << std::endl;
      stringStream << "families select (fs)\tSelect a device family" << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (BaseLib::HelperFunctions::checkCliCommand(command, "families list", "fl", "ls", 0, arguments, showHelp)) {
      std::string bar(" │ ");
      const int32_t idWidth = 5;
      const int32_t nameWidth = 30;
      std::string nameHeader = "Name";
      nameHeader.resize(nameWidth, ' ');
      stringStream << std::setfill(' ')
                   << std::setw(idWidth) << "ID" << bar
                   << nameHeader
                   << std::endl;
      stringStream << "──────┼───────────────────────────────" << std::endl;
      auto families = GD::familyController->getFamilies();
      for (auto &family: families) {
        if (family.first == -1) continue;
        std::string name = family.second->getName();
        name.resize(nameWidth, ' ');
        stringStream << std::setw(idWidth) << std::setfill(' ') << (int32_t)family.first << bar << name
                     << std::endl;
      }
      stringStream << "──────┴───────────────────────────────" << std::endl;
      return std::make_shared<BaseLib::Variable>(stringStream.str());
    } else if (command.compare(0, 5, "users") == 0 || command.compare(0, 6, "groups") == 0 || (BaseLib::HelperFunctions::isShortCliCommand(command) && command.at(0) == 'u')
        || (BaseLib::HelperFunctions::isShortCliCommand(command) && command.at(0) == 'g'))
      return std::make_shared<BaseLib::Variable>(userCommand(command));
    else if ((command.compare(0, 7, "modules") == 0 || (BaseLib::HelperFunctions::isShortCliCommand(command) && command.at(0) == 'm'))) return std::make_shared<BaseLib::Variable>(moduleCommand(command));

    return std::make_shared<BaseLib::Variable>(std::string("Unknown command."));
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return std::make_shared<BaseLib::Variable>(std::string("Error executing command. See log file for more details.\n"));
}

std::string CliServer::familyCommand(int32_t familyId, std::string &command) {
  try {
    if (command.empty()) return "";

    auto family = GD::familyController->getFamily(familyId);
    if (!family) return "Unknown family.";

    return family->handleCliCommand(command);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "Error executing command. See log file for more details.\n";
}

std::string CliServer::peerCommand(uint64_t peerId, std::string &command) {
  try {
    if (command.empty()) return "";

    auto families = GD::familyController->getFamilies();
    for (auto &family: families) {
      auto central = family.second->getCentral();
      if (!central) continue;

      auto peer = central->getPeer(peerId);
      if (!peer) continue;

      return peer->handleCliCommand(command);
    }

    return "Unknown peer.";
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  return "Error executing command. See log file for more details.\n";
}

#ifndef NO_SCRIPTENGINE
void CliServer::scriptFinished(const BaseLib::ScriptEngine::PScriptInfo &scriptInfo, int32_t exitCode) {
  try {
    std::unique_lock<std::mutex> waitLock(_waitMutex);
    _scriptFinished = true;
    waitLock.unlock();
    _waitConditionVariable.notify_all();
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

void CliServer::scriptOutput(const PScriptInfo &scriptInfo, const std::string &output, bool error) {
  try {
    //Warning: scriptInfo might be nullptr.

    std::string methodName = "cliOutput-" + std::to_string(_clientId);

    BaseLib::PArray parameters = std::make_shared<BaseLib::Array>();
    auto outputStruct = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
    outputStruct->structValue->emplace("errorOutput", std::make_shared<BaseLib::Variable>(error));
    outputStruct->structValue->emplace("output", std::make_shared<BaseLib::Variable>(output));
    parameters->push_back(outputStruct);

    auto result = GD::ipcServer->callRpcMethod(_dummyClientInfo, methodName, parameters);
    if (result->errorStruct) GD::out.printError("Error calling method \"cliOutput\": " + result->structValue->at("faultString")->stringValue);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}
#endif

}