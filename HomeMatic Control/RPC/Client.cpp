#include "Client.h"

namespace RPC
{
void Client::broadcastEvent(std::string deviceAddress, std::shared_ptr<std::vector<std::string>> valueKeys, std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> values)
{
	if(!valueKeys || !values || valueKeys->size() != values->size()) return;
	std::string methodName("event");
	std::shared_ptr<std::vector<std::shared_ptr<RemoteRPCServer>>> servers = _client.getServers();
	for(std::vector<std::shared_ptr<RemoteRPCServer>>::iterator server = servers->begin(); server != servers->end(); ++server)
	{
		std::shared_ptr<std::list<std::shared_ptr<RPCVariable>>> parameters(new std::list<std::shared_ptr<RPCVariable>>());
		std::shared_ptr<RPCVariable> array(new RPCVariable(RPCVariableType::rpcArray));
		std::shared_ptr<RPCVariable> method;
		for(uint32_t i = 0; i < valueKeys->size(); i++)
		{
			method.reset(new RPCVariable(RPCVariableType::rpcStruct));
			array->arrayValue->push_back(method);
			method->structValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable("methodName", methodName)));
			std::shared_ptr<RPCVariable> params(new RPCVariable(RPCVariableType::rpcArray));
			method->structValue->push_back(params);
			params->name = "params";
			params->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable((*server)->id)));
			params->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(deviceAddress)));
			params->arrayValue->push_back(std::shared_ptr<RPCVariable>(new RPCVariable(valueKeys->at(i))));
			params->arrayValue->push_back(values->at(i));
		}
		parameters->push_back(array);
		//Sadly some clients only support multicall and not "event" directly for single events. That's why we use multicall even if there is only one value.
		std::thread t(&RPCClient::invokeBroadcast, &_client, (*server)->address.first, (*server)->address.second, "system.multicall", parameters);
		t.detach();
	}
}

} /* namespace RPC */
