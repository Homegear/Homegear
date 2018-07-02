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

#include "CliClient.h"
#include "../GD/GD.h"

#include <readline/readline.h>
#include <readline/history.h>

CliClient::CliClient(std::string socketPath) : IIpcClient(socketPath)
{
	_localRpcMethods.emplace("broadcastEvent", std::bind(&CliClient::broadcastEvent, this, std::placeholders::_1));
	_localRpcMethods.emplace("cliOutput", std::bind(&CliClient::output, this, std::placeholders::_1));
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
        if (result->errorStruct)
        {
            error = true;
            Ipc::Output::printCritical("Critical: Could not register RPC method cliOutput: " + result->structValue->at("faultString")->stringValue);
        }
        if (error) return;

        std::unique_lock<std::mutex> waitLock(_onConnectWaitMutex);
        waitLock.unlock();
        _onConnectConditionVariable.notify_all();
    }
    catch (const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (Ipc::IpcException& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

int32_t CliClient::terminal(std::string& command)
{
    try
    {
        start();

        std::unique_lock<std::mutex> waitLock(_onConnectWaitMutex);
        int64_t startTime = Ipc::HelperFunctions::getTime();
        while (!_onConnectConditionVariable.wait_for(waitLock, std::chrono::milliseconds(2000), [&]
        {
            if(Ipc::HelperFunctions::getTime() - startTime > 2000) return true;
            else return connected();
        }));

        if(!connected())
        {
            GD::out.printError("Could not connect to socket. Error: " + std::string(strerror(errno)));
            return 3;
        }

        if(GD::bl->debugLevel >= 4) std::cout << "Connected to Homegear (version " + GD::bl->version() + ")." << std::endl;

        rl_bind_key('\t', rl_abort); //no autocompletion

        std::string level = "";
        std::string lastCommand;
        std::string currentCommand;
        char* sendBuffer;
        std::array<char, 1025> receiveBuffer;
        int32_t bytes = 0;
        int64_t timeoutCounter = 0;
        while(!command.empty() || (sendBuffer = readline((level + "> ").c_str())) != NULL)
        {
            if(command.empty())
            {
                if(!connected()) break;
                bytes = strnlen(sendBuffer, 1000000);
                if(bytes == 0 || sendBuffer[0] == '\n' || sendBuffer[0] == 0) continue;
                if(bytes >= 4 && (strncmp(sendBuffer, "quit", 4) == 0 || strncmp(sendBuffer, "exit", 4) == 0 || strncmp(sendBuffer, "moin", 4) == 0))
                {
                    stop();
                    free(sendBuffer);
                    return 4;
                }

                //Send command

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
            else
            {
                //Send command
            }
        }
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return 0;
}

// {{{ RPC methods
Ipc::PVariable CliClient::output(Ipc::PArray& parameters)
{
    try
    {
        if(parameters->size() != 1) return Ipc::Variable::createError(-1, "Wrong parameter count.");
        if(parameters->at(0)->type != Ipc::VariableType::tString) return Ipc::Variable::createError(-1, "Parameter is not of type String.");

        std::cout << parameters->at(0)->stringValue << std::endl;

        return std::make_shared<Ipc::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Ipc::Variable::createError(-32500, "Unknown application error.");
}

Ipc::PVariable CliClient::broadcastEvent(Ipc::PArray& parameters)
{
    try
    {
        if(parameters->size() != 5) return Ipc::Variable::createError(-1, "Wrong parameter count.");



        return std::make_shared<Ipc::Variable>();
    }
    catch(const std::exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return Ipc::Variable::createError(-32500, "Unknown application error.");
}
// }}}