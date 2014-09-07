/* Copyright 2013-2014 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Homegear.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

#include "HueBridge.h"
#include "../GD.h"

namespace PhilipsHue
{
HueBridge::HueBridge(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : IPhilipsHueInterface(settings)
{
	_out.init(GD::bl);
	_out.setPrefix(GD::out.getPrefix() + "Philips hue bridge \"" + settings->id + "\": ");

	signal(SIGPIPE, SIG_IGN);

	if(!settings)
	{
		_out.printCritical("Critical: Error initializing. Settings pointer is empty.");
		return;
	}
	if(settings->host.empty()) _out.printCritical("Critical: Error initializing. Hostname is not defined in settings file.");
	_hostname = settings->host;
	_port = BaseLib::Math::getNumber(settings->port);
	if(_port < 1 || _port > 65535) _port = 80;
}

HueBridge::~HueBridge()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
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

void HueBridge::sendPacket(std::shared_ptr<BaseLib::Systems::Packet> packet)
{
	try
	{
		if(!packet)
		{
			_out.printWarning("Warning: Packet was nullptr.");
			return;
		}
		_lastAction = BaseLib::HelperFunctions::getTime();

		std::shared_ptr<PhilipsHuePacket> huePacket(std::dynamic_pointer_cast<PhilipsHuePacket>(packet));
		if(!huePacket) return;

		std::shared_ptr<Json::Value> jsonRequest = huePacket->getJson();
		if(!jsonRequest) return;

		Json::FastWriter writer;
		std::string data = writer.write(*jsonRequest);
    	std::string header = "PUT /api/homegear" + _settings->id + "/lights/" + std::to_string(packet->destinationAddress()) + "/state HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(data.size()) + "\r\nConnection: Keep-Alive\r\n\r\n";
    	data.insert(data.begin(), header.begin(), header.end());
		data.push_back('\r');
		data.push_back('\n');
    	std::string response;

    	_client->sendRequest(data, response);

    	Json::Value json;
		if(!getJson(response, json)) return;

		if(json.isMember("error"))
		{
			json = json["error"];
			if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
			else _out.printError("Unknown error sending packet. Response was: " + response);
		}

		std::string getData = "GET /api/homegear" + _settings->id + "/lights/" + std::to_string(packet->destinationAddress()) + " HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nConnection: Keep-Alive\r\n\r\n";
		_client->sendRequest(getData, response);

		if(!getJson(response, json)) return;
		if(json.isMember("error"))
		{
			json = json["error"];
			if(json.isMember("type") && json["type"].asInt() == 1)
			{
				createUser();
			}
			else
			{
				if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
				else _out.printError("Unknown error during polling. Response was: " + response);
			}
			return;
		}

		if(json.isMember("state"))
		{
			std::shared_ptr<Json::Value> packetJson(new Json::Value());
			*packetJson = json;
			std::shared_ptr<PhilipsHuePacket> packet(new PhilipsHuePacket(huePacket->destinationAddress(), 0, 1, packetJson, BaseLib::HelperFunctions::getTime()));
			raisePacketReceived(packet);
		}

		_lastPacketSent = BaseLib::HelperFunctions::getTime();
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

void HueBridge::startListening()
{
	try
	{
		stopListening();
		_client = std::unique_ptr<BaseLib::HTTPClient>(new BaseLib::HTTPClient(_bl, _hostname, _port, false, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_listenThread = std::thread(&HueBridge::listen, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
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

void HueBridge::stopListening()
{
	try
	{
		_stopCallbackThread = true;
		if(_listenThread.joinable()) _listenThread.join();
		_stopCallbackThread = false;
		if(_client)
		{
			_client->disconnect();
			_client.reset();
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

void HueBridge::createUser()
{
	try
    {
		std::string data = "{\"devicetype\":\"Homegear\",\"username\":\"homegear" + _settings->id + "\"}";
    	std::string header = "POST /api HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(data.size()) + "\r\nConnection: Keep-Alive\r\n\r\n";
    	data.insert(data.begin(), header.begin(), header.end());
		data.push_back('\r');
		data.push_back('\n');
    	std::string response;

    	_client->sendRequest(data, response);

    	Json::Value json;
		if(!getJson(response, json)) return;

		if(json.isMember("error"))
		{
			json = json["error"];
			if(json.isMember("type") && json["type"].asInt() == 101) _out.printError("Please press the link button on your hue bridge to initialize the connection to Homegear.");
			else if(json.isMember("type") && json["type"].asInt() == 7) _out.printError("Error: Please change the bridge's id in physicalinterfaces.conf to only contain alphanumerical characters and \"-\".");
			else
			{
				if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
				else _out.printError("Unknown error during user creation. Response was: " + response);
			}
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

void HueBridge::searchLights()
{
	try
    {
    	std::string header = "POST /api/homegear" + _settings->id + "/lights HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nContent-Type: application/json\r\nContent-Length: 0\r\nConnection: Keep-Alive\r\n\r\n";
    	std::string response;

    	_client->sendRequest(header, response);

    	Json::Value json;
		if(!getJson(response, json)) return;

		if(json.isMember("error"))
		{
			json = json["error"];
			if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
			else _out.printError("Unknown error during user creation. Response was: " + response);
		}

		header = "GET /api/homegear" + _settings->id + "/lights/new HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nConnection: Keep-Alive\r\n\r\n";

    	_client->sendRequest(header, response);

		if(!getJson(response, json)) return;

		if(json.isMember("error"))
		{
			json = json["error"];
			if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
			else _out.printError("Unknown error during user creation. Response was: " + response);
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

std::vector<std::shared_ptr<PhilipsHuePacket>> HueBridge::getPeerInfo()
{
	try
	{
		std::string getAllData = "GET /api/homegear" + _settings->id + " HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nConnection: Keep-Alive\r\n\r\n";
		std::string response;
		_client->sendRequest(getAllData, response);

		Json::Value json;
		if(!getJson(response, json)) return std::vector<std::shared_ptr<PhilipsHuePacket>>();

		if(json.isMember("error"))
		{
			if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
			else _out.printError("Unknown error during polling. Response was: " + response);
			return std::vector<std::shared_ptr<PhilipsHuePacket>>();
		}

		std::vector<std::shared_ptr<PhilipsHuePacket>> peers;
		if(json.isMember("lights"))
		{
			json = json["lights"];
			std::vector<std::string> memberNames = json.getMemberNames();
			for(std::vector<std::string>::iterator i = memberNames.begin(); i != memberNames.end(); ++i)
			{
				std::shared_ptr<Json::Value> packetJson(new Json::Value());
				*packetJson = json[*i];
				std::shared_ptr<PhilipsHuePacket> packet(new PhilipsHuePacket(BaseLib::Math::getNumber(*i), 0, 1, packetJson, BaseLib::HelperFunctions::getTime()));
				peers.push_back(packet);
			}
		}
		return peers;
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
    return std::vector<std::shared_ptr<PhilipsHuePacket>>();
}

bool HueBridge::getJson(std::string& jsonString, Json::Value& json)
{
	try
	{
		if(jsonString.size() < 4) return false;;
		if(jsonString.front() == '[')
		{
			if(jsonString.at(jsonString.size() - 2) == ']') jsonString = jsonString.substr(1, jsonString.size() - 3);
			else
			{
				int32_t endPos = jsonString.find_last_of(']');
				if(endPos != (signed)std::string::npos) jsonString = jsonString.substr(1, endPos - 1);
			}
		}

		std::istringstream inputStream(jsonString);
		if(!_jsonReader.parse(inputStream, json, false))
		{
			_out.printError("Error parsing json. Data was: " + jsonString);
			return false;
		}
		return true;
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
    return false;
}

void HueBridge::listen()
{
    try
    {
    	std::string getAllData = "GET /api/homegear" + _settings->id + " HTTP/1.1\r\nUser-Agent: Homegear\r\nHost: " + _hostname + ":" + std::to_string(_port) + "\r\nConnection: Keep-Alive\r\n\r\n";
    	std::string response;

        while(!_stopCallbackThread)
        {
			try
			{
				for(int32_t i = 0; i < 30; i++)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					if(_stopCallbackThread) return;
				}
				_client->sendRequest(getAllData, response);
			}
			catch(const BaseLib::HTTPClientException& ex)
			{
				_out.printError("Error: " + ex.what());
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				continue;
			}

			Json::Value json;
			if(!getJson(response, json)) continue;

			if(json.isMember("error"))
			{
				json = json["error"];
				if(json.isMember("type") && json["type"].asInt() == 1)
				{
					createUser();
				}
				else
				{
					if(json.isMember("description")) _out.printError("Error: " + json["description"].asString());
					else _out.printError("Unknown error during polling. Response was: " + response);
				}
				continue;
			}

			if(json.isMember("lights"))
			{
				json = json["lights"];
				std::vector<std::string> memberNames = json.getMemberNames();
				for(std::vector<std::string>::iterator i = memberNames.begin(); i != memberNames.end(); ++i)
				{
					std::shared_ptr<Json::Value> packetJson(new Json::Value());
					*packetJson = json[*i];
					std::shared_ptr<PhilipsHuePacket> packet(new PhilipsHuePacket(BaseLib::Math::getNumber(*i), 0, 1, packetJson, BaseLib::HelperFunctions::getTime()));
					raisePacketReceived(packet);
				}
			}

        	_lastPacketReceived = BaseLib::HelperFunctions::getTime();
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
}
