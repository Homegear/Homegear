#include "XMLRPCServer.h"

#include "GD.h"

using namespace XMLRPC;

Server::Server()
{
}

Server::~Server()
{
	shutdown();
	if(_abyssServer != nullptr) delete _abyssServer;
}

void Server::run()
{
	_runThread = std::thread(&Server::runThread, this);
	_runThread.detach();
}

void Server::shutdown()
{
	if(_abyssServer != nullptr && _runThread.joinable()) _abyssServer->terminate();
}

void Server::runThread()
{
	try
	{
		xmlrpc_c::registry myRegistry;
		xmlrpc_c::methodPtr const setValueP(new setValue);
		myRegistry.addMethod("setValue", setValueP);
		xmlrpc_c::methodPtr const getValueP(new getValue);
		myRegistry.addMethod("getValue", getValueP);
		_abyssServer = new xmlrpc_c::serverAbyss(xmlrpc_c::serverAbyss::constrOpt().registryP(&myRegistry).portNumber(2001).uriPath("/"));
		_abyssServer->run();
		if(GD::debugLevel >= 4) std::cout << "XML RPC server successfully shut down." << std::endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}

getValue::getValue()
{
	this->_signature = "i:ss,b:ss,d:ss,s:ss";
	this->_help = "Gets a single value of the parameter set \"VALUES\".";
}

void getValue::execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) {
	paramList.verifyEnd(2);
	xmlrpc_c::value result = GD::devices.get(paramList.getString(0))->getValue(paramList);
	*retvalP = result;
}

setValue::setValue()
{
	this->_signature = "n:ssi,n:ssb,n:ssd,n:sss";
	this->_help = "Sets a single value of the parameter set \"VALUES\".";
}

void setValue::execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP) {
	paramList.verifyEnd(3);
	GD::devices.get(paramList.getString(0))->setValue(paramList);
	*retvalP = xmlrpc_c::value_nil();
}
