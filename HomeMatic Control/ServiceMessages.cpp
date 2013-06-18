#include "ServiceMessages.h"
#include "GD.h"

ServiceMessages::ServiceMessages(std::string peerSerialNumber, std::string serializedObject) : ServiceMessages(peerSerialNumber)
{
	if(serializedObject.empty()) return;
	if(GD::debugLevel >= 5) std::cout << "Unserializing service message: " << serializedObject << std::endl;

	std::istringstream stringstream(serializedObject);
	uint32_t pos = 0;
	unreach = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	stickyUnreach = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	configPending = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	lowbat = std::stoll(serializedObject.substr(pos, 1)); pos += 1;
	rssiDevice = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
	rssiPeer = std::stoll(serializedObject.substr(pos, 8), 0, 16); pos += 8;
}

std::string ServiceMessages::serialize()
{
	std::ostringstream stringstream;
	stringstream << std::hex << std::uppercase << std::setfill('0');
	stringstream << std::setw(1) << (int32_t)unreach;
	stringstream << std::setw(1) << (int32_t)stickyUnreach;
	stringstream << std::setw(1) << (int32_t)configPending;
	stringstream << std::setw(1) << (int32_t)lowbat;
	stringstream << std::setw(8) << rssiDevice;
	stringstream << std::setw(8) << rssiPeer;
	stringstream << std::dec;
	return stringstream.str();
}

bool ServiceMessages::set(std::string id, std::shared_ptr<RPC::RPCVariable> value)
{
	try
	{
		if(id == "UNREACH") unreach = value->booleanValue;
		else if(id == "STICKY_UNREACH") stickyUnreach = value->booleanValue;
		else if(id == "CONFIG_PENDING") configPending = value->booleanValue;
		else if(id == "LOWBAT") lowbat = value->booleanValue;
		else return false;
		return true;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
}

std::shared_ptr<RPC::RPCVariable> ServiceMessages::get()
{
	try
	{
		std::shared_ptr<RPC::RPCVariable> serviceMessages(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
		std::shared_ptr<RPC::RPCVariable> array;
		if(unreach)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerialNumber + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("UNREACH"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(stickyUnreach)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerialNumber + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("STICKY_UNREACH"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(configPending)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerialNumber + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("CONFIG_PENDING"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		if(lowbat)
		{
			array.reset(new RPC::RPCVariable(RPC::RPCVariableType::rpcArray));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(_peerSerialNumber + ":0")));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(std::string("LOWBAT"))));
			array->arrayValue->push_back(std::shared_ptr<RPC::RPCVariable>(new RPC::RPCVariable(true)));
			serviceMessages->arrayValue->push_back(array);
		}
		return serviceMessages;
	}
	catch(const std::exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(const Exception& ex)
    {
    	std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
    }
    catch(...)
    {
    	std::cerr << "Unknown error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ << "." << std::endl;
    }
    return RPC::RPCVariable::createError(-32500, "Unknown application error.");
}
