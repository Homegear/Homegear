#include "../GD/GD.h"
#include "../UPnP/UPnP.h"
#include "WebServer.h"

#include <homegear-base/Encoding/GZip.h>

namespace Homegear {

namespace WebServer {

WebServer::WebServer(std::shared_ptr<BaseLib::Rpc::ServerInfo::Info> &serverInfo) {
  _out.init(GD::bl.get());

  _serverInfo = serverInfo;

  _out.setPrefix("Web server (Port " + std::to_string(serverInfo->port) + "): ");
}

WebServer::~WebServer() {
}

void WebServer::get(BaseLib::PRpcClientInfo clientInfo, BaseLib::Http &http, std::shared_ptr<C1Net::TcpSocket> socket, int32_t cacheTime) {
  try {
    if (!socket) {
      _out.printError("Error: Socket is nullptr.");
      return;
    }
    std::string path = http.getHeader().path;

    BaseLib::EventHandlers eventHandlers = getEventHandlers();
    for (const auto &eventHandler: eventHandlers) {
      eventHandler.second->lock();
      try {
        if (eventHandler.second->handler() && GD::bl->settings.devLog()) GD::out.printInfo("Devlog: Calling onGet event handler.");
        if (eventHandler.second->handler() && ((BaseLib::Rpc::IWebserverEventSink *)eventHandler.second->handler())->onGet(_serverInfo, http, socket, path)) {
          eventHandler.second->unlock();
          if (eventHandler.second->handler() && GD::bl->settings.devLog()) GD::out.printInfo("Devlog: onGet event handler handled event.");
          return;
        }
        if (eventHandler.second->handler() && GD::bl->settings.devLog()) GD::out.printInfo("Devlog: onGet event handler did not handle event.");
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
      eventHandler.second->unlock();
    }

    std::vector<char> content;
    if (!path.empty() && path.front() == '/') path = path.substr(1);

    if (GD::bl->settings.enableNodeBlue() && (path.compare(0, 6, "flows/") == 0 || path == "flows" || path.compare(0, 3, "nb/") == 0 || path == "nb")) {
      path = "/node-blue/";
      std::vector<std::string> additionalHeaders({std::string("Location: ") + path});
      getError(301, "Moved Permanently", "The document has moved <a href=\"" + path + "\">here</a>.", content, additionalHeaders);
      send(socket, content);
      socket->Shutdown();
      return;
    }

    bool isDirectory = false;
    BaseLib::Io::isDirectory(_serverInfo->contentPath + path, isDirectory);
    if (isDirectory || path == "node-blue" || path == "node-blue/" || path == "ui" || path == "ui/" || path == "admin" || path == "admin/" || path == "web-ssh" || path == "web-ssh/") {
      if (!path.empty() && path.back() != '/') {
        path = '/' + path + '/';
        std::vector<std::string> additionalHeaders({std::string("Location: ") + path});
        getError(301, "Moved Permanently", "The document has moved <a href=\"" + path + "\">here</a>.", content, additionalHeaders);
        send(socket, content);
        socket->Shutdown();
        return;
      }
      if (path == "node-blue/") path = "node-blue/index.php";
      else if (path.compare(0, 3, "ui/") == 0 && (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.hgs"))) {}
      else if (path.compare(0, 6, "admin/") == 0 && (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.hgs"))) {}
      else if (path.compare(0, 8, "web-ssh/") == 0 && (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.hgs"))) {}
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.php")) path += "index.php";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.php5")) path += "index.php5";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.php7")) path += "index.php7";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.hgs")) path += "index.hgs";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.html")) path += "index.html";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.htm")) path += "index.htm";
      else {
        getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
        send(socket, content);
        socket->Shutdown();
        return;
      }
    }

    if (GD::bl->settings.enableNodeBlue() && path.compare(0, 10, "node-blue/") == 0) {
      _out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to " + _serverInfo->contentPath + path + ", method: GET)");
      std::string responseEncoding;
      std::string responseHeader;
      std::string contentString = GD::nodeBlueServer->handleGet(path, http, responseEncoding, responseHeader);
      if (contentString == "unauthorized") {
        getError(401, _http.getStatusText(401), "You are not logged in.", content);
        send(socket, content);
        socket->Shutdown();
        return;
      } else if (!responseHeader.empty() || !contentString.empty()) {
        if (responseHeader.empty()) {
          std::vector<std::string> headers;
          BaseLib::Http::constructHeader(contentString.size(), responseEncoding, 200, "OK", headers, responseHeader);
        }
        content.insert(content.end(), responseHeader.begin(), responseHeader.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
        send(socket, content);
        socket->Shutdown();
        return;
      }
    }

    if (!BaseLib::Io::fileExists(_serverInfo->contentPath + path) && path != "node-blue/index.php" && path != "node-blue/signin.php" && path.compare(0, 3, "ui/") != 0 && path.compare(0, 6, "admin/") != 0 && path.compare(0, 8, "web-ssh/") != 0) {
      GD::out.printWarning("Warning: Requested URL not found: " + path);
      getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
      send(socket, content);
      socket->Shutdown();
      return;
    }

    try {
      _out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to " + _serverInfo->contentPath + path + ", method: GET)");

      if (path.compare(0, 3, "ui/") == 0 && path.size() > 3) {
        if (!BaseLib::Io::fileExists(GD::bl->settings.uiPath() + path.substr(3))) {
          if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.hgs")) {
            http.setRedirectUrl('/' + path);
            http.setRedirectQueryString(http.getHeader().args);
            http.setRedirectStatus(200);

            if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php")) path = "ui/index.php";
            else path = "ui/index.hgs";
          }
        }
      } else if (path.compare(0, 6, "admin/") == 0 && path.size() > 6) {
        if (!BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + path.substr(6))) {
          if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.hgs")) {
            http.setRedirectUrl('/' + path);
            http.setRedirectQueryString(http.getHeader().args);
            http.setRedirectStatus(200);

            if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php")) path = "admin/index.php";
            else path = "admin/index.hgs";
          }
        }
      } else if (path.compare(0, 8, "web-ssh/") == 0 && path.size() > 8) {
        if (!BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + path.substr(8))) {
          if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.hgs")) {
            http.setRedirectUrl('/' + path);
            http.setRedirectQueryString(http.getHeader().args);
            http.setRedirectStatus(200);

            if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php")) path = "web-ssh/index.php";
            else path = "web-ssh/index.hgs";
          }
        }
      }

      std::string ending;
      int32_t pos = path.find_last_of('.');
      if (pos != (signed)std::string::npos && (unsigned)pos < path.size() - 1) ending = path.substr(pos + 1);
      GD::bl->hf.toLower(ending);
      std::string contentPath = _serverInfo->contentPath;
      std::string fullPath;
      std::string relativePath = '/' + path;

      if (path == "node-blue/index.php") {
        fullPath = GD::bl->settings.nodeBluePath() + "www/index.php";
        contentPath = GD::bl->settings.nodeBluePath();
        if (!BaseLib::Io::fileExists(fullPath)) {
          getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
          send(socket, content);
          socket->Shutdown();
          return;
        }
      } else if (path == "node-blue/signin.php") {
        fullPath = GD::bl->settings.nodeBluePath() + "www/signin.php";
        contentPath = GD::bl->settings.nodeBluePath();
        if (!BaseLib::Io::fileExists(fullPath)) {
          getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
          send(socket, content);
          socket->Shutdown();
          return;
        }
      } else if (path.compare(0, 6, "admin/") == 0) {
        if (path == "admin/") {
          if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php")) {
            fullPath = GD::bl->settings.adminUiPath() + "index.php";
            relativePath += "index.php";
            ending = "php";
          } else if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.hgs")) {
            fullPath = GD::bl->settings.adminUiPath() + "index.hgs";
            relativePath += "index.hgs";
            ending = "hgs";
          } else {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            socket->Shutdown();
            return;
          }
        } else fullPath = GD::bl->settings.adminUiPath() + path.substr(6);
        contentPath = GD::bl->settings.adminUiPath();
      } else if (path.compare(0, 3, "ui/") == 0) {
        if (path == "ui/") {
          if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php")) {
            fullPath = GD::bl->settings.uiPath() + "index.php";
            relativePath += "index.php";
            ending = "php";
          } else if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.hgs")) {
            fullPath = GD::bl->settings.uiPath() + "index.hgs";
            relativePath += "index.hgs";
            ending = "hgs";
          } else {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            socket->Shutdown();
            return;
          }
        } else fullPath = GD::bl->settings.uiPath() + path.substr(3);
        contentPath = GD::bl->settings.uiPath();
      } else if (path.compare(0, 8, "web-ssh/") == 0) {
        if (path == "web-ssh/") {
          if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php")) {
            fullPath = GD::bl->settings.webSshPath() + "index.php";
            relativePath += "index.php";
            ending = "php";
          } else if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.hgs")) {
            fullPath = GD::bl->settings.webSshPath() + "index.hgs";
            relativePath += "index.hgs";
            ending = "hgs";
          } else {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            socket->Shutdown();
            return;
          }
        } else fullPath = GD::bl->settings.webSshPath() + path.substr(8);
        contentPath = GD::bl->settings.webSshPath();
      } else fullPath = _serverInfo->contentPath + path;

#ifndef NO_SCRIPTENGINE
      if (ending == "php" || ending == "php5" || ending == "php7" || ending == "hgs") {
        BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::web, contentPath, fullPath, relativePath, http, _serverInfo, clientInfo));
        scriptInfo->socket = socket;
        scriptInfo->scriptHeadersCallback = std::bind(&WebServer::sendHeaders, this, std::placeholders::_1, std::placeholders::_2);
        scriptInfo->scriptOutputCallback = std::bind(&WebServer::sendOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        GD::scriptEngineServer->executeScript(scriptInfo, true);
        if (scriptInfo->http.getHeader().acceptEncoding & BaseLib::Http::AcceptEncoding::gzip) {
          std::vector<char> newContent = BaseLib::GZip::compress<std::vector<char>, std::string>(scriptInfo->output, 5);
          try {
            scriptInfo->socket->Send((uint8_t *)newContent.data(), newContent.size());
          }
          catch (const C1Net::Exception &ex) {
            GD::out.printError("Error: " + std::string(ex.what()));
          }
          catch (const std::exception &ex) {
            GD::out.printError(std::string("Error: ") + ex.what());
          }
        }
        socket->Shutdown();
        return;
      }
#endif
      std::string contentType = _http.getMimeType(ending);
      std::vector<char> contentString;
      std::vector<std::string> headers;
      headers.reserve(2);
      if (contentType.empty()) contentType = "application/octet-stream";
      else {
        if (cacheTime > 0) headers.push_back("Cache-Control: max-age=" + std::to_string(cacheTime) + ", private"); //Cache known content type
        else headers.push_back("Cache-Control: no-cache");
      }
      //Don't return content when method is "HEAD"
      if (http.getHeader().method == "GET") {
        contentString = GD::bl->io.getBinaryFileContent(fullPath);

        if (http.getHeader().acceptEncoding & BaseLib::Http::AcceptEncoding::gzip) {
          auto newContent = BaseLib::GZip::compress<std::vector<char>, std::vector<char>>(contentString, 5);
          contentString.swap(newContent);
          headers.push_back("Content-Encoding: gzip");
        }
      }
      std::string header;
      _http.constructHeader(contentString.size(), contentType, 200, "OK", headers, header);
      content.insert(content.end(), header.begin(), header.end());
      if (!contentString.empty()) content.insert(content.end(), contentString.begin(), contentString.end());
      send(socket, content);
      socket->Shutdown();
    }
    catch (const std::exception &ex) {
      getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
      send(socket, content);
      return;
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::post(BaseLib::PRpcClientInfo clientInfo, BaseLib::Http &http, std::shared_ptr<C1Net::TcpSocket> socket) {
  try {
    std::vector<char> content;
    if (!socket) {
      _out.printError("Error: Socket is nullptr.");
      return;
    }
    std::string path = http.getHeader().path;

    BaseLib::EventHandlers eventHandlers = getEventHandlers();
    for (const auto &eventHandler: eventHandlers) {
      eventHandler.second->lock();
      try {
        if (eventHandler.second->handler() && ((BaseLib::Rpc::IWebserverEventSink *)eventHandler.second->handler())->onPost(_serverInfo, http, socket, path)) {
          eventHandler.second->unlock();
          return;
        }
      }
      catch (const std::exception &ex) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
      catch (...) {
        _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
      }
      eventHandler.second->unlock();
    }

    if (!path.empty() && path.front() == '/') path = path.substr(1);

    bool isDirectory = false;
    BaseLib::Io::isDirectory(_serverInfo->contentPath + path, isDirectory);
    if (isDirectory || path == "node-blue" || path == "node-blue/" || path == "ui" || path == "ui/" || path == "admin" || path == "admin/" || path == "web-ssh" || path == "web-ssh/") {
      if (!path.empty() && path.back() != '/') {
        path = '/' + path + '/';
        std::vector<std::string> additionalHeaders({std::string("Location: ") + path});
        getError(301, "Moved Permanently", "The document has moved <a href=\"" + path + "\">here</a>.", content, additionalHeaders);
        send(socket, content);
        socket->Shutdown();
        return;
      }
      if (path == "node-blue/") path = "node-blue/index.php";
      else if (path.compare(0, 3, "ui/") == 0 && (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.hgs"))) {}
      else if (path.compare(0, 6, "admin/") == 0 && (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.hgs"))) {}
      else if (path.compare(0, 8, "web-ssh/") == 0 && (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.hgs"))) {}
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.php")) path += "index.php";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.php5")) path += "index.php5";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.php7")) path += "index.php7";
      else if (BaseLib::Io::fileExists(_serverInfo->contentPath + path + "index.hgs")) path += "index.hgs";
      else {
        getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
        send(socket, content);
        socket->Shutdown();
        return;
      }
    }

    if (GD::bl->settings.enableNodeBlue() && path.compare(0, 10, "node-blue/") == 0) {
      _out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to " + _serverInfo->contentPath + path + ", method: POST)");
      std::string responseEncoding;
      std::string responseHeader;
      std::string contentString = GD::nodeBlueServer->handlePost(path, http, responseEncoding, responseHeader);
      if (contentString == "unauthorized") {
        getError(401, _http.getStatusText(401), "You are not logged in.", content);
        send(socket, content);
        socket->Shutdown();
        return;
      } else if (!responseHeader.empty() || !contentString.empty()) {
        if (responseHeader.empty()) {
          std::vector<std::string> headers;
          BaseLib::Http::constructHeader(contentString.size(), responseEncoding, 200, "OK", headers, responseHeader);
        }
        content.insert(content.end(), responseHeader.begin(), responseHeader.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
        send(socket, content);
        socket->Shutdown();
        return;
      }
    }

    if (!BaseLib::Io::fileExists(_serverInfo->contentPath + path) && path != "node-blue/index.php" && path != "node-blue/signin.php" && path.compare(0, 3, "ui/") != 0 && path.compare(0, 6, "admin/") != 0 && path.compare(0, 8, "web-ssh/") != 0) {
      getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
      send(socket, content);
      socket->Shutdown();
      return;
    }

#ifndef NO_SCRIPTENGINE
    if (path.compare(0, 3, "ui/") == 0 && path.size() > 3) {
      if (!BaseLib::Io::fileExists(GD::bl->settings.uiPath() + path.substr(3))) {
        if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.hgs")) {
          http.setRedirectUrl('/' + path);
          http.setRedirectQueryString(http.getHeader().args);
          http.setRedirectStatus(200);

          if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php")) path = "ui/index.php";
          else path = "ui/index.hgs";
        }
      }
    } else if (path.compare(0, 6, "admin/") == 0 && path.size() > 6) {
      if (!BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + path.substr(6))) {
        if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.hgs")) {
          http.setRedirectUrl('/' + path);
          http.setRedirectQueryString(http.getHeader().args);
          http.setRedirectStatus(200);

          if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php")) path = "admin/index.php";
          else path = "admin/index.hgs";
        }
      }
    } else if (path.compare(0, 8, "web-ssh/") == 0 && path.size() > 6) {
      if (!BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + path.substr(8))) {
        if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php") || BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.hgs")) {
          http.setRedirectUrl('/' + path);
          http.setRedirectQueryString(http.getHeader().args);
          http.setRedirectStatus(200);

          if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php")) path = "web-ssh/index.php";
          else path = "web-ssh/index.hgs";
        }
      }
    }

    try {
      _out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to: \"" + _serverInfo->contentPath + path + "\", method: POST)");
      std::string fullPath;
      std::string contentPath = _serverInfo->contentPath;
      std::string relativePath = '/' + path;
      if (path == "node-blue/index.php") {
        fullPath = GD::bl->settings.nodeBluePath() + "www/index.php";
        contentPath = GD::bl->settings.nodeBluePath();
        if (!BaseLib::Io::fileExists(fullPath)) {
          getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
          send(socket, content);
          socket->Shutdown();
          return;
        }
      } else if (path == "node-blue/signin.php") {
        fullPath = GD::bl->settings.nodeBluePath() + "www/signin.php";
        contentPath = GD::bl->settings.nodeBluePath();
        if (!BaseLib::Io::fileExists(fullPath)) {
          getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
          send(socket, content);
          socket->Shutdown();
          return;
        }
      } else if (path.compare(0, 6, "admin/") == 0) {
        if (path == "admin/") {
          if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.php")) fullPath = GD::bl->settings.adminUiPath() + "index.php";
          else if (BaseLib::Io::fileExists(GD::bl->settings.adminUiPath() + "index.hgs")) fullPath = GD::bl->settings.adminUiPath() + "index.hgs";
          else {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            socket->Shutdown();
            return;
          }
        } else fullPath = GD::bl->settings.adminUiPath() + path.substr(6);
        contentPath = GD::bl->settings.adminUiPath();
      } else if (path.compare(0, 3, "ui/") == 0) {
        if (path == "ui/") {
          if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.php")) {
            fullPath = GD::bl->settings.uiPath() + "index.php";
            relativePath += "index.php";
          } else if (BaseLib::Io::fileExists(GD::bl->settings.uiPath() + "index.hgs")) {
            fullPath = GD::bl->settings.uiPath() + "index.hgs";
            relativePath += "index.hgs";
          } else {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            socket->Shutdown();
            return;
          }
        } else fullPath = GD::bl->settings.uiPath() + path.substr(3);
        contentPath = GD::bl->settings.uiPath();
      } else if (path.compare(0, 8, "web-ssh/") == 0) {
        if (path == "web-ssh/") {
          if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.php")) fullPath = GD::bl->settings.webSshPath() + "index.php";
          else if (BaseLib::Io::fileExists(GD::bl->settings.webSshPath() + "index.hgs")) fullPath = GD::bl->settings.webSshPath() + "index.hgs";
          else {
            getError(404, "Not Found", "The requested URL " + path + " was not found on this server.", content);
            send(socket, content);
            socket->Shutdown();
            return;
          }
        } else fullPath = GD::bl->settings.webSshPath() + path.substr(8);
        contentPath = GD::bl->settings.webSshPath();
      } else fullPath = _serverInfo->contentPath + path;
      BaseLib::ScriptEngine::PScriptInfo scriptInfo(new BaseLib::ScriptEngine::ScriptInfo(BaseLib::ScriptEngine::ScriptInfo::ScriptType::web, contentPath, fullPath, relativePath, http, _serverInfo, clientInfo));
      scriptInfo->socket = socket;
      scriptInfo->scriptHeadersCallback = std::bind(&WebServer::sendHeaders, this, std::placeholders::_1, std::placeholders::_2);
      scriptInfo->scriptOutputCallback = std::bind(&WebServer::sendOutput, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
      GD::scriptEngineServer->executeScript(scriptInfo, true);
      if (scriptInfo->http.getHeader().acceptEncoding & BaseLib::Http::AcceptEncoding::gzip) {
        std::vector<char> newContent = BaseLib::GZip::compress<std::vector<char>, std::string>(scriptInfo->output, 5);
        try {
          scriptInfo->socket->Send((uint8_t *)newContent.data(), newContent.size());
        }
        catch (const C1Net::Exception &ex) {
          GD::out.printError("Error: " + std::string(ex.what()));
        }
        catch (const std::exception &ex) {
          GD::out.printError(std::string("Error: ") + ex.what());
        }
      }
      socket->Shutdown();
    }
    catch (const std::exception &ex) {
      getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
      send(socket, content);
      socket->Shutdown();
      return;
    }
#else
    getError(500, _http.getStatusText(500), "Homegear is compiled without script engine.", content);
    send(socket, content);
    socket->close();
#endif
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::put(BaseLib::PRpcClientInfo clientInfo, BaseLib::Http &http, std::shared_ptr<C1Net::TcpSocket> socket) {
  try {
    if (!socket) {
      _out.printError("Error: Socket is nullptr.");
      return;
    }
    std::string path = http.getHeader().path;

    std::vector<char> content;
    if (!path.empty() && path.front() == '/') path = path.substr(1);

    if (GD::bl->settings.enableNodeBlue() && path.compare(0, 10, "node-blue/") == 0) {
      _out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to " + _serverInfo->contentPath + path + ", method: DELETE)");
      std::string responseEncoding;
      std::string responseHeader;
      std::string contentString = GD::nodeBlueServer->handlePut(path, http, responseEncoding, responseHeader);
      if (contentString == "unauthorized") {
        getError(401, _http.getStatusText(401), "You are not logged in.", content);
        send(socket, content);
        return;
      } else if (!responseHeader.empty() || !contentString.empty()) {
        if (responseHeader.empty()) {
          std::vector<std::string> headers;
          BaseLib::Http::constructHeader(contentString.size(), responseEncoding, 200, "OK", headers, responseHeader);
        }
        content.insert(content.end(), responseHeader.begin(), responseHeader.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
        send(socket, content);
        return;
      } else {
        if (responseHeader.empty()) {
          std::vector<std::string> headers;
          BaseLib::Http::constructHeader(contentString.size(), responseEncoding, 204, "No Content", headers, responseHeader);
        }
        content.insert(content.end(), responseHeader.begin(), responseHeader.end());
        send(socket, content);
        return;
      }
    }

    GD::out.printWarning("Warning: Requested URL not found: " + path);
    getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
    send(socket, content);
    return;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::delete_(BaseLib::PRpcClientInfo clientInfo, BaseLib::Http &http, std::shared_ptr<C1Net::TcpSocket> socket) {
  try {
    if (!socket) {
      _out.printError("Error: Socket is nullptr.");
      return;
    }
    std::string path = http.getHeader().path;

    std::vector<char> content;
    if (!path.empty() && path.front() == '/') path = path.substr(1);

    if (GD::bl->settings.enableNodeBlue() && path.compare(0, 10, "node-blue/") == 0) {
      _out.printInfo("Client is requesting: " + http.getHeader().path + " (translated to " + _serverInfo->contentPath + path + ", method: DELETE)");
      std::string responseEncoding;
      std::string responseHeader;
      std::string contentString = GD::nodeBlueServer->handleDelete(path, http, responseEncoding, responseHeader);
      if (contentString == "unauthorized") {
        getError(401, _http.getStatusText(401), "You are not logged in.", content);
        send(socket, content);
        return;
      } else if (!responseHeader.empty() || !contentString.empty()) {
        if (responseHeader.empty()) {
          std::vector<std::string> headers;
          BaseLib::Http::constructHeader(contentString.size(), responseEncoding, 200, "OK", headers, responseHeader);
        }
        content.insert(content.end(), responseHeader.begin(), responseHeader.end());
        content.insert(content.end(), contentString.begin(), contentString.end());
        send(socket, content);
        return;
      } else {
        if (responseHeader.empty()) {
          std::vector<std::string> headers;
          BaseLib::Http::constructHeader(contentString.size(), responseEncoding, 204, "No Content", headers, responseHeader);
        }
        content.insert(content.end(), responseHeader.begin(), responseHeader.end());
        send(socket, content);
        return;
      }
    }

    GD::out.printWarning("Warning: Requested URL not found: " + path);
    getError(404, _http.getStatusText(404), "The requested URL " + path + " was not found on this server.", content);
    send(socket, content);
    return;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char> &content) {
  try {
    std::vector<std::string> additionalHeaders;
    std::string contentString =
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear "
            + GD::baseLibVersion + " at Port " + std::to_string(_serverInfo->port) + "</address></body></html>";
    std::string header;
    BaseLib::Http::constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
    content.insert(content.end(), header.begin(), header.end());
    content.insert(content.end(), contentString.begin(), contentString.end());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::getError(int32_t code, const std::string &codeDescription, const std::string &longDescription, std::vector<char> &content, std::vector<std::string> &additionalHeaders) {
  try {
    std::string contentString =
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\"><html><head><title>" + std::to_string(code) + " " + codeDescription + "</title></head><body><h1>" + codeDescription + "</h1><p>" + longDescription + "<br/></p><hr><address>Homegear "
            + GD::baseLibVersion + " at Port " + std::to_string(_serverInfo->port) + "</address></body></html>";
    std::string header;
    BaseLib::Http::constructHeader(contentString.size(), "text/html", code, codeDescription, additionalHeaders, header);
    content.insert(content.end(), header.begin(), header.end());
    content.insert(content.end(), contentString.begin(), contentString.end());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::send(std::shared_ptr<C1Net::TcpSocket> &socket, std::vector<char> &data) {
  try {
    if (data.empty()) return;
    try {
      socket->Send((uint8_t *)data.data(), data.size());
    }
    catch (const C1Net::Exception &ex) {
      _out.printInfo("Info: " + std::string(ex.what()));
    }
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::sendHeaders(BaseLib::ScriptEngine::PScriptInfo &scriptInfo, BaseLib::PVariable &headers) {
  try {
    if (!scriptInfo || !scriptInfo->socket || !headers) return;
    BaseLib::Struct::iterator headerIterator = headers->structValue->find("RESPONSE_CODE");
    int32_t responseCode = 500;
    if (headerIterator != headers->structValue->end()) responseCode = headerIterator->second->integerValue;
    if (responseCode == 0) responseCode = 500;
    scriptInfo->http.getHeader().responseCode = responseCode;
    headers->structValue->erase("RESPONSE_CODE");

    {
      std::lock_guard<std::mutex> sendHeaderGuard(_sendHeaderHookMutex);
      for (auto &hook: _sendHeaderHooks) {
        hook.second(scriptInfo->http, headers);
      }
    }

    std::string output;
    output.reserve(1024);
    output.append("HTTP/1.1 " + std::to_string(responseCode) + ' ' + scriptInfo->http.getStatusText(responseCode) + "\r\n");

    if (scriptInfo->http.getHeader().acceptEncoding & BaseLib::Http::AcceptEncoding::Enum::gzip) {
      output.append("Content-Encoding: gzip\r\n");
    }

    for (auto &headerArray: *headers->structValue) {
      for (auto &header: *headerArray.second->arrayValue) {
        if (output.size() + headerArray.first.size() + header->stringValue.size() + 6 > output.capacity()) output.reserve(output.capacity() + 1024 + headerArray.first.size() + header->stringValue.size() + 6);
        output.append(headerArray.first + ": " + header->stringValue + "\r\n");
      }
    }

    output.append("\r\n");
    scriptInfo->socket->Send((uint8_t *)output.c_str(), output.size());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void WebServer::sendOutput(BaseLib::ScriptEngine::PScriptInfo &scriptInfo, std::string &output, bool error) {
  try {
    if (!scriptInfo) return;

    if (scriptInfo->http.getHeader().acceptEncoding & BaseLib::Http::AcceptEncoding::gzip) {
      if (scriptInfo->output.size() + output.size() > scriptInfo->output.capacity()) scriptInfo->output.reserve(scriptInfo->output.capacity() + (((output.size() / 1024) + 1) * 1024));
      scriptInfo->output.append(output);
    } else scriptInfo->socket->Send((uint8_t *)output.c_str(), output.size());
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

// {{{ Hooks
void WebServer::registerSendHeadersHook(std::string &moduleName, std::function<void(BaseLib::Http &http, BaseLib::PVariable &headers)> &callback) {
  try {
    if (!callback) return;
    std::lock_guard<std::mutex> sendHeaderGuard(_sendHeaderHookMutex);
    _sendHeaderHooks[moduleName] = callback;
  }
  catch (const std::exception &ex) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    _out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}
// }}}

}

}