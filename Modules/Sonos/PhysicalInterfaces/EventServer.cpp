/* Copyright 2013-2015 Sathya Laufer
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

#include "EventServer.h"
#include "../GD.h"

namespace Sonos
{
EventServer::EventServer(std::shared_ptr<BaseLib::Systems::PhysicalInterfaceSettings> settings) : ISonosInterface(settings)
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
	_listenAddress = settings->host;
	_listenPort = BaseLib::Math::getNumber(settings->port);
}

EventServer::~EventServer()
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

void EventServer::startListening()
{
	try
	{
		stopListening();
		_client = std::unique_ptr<BaseLib::HTTPClient>(new BaseLib::HTTPClient(_bl, _hostname, _port, false, _settings->ssl, _settings->caFile, _settings->verifyCertificate));
		_listenThread = std::thread(&HueBridge::listen, this);
		if(_settings->listenThreadPriority > -1) BaseLib::Threads::setThreadPriority(_bl, _listenThread.native_handle(), _settings->listenThreadPriority, _settings->listenThreadPolicy);
		IPhysicalInterface::startListening();
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

void EventServer::stopListening()
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
		IPhysicalInterface::stopListening();
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

void EventServer::listen()
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

			std::shared_ptr<BaseLib::RPC::Variable> json = getJson(response);
			if(!json) return;

			if(!json->arrayValue->empty() && json->arrayValue->at(0)->structValue->find("error") != json->arrayValue->at(0)->structValue->end())
			{
				json = json->arrayValue->at(0)->structValue->at("error");
				if(json->structValue->find("type") != json->structValue->end() && json->structValue->at("type")->integerValue == 1)
				{
					createUser();
				}
				else
				{
					if(json->structValue->find("description") != json->structValue->end()) _out.printError("Error: " + json->structValue->at("description")->stringValue);
					else _out.printError("Unknown error during polling. Response was: " + response);
				}
				continue;
			}

			if(json->structValue->find("lights") != json->structValue->end())
			{
				json = json->structValue->at("lights");
				for(std::map<std::string, std::shared_ptr<BaseLib::RPC::Variable>>::iterator i = json->structValue->begin(); i != json->structValue->end(); ++i)
				{
					std::string address = i->first;
					std::shared_ptr<PhilipsHuePacket> packet(new PhilipsHuePacket(BaseLib::Math::getNumber(address), 0, 1, i->second, BaseLib::HelperFunctions::getTime()));
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
