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

#ifndef RPCMETHODS_H_
#define RPCMETHODS_H_

#include "RPCServer.h"
#include "../../Modules/Base/BaseLib.h"
#include "RPCMethod.h"
#include "../Events/EventHandler.h"

#include <vector>
#include <memory>
#include <cstdlib>

namespace RPC
{
class RPCSystemGetCapabilities : public RPCMethod
{
public:
	RPCSystemGetCapabilities()
	{
		setHelp("Lists server's XML RPC capabilities.");
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSystemListMethods : public RPCMethod
{
public:
	RPCSystemListMethods(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Lists all XML RPC methods.");
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
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
		addSignature(BaseLib::RPC::VariableType::rpcString, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
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
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
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
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcArray});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCAbortEventReset : public RPCMethod
{
public:
	RPCAbortEventReset()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCAddDevice : public RPCMethod
{
public:
	RPCAddDevice()
	{
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCAddEvent : public RPCMethod
{
public:
	RPCAddEvent()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcStruct});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCAddLink : public RPCMethod
{
public:
	RPCAddLink()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCClientServerInitialized : public RPCMethod
{
public:
	RPCClientServerInitialized()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>({BaseLib::RPC::VariableType::rpcString}));
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCDeleteDevice : public RPCMethod
{
public:
	RPCDeleteDevice()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCDeleteMetadata : public RPCMethod
{
public:
	RPCDeleteMetadata()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCDeleteSystemVariable : public RPCMethod
{
public:
	RPCDeleteSystemVariable()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCEnableEvent : public RPCMethod
{
public:
	RPCEnableEvent()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetAllMetadata : public RPCMethod
{
public:
	RPCGetAllMetadata()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetAllValues : public RPCMethod
{
public:
	RPCGetAllValues()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcBoolean});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetAllSystemVariables : public RPCMethod
{
public:
	RPCGetAllSystemVariables()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetDeviceDescription : public RPCMethod
{
public:
	RPCGetDeviceDescription()
	{
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetDeviceInfo : public RPCMethod
{
public:
	RPCGetDeviceInfo()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcArray});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcArray});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetEvent : public RPCMethod
{
public:
	RPCGetEvent()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetInstallMode : public RPCMethod
{
public:
	RPCGetInstallMode()
	{
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetKeyMismatchDevice : public RPCMethod
{
public:
	RPCGetKeyMismatchDevice()
	{
		addSignature(BaseLib::RPC::VariableType::rpcString, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcBoolean});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetLinkInfo : public RPCMethod
{
public:
	RPCGetLinkInfo()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetLinkPeers : public RPCMethod
{
public:
	RPCGetLinkPeers()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetLinks : public RPCMethod
{
public:
	RPCGetLinks()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetMetadata : public RPCMethod
{
public:
	RPCGetMetadata()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetParamsetDescription : public RPCMethod
{
public:
	RPCGetParamsetDescription()
	{
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetParamsetId : public RPCMethod
{
public:
	RPCGetParamsetId()
	{
		addSignature(BaseLib::RPC::VariableType::rpcString, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcString, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcString, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetParamset : public RPCMethod
{
public:
	RPCGetParamset()
	{
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetPeerId : public RPCMethod
{
public:
	RPCGetPeerId()
	{
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetServiceMessages : public RPCMethod
{
public:
	RPCGetServiceMessages()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcBoolean});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetSystemVariable : public RPCMethod
{
public:
	RPCGetSystemVariable()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetUpdateStatus : public RPCMethod
{
public:
	RPCGetUpdateStatus()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetValue : public RPCMethod
{
public:
	RPCGetValue()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVariant, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCGetVersion : public RPCMethod
{
public:
	RPCGetVersion()
	{
		addSignature(BaseLib::RPC::VariableType::rpcString, std::vector<BaseLib::RPC::VariableType>{});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCInit : public RPCMethod
{
public:
	RPCInit()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcInteger});
	}
	virtual ~RPCInit();

	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
private:
	bool _disposing = false;
	std::mutex _initServerThreadMutex;
	std::thread _initServerThread;
};

class RPCListBidcosInterfaces : public RPCMethod
{
public:
	RPCListBidcosInterfaces()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCListClientServers : public RPCMethod
{
public:
	RPCListClientServers()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>({BaseLib::RPC::VariableType::rpcString}));
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCListDevices : public RPCMethod
{
public:
	RPCListDevices()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>({BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcArray}));
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCListEvents : public RPCMethod
{
public:
	RPCListEvents()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>({BaseLib::RPC::VariableType::rpcInteger}));
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>({BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger}));
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>({BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString}));
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCListFamilies : public RPCMethod
{
public:
	RPCListFamilies()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCListInterfaces : public RPCMethod
{
public:
	RPCListInterfaces()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCListTeams : public RPCMethod
{
public:
	RPCListTeams()
	{
		addSignature(BaseLib::RPC::VariableType::rpcArray, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCLogLevel : public RPCMethod
{
public:
	RPCLogLevel()
	{
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCPutParamset : public RPCMethod
{
public:
	RPCPutParamset()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcStruct});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcStruct});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcStruct});

	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCRemoveEvent : public RPCMethod
{
public:
	RPCRemoveEvent()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCRemoveLink : public RPCMethod
{
public:
	RPCRemoveLink()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCRssiInfo : public RPCMethod
{
public:
	RPCRssiInfo()
	{
		addSignature(BaseLib::RPC::VariableType::rpcStruct, std::vector<BaseLib::RPC::VariableType>());
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCRunScript : public RPCMethod
{
public:
	RPCRunScript()
	{
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean});
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcBoolean});
	}
	virtual ~RPCRunScript();
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
protected:
	bool _disposing = false;
	std::mutex _runScriptThreadMutex;
	std::thread _runScriptThread;
};

class RPCSearchDevices : public RPCMethod
{
public:
	RPCSearchDevices()
	{
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>());
		addSignature(BaseLib::RPC::VariableType::rpcInteger, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetLinkInfo : public RPCMethod
{
public:
	RPCSetLinkInfo()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetMetadata : public RPCMethod
{
public:
	RPCSetMetadata()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetId : public RPCMethod
{
public:
	RPCSetId()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetInstallMode : public RPCMethod
{
public:
	RPCSetInstallMode()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcBoolean});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetInterface : public RPCMethod
{
public:
	RPCSetInterface()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetName : public RPCMethod
{
public:
	RPCSetName()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetSystemVariable : public RPCMethod
{
public:
	RPCSetSystemVariable()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetTeam : public RPCMethod
{
public:
	RPCSetTeam()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCSetValue : public RPCMethod
{
public:
	RPCSetValue()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcString});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString, BaseLib::RPC::VariableType::rpcVariant});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCTriggerEvent : public RPCMethod
{
public:
	RPCTriggerEvent()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcString});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

class RPCUpdateFirmware : public RPCMethod
{
public:
	RPCUpdateFirmware()
	{
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcInteger, BaseLib::RPC::VariableType::rpcBoolean});
		addSignature(BaseLib::RPC::VariableType::rpcVoid, std::vector<BaseLib::RPC::VariableType>{BaseLib::RPC::VariableType::rpcArray});
	}
	std::shared_ptr<BaseLib::RPC::Variable> invoke(std::shared_ptr<std::vector<std::shared_ptr<BaseLib::RPC::Variable>>> parameters);
};

} /* namespace RPC */
#endif /* RPCMETHODS_H_ */
