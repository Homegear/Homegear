#include "../GD/GD.h"
#include "RestServer.h"

namespace Homegear
{

namespace Rpc
{

RestServer::RestServer(std::shared_ptr<BaseLib::Rpc::ServerInfo::Info>& serverInfo)
{
    _out.init(GD::bl.get());

    _jsonEncoder.reset(new BaseLib::Rpc::JsonEncoder(GD::bl.get()));
    _jsonDecoder.reset(new BaseLib::Rpc::JsonDecoder(GD::bl.get()));

    _serverInfo = serverInfo;

    _out.setPrefix("REST server (Port " + std::to_string(serverInfo->port) + "): ");
}

RestServer::~RestServer()
{
}

void RestServer::getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content)
{
    try
    {
        std::vector<std::string> additionalHeaders;
        std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear " + GD::homegearVersion + " at Port " + std::to_string(_serverInfo->port) + "</address></body></html>";
        std::string header;
        _http.constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
        content.insert(content.end(), header.begin(), header.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RestServer::getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content, std::vector<std::string>& additionalHeaders)
{
    try
    {
        std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear " + GD::homegearVersion + " at Port " + std::to_string(_serverInfo->port) + "</address></body></html>";
        std::string header;
        _http.constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
        content.insert(content.end(), header.begin(), header.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void RestServer::process(BaseLib::PRpcClientInfo clientInfo, BaseLib::Http& http, std::shared_ptr<BaseLib::TcpSocket> socket)
{
    std::vector<char> content;
    try
    {
        if(!socket)
        {
            _out.printError("Error: Socket is nullptr.");
            return;
        }

        std::string path = http.getHeader().path;

        std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(path, '/');
        if(parts.size() < 4)
        {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            return;
        }

        std::vector<std::string> headers;
        std::string header;
        std::string contentType = "application/json";
        std::string contentString;

        if(parts.at(2) != "v1")
        {
            contentString = "{\"result\":\"error\",\"message\":\"Unknown API version.\"}";
            _http.constructHeader(contentString.size(), contentType, 200, "OK", headers, header);
            content.insert(content.end(), header.begin(), header.end());
            content.insert(content.end(), contentString.begin(), contentString.end());
            send(socket, content);
            return;
        }

        std::string request = parts.at(3);

        if(http.getHeader().method == "GET")
        {
            if(request == "get" || request == "variable")
            {
                if(parts.size() != 7)
                {
                    getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
                    send(socket, content);
                    return;
                }

                uint64_t peerId = BaseLib::Math::getNumber64(parts.at(4));
                int32_t channel = BaseLib::Math::getNumber(parts.at(5));
                std::string typeOrVariable = parts.at(6);

                GD::out.printInfo("Info: REST RPC call received. Method: getValue");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                parameters->arrayValue->reserve(3);
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t) peerId)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(typeOrVariable)));
                std::string methodName = "getValue";
                BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                else
                {
                    BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    responseJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>("success"));
                    responseJson->structValue->emplace("value", response);
                    _jsonEncoder->encode(responseJson, contentString);
                }
            }
            else if(request == "config")
            {
                if(parts.size() != 7)
                {
                    getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
                    send(socket, content);
                    return;
                }

                uint64_t peerId = BaseLib::Math::getNumber64(parts.at(4));
                int32_t channel = BaseLib::Math::getNumber(parts.at(5));
                std::string typeOrVariable = parts.at(6);

                GD::out.printInfo("Info: REST RPC call received. Method: getParamset");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                parameters->arrayValue->reserve(3);
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t) peerId)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(typeOrVariable)));;
                std::string methodName = "getParamset";
                BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                else
                {
                    BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    responseJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>("success"));
                    responseJson->structValue->emplace("value", response);
                    _jsonEncoder->encode(responseJson, contentString);
                }
            }
            else if(request == "families")
            {
                GD::out.printInfo("Info: REST RPC call received. Method: listFamilies");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                std::string methodName = "listFamilies";
                BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                else
                {
                    BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    responseJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>("success"));
                    responseJson->structValue->emplace("value", response);
                    _jsonEncoder->encode(responseJson, contentString);
                }
            }
            else if(request == "devices")
            {
                GD::out.printInfo("Info: REST RPC call received. Method: listDevices");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                parameters->arrayValue->reserve(1);
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(false)));
                std::string methodName = "listDevices";
                BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                else
                {
                    BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    responseJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>("success"));
                    responseJson->structValue->emplace("value", response);
                    _jsonEncoder->encode(responseJson, contentString);
                }
            }
            else if(request == "channels")
            {
                GD::out.printInfo("Info: REST RPC call received. Method: listDevices (with channels)");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                parameters->arrayValue->reserve(1);
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(true)));
                std::string methodName = "listDevices";
                BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                else
                {
                    BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    responseJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>("success"));
                    responseJson->structValue->emplace("value", response);
                    _jsonEncoder->encode(responseJson, contentString);
                }
            }
            else if(request == "channelinfo")
            {
                if(parts.size() != 6)
                {
                    getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
                    send(socket, content);
                    return;
                }

                uint64_t peerId = BaseLib::Math::getNumber64(parts.at(4));
                int32_t channel = BaseLib::Math::getNumber(parts.at(5));

                GD::out.printInfo("Info: REST RPC call received. Method: getParamset");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                parameters->arrayValue->reserve(3);
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(peerId)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable("MASTER")));
                std::string methodName = "getParamsetDescription";
                BaseLib::PVariable response1 = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                parameters->arrayValue->at(2)->stringValue = "LINK";
                BaseLib::PVariable response2 = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                parameters->arrayValue->at(2)->stringValue = "VALUES";
                BaseLib::PVariable response3 = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);

                if(response1->errorStruct && response2->errorStruct && response3->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response1->structValue->at("faultString")->stringValue + "\"}";
                else
                {
                    if(response1->errorStruct) response1 = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    if(response2->errorStruct) response2 = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    if(response3->errorStruct) response3 = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);

                    BaseLib::PVariable response = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    response->structValue->emplace("MASTER", response1);
                    response->structValue->emplace("LINK", response2);
                    response->structValue->emplace("VALUES", response3);

                    BaseLib::PVariable responseJson = std::make_shared<BaseLib::Variable>(BaseLib::VariableType::tStruct);
                    responseJson->structValue->emplace("result", std::make_shared<BaseLib::Variable>("success"));
                    responseJson->structValue->emplace("value", response);
                    _jsonEncoder->encode(responseJson, contentString);
                }
            }
            else
            {
                contentString = "{\"result\":\"error\",\"message\":\"Unknown method.\"}";
            }
        }
        else if(http.getHeader().method == "POST" || http.getHeader().method == "PUT")
        {
            BaseLib::PVariable json;

            try
            {
                json = _jsonDecoder->decode(http.getContent());
            }
            catch(std::exception& ex)
            {
                getError(500, "Server error", std::string("Could not decode JSON: ") + ex.what(), content);
                send(socket, content);
                return;
            }

            if(request == "set" || request == "variable")
            {
                if(parts.size() != 7)
                {
                    getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
                    send(socket, content);
                    return;
                }

                uint64_t peerId = BaseLib::Math::getNumber64(parts.at(4));
                int32_t channel = BaseLib::Math::getNumber(parts.at(5));
                std::string typeOrVariable = parts.at(6);

                GD::out.printInfo("Info: REST RPC call received. Method: setValue");

                auto structIterator = json->structValue->find("value");
                if(structIterator == json->structValue->end()) contentString = "{\"result\":\"error\",\"message\":\"No value specified.\"}";
                else
                {
                    BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                    parameters->arrayValue->reserve(4);
                    parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t) peerId)));
                    parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
                    parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(typeOrVariable)));
                    parameters->arrayValue->push_back(structIterator->second);
                    std::string methodName = "setValue";
                    BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                    if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                    else contentString = "{\"result\":\"success\"}";
                }
            }
            else if(request == "config")
            {
                if(parts.size() != 7)
                {
                    getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
                    send(socket, content);
                    return;
                }

                uint64_t peerId = BaseLib::Math::getNumber64(parts.at(4));
                int32_t channel = BaseLib::Math::getNumber(parts.at(5));
                std::string typeOrVariable = parts.at(6);

                GD::out.printInfo("Info: REST RPC call received. Method: putParamset");
                BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
                parameters->arrayValue->reserve(4);
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t) peerId)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
                parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(typeOrVariable)));
                parameters->arrayValue->push_back(json);
                std::string methodName = "putParamset";
                BaseLib::PVariable response = GD::rpcServers.begin()->second->callMethod(clientInfo, methodName, parameters);
                if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
                else contentString = "{\"result\":\"success\"}";
            }
            else
            {
                contentString = "{\"result\":\"error\",\"message\":\"Unknown method.\"}";
            }
        }

        _http.constructHeader(contentString.size(), contentType, 200, "OK", headers, header);
        content.insert(content.end(), header.begin(), header.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
        send(socket, content);
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        getError(500, "Server error", ex.what(), content);
        send(socket, content);
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        getError(500, "Server error", "Unknown error.", content);
        send(socket, content);
    }
}

void RestServer::send(std::shared_ptr<BaseLib::TcpSocket>& socket, std::vector<char>& data)
{
    try
    {
        if(data.empty()) return;
        try
        {
            socket->proofwrite(data);
        }
        catch(BaseLib::SocketDataLimitException& ex)
        {
            _out.printWarning(std::string("Warning: ") + ex.what());
        }
        catch(const BaseLib::SocketOperationException& ex)
        {
            _out.printInfo(std::string("Info: ") + ex.what());
        }
        socket->close();
    }
    catch(const std::exception& ex)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}

}