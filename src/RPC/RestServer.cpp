#include "../GD/GD.h"
#include "RestServer.h"

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
		std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear " + VERSION + " at Port " + std::to_string(_serverInfo->port) + "</address></body></html>";
		std::string header;
		_http.constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
		content.insert(content.end(), header.begin(), header.end());
		content.insert(content.end(), contentString.begin(), contentString.end());
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
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
		std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear " + VERSION + " at Port " + std::to_string(_serverInfo->port) + "</address></body></html>";
		std::string header;
		_http.constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
		content.insert(content.end(), header.begin(), header.end());
		content.insert(content.end(), contentString.begin(), contentString.end());
	}
	catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
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
		{	_out.printError("Error: Socket is nullptr.");
			return;
		}

		std::string path = http.getHeader().path;

		std::vector<std::string> parts = BaseLib::HelperFunctions::splitAll(path, '/');
		if(parts.size() != 7)
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
		uint64_t peerId = BaseLib::Math::getNumber64(parts.at(4));
		int32_t channel = BaseLib::Math::getNumber(parts.at(5));
		std::string typeOrVariable = parts.at(6);

		if(http.getHeader().method == "GET")
		{
			if(request == "get")
			{
				GD::out.printInfo("Info: REST RPC call received. Method: getValue");
				BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
				parameters->arrayValue->reserve(3);
				parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
				parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
				parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(typeOrVariable)));
				BaseLib::PVariable response = GD::rpcServers.begin()->second.callMethod(clientInfo, "getValue", parameters);
				if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
				else
				{
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
			catch(BaseLib::Exception& ex)
			{
				getError(500, "Server error", "Could not decode JSON: " + ex.what(), content);
				send(socket, content);
				return;
			}

			auto structIterator = json->structValue->find("value");
			if(request == "set")
			{
				GD::out.printInfo("Info: REST RPC call received. Method: setValue");

				if(structIterator == json->structValue->end()) contentString = "{\"result\":\"error\",\"message\":\"No value specified.\"}";
				else
				{
					BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
					parameters->arrayValue->reserve(4);
					parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
					parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
					parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(typeOrVariable)));
					parameters->arrayValue->push_back(structIterator->second);
					BaseLib::PVariable response = GD::rpcServers.begin()->second.callMethod(clientInfo, "setValue", parameters);
					if(response->errorStruct) contentString = "{\"result\":\"error\",\"message\":\"" + response->structValue->at("faultString")->stringValue + "\"}";
					else contentString = "{\"result\":\"success\"}";
				}
			}
			else if(request == "config")
			{
				GD::out.printInfo("Info: REST RPC call received. Method: putParamset");
				BaseLib::PVariable parameters(new BaseLib::Variable(BaseLib::VariableType::tArray));
				parameters->arrayValue->reserve(4);
				parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable((uint32_t)peerId)));
				parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(channel)));
				parameters->arrayValue->push_back(BaseLib::PVariable(new BaseLib::Variable(parts.at(5))));
				BaseLib::PVariable value = structIterator->second;
				if(value) parameters->arrayValue->push_back(value);
				BaseLib::PVariable response = GD::rpcServers.begin()->second.callMethod(clientInfo, "putParamset", parameters);
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
    catch(BaseLib::Exception& ex)
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
			_out.printWarning("Warning: " + ex.what());
		}
		catch(const BaseLib::SocketOperationException& ex)
		{
			_out.printInfo("Info: " + ex.what());
		}
		socket->close();
	}
    catch(const std::exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(BaseLib::Exception& ex)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

}
