#include "Webserver.h"
#include "../GD/GD.h"
#include "../UPnP/UPnP.h"

namespace RPC
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

void WebServer::get(BaseLib::HTTP& http, std::shared_ptr<BaseLib::SocketOperations> socket)
{
	try
	{
		if(!socket)
		{	_out.printError("Error: Socket is nullptr.");
			return;
		}
		std::string path = http.getHeader()->path;

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
				getError(404, "Not Found", "The requested URL / was not found on this server.", content);
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
			_out.printInfo("Client is requesting: " + http.getHeader()->path + " (translated to " + _serverInfo->contentPath + path + ", method: GET)");
			std::string ending = "";
			int32_t pos = path.find_last_of('.');
			if(pos != (signed)std::string::npos && (unsigned)pos < path.size() - 1) ending = path.substr(pos + 1);
			GD::bl->hf.toLower(ending);
			std::string contentString;
#ifdef SCRIPTENGINE
			if(ending == "php" || ending == "php5" || ending == "php7" || ending == "hgs")
			{
				GD::scriptEngine->executeWebRequest(_serverInfo->contentPath + path, http, _serverInfo, socket);
				socket->close();
				return;
			}
#endif
			std::string contentType = _http.getMimeType(ending);
			if(contentType.empty()) contentType = "application/octet-stream";
			//Don't return content when method is "HEAD"
			if(http.getHeader()->method == "GET") contentString = GD::bl->io.getFileContent(_serverInfo->contentPath + path);
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

void WebServer::post(BaseLib::HTTP& http, std::shared_ptr<BaseLib::SocketOperations> socket)
{
	try
	{
		std::vector<char> content;
#ifdef SCRIPTENGINE
		if(!socket)
		{	_out.printError("Error: Socket is nullptr.");
			return;
		}
		std::string path = http.getHeader()->path;

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
				getError(404, _http.getStatusText(404), "The requested URL / was not found on this server.", content);
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
			_out.printInfo("Client is requesting: " + http.getHeader()->path + " (translated to: \"" + _serverInfo->contentPath + path + "\", method: POST)");
			GD::scriptEngine->executeWebRequest(_serverInfo->contentPath + path, http, _serverInfo, socket);
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
#else
		getError(304, _http.getStatusText(304), "Homegear is compiled without script engine.", content);
		send(socket, content);
#endif
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

}
