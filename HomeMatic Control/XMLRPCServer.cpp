#include "XMLRPCServer.h"

XMLRPCServer::XMLRPCServer() {


}

XMLRPCServer::~XMLRPCServer() {
	shutdown();
	if(_abyssServer != nullptr) delete _abyssServer;
}

void XMLRPCServer::run()
{
	_runThread = std::thread(&XMLRPCServer::runThread, this);
	_runThread.detach();
}

void XMLRPCServer::shutdown()
{
	if(_abyssServer != nullptr && _runThread.joinable()) _abyssServer->terminate();
}

void XMLRPCServer::runThread()
{
	try
	{
		xmlrpc_c::registry myRegistry;
		xmlrpc_c::methodPtr const sampleAddMethodP(new sampleAddMethod);
		myRegistry.addMethod("sample.add", sampleAddMethodP);
		_abyssServer = new xmlrpc_c::serverAbyss(xmlrpc_c::serverAbyss::constrOpt().registryP(&myRegistry).portNumber(2001));
		_abyssServer->run();
		std::cout << "XML RPC server successfully shut down." << std::endl;
	}
	catch(const std::exception& ex)
	{
		std::cerr << "Error in file " << __FILE__ " line " << __LINE__ << " in function " << __PRETTY_FUNCTION__ <<": " << ex.what() << std::endl;
	}
}
