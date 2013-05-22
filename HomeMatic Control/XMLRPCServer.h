#ifndef XMLRPCSERVER_H_
#define XMLRPCSERVER_H_

#include <thread>
#include <iostream>
#include <string>

#include <unistd.h>
#include <xmlrpc-c/base.hpp>
#include <xmlrpc-c/registry.hpp>
#include <xmlrpc-c/server_abyss.hpp>

namespace XMLRPC
{
	class getValue : public xmlrpc_c::method
	{
		public:
			getValue();
			void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP);
	};

	class setValue : public xmlrpc_c::method
	{
		public:
			setValue();
			void execute(xmlrpc_c::paramList const& paramList, xmlrpc_c::value* const retvalP);
	};

	class Server {
		public:
			Server();
			virtual ~Server();

			void run();
			void shutdown();
		protected:
		private:
			std::thread _runThread;
			xmlrpc_c::serverAbyss* _abyssServer = nullptr;

			void runThread();
	};
}
#endif /* XMLRPCSERVER_H_ */
