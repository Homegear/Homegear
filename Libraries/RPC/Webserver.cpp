#include "Webserver.h"
#include "../GD/GD.h"
#include "../../Version.h"

namespace RPC
{
WebServer::WebServer(std::shared_ptr<ServerSettings::Settings>& settings)
{
	_out.init(GD::bl.get());

	_settings = settings;

	_out.setPrefix("Web Server (Port " + std::to_string(settings->port) + "): ");
}

WebServer::~WebServer()
{
}

std::string WebServer::decodeURL(const std::string& url)
{
	try
	{
		std::ostringstream decoded;
		char character;
		for(std::string::const_iterator i = url.begin(); i != url.end(); ++i)
		{
			if(*i == '%')
			{
				i++;
				if(i == url.end()) return decoded.str();
				character = (char)(_math.getNumber(*i) << 4);
				i++;
				if(i == url.end()) return decoded.str();
				character += (char)_math.getNumber(*i);
				decoded << character;
			}
			else decoded << *i;
		}
		return decoded.str();
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

void WebServer::get(std::string path, std::vector<char>& request, std::vector<char>& content)
{
	try
	{
		uint32_t pos = path.find('?');
		if(pos != std::string::npos) path = path.substr(0, pos);
		path = decodeURL(path);
		BaseLib::HelperFunctions::stringReplace(path, "../", "");

		bool isDirectory = false;
		BaseLib::HelperFunctions::isDirectory(_settings->contentPath + path, isDirectory);
		if(isDirectory)
		{
			if(GD::bl->hf.fileExists(_settings->contentPath + "index.html")) path = "index.html";
			else if(GD::bl->hf.fileExists(_settings->contentPath + "index.htm")) path = "index.htm";
			else if(GD::bl->hf.fileExists(_settings->contentPath + "index.php")) path = "index.php";
			else if(GD::bl->hf.fileExists(_settings->contentPath + "index.php5")) path = "index.php5";
			else
			{
				getError(404, "Not Found", "The requested URL / was not found on this server.", content);
				return;
			}
		}
		try
		{
			if(path.front() == '/') path = path.substr(1);
			std::string contentType = "application/octet-stream";
			std::string ending = "";
			pos = path.find_last_of('.');
			if(pos != std::string::npos && (unsigned)pos < path.size() - 1) ending = path.substr(pos + 1);
			GD::bl->hf.toLower(ending);
			std::string contentString;
			std::vector<std::string> headers;
			if(ending == "html" || ending == "htm") contentType = "text/html";
			else if(ending == "xhtml") contentType = "application/xhtml+xml";
			else if(ending == "css") contentType = "text/css";
			else if(ending == "js") contentType = "application/javascript";
			else if(ending == "json") contentType = "application/json";
			else if(ending == "csv") contentType = "text/csv";
			else if(ending == "dtd") contentType = "application/xml-dtd";
			else if(ending == "xslt") contentType = "application/xslt+xml";
			else if(ending == "xml") contentType = "text/xml";
			else if(ending == "java") contentType = "text/x-java-source,java";
			else if(ending == "pdf") contentType = "application/pdf";
			else if(ending == "jpg" || ending == "jpeg") contentType = "image/jpeg";
			else if(ending == "gif") contentType = "image/gif";
			else if(ending == "png") contentType = "image/png";
			else if(ending == "bmp") contentType = "image/bmp";
			else if(ending == "tiff") contentType = "image/tiff";
			else if(ending == "svg") contentType = "image/svg+xml";
			else if(ending == "zip") contentType = "application/zip";
			else if(ending == "php" || ending == "php5")
			{
				contentType = "text/html";
				GD::scriptEngine.executeWebRequest(_settings->contentPath + path, request, content, headers);
			}
			if(content.empty())
			{
				contentString = GD::bl->hf.getFileContent(_settings->contentPath + path);
				std::string header = getHeader(contentString.size(), contentType, 200, "OK", headers);
				content.insert(content.end(), header.begin(), header.end());
				content.insert(content.end(), contentString.begin(), contentString.end());
			}
			else
			{
				std::string header = getHeader(content.size(), contentType, 200, "OK", headers);
				content.insert(content.begin(), header.begin(), header.end());
			}
		}
		catch(const std::exception& ex)
		{
			getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
			return;
		}
		catch(BaseLib::Exception& ex)
		{
			getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
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

void WebServer::post(std::string path, std::vector<char>& request, std::vector<char>& content)
{
	try
	{
		int32_t pos = path.find('?');
		if((unsigned)pos != std::string::npos) path = path.substr(0, pos);
		path = decodeURL(path);
		BaseLib::HelperFunctions::stringReplace(path, "../", "");

		bool isDirectory = false;
		BaseLib::HelperFunctions::isDirectory(_settings->contentPath + path, isDirectory);
		if(isDirectory)
		{
			if(GD::bl->hf.fileExists(_settings->contentPath + "index.php")) path = "index.php";
			else if(GD::bl->hf.fileExists(_settings->contentPath + "index.php5")) path = "index.php5";
			else
			{
				getError(404, "Not Found", "The requested URL / was not found on this server.", content);
				return;
			}
		}
		try
		{
			std::vector<std::string> headers;
			if(path.front() == '/') path = path.substr(1);
			GD::scriptEngine.executeWebRequest(_settings->contentPath + path, request, content, headers);
			std::string header = getHeader(content.size(), "text/html", 200, "OK", headers);
			content.insert(content.begin(), header.begin(), header.end());
		}
		catch(const std::exception& ex)
		{
			getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
			return;
		}
		catch(BaseLib::Exception& ex)
		{
			getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
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
		std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear " + VERSION + " at Port " + std::to_string(_settings->port) + "</address></body></html>";
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
		std::string contentString = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear " + VERSION + " at Port " + std::to_string(_settings->port) + "</address></body></html>";
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
