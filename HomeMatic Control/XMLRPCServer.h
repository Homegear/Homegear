#ifndef XMLRPCSERVER_H_
#define XMLRPCSERVER_H_

#include <thread>
#include <iostream>
#include <string>

#include <unistd.h>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

class sampleAddMethod : public xmlrpc_c::method {
public:
    sampleAddMethod() {
        // signature and help strings are documentation -- the client
        // can query this information with a system.methodSignature and
        // system.methodHelp RPC.
        this->_signature = "i:ii";
            // method's result and two arguments are integers
        this->_help = "This method adds two integers together";
    }
    void
    execute(xmlrpc_c::paramList const& paramList,
            xmlrpc_c::value *   const  retvalP) {

        int const addend(paramList.getInt(0));
        int const adder(paramList.getInt(1));

        paramList.verifyEnd(2);

        *retvalP = xmlrpc_c::value_int(addend + adder);

        // Sometimes, make it look hard (so client can see what it's like
        // to do an RPC that takes a while).
        if (adder == 1)
            sleep(2);
    }
};

class XMLRPCServer {
public:
	XMLRPCServer();
	virtual ~XMLRPCServer();

	void run();
	void shutdown();
protected:
private:
	std::thread _runThread;
	xmlrpc_c::serverAbyss* _abyssServer = nullptr;

	void runThread();
};

#endif /* XMLRPCSERVER_H_ */
