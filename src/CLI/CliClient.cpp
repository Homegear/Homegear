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

#include "CliClient.h"
#include "../GD/GD.h"

#include <readline/readline.h>
#include <readline/history.h>

namespace Homegear
{

CliClient::CliClient(std::string socketPath) : IIpcClient(socketPath)
{
    _localRpcMethods.emplace("broadcastEvent", std::bind(&CliClient::broadcastEvent, this, std::placeholders::_1));
    _localRpcMethods.emplace("cliOutput", std::bind(&CliClient::output, this, std::placeholders::_1));

    _printEvents = false;
}

CliClient::~CliClient()
{
}

void CliClient::onConnect()
{
    try
    {
        bool error = false;

        auto parameters = std::make_shared<Ipc::Array>();
        parameters->reserve(2);
        parameters->push_back(std::make_shared<Ipc::Variable>("cliOutput"));
        parameters->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray)); //Outer array
        auto signature = std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray); //Inner array (= signature)
        signature->arrayValue->reserve(6);
        signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tVoid)); //Return value
        signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tString)); //1st parameter
        parameters->back()->arrayValue->push_back(signature);
        auto result = invoke("registerRpcMethod", parameters);
        if(result->errorStruct)
        {
            error = true;
            Ipc::Output::printCritical("Critical: Could not register RPC method cliOutput: " + result->structValue->at("faultString")->stringValue);
        }
        if(error) return;

        parameters = std::make_shared<Ipc::Array>();
        result = invoke("getClientId", parameters);
        if(result->errorStruct)
        {
            error = true;
            Ipc::Output::printCritical("Critical: Could not get my own client ID: " + result->structValue->at("faultString")->stringValue);
        }
        if(error) return;
        int32_t clientId = result->integerValue;

        parameters = std::make_shared<Ipc::Array>();
        parameters->reserve(2);
        parameters->push_back(std::make_shared<Ipc::Variable>("cliOutput-" + std::to_string(clientId)));
        parameters->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray)); //Outer array
        signature = std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray); //Inner array (= signature)
        signature->arrayValue->reserve(6);
        signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tVoid)); //Return value
        signature->arrayValue->push_back(std::make_shared<Ipc::Variable>(Ipc::VariableType::tString)); //1st parameter
        parameters->back()->arrayValue->push_back(signature);
        result = invoke("registerRpcMethod", parameters);
        if(result->errorStruct)
        {
            error = true;
            Ipc::Output::printCritical("Critical: Could not register RPC method cliOutput-" + std::to_string(clientId) + ": " + result->structValue->at("faultString")->stringValue);
        }
        if(error) return;

        //Does not invalidate iterators or references so no mutex is needed
        _localRpcMethods.emplace("cliOutput-" + std::to_string(clientId), std::bind(&CliClient::output, this, std::placeholders::_1));

        std::unique_lock<std::mutex> waitLock(_onConnectWaitMutex);
        waitLock.unlock();
        _onConnectConditionVariable.notify_all();
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void CliClient::onDisconnect()
{
    //{{{ Reenable buffered input
    struct termios t{};
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ICANON;
    t.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    //}}}

    standardOutput("Disconnected from Homegear.\n");
    exit(6);
}

std::string CliClient::getBreadcrumb()
{
    if(_currentFamily != -1)
    {
        if(_currentPeer != 0) return "Family " + std::to_string(_currentFamily) + " - " + "peer " + (_currentPeer > 999999 ? "0x" + BaseLib::HelperFunctions::getHexString(_currentPeer, 8) : std::to_string(_currentPeer));
        else return "Family " + std::to_string(_currentFamily);
    }
    else if(_currentPeer != 0) return "Peer " + (_currentPeer > 999999 ? "0x" + BaseLib::HelperFunctions::getHexString(_currentPeer, 8) : std::to_string(_currentPeer));
    else return "";
}

int32_t CliClient::terminal(std::string& command)
{
    try
    {
        start();

        std::unique_lock<std::mutex> waitLock(_onConnectWaitMutex);
        int64_t startTime = Ipc::HelperFunctions::getTime();
        while(!_onConnectConditionVariable.wait_for(waitLock, std::chrono::milliseconds(2000), [&]
        {
            if(Ipc::HelperFunctions::getTime() - startTime > 2000) return true;
            else return connected();
        }));

        if(!connected())
        {
            Ipc::Output::printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
            return 3;
        }

        if(command.empty())
        {
            standardOutput("Connected to Homegear (version " + GD::baseLibVersion + ").\n\n");
            standardOutput("Please type >>help<< to list all available commands.\n");
        }

        rl_bind_key('\t', rl_abort); //no autocompletion

        std::string level = "";
        std::string lastCommand;
        std::string currentCommand;
        char* sendBuffer;
        int32_t bytes = 0;
        while(!command.empty() || (sendBuffer = readline((getBreadcrumb() + "> ").c_str())) != NULL)
        {
            if(command.empty())
            {
                if(!connected())
                {
                    standardOutput("Disconnected from Homegear.\n");
                    stop();
                    free(sendBuffer);
                    return 5;
                }
                bytes = strnlen(sendBuffer, 1000000);
                if(bytes == 0 || sendBuffer[0] == '\n' || sendBuffer[0] == 0) continue;
                if(bytes >= 4 && (strncmp(sendBuffer, "quit", 4) == 0 || strncmp(sendBuffer, "exit", 4) == 0 || strncmp(sendBuffer, "moin", 4) == 0))
                {
                    stop();
                    free(sendBuffer);
                    return 4;
                }

                currentCommand = std::string(sendBuffer);
                if(currentCommand != lastCommand)
                {
                    lastCommand = currentCommand;
                    add_history(sendBuffer); //Sets sendBuffer to nullptr
                }
                else
                {
                    free(sendBuffer);
                    sendBuffer = nullptr;
                }
            }
            else currentCommand = command;

            std::ostringstream stringStream;
            std::vector<std::string> arguments;
            bool showHelp = false;

            if(currentCommand.empty()) continue;
            else if(BaseLib::HelperFunctions::checkCliCommand(currentCommand, "unselect", "u", "..", 0, arguments, showHelp))
            {
                if(_currentPeer > 0) _currentPeer = 0;
                else if(_currentFamily != -1) _currentFamily = -1;
            }
            else if(BaseLib::HelperFunctions::checkCliCommand(currentCommand, "families select", "fs", "", 1, arguments, showHelp))
            {
                if(showHelp)
                {
                    stringStream << "Description: This command selects a device family." << std::endl;
                    stringStream << "Usage: families select FAMILYID" << std::endl << std::endl;
                    stringStream << "Parameters:" << std::endl;
                    stringStream
                            << "  FAMILYID:\tThe id of the family to select. Type >>families list<< to get a list of supported families. Example: 1"
                            << std::endl;
                    standardOutput(stringStream.str() + "\n");
                }
                else
                {
                    int32_t family = BaseLib::Math::getNumber(arguments.at(0));

                    Ipc::PArray parameters = std::make_shared<Ipc::Array>();
                    parameters->push_back(std::make_shared<Ipc::Variable>(family));
                    Ipc::PVariable result = invoke("familyExists", parameters);
                    if(result->errorStruct)
                    {
                        errorOutput("Error executing command: " + result->structValue->at("faultString")->stringValue + "\n");
                    }
                    else if(result->booleanValue)
                    {
                        _currentFamily = family;
                        standardOutput("For a list of available family commands type >>help<<.\n");
                    }
                    else standardOutput("Unknown family.\n");
                }
            }
            else if(BaseLib::HelperFunctions::checkCliCommand(currentCommand, "peers select", "ps", "", 1, arguments, showHelp))
            {
                if(showHelp)
                {
                    stringStream << "Description: This command selects a peer." << std::endl;
                    stringStream << "Usage: peers select PEERID" << std::endl << std::endl;
                    stringStream << "Parameters:" << std::endl;
                    stringStream << "  PEERID:\tThe id of the peer to select. Example: 513" << std::endl;
                    standardOutput(stringStream.str() + "\n");
                }
                else
                {
                    uint64_t peerId = (uint64_t) BaseLib::Math::getNumber64(arguments.at(0));

                    Ipc::PArray parameters = std::make_shared<Ipc::Array>();
                    parameters->push_back(std::make_shared<Ipc::Variable>(peerId));
                    Ipc::PVariable result = invoke("peerExists", parameters);
                    if(result->errorStruct)
                    {
                        errorOutput("Error executing command: " + result->structValue->at("faultString")->stringValue + "\n");
                    }
                    else if(result->booleanValue)
                    {
                        _currentPeer = peerId;
                        standardOutput("For a list of available peer commands type >>help<<.\n");
                    }
                    else standardOutput("Unknown peer.\n");
                }
            }
            else if(BaseLib::HelperFunctions::checkCliCommand(currentCommand, "events", "ev", "", 0, arguments, showHelp))
            {
                if(showHelp)
                {
                    stringStream
                            << "Description: This command prints events from Homegear to the standard output. When no family and no peer is selected, updates from all devices and including system variables are output. When a family or peer is selected, only events from this family or peer are output."
                            << std::endl << std::endl;
                    stringStream << "Usage: events" << std::endl;
                    standardOutput(stringStream.str() + "\n");
                }
                else
                {
                    standardOutput("Listening for variable updates");
                    if(_currentPeer != 0) standardOutput(" of peer " + std::to_string(_currentPeer));
                    else if(_currentFamily != -1) standardOutput(" in family " + std::to_string(_currentFamily));
                    standardOutput(". Press >>ESC<< or >>q<< to abort.\n");

                    //{{{ Disable buffered input
                    struct termios t{};
                    tcgetattr(STDIN_FILENO, &t);
                    t.c_lflag &= ~ICANON;
                    t.c_lflag &= ~ECHO;
                    tcsetattr(STDIN_FILENO, TCSANOW, &t);
                    //}}}

                    _printEvents = true;

                    int charCode = -1;
                    do
                    {
                        charCode = std::getchar();
                    } while(connected() && charCode != 27 && charCode != 81 && charCode != 113);

                    _printEvents = false;

                    //{{{ Reenable buffered input
                    tcgetattr(STDIN_FILENO, &t);
                    t.c_lflag |= ICANON;
                    t.c_lflag |= ECHO;
                    tcsetattr(STDIN_FILENO, TCSANOW, &t);
                    //}}}
                }
            }
            else if(_currentPeer != 0)
            {
                Ipc::PArray parameters = std::make_shared<Ipc::Array>();
                parameters->reserve(2);
                parameters->push_back(std::make_shared<Ipc::Variable>(_currentPeer));
                parameters->push_back(std::make_shared<Ipc::Variable>(currentCommand));
                Ipc::PVariable result = invoke("cliPeerCommand", parameters);
                if(result->errorStruct)
                {
                    errorOutput("Error executing command: " + result->structValue->at("faultString")->stringValue + "\n");
                }
                else standardOutputReference(result->stringValue);
            }
            else if(_currentFamily != -1)
            {
                Ipc::PArray parameters = std::make_shared<Ipc::Array>();
                parameters->reserve(2);
                parameters->push_back(std::make_shared<Ipc::Variable>(_currentFamily));
                parameters->push_back(std::make_shared<Ipc::Variable>(currentCommand));
                Ipc::PVariable result = invoke("cliFamilyCommand", parameters);
                if(result->errorStruct)
                {
                    errorOutput("Error executing command: " + result->structValue->at("faultString")->stringValue + "\n");
                }
                else standardOutputReference(result->stringValue);
            }
            else
            {
                Ipc::PArray parameters = std::make_shared<Ipc::Array>();
                parameters->push_back(std::make_shared<Ipc::Variable>(currentCommand));
                Ipc::PVariable result = invoke("cliGeneralCommand", parameters);
                if(result->errorStruct)
                {
                    errorOutput("Error executing command: " + result->structValue->at("faultString")->stringValue + "\n");
                }
                else
                {
                    if(result->type == Ipc::VariableType::tString) standardOutputReference(result->stringValue);
                    else if(result->type == Ipc::VariableType::tStruct)
                    {
                        auto outputIterator = result->structValue->find("output");
                        if(outputIterator != result->structValue->end()) standardOutputReference(outputIterator->second->stringValue);

                        if(!command.empty())
                        {
                            auto exitCodeIterator = result->structValue->find("exitCode");
                            if(exitCodeIterator != result->structValue->end()) return exitCodeIterator->second->integerValue;
                        }
                    }
                }
            }

            if(!command.empty()) break;

            currentCommand = "";
        }
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return 0;
}

void CliClient::standardOutputReference(std::string& text)
{
    try
    {
        std::lock_guard<std::mutex> outputGuard(_outputMutex);
        std::cout << text;
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void CliClient::standardOutput(std::string text)
{
    try
    {
        std::lock_guard<std::mutex> outputGuard(_outputMutex);
        std::cout << text;
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void CliClient::errorOutputReference(std::string& text)
{
    try
    {
        std::lock_guard<std::mutex> outputGuard(_outputMutex);
        std::cerr << text;
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

void CliClient::errorOutput(std::string text)
{
    try
    {
        std::lock_guard<std::mutex> outputGuard(_outputMutex);
        std::cerr << text;
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
}

// {{{ RPC methods
Ipc::PVariable CliClient::output(Ipc::PArray& parameters)
{
    try
    {
        if(parameters->size() != 1) return Ipc::Variable::createError(-1, "Wrong parameter count.");
        if(parameters->at(0)->type != Ipc::VariableType::tStruct) return Ipc::Variable::createError(-1, "Parameter is not of type Struct.");

        auto errorIterator = parameters->at(0)->structValue->find("errorOutput");
        bool errorOutput = errorIterator != parameters->at(0)->structValue->end() && errorIterator->second->booleanValue;

        auto outputIterator = parameters->at(0)->structValue->find("output");
        if(outputIterator != parameters->at(0)->structValue->end())
        {
            std::lock_guard<std::mutex> outputGuard(_outputMutex);
            if(errorOutput) std::cerr << outputIterator->second->stringValue << std::flush;
            else std::cout << outputIterator->second->stringValue << std::flush;
        }

        return std::make_shared<Ipc::Variable>();
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return Ipc::Variable::createError(-32500, "Unknown application error.");
}

Ipc::PVariable CliClient::broadcastEvent(Ipc::PArray& parameters)
{
    try
    {
        if(parameters->size() != 5) return Ipc::Variable::createError(-1, "Wrong parameter count.");

        if(_printEvents)
        {
            uint64_t peerId = (uint64_t) parameters->at(1)->integerValue64;

            if(_currentPeer != 0 && peerId != _currentPeer) return std::make_shared<Ipc::Variable>();
            else if(_currentFamily != -1)
            {
                if(peerId == 0) return std::make_shared<Ipc::Variable>();
                auto parameters = std::make_shared<Ipc::Array>();
                parameters->reserve(3);
                parameters->push_back(std::make_shared<Ipc::Variable>(peerId));
                parameters->push_back(std::make_shared<Ipc::Variable>(-1));
                auto fields = std::make_shared<Ipc::Variable>(Ipc::VariableType::tArray);
                fields->arrayValue->push_back(std::make_shared<Ipc::Variable>(std::string("FAMILY")));
                parameters->push_back(fields);
                auto result = invoke("getDeviceDescription", parameters);
                if(result->errorStruct || result->structValue->at("FAMILY")->integerValue != _currentFamily) return std::make_shared<Ipc::Variable>();
            }

            for(uint32_t i = 0; i < parameters->at(3)->arrayValue->size(); ++i)
            {
                std::string value = parameters->at(4)->arrayValue->at(i)->print(false, false, true);
                BaseLib::HelperFunctions::trim(value);

                standardOutput(BaseLib::HelperFunctions::getTimeString() + ": ID >>" + (peerId > 999999 ? "0x" + BaseLib::HelperFunctions::getHexString(peerId, 8) : std::to_string(peerId)) + "<<, channel >>" + std::to_string(parameters->at(2)->integerValue) + "<<, variable >>" + parameters->at(3)->arrayValue->at(i)->stringValue + "<<, source >>" + parameters->at(0)->stringValue + "<<, value >>" + value + "<<\n");
            }
        }

        return std::make_shared<Ipc::Variable>();
    }
    catch(const std::exception& ex)
    {
        Ipc::Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    return Ipc::Variable::createError(-32500, "Unknown application error.");
}
// }}}

}