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

#include "SonosPeer.h"
#include "LogicalDevices/SonosCentral.h"
#include "SonosPacket.h"
#include "GD.h"
#include "sys/wait.h"

namespace Sonos
{
std::shared_ptr<BaseLib::Systems::Central> SonosPeer::getCentral()
{
	try
	{
		if(_central) return _central;
		_central = GD::family->getCentral();
		return _central;
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
	return std::shared_ptr<BaseLib::Systems::Central>();
}

std::shared_ptr<BaseLib::Systems::LogicalDevice> SonosPeer::getDevice(int32_t address)
{
	try
	{
		return GD::family->get(address);
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
	return std::shared_ptr<BaseLib::Systems::LogicalDevice>();
}

SonosPeer::SonosPeer(uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, parentID, centralFeatures, eventHandler)
{
	_binaryEncoder.reset(new BaseLib::RPC::RPCEncoder(GD::bl));
	_binaryDecoder.reset(new BaseLib::RPC::RPCDecoder(GD::bl));
}

SonosPeer::SonosPeer(int32_t id, std::string serialNumber, uint32_t parentID, bool centralFeatures, IPeerEventSink* eventHandler) : BaseLib::Systems::Peer(GD::bl, id, -1, serialNumber, parentID, centralFeatures, eventHandler)
{
	_binaryEncoder.reset(new BaseLib::RPC::RPCEncoder(GD::bl));
	_binaryDecoder.reset(new BaseLib::RPC::RPCDecoder(GD::bl));
}

SonosPeer::~SonosPeer()
{
	try
	{

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
}

void SonosPeer::setIp(std::string value)
{
	try
	{
		Peer::setIp(value);
		_httpClient.reset(new BaseLib::HTTPClient(GD::bl, _ip, 1400, false));
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
}

void SonosPeer::worker()
{
	try
	{
		if(_shuttingDown) return;
		if( BaseLib::HelperFunctions::getTimeSeconds() - _lastPositionInfo > 5)
		{
			_lastPositionInfo = BaseLib::HelperFunctions::getTimeSeconds();
			//Get position info if TRANSPORT_STATE is PLAYING
			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelIterator = valuesCentral.find(2);
			if(channelIterator != valuesCentral.end())
			{
				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelIterator->second.find("TRANSPORT_STATE");
				if(parameterIterator != channelIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> transportState = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(transportState)
					{
						if(transportState->stringValue == "PLAYING" || _getOneMorePositionInfo)
						{
							_getOneMorePositionInfo = (transportState->stringValue == "PLAYING");
							std::string functionName = "GetPositionInfo";
							std::string service = "urn:schemas-upnp-org:service:AVTransport:1";
							std::string path = "/MediaRenderer/AVTransport/Control";
							std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0") });

							getSoapData(functionName, service, path, soapValues);
						}
					}
				}
			}
		}
		if(BaseLib::HelperFunctions::getTimeSeconds() - _lastAvTransportSubscription > 60)
		{
			_lastAvTransportSubscription = BaseLib::HelperFunctions::getTimeSeconds();
			if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Peer " + std::to_string(_peerID) + " is calling SubscribeMRAVTransport...");
			std::string subscriptionPacket1 = "SUBSCRIBE /ZoneGroupTopology/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			std::string subscriptionPacket2 = "SUBSCRIBE /MediaRenderer/RenderingControl/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			std::string subscriptionPacket3 = "SUBSCRIBE /MediaRenderer/AVTransport/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			std::string subscriptionPacket4 = "SUBSCRIBE /MediaServer/ContentDirectory/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			std::string subscriptionPacket5 = "SUBSCRIBE /AlarmClock/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			std::string subscriptionPacket6 = "SUBSCRIBE /SystemProperties/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			std::string subscriptionPacket7 = "SUBSCRIBE /MusicServices/Event HTTP/1.1\r\nHOST: " + _ip + ":1400\r\nCALLBACK: <http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + ">\r\nNT: upnp:event\r\nTIMEOUT: Second-1800\r\nContent-Length: 0\r\n\r\n";
			if(_httpClient)
			{
				std::string response;
				try
				{
					_httpClient->sendRequest(subscriptionPacket1, response, true);
					_httpClient->sendRequest(subscriptionPacket2, response, true);
					_httpClient->sendRequest(subscriptionPacket3, response, true);
					_httpClient->sendRequest(subscriptionPacket4, response, true);
					_httpClient->sendRequest(subscriptionPacket5, response, true);
					_httpClient->sendRequest(subscriptionPacket6, response, true);
					_httpClient->sendRequest(subscriptionPacket7, response, true);
					if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
					serviceMessages->setUnreach(false, true);
				}
				catch(BaseLib::HTTPClientException& ex)
				{
					GD::out.printWarning("Warning: Error sending value to Sonos device: " + ex.what());
					serviceMessages->setUnreach(true, false);
				}
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
}

void SonosPeer::homegearShuttingDown()
{
	try
	{
		_shuttingDown = true;
		Peer::homegearShuttingDown();
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
}

std::string SonosPeer::handleCLICommand(std::string command)
{
	try
	{
		std::ostringstream stringStream;

		if(command == "help")
		{
			stringStream << "List of commands:" << std::endl << std::endl;
			stringStream << "For more information about the individual command type: COMMAND help" << std::endl << std::endl;
			stringStream << "unselect\t\tUnselect this peer" << std::endl;
			stringStream << "channel count\t\tPrint the number of channels of this peer" << std::endl;
			stringStream << "config print\t\tPrints all configuration parameters and their values" << std::endl;
			return stringStream.str();
		}
		if(command.compare(0, 13, "channel count") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints this peer's number of channels." << std::endl;
						stringStream << "Usage: channel count" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			stringStream << "Peer has " << rpcDevice->channels.size() << " channels." << std::endl;
			return stringStream.str();
		}
		else if(command.compare(0, 12, "config print") == 0)
		{
			std::stringstream stream(command);
			std::string element;
			int32_t index = 0;
			while(std::getline(stream, element, ' '))
			{
				if(index < 2)
				{
					index++;
					continue;
				}
				else if(index == 2)
				{
					if(element == "help")
					{
						stringStream << "Description: This command prints all configuration parameters of this peer. The values are in BidCoS packet format." << std::endl;
						stringStream << "Usage: config print" << std::endl << std::endl;
						stringStream << "Parameters:" << std::endl;
						stringStream << "  There are no parameters." << std::endl;
						return stringStream.str();
					}
				}
				index++;
			}

			return printConfig();
		}
		else return "Unknown command.\n";
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
    return "Error executing command. See log file for more details.\n";
}

std::string SonosPeer::printConfig()
{
	try
	{
		std::ostringstream stringStream;
		stringStream << "MASTER" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		stringStream << "VALUES" << std::endl;
		stringStream << "{" << std::endl;
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::const_iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			stringStream << "\t" << "Channel: " << std::dec << i->first << std::endl;
			stringStream << "\t{" << std::endl;
			for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				stringStream << "\t\t[" << j->first << "]: ";
				if(!j->second.rpcParameter) stringStream << "(No RPC parameter) ";
				for(std::vector<uint8_t>::const_iterator k = j->second.data.begin(); k != j->second.data.end(); ++k)
				{
					stringStream << std::hex << std::setfill('0') << std::setw(2) << (int32_t)*k << " ";
				}
				stringStream << std::endl;
			}
			stringStream << "\t}" << std::endl;
		}
		stringStream << "}" << std::endl << std::endl;

		return stringStream.str();
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
    return "";
}


void SonosPeer::loadVariables(BaseLib::Systems::LogicalDevice* device, std::shared_ptr<BaseLib::Database::DataTable> rows)
{
	try
	{
		if(!rows) rows = raiseGetPeerVariables();
		Peer::loadVariables(device, rows);
		_databaseMutex.lock();
		for(BaseLib::Database::DataTable::iterator row = rows->begin(); row != rows->end(); ++row)
		{
			_variableDatabaseIDs[row->second.at(2)->intValue] = row->second.at(0)->intValue;
			switch(row->second.at(2)->intValue)
			{
			case 1:
				_ip = row->second.at(4)->textValue;
				_httpClient.reset(new BaseLib::HTTPClient(GD::bl, _ip, 1400, false));
				break;
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
    _databaseMutex.unlock();
}

bool SonosPeer::load(BaseLib::Systems::LogicalDevice* device)
{
	try
	{
		loadVariables((SonosDevice*)device);

		rpcDevice = GD::rpcDevices.find(_deviceType, 0x10, -1);
		if(!rpcDevice)
		{
			GD::out.printError("Error loading Sonos peer " + std::to_string(_peerID) + ": Device type not found: 0x" + BaseLib::HelperFunctions::getHexString((uint32_t)_deviceType.type()) + " Firmware version: " + std::to_string(_firmwareVersion));
			return false;
		}
		initializeTypeString();
		std::string entry;
		loadConfig();
		initializeCentralConfig();

		serviceMessages.reset(new BaseLib::Systems::ServiceMessages(_bl, _peerID, _serialNumber, this));
		serviceMessages->load();

		return true;
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
    return false;
}

void SonosPeer::saveVariables()
{
	try
	{
		if(_peerID == 0) return;
		Peer::saveVariables();
		saveVariable(1, _ip);
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
}

void SonosPeer::getValuesFromPacket(std::shared_ptr<SonosPacket> packet, std::vector<FrameValues>& frameValues)
{
	try
	{
		if(!rpcDevice) return;
		//equal_range returns all elements with "0" or an unknown element as argument
		if(rpcDevice->framesByFunction2.find(packet->functionName()) == rpcDevice->framesByFunction2.end()) return;
		std::pair<std::multimap<std::string, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator,std::multimap<std::string, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator> range = rpcDevice->framesByFunction2.equal_range(packet->functionName());
		if(range.first == rpcDevice->framesByFunction2.end()) return;
		std::multimap<std::string, std::shared_ptr<BaseLib::RPC::DeviceFrame>>::iterator i = range.first;
		std::shared_ptr<std::unordered_map<std::string, std::string>> soapValues;
		std::string field;
		do
		{
			FrameValues currentFrameValues;
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame(i->second);
			if(!frame) continue;
			if(frame->direction != BaseLib::RPC::DeviceFrame::Direction::Enum::fromDevice) continue;
			int32_t channel = -1;
			if(frame->fixedChannel > -1) channel = frame->fixedChannel;
			currentFrameValues.frameID = frame->id;

			for(std::vector<BaseLib::RPC::Parameter>::iterator j = frame->parameters.begin(); j != frame->parameters.end(); ++j)
			{
				if(j->field == "CurrentTrackMetaData" || j->field == "TrackMetaData")
				{
					soapValues = packet->currentTrackMetadata();
					if(!soapValues || soapValues->find(j->subfield) == soapValues->end()) continue;
					field = j->subfield;
				}
				else if(j->field == "r:NextTrackMetaData")
				{
					soapValues = packet->nextTrackMetadata();
					if(!soapValues || soapValues->find(j->subfield) == soapValues->end()) continue;
					field = j->subfield;
				}
				else if(j->field == "r:EnqueuedTransportURIMetaData")
				{
					soapValues = packet->currentTrackMetadata();
					if(!soapValues || soapValues->find(j->subfield) == soapValues->end()) continue;
					field = j->subfield;
				}
				else
				{
					soapValues = packet->values();
					if(!soapValues || soapValues->find(j->field) == soapValues->end()) continue;
					field = j->field;
				}
				if(soapValues->find(field) == soapValues->end()) continue;

				for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator k = frame->associatedValues.begin(); k != frame->associatedValues.end(); ++k)
				{
					if((*k)->physicalParameter->valueID != j->param) continue;
					currentFrameValues.parameterSetType = (*k)->parentParameterSet->type;
					bool setValues = false;
					if(currentFrameValues.paramsetChannels.empty()) //Fill paramsetChannels
					{
						int32_t startChannel = (channel < 0) ? 0 : channel;
						int32_t endChannel;
						//When fixedChannel is -2 (means '*') cycle through all channels
						if(frame->fixedChannel == -2)
						{
							startChannel = 0;
							endChannel = (rpcDevice->channels.end()--)->first;
						}
						else endChannel = startChannel;
						for(int32_t l = startChannel; l <= endChannel; l++)
						{
							if(rpcDevice->channels.find(l) == rpcDevice->channels.end()) continue;
							if(rpcDevice->channels.at(l)->parameterSets.find(currentFrameValues.parameterSetType) == rpcDevice->channels.at(l)->parameterSets.end()) continue;
							if(!rpcDevice->channels.at(l)->parameterSets.at(currentFrameValues.parameterSetType)->getParameter((*k)->id)) continue;
							currentFrameValues.paramsetChannels.push_back(l);
							currentFrameValues.values[(*k)->id].channels.push_back(l);
							setValues = true;
						}
					}
					else //Use paramsetChannels
					{
						for(std::list<uint32_t>::const_iterator l = currentFrameValues.paramsetChannels.begin(); l != currentFrameValues.paramsetChannels.end(); ++l)
						{
							if(rpcDevice->channels.find(*l) == rpcDevice->channels.end()) continue;
							if(rpcDevice->channels.at(*l)->parameterSets.find(currentFrameValues.parameterSetType) == rpcDevice->channels.at(*l)->parameterSets.end()) continue;
							if(!rpcDevice->channels.at(*l)->parameterSets.at(currentFrameValues.parameterSetType)->getParameter((*k)->id)) continue;
							currentFrameValues.values[(*k)->id].channels.push_back(*l);
							setValues = true;
						}
					}

					if(setValues)
					{
						//This is a little nasty and costs a lot of resources, but we need to run the data through the packet converter
						std::vector<uint8_t> encodedData;
						_binaryEncoder->encodeResponse(BaseLib::RPC::Variable::fromString(soapValues->at(field), (*k)->physicalParameter->type), encodedData);
						std::shared_ptr<BaseLib::RPC::Variable> data = (*k)->convertFromPacket(encodedData, true);
						(*k)->convertToPacket(data, currentFrameValues.values[(*k)->id].value);
					}
				}
			}
			if(!currentFrameValues.values.empty()) frameValues.push_back(currentFrameValues);
		} while(++i != range.second && i != rpcDevice->framesByFunction2.end());
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
}

void SonosPeer::packetReceived(std::shared_ptr<SonosPacket> packet)
{
	try
	{
		if(!packet) return;
		if(!_centralFeatures || _disposing) return;
		if(!rpcDevice) return;
		setLastPacketReceived();
		std::vector<FrameValues> frameValues;
		getValuesFromPacket(packet, frameValues);
		std::map<uint32_t, std::shared_ptr<std::vector<std::string>>> valueKeys;
		std::map<uint32_t, std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>>> rpcValues;

		//Loop through all matching frames
		for(std::vector<FrameValues>::iterator a = frameValues.begin(); a != frameValues.end(); ++a)
		{
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame;
			if(!a->frameID.empty()) frame = rpcDevice->framesByID.at(a->frameID);

			for(std::unordered_map<std::string, FrameValue>::iterator i = a->values.begin(); i != a->values.end(); ++i)
			{
				for(std::list<uint32_t>::const_iterator j = a->paramsetChannels.begin(); j != a->paramsetChannels.end(); ++j)
				{
					if(std::find(i->second.channels.begin(), i->second.channels.end(), *j) == i->second.channels.end()) continue;

					BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[*j][i->first];
					if(parameter->data == i->second.value) continue;

					if(!valueKeys[*j] || !rpcValues[*j])
					{
						valueKeys[*j].reset(new std::vector<std::string>());
						rpcValues[*j].reset(new std::vector<std::shared_ptr<BaseLib::RPC::Variable>>());
					}

					parameter->data = i->second.value;
					if(parameter->databaseID > 0) saveParameter(parameter->databaseID, parameter->data);
					else saveParameter(0, BaseLib::RPC::ParameterSet::Type::Enum::values, *j, i->first, parameter->data);
					if(_bl->debugLevel >= 4) GD::out.printInfo("Info: " + i->first + " of peer " + std::to_string(_peerID) + " with serial number " + _serialNumber + ":" + std::to_string(*j) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(i->second.value) + ".");

					if(parameter->rpcParameter)
					{
						//Process service messages
						if((parameter->rpcParameter->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !i->second.value.empty())
						{
							if(parameter->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeEnum)
							{
								serviceMessages->set(i->first, i->second.value.at(i->second.value.size() - 1), *j);
							}
							else if(parameter->rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::Enum::typeBoolean)
							{
								serviceMessages->set(i->first, (bool)i->second.value.at(i->second.value.size() - 1));
							}
						}

						valueKeys[*j]->push_back(i->first);
						rpcValues[*j]->push_back(parameter->rpcParameter->convertFromPacket(i->second.value, true));
					}
				}
			}
		}

		if(!rpcValues.empty())
		{
			for(std::map<uint32_t, std::shared_ptr<std::vector<std::string>>>::const_iterator j = valueKeys.begin(); j != valueKeys.end(); ++j)
			{
				if(j->second->empty()) continue;

				std::string address(_serialNumber + ":" + std::to_string(j->first));
				raiseEvent(_peerID, j->first, j->second, rpcValues.at(j->first));
				raiseRPCEvent(_peerID, j->first, address, j->second, rpcValues.at(j->first));
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
}

void SonosPeer::getSoapData(std::string& functionName, std::string& service, std::string& path, std::shared_ptr<std::vector<std::pair<std::string, std::string>>>& soapValues, bool ignoreErrors)
{
	try
	{
		std::string soapRequest;
		std::string headerSoapRequest = service + '#' + functionName;
		SonosPacket packet(_ip, path, headerSoapRequest, service, functionName, soapValues);
		packet.getSoapRequest(soapRequest);
		if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + soapRequest);
		if(_httpClient)
		{
			std::string response;
			try
			{
				_httpClient->sendRequest(soapRequest, response);
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
				std::shared_ptr<SonosPacket> responsePacket(new SonosPacket(response));
				packetReceived(responsePacket);
				serviceMessages->setUnreach(false, true);
			}
			catch(BaseLib::HTTPClientException& ex)
			{
				if(ignoreErrors) return;
				GD::out.printWarning("Warning: Error in UPnP request: " + ex.what());
				GD::out.printMessage("Request was: \n" + soapRequest);
				serviceMessages->setUnreach(true, false);
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

}

std::shared_ptr<BaseLib::RPC::Variable> SonosPeer::getValueFromDevice(std::shared_ptr<BaseLib::RPC::Parameter>& parameter, int32_t channel, bool asynchronous)
{
	try
	{
		if(!parameter) return BaseLib::RPC::Variable::createError(-32500, "parameter is nullptr.");
		std::string getRequestFrame = parameter->physicalParameter->getRequest;
		std::string getResponseFrame = parameter->physicalParameter->getResponse;
		if(getRequestFrame.empty()) return BaseLib::RPC::Variable::createError(-6, "Parameter can't be requested actively.");
		if(rpcDevice->framesByID.find(getRequestFrame) == rpcDevice->framesByID.end()) return BaseLib::RPC::Variable::createError(-6, "No frame was found for parameter " + parameter->id);
		std::shared_ptr<BaseLib::RPC::DeviceFrame> frame = rpcDevice->framesByID[getRequestFrame];
		std::shared_ptr<BaseLib::RPC::DeviceFrame> responseFrame;
		if(rpcDevice->framesByID.find(getResponseFrame) != rpcDevice->framesByID.end()) responseFrame = rpcDevice->framesByID[getResponseFrame];

		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(valuesCentral[channel].find(parameter->id) == valuesCentral[channel].end()) return BaseLib::RPC::Variable::createError(-5, "Unknown parameter.");

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = getParameterSet(channel, BaseLib::RPC::ParameterSet::Type::Enum::values);
		if(!parameterSet) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");

		std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>());
		for(std::vector<BaseLib::RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
		{
			if(i->constValue > -1)
			{
				if(i->field.empty()) continue;
				soapValues->push_back(std::pair<std::string, std::string>(i->field, std::to_string(i->constValue)));
				continue;
			}
			else if(!i->constValueString.empty())
			{
				if(i->field.empty()) continue;
				soapValues->push_back(std::pair<std::string, std::string>(i->field, i->constValueString));
				continue;
			}

			bool paramFound = false;
			for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator j = parameterSet->parameters.begin(); j != parameterSet->parameters.end(); ++j)
			{
				if(i->param == (*j)->physicalParameter->id)
				{
					if(i->field.empty()) continue;
					soapValues->push_back(std::pair<std::string, std::string>(i->field, _binaryDecoder->decodeResponse((valuesCentral[channel][(*j)->id].data))->toString()));
					paramFound = true;
					break;
				}
			}
			if(!paramFound) GD::out.printError("Error constructing packet. param \"" + i->param + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
		}

		std::string soapRequest;
		SonosPacket packet(_ip, frame->metaString1, frame->function1, frame->metaString2, frame->function2, soapValues);
		packet.getSoapRequest(soapRequest);
		if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + soapRequest);
		if(_httpClient)
		{
			std::string response;
			try
			{
				_httpClient->sendRequest(soapRequest, response);
				if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
				std::shared_ptr<SonosPacket> responsePacket(new SonosPacket(response));
				packetReceived(responsePacket);
				serviceMessages->setUnreach(false, true);
			}
			catch(BaseLib::HTTPClientException& ex)
			{
				return BaseLib::RPC::Variable::createError(-100, "Error sending value to Sonos device: " + ex.what());
				serviceMessages->setUnreach(true, false);
			}
		}

		return parameter->convertFromPacket(valuesCentral[channel][parameter->id].data, true);
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
	return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::ParameterSet> SonosPeer::getParameterSet(int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels.at(channel);
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end())
		{
			GD::out.printDebug("Debug: Parameter set of type " + std::to_string(type) + " not found for channel " + std::to_string(channel));
			return std::shared_ptr<BaseLib::RPC::ParameterSet>();
		}
		return rpcChannel->parameterSets[type];
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
	return std::shared_ptr<BaseLib::RPC::ParameterSet>();
}

std::shared_ptr<BaseLib::RPC::Variable> SonosPeer::getDeviceInfo(int32_t clientID, std::map<std::string, bool> fields)
{
	try
	{
		std::shared_ptr<BaseLib::RPC::Variable> info(Peer::getDeviceInfo(clientID, fields));
		return info;
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
    return std::shared_ptr<BaseLib::RPC::Variable>();
}

std::shared_ptr<BaseLib::RPC::Variable> SonosPeer::getParamset(int32_t clientID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		std::shared_ptr<BaseLib::RPC::DeviceChannel> rpcChannel = rpcDevice->channels[channel];
		if(rpcChannel->parameterSets.find(type) == rpcChannel->parameterSets.end()) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcChannel->parameterSets[type];
		if(!parameterSet) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::Variable> variables(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcStruct));

		for(std::vector<std::shared_ptr<BaseLib::RPC::Parameter>>::iterator i = parameterSet->parameters.begin(); i != parameterSet->parameters.end(); ++i)
		{
			if((*i)->id.empty() || (*i)->hidden) continue;
			if(!((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::visible) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::service) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::internal) && !((*i)->uiFlags & BaseLib::RPC::Parameter::UIFlags::Enum::transform))
			{
				GD::out.printDebug("Debug: Omitting parameter " + (*i)->id + " because of it's ui flag.");
				continue;
			}
			std::shared_ptr<BaseLib::RPC::Variable> element;
			if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
			{
				if(!((*i)->operations & BaseLib::RPC::Parameter::Operations::read) && !((*i)->operations & BaseLib::RPC::Parameter::Operations::event)) continue;
				if(valuesCentral.find(channel) == valuesCentral.end()) continue;
				if(valuesCentral[channel].find((*i)->id) == valuesCentral[channel].end()) continue;
				element = (*i)->convertFromPacket(valuesCentral[channel][(*i)->id].data);
			}
			else if(type == BaseLib::RPC::ParameterSet::Type::Enum::master)
			{
				if(configCentral.find(channel) == configCentral.end()) continue;
				if(configCentral[channel].find((*i)->id) == configCentral[channel].end()) continue;
				element = (*i)->convertFromPacket(configCentral[channel][(*i)->id].data);
			}
			else if(type == BaseLib::RPC::ParameterSet::Type::Enum::link)
			{
				return BaseLib::RPC::Variable::createError(-3, "Parameter set type is not supported.");
			}

			if(!element) continue;
			if(element->type == BaseLib::RPC::VariableType::rpcVoid) continue;
			variables->structValue->insert(BaseLib::RPC::RPCStructElement((*i)->id, element));
		}
		return variables;
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> SonosPeer::getParamsetDescription(int32_t clientID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(channel < 0) channel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel");
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set");
		if(type == BaseLib::RPC::ParameterSet::Type::link && remoteID > 0)
		{
			std::shared_ptr<BaseLib::Systems::BasicPeer> remotePeer = getPeer(channel, remoteID, remoteChannel);
			if(!remotePeer) return BaseLib::RPC::Variable::createError(-2, "Unknown remote peer.");
		}

		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets[type];
		return Peer::getParamsetDescription(clientID, parameterSet);
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> SonosPeer::putParamset(int32_t clientID, int32_t channel, BaseLib::RPC::ParameterSet::Type::Enum type, uint64_t remoteID, int32_t remoteChannel, std::shared_ptr<BaseLib::RPC::Variable> variables, bool onlyPushing)
{
	try
	{
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::Variable::createError(-2, "Not a central peer.");
		if(channel < 0) channel = 0;
		if(remoteChannel < 0) remoteChannel = 0;
		if(rpcDevice->channels.find(channel) == rpcDevice->channels.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(type == BaseLib::RPC::ParameterSet::Type::none) type = BaseLib::RPC::ParameterSet::Type::link;
		if(rpcDevice->channels[channel]->parameterSets.find(type) == rpcDevice->channels[channel]->parameterSets.end()) return BaseLib::RPC::Variable::createError(-3, "Unknown parameter set.");
		std::shared_ptr<BaseLib::RPC::ParameterSet> parameterSet = rpcDevice->channels[channel]->parameterSets.at(type);
		if(variables->structValue->empty()) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));

		if(type == BaseLib::RPC::ParameterSet::Type::Enum::master)
		{
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> changedParameters;
			//allParameters is necessary to temporarily store all values. It is used to set changedParameters.
			//This is necessary when there are multiple variables per index and not all of them are changed.
			std::map<int32_t, std::map<int32_t, std::vector<uint8_t>>> allParameters;
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				std::vector<uint8_t> value;
				if(configCentral[channel].find(i->first) == configCentral[channel].end()) continue;
				BaseLib::Systems::RPCConfigurationParameter* parameter = &configCentral[channel][i->first];
				if(!parameter->rpcParameter) continue;
				parameter->rpcParameter->convertToPacket(i->second, value);
				std::vector<uint8_t> shiftedValue = value;
				parameter->rpcParameter->adjustBitPosition(shiftedValue);
				int32_t intIndex = (int32_t)parameter->rpcParameter->physicalParameter->index;
				int32_t list = parameter->rpcParameter->physicalParameter->list;
				if(list == 9999) list = 0;
				if(allParameters[list].find(intIndex) == allParameters[list].end()) allParameters[list][intIndex] = shiftedValue;
				else
				{
					uint32_t index = 0;
					for(std::vector<uint8_t>::iterator j = shiftedValue.begin(); j != shiftedValue.end(); ++j)
					{
						if(index >= allParameters[list][intIndex].size()) allParameters[list][intIndex].push_back(0);
						allParameters[list][intIndex].at(index) |= *j;
						index++;
					}
				}
				parameter->data = value;
				if(parameter->databaseID > 0) saveParameter(parameter->databaseID, parameter->data);
				else saveParameter(0, BaseLib::RPC::ParameterSet::Type::Enum::master, channel, i->first, parameter->data);
				GD::out.printInfo("Info: Parameter " + i->first + " of peer " + std::to_string(_peerID) + " and channel " + std::to_string(channel) + " was set to 0x" + BaseLib::HelperFunctions::getHexString(allParameters[list][intIndex]) + ".");
				//Only send to device when parameter is of type config
				if(parameter->rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::config && parameter->rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::configString) continue;
				changedParameters[list][intIndex] = allParameters[list][intIndex];
			}

			if(!changedParameters.empty() && !changedParameters.begin()->second.empty()) raiseRPCUpdateDevice(_peerID, channel, _serialNumber + ":" + std::to_string(channel), 0);
		}
		else if(type == BaseLib::RPC::ParameterSet::Type::Enum::values)
		{
			for(BaseLib::RPC::RPCStruct::iterator i = variables->structValue->begin(); i != variables->structValue->end(); ++i)
			{
				if(i->first.empty() || !i->second) continue;
				setValue(clientID, channel, i->first, i->second);
			}
		}
		else
		{
			return BaseLib::RPC::Variable::createError(-3, "Parameter set type is not supported.");
		}
		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<BaseLib::RPC::Variable> SonosPeer::setValue(int32_t clientID, uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::Variable> value)
{
	try
	{
		Peer::setValue(clientID, channel, valueKey, value); //Ignore result, otherwise setHomegerValue might not be executed
		if(_disposing) return BaseLib::RPC::Variable::createError(-32500, "Peer is disposing.");
		if(!_centralFeatures) return BaseLib::RPC::Variable::createError(-2, "Not a central peer.");
		if(valueKey.empty()) return BaseLib::RPC::Variable::createError(-5, "Value key is empty.");
		if(valuesCentral.find(channel) == valuesCentral.end()) return BaseLib::RPC::Variable::createError(-2, "Unknown channel.");
		if(setHomegearValue(channel, valueKey, value)) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
		if(valuesCentral[channel].find(valueKey) == valuesCentral[channel].end()) return BaseLib::RPC::Variable::createError(-5, "Unknown parameter.");
		std::shared_ptr<BaseLib::RPC::Parameter> rpcParameter = valuesCentral[channel][valueKey].rpcParameter;
		if(!rpcParameter) return BaseLib::RPC::Variable::createError(-5, "Unknown parameter.");
		if(rpcParameter->uiFlags & BaseLib::RPC::Parameter::UIFlags::service)
		{
			if(channel == 0 && value->type == BaseLib::RPC::VariableType::rpcBoolean)
			{
				if(serviceMessages->set(valueKey, value->booleanValue)) return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
			}
			else if(value->type == BaseLib::RPC::VariableType::rpcInteger) serviceMessages->set(valueKey, value->integerValue, channel);
		}
		if(rpcParameter->logicalParameter->type == BaseLib::RPC::LogicalParameter::Type::typeAction && !value->booleanValue) return BaseLib::RPC::Variable::createError(-5, "Parameter of type action cannot be set to \"false\".");
		if(!(rpcParameter->operations & BaseLib::RPC::Parameter::Operations::write) && clientID != -1 && !((rpcParameter->operations & BaseLib::RPC::Parameter::Operations::addonWrite) && raiseIsAddonClient(clientID) == 1)) return BaseLib::RPC::Variable::createError(-6, "parameter is read only");
		BaseLib::Systems::RPCConfigurationParameter* parameter = &valuesCentral[channel][valueKey];
		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>());
		std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> values(new std::vector<std::shared_ptr<BaseLib::RPC::Variable>>());

		rpcParameter->convertToPacket(value, parameter->data);
		value = rpcParameter->convertFromPacket(parameter->data, false);
		if(parameter->databaseID > 0) saveParameter(parameter->databaseID, parameter->data);
		else saveParameter(0, BaseLib::RPC::ParameterSet::Type::Enum::values, channel, valueKey, parameter->data);

		valueKeys->push_back(valueKey);
		values->push_back(value);

		if(rpcParameter->physicalParameter->interface == BaseLib::RPC::PhysicalParameter::Interface::Enum::command)
		{
			std::string setRequest = rpcParameter->physicalParameter->setRequest;
			if(setRequest.empty()) return BaseLib::RPC::Variable::createError(-6, "parameter is read only");
			if(rpcDevice->framesByID.find(setRequest) == rpcDevice->framesByID.end()) return BaseLib::RPC::Variable::createError(-6, "No frame was found for parameter " + valueKey);
			std::shared_ptr<BaseLib::RPC::DeviceFrame> frame = rpcDevice->framesByID[setRequest];

			std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>());
			for(std::vector<BaseLib::RPC::Parameter>::iterator i = frame->parameters.begin(); i != frame->parameters.end(); ++i)
			{
				if(i->constValue > -1)
				{
					if(i->field.empty()) continue;
					soapValues->push_back(std::pair<std::string, std::string>(i->field, std::to_string(i->constValue)));
					continue;
				}
				else if(!i->constValueString.empty())
				{
					if(i->field.empty()) continue;
					soapValues->push_back(std::pair<std::string, std::string>(i->field, i->constValueString));
					continue;
				}
				if(i->param == rpcParameter->physicalParameter->valueID || i->param == rpcParameter->physicalParameter->id)
				{
					if(i->field.empty()) continue;
					soapValues->push_back(std::pair<std::string, std::string>(i->field, _binaryDecoder->decodeResponse(parameter->data)->toString()));
				}
				//Search for all other parameters
				else
				{
					bool paramFound = false;
					for(std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator j = valuesCentral[channel].begin(); j != valuesCentral[channel].end(); ++j)
					{
						if(!j->second.rpcParameter) continue;
						if(i->param == j->second.rpcParameter->physicalParameter->id)
						{
							if(i->field.empty()) continue;
							soapValues->push_back(std::pair<std::string, std::string>(i->field, _binaryDecoder->decodeResponse(j->second.data)->toString()));
							paramFound = true;
							break;
						}
					}
					if(!paramFound) GD::out.printError("Error constructing packet. param \"" + i->param + "\" not found. Peer: " + std::to_string(_peerID) + " Serial number: " + _serialNumber + " Frame: " + frame->id);
				}
			}

			std::string soapRequest;
			SonosPacket packet(_ip, frame->metaString1, frame->function1, frame->metaString2, frame->function2, soapValues);
			packet.getSoapRequest(soapRequest);
			if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: Sending SOAP request:\n" + soapRequest);
			if(_httpClient)
			{
				std::string response;
				try
				{
					_httpClient->sendRequest(soapRequest, response);
					if(GD::bl->debugLevel >= 5) GD::out.printDebug("Debug: SOAP response:\n" + response);
				}
				catch(BaseLib::HTTPClientException& ex)
				{
					return BaseLib::RPC::Variable::createError(-100, "Error sending value to Sonos device: " + ex.what());
				}

			}
		}
		else if(rpcParameter->physicalParameter->interface != BaseLib::RPC::PhysicalParameter::Interface::Enum::store) return BaseLib::RPC::Variable::createError(-6, "Only interface types \"store\" and \"command\" are supported for this device family.");

		raiseEvent(_peerID, channel, valueKeys, values);
		raiseRPCEvent(_peerID, channel, _serialNumber + ":" + std::to_string(channel), valueKeys, values);

		return std::shared_ptr<BaseLib::RPC::Variable>(new BaseLib::RPC::Variable(BaseLib::RPC::VariableType::rpcVoid));
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
    return BaseLib::RPC::Variable::createError(-32500, "Unknown application error. See error log for more details.");
}

bool SonosPeer::setHomegearValue(uint32_t channel, std::string valueKey, std::shared_ptr<BaseLib::RPC::Variable> value)
{
	try
	{
		if(valueKey == "PLAY_TTS")
		{
			if(value->stringValue.empty()) return true;
			std::string ttsProgram = GD::physicalInterface->ttsProgram();
			if(ttsProgram.empty())
			{
				GD::out.printError("Error: No program to generate TTS audio file specified in physicalinterfaces.conf");
				return true;
			}

			std::string functionName = "GetPositionInfo";
			std::string service = "urn:schemas-upnp-org:service:AVTransport:1";
			std::string path = "/MediaRenderer/AVTransport/Control";
			std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0") });
			getSoapData(functionName, service, path, soapValues);

			functionName = "GetVolume";
			service = "urn:schemas-upnp-org:service:RenderingControl:1";
			path = "/MediaRenderer/RenderingControl/Control";
			soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Channel", "Master") });
			getSoapData(functionName, service, path, soapValues);

			functionName = "GetMute";
			service = "urn:schemas-upnp-org:service:RenderingControl:1";
			path = "/MediaRenderer/RenderingControl/Control";
			soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Channel", "Master") });
			getSoapData(functionName, service, path, soapValues);

			functionName = "GetTransportInfo";
			service = "urn:schemas-upnp-org:service:AVTransport:1";
			path = "/MediaRenderer/AVTransport/Control";
			soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0") });
			getSoapData(functionName, service, path, soapValues);

			bool unmute = true;
			int32_t volume = -1;
			std::string language;

			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelOneIterator = valuesCentral.find(1);
			if(channelOneIterator != valuesCentral.end())
			{
				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("PLAY_TTS_UNMUTE");
				if(parameterIterator != channelOneIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) unmute = variable->booleanValue;
				}

				parameterIterator = channelOneIterator->second.find("PLAY_TTS_VOLUME");
				if(parameterIterator != channelOneIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) volume = variable->integerValue;
				}

				parameterIterator = channelOneIterator->second.find("PLAY_TTS_LANGUAGE");
				if(parameterIterator != channelOneIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) language = variable->stringValue;
					if(!BaseLib::HelperFunctions::isAlphaNumeric(language))
					{
						GD::out.printError("Error: Language is not alphanumeric.");
						language = "en";
					}
				}
			}

			std::string audioPath = GD::bl->settings.tempPath() + "sonos/";
			std::string filename;
			BaseLib::HelperFunctions::stringReplace(value->stringValue, "\"", "");
			std::string execPath = ttsProgram + ' ' + language + " \"" + value->stringValue + "\"";
			if(BaseLib::HelperFunctions::exec(execPath, filename) != 0)
			{
				GD::out.printError("Error: Error executing program to generate TTS audio file: \"" + ttsProgram + ' ' + language + ' ' + value->stringValue + "\"");
				return true;
			}
			BaseLib::HelperFunctions::trim(filename);
			if(filename.size() <= audioPath.size() || filename.compare(0, audioPath.size(), audioPath) != 0 || !BaseLib::Io::fileExists(filename))
			{
				GD::out.printError("Error: Error executing program to generate TTS audio file. Output needs to be the full path to the TTS audio file, but was: \"" + filename + "\"");
				return true;
			}
			filename = filename.substr(audioPath.size());

			std::string playlistFilename = filename;
			BaseLib::HelperFunctions::stringReplace(playlistFilename, ".mp3", ".m3u");
			if(!BaseLib::Io::fileExists(playlistFilename))
			{
				std::string playlistContent = "#EXTM3U\n#EXTINF:0,<Homegear><TTS><TTS>\nhttp://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + filename + '\n';
				std::string playlistFilepath = audioPath + playlistFilename;
				BaseLib::Io::writeFile(playlistFilepath, playlistContent);
			}

			std::string currentTrackUri;
			std::string transportState;
			int32_t volumeState = -1;
			bool muteState = false;
			int32_t trackNumberState = -1;
			std::string seekTimeState;

			std::unordered_map<uint32_t, std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>>::iterator channelTwoIterator = valuesCentral.find(2);
			if(channelTwoIterator != valuesCentral.end())
			{
				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelTwoIterator->second.find("CURRENT_TRACK_URI");
				if(parameterIterator != channelTwoIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) currentTrackUri = variable->stringValue;
				}

				parameterIterator = channelTwoIterator->second.find("VOLUME");
				if(parameterIterator != channelTwoIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) volumeState = variable->integerValue;
				}

				parameterIterator = channelTwoIterator->second.find("MUTE");
				if(parameterIterator != channelTwoIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) muteState = (variable->stringValue == "1");
				}

				parameterIterator = channelTwoIterator->second.find("CURRENT_TRACK");
				if(parameterIterator != channelTwoIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) trackNumberState = variable->integerValue;
				}

				parameterIterator = channelTwoIterator->second.find("CURRENT_TRACK_RELATIVE_TIME");
				if(parameterIterator != channelTwoIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) seekTimeState = variable->stringValue;
				}

				parameterIterator = channelTwoIterator->second.find("TRANSPORT_STATE");
				if(parameterIterator != channelTwoIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) transportState = variable->stringValue;
				}
			}

			functionName = "Pause";
			service = "urn:schemas-upnp-org:service:AVTransport:1";
			path = "/MediaRenderer/AVTransport/Control";
			soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0") });
			getSoapData(functionName, service, path, soapValues, true);

			if(unmute)
			{
				functionName = "SetMute";
				service = "urn:schemas-upnp-org:service:RenderingControl:1";
				path = "/MediaRenderer/RenderingControl/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Channel", "Master"), std::pair<std::string, std::string>("DesiredMute", "0") });
				getSoapData(functionName, service, path, soapValues);
			}

			if(volume > 0)
			{
				functionName = "SetVolume";
				service = "urn:schemas-upnp-org:service:RenderingControl:1";
				path = "/MediaRenderer/RenderingControl/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Channel", "Master"), std::pair<std::string, std::string>("DesiredVolume", std::to_string(volume)) });
				getSoapData(functionName, service, path, soapValues);
			}

			if(currentTrackUri.find(".mp3") != std::string::npos || currentTrackUri.compare(0, 14, "x-file-cifs://") == 0)
			{
				functionName = "AddURIToQueue";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("EnqueuedURI", "http://" + GD::physicalInterface->listenAddress() + ':' + std::to_string(GD::physicalInterface->listenPort()) + '/' + playlistFilename), std::pair<std::string, std::string>("EnqueuedURIMetaData", ""), std::pair<std::string, std::string>("DesiredFirstTrackNumberEnqueued", "0"), std::pair<std::string, std::string>("EnqueueAsNext", "0") });
				getSoapData(functionName, service, path, soapValues);

				std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelOneIterator->second.find("FIRST_TRACK_NUMBER_ENQUEUED");
				int32_t trackNumber = 0;
				if(parameterIterator != channelOneIterator->second.end())
				{
					std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
					if(variable) trackNumber = variable->integerValue;
				}

				functionName = "Seek";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Unit", "TRACK_NR"), std::pair<std::string, std::string>("Target", std::to_string(trackNumber)) });
				getSoapData(functionName, service, path, soapValues);

				functionName = "Play";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Speed", "1") });
				getSoapData(functionName, service, path, soapValues);

				std::string functionName = "GetPositionInfo";
				std::string service = "urn:schemas-upnp-org:service:AVTransport:1";
				std::string path = "/MediaRenderer/AVTransport/Control";
				std::shared_ptr<std::vector<std::pair<std::string, std::string>>> soapValues(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0") });
				while(!serviceMessages->getUnreach())
				{
					getSoapData(functionName, service, path, soapValues);

					std::unordered_map<std::string, BaseLib::Systems::RPCConfigurationParameter>::iterator parameterIterator = channelTwoIterator->second.find("CURRENT_TRACK");
					if(parameterIterator != channelTwoIterator->second.end())
					{
						std::shared_ptr<BaseLib::RPC::Variable> variable = _binaryDecoder->decodeResponse(parameterIterator->second.data);
						if(!variable || trackNumber != variable->integerValue)
						{
							break;
						}
					}
					else break;
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}

				functionName = "Pause";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0") });
				getSoapData(functionName, service, path, soapValues, true);

				functionName = "SetVolume";
				service = "urn:schemas-upnp-org:service:RenderingControl:1";
				path = "/MediaRenderer/RenderingControl/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Channel", "Master"), std::pair<std::string, std::string>("DesiredVolume", std::to_string(volumeState)) });
				getSoapData(functionName, service, path, soapValues);

				functionName = "SetMute";
				service = "urn:schemas-upnp-org:service:RenderingControl:1";
				path = "/MediaRenderer/RenderingControl/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Channel", "Master"), std::pair<std::string, std::string>("DesiredMute", std::to_string((int32_t)muteState)) });
				getSoapData(functionName, service, path, soapValues);

				functionName = "Seek";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Unit", "TRACK_NR"), std::pair<std::string, std::string>("Target", std::to_string(trackNumberState)) });
				getSoapData(functionName, service, path, soapValues);

				functionName = "Seek";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Unit", "REL_TIME"), std::pair<std::string, std::string>("Target", seekTimeState) });
				getSoapData(functionName, service, path, soapValues);

				functionName = "RemoveTrackFromQueue";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("ObjectID", "Q:0/" + std::to_string(trackNumber)) });
				getSoapData(functionName, service, path, soapValues);
			}

			if(transportState == "PLAYING")
			{
				functionName = "Play";
				service = "urn:schemas-upnp-org:service:AVTransport:1";
				path = "/MediaRenderer/AVTransport/Control";
				soapValues.reset(new std::vector<std::pair<std::string, std::string>>{ std::pair<std::string, std::string>("InstanceID", "0"), std::pair<std::string, std::string>("Speed", "1") });
				getSoapData(functionName, service, path, soapValues);
			}

			return true;
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
	return false;
}

}
