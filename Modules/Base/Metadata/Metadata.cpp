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

#include "Metadata.h"
#include "../BaseLib.h"

RPC::RPCEncoder Metadata::_rpcEncoder;
RPC::RPCDecoder Metadata::_rpcDecoder;

std::shared_ptr<RPC::RPCVariable> Metadata::getAllMetadata(std::string objectID)
{
	try
	{
		if(objectID.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(objectID)));

		DataTable rows = BaseLib::db.executeCommand("SELECT dataID, serializedObject FROM metadata WHERE objectID=?", data);
		if(rows.empty()) return RPC::RPCVariable::createError(-1, "No metadata found.");

		std::shared_ptr<RPC::RPCVariable> metadataStruct(new RPC::RPCVariable(RPC::RPCVariableType::rpcStruct));
		for(DataTable::iterator i = rows.begin(); i != rows.end(); ++i)
		{
			if(i->second.size() < 2) continue;
			std::shared_ptr<RPC::RPCVariable> metadata = _rpcDecoder.decodeResponse(i->second.at(1)->binaryValue);
			metadataStruct->structValue->insert(RPC::RPCStructElement(i->second.at(0)->textValue, metadata));
		}

		//getAllMetadata is called repetitively for all central peers. That takes a lot of ressources, so we wait a little after each call.
		std::this_thread::sleep_for(std::chrono::milliseconds(3));

		return metadataStruct;
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Metadata::getMetadata(std::string objectID, std::string dataID)
{
	try
	{
		if(objectID.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(dataID.size() > 250) return RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(objectID)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(dataID)));

		DataTable rows = BaseLib::db.executeCommand("SELECT serializedObject FROM metadata WHERE objectID=? AND dataID=?", data);
		if(rows.empty() || rows.at(0).empty()) return RPC::RPCVariable::createError(-1, "No metadata found.");

		std::shared_ptr<RPC::RPCVariable> metadata = _rpcDecoder.decodeResponse(rows.at(0).at(0)->binaryValue);
		return metadata;
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Metadata::setMetadata(std::string objectID, std::string dataID, std::shared_ptr<RPC::RPCVariable> metadata)
{
	try
	{
		if(!metadata) return RPC::RPCVariable::createError(-32602, "Could not parse data.");
		if(objectID.size() > 250) return RPC::RPCVariable::createError(-32602, "objectID has more than 250 characters.");
		if(dataID.size() > 250) return RPC::RPCVariable::createError(-32602, "dataID has more than 250 characters.");
		//Don't check for type here, so base64, string and future data types that use stringValue are handled
		if(metadata->stringValue.size() > 1000) return RPC::RPCVariable::createError(-32602, "Data has more than 1000 characters.");
		if(metadata->type != RPC::RPCVariableType::rpcBase64 && metadata->type != RPC::RPCVariableType::rpcString && metadata->type != RPC::RPCVariableType::rpcInteger && metadata->type != RPC::RPCVariableType::rpcFloat && metadata->type != RPC::RPCVariableType::rpcBoolean) return RPC::RPCVariable::createError(-32602, "Type " + RPC::RPCVariable::getTypeString(metadata->type) + " is currently not supported.");

		DataTable rows = BaseLib::db.executeCommand("SELECT COUNT(*) FROM metadata");
		if(rows.size() == 0 || rows.at(0).size() == 0) return RPC::RPCVariable::createError(-32500, "Error counting metadata in database.");
		if(rows.at(0).at(0)->intValue > 1000000) return RPC::RPCVariable::createError(-32500, "Reached limit of 1000000 metadata entries. Please delete metadata before adding new entries.");

		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(objectID)));
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(dataID)));
		BaseLib::db.executeCommand("DELETE FROM metadata WHERE objectID=? AND dataID=?", data);

		std::shared_ptr<std::vector<char>> value = _rpcEncoder.encodeResponse(metadata);
		if(!value) return RPC::RPCVariable::createError(-32700, "Could not encode data.");
		if(value->size() > 1000) return RPC::RPCVariable::createError(-32602, "Data is larger than 1000 bytes.");
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(value)));

		BaseLib::db.executeCommand("INSERT INTO metadata VALUES(?, ?, ?)", data);

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
	return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

std::shared_ptr<RPC::RPCVariable> Metadata::deleteMetadata(std::string objectID, std::string dataID)
{
	try
	{
		DataColumnVector data;
		data.push_back(std::shared_ptr<DataColumn>(new DataColumn(objectID)));
		std::string command("DELETE FROM metadata WHERE objectID=?");
		if(!dataID.empty())
		{
			data.push_back(std::shared_ptr<DataColumn>(new DataColumn(dataID)));
			command.append(" AND dataID=?");
		}
		BaseLib::db.executeCommand(command, data);

		return std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(RPC::RPCVariableType::rpcVoid));
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
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
