#include "../GD/GD.h"
#include "../UPnP/UPnP.h"
#include "WebServer.h"

namespace WebServer
{
WebServer::WebServer(std::shared_ptr<BaseLib::Rpc::ServerInfo::Info>& serverInfo)
{
	_out.init(GD::bl.get());

	_serverInfo = serverInfo;

	_out.setPrefix("Web server (Port " + std::to_string(serverInfo->port) + "): ");
}

WebServer::~WebServer()
{
}

void WebServer::get(BaseLib::Http& http, std::shared_ptr<BaseLib::SocketOperations> socket)
{
	try
	{
		if(!socket)
		{	_out.printError("Error: Socket is nullptr.");
			return;
		}
		std::string path = http.getHeader().path;

		BaseLib::EventHandlers eventHandlers = getEventHandlers();
		for(BaseLib::EventHandlers::const_iterator i = eventHandlers.begin(); i != eventHandlers.end(); ++i)
		{
			i->second->lock();
			try
			{
				if(i->second->handler() && ((BaseLib::Rpc::IWebserverEventSink*)i->second->handler())->onGet(_serverInfo, http, socket, path))
				{
					i->second->unlock();
					return;
				}
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
			i->second->unlock();
		}

		std::vector<std::string> headers;
		std::vector<char> content;
		if(!path.empty() && path.front() == '/') path = path.substr(1);

		bool isDirectory = false;
		BaseLib::Io::isDirectory(_serverInfo->contentPath + path, isDirectory);
		if(isDirectory)
		{
			if(!path.empty() && path.back() != '/')
			{
				path.push_back('/');
				std::vector<std::string> additionalHeaders({std::string("Location: ") + path});
				getError(301, "Moved Permanently", "The document has moved <a href=\"" + path + "\">here</a>.", content, additionalHeaders);
				send(socket, content);
				return;
			}
			if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.php")) path += "index.php";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.php5")) path += "index.php5";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.php7")) path += "index.php7";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.hgs")) path += "index.hgs";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.html")) path += "index.html";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.htm")) path += "index.htm";
			else
			{
				getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
				send(socket, content);
				return;
			}
		}

		// {{{ Node-RED test
		if(path == "node-red/public/locales/editor" || path == "node-red/public/locales/node-red")
		{
			path = "node-red/public/staticTemp" + path.substr(15);
			std::string contentString;
			if(http.getHeader().method == "GET") contentString = GD::bl->io.getFileContent(_serverInfo->contentPath + path);
			std::string header;
			_http.constructHeader(contentString.size(), "application/json", 304, "Not Modified", headers, header);
			content.insert(content.end(), header.begin(), header.end());
			if(!contentString.empty()) content.insert(content.end(), contentString.begin(), contentString.end());
			send(socket, content);
			return;
		}
		else if(path == "node-red/public/settings" || path == "node-red/public/library/flows")
		{
			path = "node-red/public/staticTemp" + path.substr(15);
			std::string contentString;
			if(http.getHeader().method == "GET") contentString = GD::bl->io.getFileContent(_serverInfo->contentPath + path);
			std::string header;
			_http.constructHeader(contentString.size(), "application/json", 200, "OK", headers, header);
			content.insert(content.end(), header.begin(), header.end());
			if(!contentString.empty()) content.insert(content.end(), contentString.begin(), contentString.end());
			send(socket, content);
			return;
		}
		else if(path == "node-red/public/nodes")
		{
			path = "node-red/public/staticTemp" + path.substr(15);

			std::string contentString;
			if(http.getHeader().method == "GET") contentString = GD::bl->io.getFileContent(_serverInfo->contentPath + path);
			std::string header;
			_http.constructHeader(contentString.size(), "text/html", 200, "OK", headers, header);
			content.insert(content.end(), header.begin(), header.end());
			if(!contentString.empty()) content.insert(content.end(), contentString.begin(), contentString.end());
			send(socket, content);
			return;
		}
		// }}}

		if(!BaseLib::Io::fileExists(_serverInfo->contentPath + path))
		{
			GD::out.printWarning("Warning: Requested URL not found: " + path);
			getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			send(socket, content);
			return;
		}

		try
		{
			_out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to " + _serverInfo->contentPath + path + ", method: GET)");
			std::string ending = "";
			int32_t pos = path.find_last_of('.');
			if(pos != (signed)std::string::npos && (unsigned)pos < path.size() - 1) ending = path.substr(pos + 1);
			GD::bl->hf.toLower(ending);
			std::string contentString;
			if(ending == "php" || ending == "php5" || ending == "php7" || ending == "hgs")
			{
				std::string fullPath = _serverInfo->contentPath + path;
				BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::web, fullPath, http, _serverInfo));
				scriptInfo->socket = socket;
				scriptInfo->scriptHeadersCallback = std::bind(&WebServer::sendHeaders, this, std::placeholders::_1, std::placeholders::_2);
				GD::scriptEngineServer->executeScript(scriptInfo, true);
				socket->close();
				return;
			}
			std::string contentType = _http.getMimeType(ending);
			if(contentType.empty()) contentType = "application/octet-stream";
			//Don't return content when method is "HEAD"
			if(http.getHeader().method == "GET") contentString = GD::bl->io.getFileContent(_serverInfo->contentPath + path);
			std::string header;
			_http.constructHeader(contentString.size(), contentType, 200, "OK", headers, header);
			content.insert(content.end(), header.begin(), header.end());
			if(!contentString.empty()) content.insert(content.end(), contentString.begin(), contentString.end());
			send(socket, content);
		}
		catch(const std::exception& ex)
		{
			getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			send(socket, content);
			return;
		}
		catch(BaseLib::Exception& ex)
		{
			getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			send(socket, content);
			return;
		}
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

void WebServer::post(BaseLib::Http& http, std::shared_ptr<BaseLib::SocketOperations> socket)
{
	try
	{
		std::vector<char> content;
		if(!socket)
		{	_out.printError("Error: Socket is nullptr.");
			return;
		}
		std::string path = http.getHeader().path;

		BaseLib::EventHandlers eventHandlers = getEventHandlers();
		for(BaseLib::EventHandlers::const_iterator i = eventHandlers.begin(); i != eventHandlers.end(); ++i)
		{
			i->second->lock();
			try
			{
				if(i->second->handler() && ((BaseLib::Rpc::IWebserverEventSink*)i->second->handler())->onGet(_serverInfo, http, socket, path))
				{
					i->second->unlock();
					return;
				}
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
			i->second->unlock();
		}

		if(!path.empty() && path.front() == '/') path = path.substr(1);

		bool isDirectory = false;
		BaseLib::Io::isDirectory(_serverInfo->contentPath + path, isDirectory);
		if(isDirectory)
		{
			if(!path.empty() && path.back() != '/')
			{
				path.push_back('/');
				std::vector<std::string> additionalHeaders({std::string("Location: ") + path});
				getError(301, "Moved Permanently", "The document has moved <a href=\"" + path + "\">here</a>.", content, additionalHeaders);
				send(socket, content);
				return;
			}
			if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.php")) path += "index.php";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.php5")) path += "index.php5";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.php7")) path += "index.php7";
			else if(GD::bl->io.fileExists(_serverInfo->contentPath + path + "index.hgs")) path += "index.hgs";
			else
			{
				getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
				send(socket, content);
				return;
			}
		}

		if(!BaseLib::Io::fileExists(_serverInfo->contentPath + path))
		{
			getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			send(socket, content);
			return;
		}

		try
		{
			_out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to: \"" + _serverInfo->contentPath + path + "\", method: POST)");
			std::string fullPath = _serverInfo->contentPath + path;
			BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::web, fullPath, http, _serverInfo));
			scriptInfo->socket = socket;
			scriptInfo->scriptHeadersCallback = std::bind(&WebServer::sendHeaders, this, std::placeholders::_1, std::placeholders::_2);
			GD::scriptEngineServer->executeScript(scriptInfo, true);
			socket->close();
		}
		catch(const std::exception& ex)
		{
			getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			send(socket, content);
			return;
		}
		catch(BaseLib::Exception& ex)
		{
			getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			send(socket, content);
			return;
		}
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

void WebServer::getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content)
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

void WebServer::getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content, std::vector<std::string>& additionalHeaders)
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

void WebServer::send(std::shared_ptr<BaseLib::SocketOperations>& socket, std::vector<char>& data)
{
	try
	{
		if(data.empty()) return;
		try
		{
			//Sleep a tiny little bit. Some clients like don't accept responses too fast.
			std::this_thread::sleep_for(std::chrono::milliseconds(22));
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

void WebServer::sendHeaders(BaseLib::ScriptEngine::PScriptInfo& scriptInfo, BaseLib::PVariable& headers)
{
	try
	{
		if(!scriptInfo || !scriptInfo->socket || !headers) return;
		BaseLib::Struct::iterator headerIterator = headers->structValue->find("RESPONSE_CODE");
		int32_t responseCode = 500;
		if(headerIterator != headers->structValue->end()) responseCode = headerIterator->second->integerValue;
		if(responseCode == 0) responseCode = 500;
		scriptInfo->http.getHeader().responseCode = responseCode;
		headers->structValue->erase("RESPONSE_CODE");

		{
			std::lock_guard<std::mutex> sendHeaderGuard(_sendHeaderHookMutex);
			for(std::map<std::string, std::function<void(BaseLib::Http& http, BaseLib::PVariable& headers)>>::iterator i = _sendHeaderHooks.begin(); i != _sendHeaderHooks.end(); ++i)
			{
				i->second(scriptInfo->http, headers);
			}
		}

		std::string output;
		output.reserve(1024);
		output.append("HTTP/1.1 " + std::to_string(responseCode) + ' ' + scriptInfo->http.getStatusText(responseCode) + "\r\n");
		for(BaseLib::Struct::iterator i = headers->structValue->begin(); i != headers->structValue->end(); ++i)
		{
			if(output.size() + i->first.size() + i->second->stringValue.size() + 6 > output.capacity()) output.reserve(output.capacity() + 1024);
			output.append(i->first + ": " + i->second->stringValue + "\r\n");
		}
		output.append("\r\n");
		scriptInfo->socket->proofwrite(output.c_str(), output.size());
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

// {{{ Hooks
void WebServer::registerSendHeadersHook(std::string& moduleName, std::function<void(BaseLib::Http& http, BaseLib::PVariable& headers)>& callback)
{
	try
	{
		if(!callback) return;
		std::lock_guard<std::mutex> sendHeaderGuard(_sendHeaderHookMutex);
		_sendHeaderHooks[moduleName] = callback;
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
// }}}

}
