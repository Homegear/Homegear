/* Copyright 2013 Sathya Laufer
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

#ifndef RPCMETHODS_H_
#define RPCMETHODS_H_

#include <vector>
#include <memory>
#include <cstdlib>

#include "RPCServer.h"
#include "RPCVariable.h"
#include "RPCMethod.h"

namespace RPC
{
class RPCSystemGetCapabilities : public RPCMethod
{
public:
	RPCSystemGetCapabilities()
	{
		setHelp("Lists server's XML RPC capabilities.");
		addSignature(RPCVariableType::rpcStruct, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCSystemListMethods : public RPCMethod
{
public:
	RPCSystemListMethods(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Lists all XML RPC methods.");
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCSystemMethodHelp : public RPCMethod
{
public:
	RPCSystemMethodHelp(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Returns a description of the method.");
		addSignature(RPCVariableType::rpcString, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCSystemMethodSignature : public RPCMethod
{
public:
	RPCSystemMethodSignature(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Returns the method's signature.");
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCSystemMulticall : public RPCMethod
{
public:
	RPCSystemMulticall(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Calls multiple XML RPC methods.");
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcArray});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCAbortEventReset : public RPCMethod
{
public:
	RPCAbortEventReset()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCAddDevice : public RPCMethod
{
public:
	RPCAddDevice()
	{
		addSignature(RPCVariableType::rpcStruct, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCAddEvent : public RPCMethod
{
public:
	RPCAddEvent()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcStruct});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCAddLink : public RPCMethod
{
public:
	RPCAddLink()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCClientServerInitialized : public RPCMethod
{
public:
	RPCClientServerInitialized()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>({RPCVariableType::rpcString}));
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCDeleteDevice : public RPCMethod
{
public:
	RPCDeleteDevice()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcInteger});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCDeleteMetadata : public RPCMethod
{
public:
	RPCDeleteMetadata()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCEnableEvent : public RPCMethod
{
public:
	RPCEnableEvent()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcBoolean});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetAllMetadata : public RPCMethod
{
public:
	RPCGetAllMetadata()
	{
		addSignature(RPCVariableType::rpcVariant, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	RPCDecoder _rpcDecoder;
};

class RPCGetDeviceDescription : public RPCMethod
{
public:
	RPCGetDeviceDescription()
	{
		addSignature(RPCVariableType::rpcStruct, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetInstallMode : public RPCMethod
{
public:
	RPCGetInstallMode()
	{
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetKeyMismatchDevice : public RPCMethod
{
public:
	RPCGetKeyMismatchDevice()
	{
		addSignature(RPCVariableType::rpcString, std::vector<RPCVariableType>{RPCVariableType::rpcBoolean});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetLinkInfo : public RPCMethod
{
public:
	RPCGetLinkInfo()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetLinkPeers : public RPCMethod
{
public:
	RPCGetLinkPeers()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetLinks : public RPCMethod
{
public:
	RPCGetLinks()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>());
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcInteger});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetMetadata : public RPCMethod
{
public:
	RPCGetMetadata()
	{
		addSignature(RPCVariableType::rpcVariant, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	RPCDecoder _rpcDecoder;
};

class RPCGetParamsetDescription : public RPCMethod
{
public:
	RPCGetParamsetDescription()
	{
		addSignature(RPCVariableType::rpcStruct, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetParamsetId : public RPCMethod
{
public:
	RPCGetParamsetId()
	{
		addSignature(RPCVariableType::rpcString, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetParamset : public RPCMethod
{
public:
	RPCGetParamset()
	{
		addSignature(RPCVariableType::rpcStruct, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetServiceMessages : public RPCMethod
{
public:
	RPCGetServiceMessages()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCGetValue : public RPCMethod
{
public:
	RPCGetValue()
	{
		addSignature(RPCVariableType::rpcVariant, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCInit : public RPCMethod
{
public:
	RPCInit()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcInteger});
	}
	virtual ~RPCInit();

	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	std::thread _initServerThread;
};

class RPCListBidcosInterfaces : public RPCMethod
{
public:
	RPCListBidcosInterfaces()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCListDevices : public RPCMethod
{
public:
	RPCListDevices()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCListEvents : public RPCMethod
{
public:
	RPCListEvents()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>({RPCVariableType::rpcInteger}));
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>({RPCVariableType::rpcString}));
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>({RPCVariableType::rpcString, RPCVariableType::rpcString}));
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCListClientServers : public RPCMethod
{
public:
	RPCListClientServers()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>());
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>({RPCVariableType::rpcString}));
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCListTeams : public RPCMethod
{
public:
	RPCListTeams()
	{
		addSignature(RPCVariableType::rpcArray, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCLogLevel : public RPCMethod
{
public:
	RPCLogLevel()
	{
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>{RPCVariableType::rpcInteger});
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>());
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCPutParamset : public RPCMethod
{
public:
	RPCPutParamset()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcStruct});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCRemoveEvent : public RPCMethod
{
public:
	RPCRemoveEvent()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCRemoveLink : public RPCMethod
{
public:
	RPCRemoveLink()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCRunScript : public RPCMethod
{
public:
	RPCRunScript()
	{
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>{RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcBoolean});
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcInteger, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcBoolean});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCSetLinkInfo : public RPCMethod
{
public:
	RPCSetLinkInfo()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCSetMetadata : public RPCMethod
{
public:
	RPCSetMetadata()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcVariant});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
private:
	RPCEncoder _rpcEncoder;
};

class RPCSetInstallMode : public RPCMethod
{
public:
	RPCSetInstallMode()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcBoolean});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCSetTeam : public RPCMethod
{
public:
	RPCSetTeam()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString});
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCSetValue : public RPCMethod
{
public:
	RPCSetValue()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString, RPCVariableType::rpcVariant});
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString, RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

class RPCTriggerEvent : public RPCMethod
{
public:
	RPCTriggerEvent()
	{
		addSignature(RPCVariableType::rpcVoid, std::vector<RPCVariableType>{RPCVariableType::rpcString});
	}
	std::shared_ptr<RPCVariable> invoke(std::shared_ptr<std::vector<std::shared_ptr<RPCVariable>>> parameters);
};

} /* namespace RPC */
#endif /* RPCMETHODS_H_ */
