#include "Webserver.h"
#include "../GD/GD.h"
#include "../../Version.h"

namespace RPC
{
WebServer::WebServer(std::shared_ptr<ServerInfo::Info>& serverInfo)
{
	_out.init(GD::bl.get());

	_serverInfo = serverInfo;

	_out.setPrefix("Web Server (Port " + std::to_string(serverInfo->port) + "): ");
}

WebServer::~WebServer()
{
}

std::string WebServer::getHeader(uint32_t contentLength, std::string contentType, int32_t code, std::string codeDescription, std::vector<std::string>& additionalHeaders)
{
	try
	{
		std::string additionalHeader;
		additionalHeader.reserve(1024);
		for(std::vector<std::string>::iterator i = additionalHeaders.begin(); i != additionalHeaders.end(); ++i)
		{
			BaseLib::HelperFunctions::trim(*i);
			if((*i).find("Location: ") == 0)
			{
				code = 301;
				codeDescription = "Moved Permanently";
			}
			if(!(*i).empty()) additionalHeader.append(*i + "\r\n");
		}

		std::string header;
		header.reserve(1024);
		header.append("HTTP/1.1 " + std::to_string(code) + " " + codeDescription + "\r\n");
		header.append("Connection: close\r\n");
		header.append("Content-Type: " + contentType + "\r\n");
		header.append(additionalHeader);
		header.append("Content-Length: ").append(std::to_string(contentLength + 21)).append("\r\n\r\n");
		return header;
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
    return "";
}

void WebServer::get(BaseLib::HTTP& http, std::vector<char>& content)
{
	try
	{
		std::string path = http.getHeader()->path;
		std::vector<std::string> headers;
		if(GD::bl->settings.enableUPnP() && path == "/description.xml")
		{
			GD::uPnP.getDescription(_serverInfo->port, content);
			if(!content.empty())
			{
				std::string header = getHeader(content.size(), "text/xml", 200, "OK", headers);
				content.insert(content.begin(), header.begin(), header.end());
				return;
			}
		}

		if(!path.empty() && path.front() == '/') path = path.substr(1);
		bool isDirectory = false;
		BaseLib::HelperFunctions::isDirectory(_serverInfo->contentPath + path, isDirectory);
		if(isDirectory)
		{
			if(!path.empty() && path.back() != '/') path.push_back('/');
			if(GD::bl->hf.fileExists(_serverInfo->contentPath + path + "index.php")) path += "index.php";
			else if(GD::bl->hf.fileExists(_serverInfo->contentPath + path + "index.php5")) path += "index.php5";
			else if(GD::bl->hf.fileExists(_serverInfo->contentPath + path + "index.html")) path += "index.html";
			else if(GD::bl->hf.fileExists(_serverInfo->contentPath + path + "index.htm")) path += "index.htm";
			else
			{
				getError(404, "Not Found", "The requested URL / was not found on this server.", content);
				return;
			}
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
			if(ending == "php" || ending == "php5")
			{
				GD::scriptEngine.executeWebRequest(_serverInfo->contentPath + path, http, _serverInfo, content);
				return;
			}
#endif
			std::string contentType = BaseLib::HTTP::getMimeType(ending);
			if(contentType.empty()) contentType = "application/octet-stream";
			//Don't return content when method is "HEAD"
			if(http.getHeader()->method == "GET") contentString = GD::bl->hf.getFileContent(_serverInfo->contentPath + path);
			std::string header = getHeader(contentString.size(), contentType, 200, "OK", headers);
			content.insert(content.end(), header.begin(), header.end());
			if(!contentString.empty()) content.insert(content.end(), contentString.begin(), contentString.end());
		}
		catch(const std::exception& ex)
		{
			getError(404, BaseLib::HTTP::getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			return;
		}
		catch(BaseLib::Exception& ex)
		{
			getError(404, BaseLib::HTTP::getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
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

void WebServer::post(BaseLib::HTTP& http, std::vector<char>& content)
{
	try
	{
#ifdef SCRIPTENGINE
		std::string path = http.getHeader()->path;
		if(!path.empty() && path.front() == '/') path = path.substr(1);

		bool isDirectory = false;
		BaseLib::HelperFunctions::isDirectory(_serverInfo->contentPath + path, isDirectory);
		if(isDirectory)
		{
			if(!path.empty() && path.back() != '/') path.push_back('/');
			if(GD::bl->hf.fileExists(_serverInfo->contentPath + path + "index.php")) path += "index.php";
			else if(GD::bl->hf.fileExists(_serverInfo->contentPath + path + "index.php5")) path += "index.php5";
			else
			{
				getError(404, BaseLib::HTTP::getStatusText(404), "The requested URL / was not found on this server.", content);
				return;
			}
		}
		try
		{
			_out.printInfo("Client is requesting: " + http.getHeader()->path + " (translated to: \"" + _serverInfo->contentPath + path + "\", method: POST)");
			GD::scriptEngine.executeWebRequest(_serverInfo->contentPath + path, http, _serverInfo, content);
		}
		catch(const std::exception& ex)
		{
			getError(404, BaseLib::HTTP::getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			return;
		}
		catch(BaseLib::Exception& ex)
		{
			getError(404, BaseLib::HTTP::getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
			return;
		}
#else
		getError(304, BaseLib::HTTP::getStatusText(304), "Homegear is compiled without script engine.", content);
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
		std::string header = getHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders);
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
		std::string header = getHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders);
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

}
