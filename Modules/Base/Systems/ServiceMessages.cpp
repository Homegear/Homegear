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

#include "ServiceMessages.h"
#include "../BaseLib.h"

namespace BaseLib
{
namespace Systems
{

ServiceMessages::ServiceMessages(uint64_t peerID, std::string peerSerial)
{
	_peerID = peerID;
	_peerSerial = peerSerial;
}

ServiceMessages::~ServiceMessages()
{

}

//Event handling
void ServiceMessages::raiseOnRPCBroadcast(uint64_t id, int32_t channel, std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> values)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IEventSink*)*i)->onRPCBroadcast(id, channel, deviceAddress, valueKeys, values);
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
    _eventHandlerMutex.unlock();
}

void ServiceMessages::raiseOnSaveParameter(std::string name, uint32_t channel, std::vector<uint8_t>& data)
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IEventSink*)*i)->onSaveParameter(name, channel, data);
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
    _eventHandlerMutex.unlock();
}

void ServiceMessages::raiseOnEnqueuePendingQueues()
{
	try
	{
		_eventHandlerMutex.lock();
		for(std::vector<IEventSinkBase*>::iterator i = _eventHandlers.begin(); i != _eventHandlers.end(); ++i)
		{
			if(*i) ((IEventSink*)*i)->onEnqueuePendingQueues();
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
    _eventHandlerMutex.unlock();
}
//End event handling

void ServiceMessages::serialize(std::vector<uint8_t>& encodedData)
{
	try
	{
		BinaryEncoder encoder;
		encoder.encodeBoolean(encodedData, _unreach);
		encoder.encodeBoolean(encodedData, _stickyUnreach);
		encoder.encodeBoolean(encodedData, _configPending);
		encoder.encodeBoolean(encodedData, _lowbat);
		_errorMutex.lock();
		encoder.encodeInteger(encodedData, _errors.size());
		for(std::map<uint32_t, std::map<std::string, uint8_t>>::iterator i = _errors.begin(); i != _errors.end(); ++i)
		{
			encoder.encodeInteger(encodedData, i->first);
			encoder.encodeInteger(encodedData, i->second.size());
			for(std::map<std::string, uint8_t>::iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				std::string name = j->first;
				encoder.encodeString(encodedData, name);
				encodedData.push_back(j->second);
			}
		}
		_errorMutex.unlock();
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
	_errorMutex.unlock();
}

void ServiceMessages::unserialize(std::shared_ptr<std::vector<char>> serializedData)
{
	try
	{
		BinaryDecoder decoder;
		uint32_t position = 0;
		_unreach = decoder.decodeBoolean(serializedData, position);
		_stickyUnreach = decoder.decodeBoolean(serializedData, position);
		_configPending = decoder.decodeBoolean(serializedData, position);
		_lowbat = decoder.decodeBoolean(serializedData, position);
		uint32_t errorSize = decoder.decodeInteger(serializedData, position);
		_errorMutex.lock();
		for(uint32_t i = 0; i < errorSize; ++i)
		{
			int32_t channel = decoder.decodeInteger(serializedData, position);
			uint32_t elementsSize = decoder.decodeInteger(serializedData, position);
			for(uint32_t j = 0; j < elementsSize; ++j)
			{
				std::string name = decoder.decodeString(serializedData, position);
				_errors[channel][name] = serializedData->at(position);
				position++;
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
    _errorMutex.unlock();
}

bool ServiceMessages::set(std::string id, bool value)
{
	try
	{
		if(_disposing) return false;
		if(id == "LOWBAT_REPORTING") id = "LOWBAT"; //HM-TC-IT-WM-W-EU
		if(id == "UNREACH" && value != _unreach) _unreach = value;
		else if(id == "STICKY_UNREACH" && value != _stickyUnreach) _stickyUnreach = value;
		else if(id == "CONFIG_PENDING" && value != _configPending) _configPending = value;
		else if((id == "LOWBAT") && value != _lowbat) _lowbat = value;
		else if(!value) //false == 0, a little dirty, but it works
		{
			_errorMutex.lock();
			for(std::map<uint32_t, std::map<std::string, uint8_t>>::iterator i = _errors.begin(); i != _errors.end(); ++i)
			{
				if(i->second.find(id) != i->second.end())
				{
					i->second.at(id) = 0;
					std::vector<uint8_t> data = { (uint8_t)value };
					raiseOnSaveParameter(id, i->first, data);

					std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({id}));
					std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
					rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)0)));
					raiseOnRPCBroadcast(_peerID, 0, _peerSerial + ":" + std::to_string(i->first), valueKeys, rpcValues);
				}
			}
			_errorMutex.unlock();
			return true;
		}

		std::vector<uint8_t> data = { (uint8_t)value };
		raiseOnSaveParameter(id, 0, data);

		std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({id}));
		std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
		rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value)));

		raiseOnRPCBroadcast(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
	}
	catch(const std::exception& ex)
    {
		_errorMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_errorMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_errorMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
    return true;
}

void ServiceMessages::set(std::string id, uint8_t value, uint32_t channel)
{
	try
	{
		if(_disposing) return;
		_errorMutex.lock();
		if(value == 0 && _errors.find(channel) != _errors.end() && _errors[channel].find(id) != _errors[channel].end())
		{
			_errors[channel].erase(id);
			if(_errors[channel].empty()) _errors.erase(channel);
		}
		if(value > 0) _errors[channel][id] = value;
		_errorMutex.unlock();

		//RPC Broadcast is done in peer's packetReceived
	}
	catch(const std::exception& ex)
    {
		_errorMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(Exception& ex)
    {
    	_errorMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch(...)
    {
    	_errorMutex.unlock();
    	Output::printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
}

std::shared_ptr<RPC::RPCVariable> ServiceMessages::get(bool returnID)
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> serviceMessages(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> array;
		if(_unreach)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			if(returnID)
			{
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID)));
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)0)));
			}
			else array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerial + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("UNREACH"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_stickyUnreach)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			if(returnID)
			{
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID)));
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)0)));
			}
			else array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerial + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("STICKY_UNREACH"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_configPending)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			if(returnID)
			{
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID)));
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)0)));
			}
			else array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerial + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("CONFIG_PENDING"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(_lowbat)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			if(returnID)
			{
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID)));
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)0)));
			}
			else array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerial + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("LOWBAT"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		_errorMutex.lock();
		for(std::map<uint32_t, std::map<std::string, uint8_t>>::const_iterator i = _errors.begin(); i != _errors.end(); ++i)
		{
			for(std::map<std::string, uint8_t>::const_iterator j = i->second.begin(); j != i->second.end(); ++j)
			{
				if(j->second == 0) continue;
				array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
				if(returnID)
				{
					array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((int32_t)_peerID)));
					array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(i->first)));
				}
				else array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerial + std::to_string(i->first))));
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(j->first)));
				array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable((uint32_t)j->second)));
				serviceMessages->arrayValue->push_back(array);
			}
		}
		_errorMutex.unlock();
		return serviceMessages;
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
    _errorMutex.unlock();
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}

void ServiceMessages::checkUnreach(int32_t cyclicTimeout, uint32_t lastPacketReceived)
{
	try
	{
		uint32_t time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if(cyclicTimeout > 0 && (time - lastPacketReceived) > cyclicTimeout && !_unreach)
		{
			_unreach = true;
			_stickyUnreach = true;

			std::vector<uint8_t> data = { 1 };
			raiseOnSaveParameter("UNREACH", 0, data);
			raiseOnSaveParameter("STICKY_UNREACH", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("UNREACH"), std::string("STICKY_UNREACH")}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));

			raiseOnRPCBroadcast(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

void ServiceMessages::endUnreach()
{
	try
	{
		if(_unreach == true)
		{
			_unreach = false;
			_unreachResendCounter = 0;

			std::vector<uint8_t> data = { 0 };
			raiseOnSaveParameter("UNREACH", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("UNREACH")}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(false)));

			raiseOnRPCBroadcast(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

void ServiceMessages::setConfigPending(bool value)
{
	try
	{
		if(value != _configPending)
		{
			_configPending = value;
			std::vector<uint8_t> data = { (uint8_t)value };
			raiseOnSaveParameter("CONFIG_PENDING", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("CONFIG_PENDING")}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value)));

			raiseOnRPCBroadcast(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

void ServiceMessages::setUnreach(bool value)
{
	try
	{
		if(value != _unreach)
		{
			if(value == true && _unreachResendCounter < 3)
			{
				raiseOnEnqueuePendingQueues();
				_unreachResendCounter++;
				return;
			}
			_unreachResendCounter = 0;
			_unreach = value;

			if(value) Output::printInfo("Info: Peer " + std::to_string(_peerID) + " is unreachable.");
			std::vector<uint8_t> data = { (uint8_t)value };
			raiseOnSaveParameter("UNREACH", 0, data);

			std::shared_ptr<std::vector<std::string>> valueKeys(new std::vector<std::string>({std::string("UNREACH")}));
			std::shared_ptr<std::vector<std::shared_ptr<RPC::RPCVariable>>> rpcValues(new std::vector<std::shared_ptr<RPC::RPCVariable>>());
			rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(value)));

			if(value)
			{
				raiseOnSaveParameter("STICKY_UNREACH", 0, data);

				valueKeys->push_back("STICKY_UNREACH");
				rpcValues->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			}

			raiseOnRPCBroadcast(_peerID, 0, _peerSerial + ":0", valueKeys, rpcValues);
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

}
}
