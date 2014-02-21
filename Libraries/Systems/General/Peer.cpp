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

#include "Peer.h"
#include "../../GD/GD.h"

Peer::Peer(uint32_t parentID, bool centralFeatures)
{
	try
	{
		_parentID = parentID;
		_centralFeatures = centralFeatures;
		_lastPacketReceived = HelperFunctions::getTimeSeconds();
		rpcDevice.reset();
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::Peer(int32_t id, int32_t address, std::string serialNumber, uint32_t parentID, bool centralFeatures) : Peer(parentID, centralFeatures)
{
	try
	{
		_peerID = id;
		_address = address;
		_serialNumber = serialNumber;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

Peer::~Peer()
{

}

RPC::Device::RXModes::Enum Peer::getRXModes()
{
	try
	{
		if(rpcDevice)
		{
			_rxModes = rpcDevice->rxModes;
			if(configCentral.find(0) != configCentral.end() && configCentral.at(0).find("BURST_RX") != configCentral.at(0).end())
			{
				RPCConfigurationParameter* parameter = &configCentral.at(0).at("BURST_RX");
				if(!parameter->rpcParameter) return _rxModes;
				if(parameter->rpcParameter->convertFromPacket(parameter->data)->booleanValue)
				{
					_rxModes = (RPC::Device::RXModes::Enum)(_rxModes | RPC::Device::RXModes::Enum::burst);
				}
			}
		}
		return _rxModes;
	}
	catch(const std::exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(Exception& ex)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	return _rxModes;
}

void Peer::setLastPacketReceived()
{
	_lastPacketReceived = HelperFunctions::getTimeSeconds();
}

void Peer::deleteFromDatabase()
{
	try
	{
		Metadata::deleteMetadata(_serialNumber);
		Metadata::deleteMetadata(std::to_string(_peerID));
		_databaseMutex.lock();
		DataColumnVector data({std::shared_ptr<DataColumn>(new DataColumn(_peerID))});
		GD::db.executeCommand("DELETE FROM parameters WHERE peerID=?", data);
		GD::db.executeCommand("DELETE FROM peerVariables WHERE peerID=?", data);
		GD::db.executeCommand("DELETE FROM peers WHERE peerID=?", data);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::save(bool savePeer, bool variables, bool centralConfig)
{
	try
	{
		if(deleting || isTeam()) return;
		GD::db.executeCommand("SAVEPOINT peer" + std::to_string(_address));
		if(savePeer)
		{
			_databaseMutex.lock();
			DataColumnVector data;
			if(_peerID > 0) data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
			else data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_parentID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_address)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_serialNumber)));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO peers VALUES(?, ?, ?, ?)", data);
			if(_peerID == 0) _peerID = result;
			_databaseMutex.unlock();
		}
		if(variables) saveVariables();
		if(centralConfig) saveConfig();
	}
	catch(const std::exception& ex)
    {
		_databaseMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_databaseMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_databaseMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE peer" + std::to_string(_address));
}

void Peer::saveParameter(uint32_t parameterID, std::vector<uint8_t>& value)
{
	try
	{
		if(parameterID == 0)
		{
			if(!isTeam()) Output::printError("Peer " + std::to_string(_peerID) + ": Tried to save parameter without parameterID");
			return;
		}
		_databaseMutex.lock();
		GD::db.executeCommand("SAVEPOINT peerParameter" + std::to_string(_address));
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameterID)));
		GD::db.executeWriteCommand("UPDATE parameters SET value=? WHERE parameterID=?", data);
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE peerParameter" + std::to_string(_address));
    _databaseMutex.unlock();
}

void Peer::saveParameter(uint32_t parameterID, uint32_t address, std::vector<uint8_t>& value)
{
	try
	{
		if(parameterID > 0)
		{
			saveParameter(parameterID, value);
			return;
		}
		if(_peerID == 0 || isTeam()) return;
		//Creates a new entry for parameter in database
		_databaseMutex.lock();
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(address)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(0)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(std::string(""))));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		uint32_t result = GD::db.executeWriteCommand("REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);

		binaryConfig[address].databaseID = result;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveParameter(uint32_t parameterID, RPC::ParameterSet::Type::Enum parameterSetType, uint32_t channel, std::string& parameterName, std::vector<uint8_t>& value, int32_t remoteAddress, uint32_t remoteChannel)
{
	try
	{
		if(parameterID > 0)
		{
			saveParameter(parameterID, value);
			return;
		}
		if(_peerID == 0 || isTeam()) return;
		//Creates a new entry for parameter in database
		_databaseMutex.lock();
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn((uint32_t)parameterSetType)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(channel)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(remoteAddress)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(remoteChannel)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(parameterName)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));
		uint32_t result = GD::db.executeWriteCommand("REPLACE INTO parameters VALUES(?, ?, ?, ?, ?, ?, ?, ?)", data);

		if(parameterSetType == RPC::ParameterSet::Type::Enum::master) configCentral[channel][parameterName].databaseID = result;
		else if(parameterSetType == RPC::ParameterSet::Type::Enum::values) valuesCentral[channel][parameterName].databaseID = result;
		else if(parameterSetType == RPC::ParameterSet::Type::Enum::link) linksCentral[channel][remoteAddress][remoteChannel][parameterName].databaseID = result;
		else if(parameterSetType == RPC::ParameterSet::Type::Enum::none) binaryConfig[channel].databaseID = result;
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, int32_t intValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		GD::db.executeCommand("SAVEPOINT peerVariable" + std::to_string(_address));
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE peerVariables SET integerValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_peerID == 0)
			{
				GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, int64_t intValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		GD::db.executeCommand("SAVEPOINT peerVariable" + std::to_string(_address));
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE peerVariables SET integerValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_peerID == 0)
			{
				GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(intValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, std::string& stringValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		GD::db.executeCommand("SAVEPOINT peerVariable" + std::to_string(_address));
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(stringValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE peerVariables SET stringValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_peerID == 0)
			{
				GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(stringValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
    _databaseMutex.unlock();
}

void Peer::saveVariable(uint32_t index, std::vector<uint8_t>& binaryValue)
{
	try
	{
		if(isTeam()) return;
		_databaseMutex.lock();
		GD::db.executeCommand("SAVEPOINT peerVariable" + std::to_string(_address));
		bool idIsKnown = _variableDatabaseIDs.find(index) != _variableDatabaseIDs.end();
		DataColumnVector data;
		if(idIsKnown)
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(binaryValue)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_variableDatabaseIDs[index])));
			GD::db.executeWriteCommand("UPDATE peerVariables SET binaryValue=? WHERE variableID=?", data);
		}
		else
		{
			if(_peerID == 0)
			{
				GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
				_databaseMutex.unlock();
				return;
			}
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(index)));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn()));
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(binaryValue)));
			int32_t result = GD::db.executeWriteCommand("REPLACE INTO peerVariables VALUES(?, ?, ?, ?, ?, ?)", data);
			_variableDatabaseIDs[index] = result;
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    GD::db.executeCommand("RELEASE peerVariable" + std::to_string(_address));
    _databaseMutex.unlock();
}

void Peer::saveConfig()
{
	try
	{
		if(_peerID == 0 || isTeam()) return;
		for(std::unordered_map<uint32_t, ConfigDataBlock>::iterator i = binaryConfig.begin(); i != binaryConfig.end(); ++i)
		{
			std::string emptyString;
			if(i->second.databaseID > 0) saveParameter(i->second.databaseID, i->second.data);
			else saveParameter(0, RPC::ParameterSet::Type::Enum::none, i->first, emptyString, i->second.data);
		}
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator i = configCentral.begin(); i != configCentral.end(); ++i)
		{
			for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!j->second.rpcParameter)
				{
					Output::printCritical("Critical: Parameter " + j->first + " has no corresponding RPC parameter. Writing dummy data. Device: " + std::to_string(_peerID) + " Channel: " + std::to_string(i->first));
					continue;
				}
				if(j->second.rpcParameter->id.size() == 0)
				{
					Output::printError("Error: Parameter has no id.");
					continue;
				}
				std::string name = j->first;
				if(j->second.databaseID > 0) saveParameter(j->second.databaseID, j->second.data);
				else saveParameter(0, RPC::ParameterSet::Type::Enum::master, i->first, name, j->second.data);
			}
		}
		for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator i = valuesCentral.begin(); i != valuesCentral.end(); ++i)
		{
			for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(!j->second.rpcParameter)
				{
					Output::printCritical("Critical: Parameter " + j->first + " has no corresponding RPC parameter. Writing dummy data. Device: " + std::to_string(_peerID) + " Channel: " + std::to_string(i->first));
					continue;
				}
				if(j->second.rpcParameter->id.size() == 0)
				{
					Output::printError("Error: Parameter has no id.");
					continue;
				}
				std::string name = j->first;
				if(j->second.databaseID > 0) saveParameter(j->second.databaseID, j->second.data);
				else saveParameter(0, RPC::ParameterSet::Type::Enum::values, i->first, name, j->second.data);
			}
		}
		for(std::unordered_map<uint32_t, std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>>::iterator i = linksCentral.begin(); i != linksCentral.end(); ++i)
		{
			for(std::unordered_map<int32_t, std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				for(std::unordered_map<uint32_t, std::unordered_map<std::string, RPCConfigurationParameter>>::iterator k = j->second.begin(); k != j->second.end(); ++k)
				{
					for(std::unordered_map<std::string, RPCConfigurationParameter>::iterator l = k->second.begin(); l != k->second.end(); ++l)
					{
						if(!l->second.rpcParameter)
						{
							Output::printCritical("Critical: Parameter " + l->first + " has no corresponding RPC parameter. Writing dummy data. Device: " + std::to_string(_peerID) + " Channel: " + std::to_string(i->first));
							continue;
						}
						if(l->second.rpcParameter->id.size() == 0)
						{
							Output::printError("Error: Parameter has no id.");
							continue;
						}
						std::string name = l->first;
						if(l->second.databaseID > 0) saveParameter(l->second.databaseID, l->second.data);
						else saveParameter(0, RPC::ParameterSet::Type::Enum::link, i->first,name, l->second.data, j->first, k->first);
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

void Peer::loadConfig()
{
	try
	{
		_databaseMutex.lock();
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
		DataTable rows = GD::db.executeCommand("SELECT * FROM parameters WHERE peerID=?", data);
		for(DataTable::iterator row = rows.begin(); row != rows.end(); ++row)
		{
			uint32_t databaseID = row->second.at(0)->intValue;
			RPC::ParameterSet::Type::Enum parameterSetType = (RPC::ParameterSet::Type::Enum)row->second.at(2)->intValue;

			if(parameterSetType == RPC::ParameterSet::Type::Enum::none)
			{
				uint32_t index = row->second.at(3)->intValue;
				ConfigDataBlock* config = &binaryConfig[index];
				config->databaseID = databaseID;
				config->data.insert(config->data.begin(), row->second.at(7)->binaryValue->begin(), row->second.at(7)->binaryValue->end());
			}
			else
			{
				uint32_t channel = row->second.at(3)->intValue;
				int32_t remoteAddress = row->second.at(4)->intValue;
				int32_t remoteChannel = row->second.at(5)->intValue;
				std::string* parameterName = &row->second.at(6)->textValue;
				if(parameterName->empty())
				{
					Output::printCritical("Critical: Added central config parameter without id. Device: " + std::to_string(_peerID) + " Channel: " + std::to_string(channel));
					_databaseMutex.lock();
					data.clear();
					data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
					data.push_back(std::shared_ptr<DataColumn>(new DataColumn(std::string(""))));
					GD::db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterID=?", data);
					_databaseMutex.unlock();
					continue;
				}

				RPCConfigurationParameter* parameter;
				if(parameterSetType == RPC::ParameterSet::Type::Enum::master) parameter = &configCentral[channel][*parameterName];
				else if(parameterSetType == RPC::ParameterSet::Type::Enum::values) parameter = &valuesCentral[channel][*parameterName];
				else if(parameterSetType == RPC::ParameterSet::Type::Enum::link) parameter = &linksCentral[channel][remoteAddress][remoteChannel][*parameterName];
				parameter->databaseID = databaseID;
				parameter->data.insert(parameter->data.begin(), row->second.at(7)->binaryValue->begin(), row->second.at(7)->binaryValue->end());
				if(!rpcDevice)
				{
					Output::printError("Critical: No xml rpc device found for peer " + std::to_string(_peerID) + ".");
					continue;
				}
				parameter->rpcParameter = rpcDevice->channels[channel]->parameterSets[parameterSetType]->getParameter(*parameterName);
				if(!parameter->rpcParameter)
				{
					Output::printError("Error: Deleting parameter " + *parameterName + ", because no corresponding RPC parameter was found. Device: " + std::to_string(_peerID) + " Channel: " + std::to_string(channel));
					DataColumnVector data;
					data.push_back(std::shared_ptr<DataColumn>(new DataColumn(_peerID)));
					data.push_back(std::shared_ptr<DataColumn>(new DataColumn((int32_t)parameterSetType)));
					data.push_back(std::shared_ptr<DataColumn>(new DataColumn(channel)));
					data.push_back(std::shared_ptr<DataColumn>(new DataColumn(*parameterName)));
					if(parameterSetType == RPC::ParameterSet::Type::Enum::master)
					{
						configCentral[channel].erase(*parameterName);
						GD::db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=?", data);
					}
					else if(parameterSetType == RPC::ParameterSet::Type::Enum::values)
					{
						valuesCentral[channel].erase(*parameterName);
						GD::db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=?", data);
					}
					else if(parameterSetType == RPC::ParameterSet::Type::Enum::link)
					{
						linksCentral[channel][remoteAddress][remoteChannel].erase(*parameterName);
						data.push_back(std::shared_ptr<DataColumn>(new DataColumn(remoteAddress)));
						data.push_back(std::shared_ptr<DataColumn>(new DataColumn(remoteChannel)));
						GD::db.executeCommand("DELETE FROM parameters WHERE peerID=? AND parameterSetType=? AND peerChannel=? AND parameterName=? AND remotePeer=? AND remoteChannel=?", data);
					}
				}
			}
		}
	}
	catch(const std::exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    _databaseMutex.unlock();
}
