/* Copyright 2013-2016 Sathya Laufer
 *
 * Homegear is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 * 
 * Homegear is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with Homegear.  If not, see
 * <http://www.gnu.org/licenses/>.
 * 
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU Lesser General Public License in all respects
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
#include <homegear-base/BaseLib.h>
#include "RPCMethod.h"
#include "../Events/EventHandler.h"

#include <vector>
#include <memory>
#include <cstdlib>

namespace RPC
{
class RPCDevTest : public RPCMethod
{
public:
	RPCDevTest()
	{
		setHelp("Test method for development.");
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSystemGetCapabilities : public RPCMethod
{
public:
	RPCSystemGetCapabilities()
	{
		setHelp("Lists server's RPC capabilities.");
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSystemListMethods : public RPCMethod
{
public:
	RPCSystemListMethods(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Lists all RPC methods.");
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCSystemMethodHelp : public RPCMethod
{
public:
	RPCSystemMethodHelp(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Returns a description of a RPC method.");
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
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
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCSystemMulticall : public RPCMethod
{
public:
	RPCSystemMulticall(std::shared_ptr<RPCServer> server)
	{
		_server = server;
		setHelp("Calls multiple RPC methods at once to reduce traffic.");
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
private:
	std::shared_ptr<RPCServer> _server;
};

class RPCAbortEventReset : public RPCMethod
{
public:
	RPCAbortEventReset()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCActivateLinkParamset : public RPCMethod
{
public:
	RPCActivateLinkParamset()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});

	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCAddDevice : public RPCMethod
{
public:
	RPCAddDevice()
	{
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCAddEvent : public RPCMethod
{
public:
	RPCAddEvent()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tStruct});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCAddLink : public RPCMethod
{
public:
	RPCAddLink()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCCopyConfig : public RPCMethod
{
public:
	RPCCopyConfig()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCClientServerInitialized : public RPCMethod
{
public:
	RPCClientServerInitialized()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCCreateDevice : public RPCMethod
{
public:
	RPCCreateDevice()
	{
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCDeleteDevice : public RPCMethod
{
public:
	RPCDeleteDevice()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCDeleteMetadata : public RPCMethod
{
public:
	RPCDeleteMetadata()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCDeleteSystemVariable : public RPCMethod
{
public:
	RPCDeleteSystemVariable()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCEnableEvent : public RPCMethod
{
public:
	RPCEnableEvent()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetAllMetadata : public RPCMethod
{
public:
	RPCGetAllMetadata()
	{
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetAllScripts : public RPCMethod
{
public:
	RPCGetAllScripts()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetAllConfig : public RPCMethod
{
public:
	RPCGetAllConfig()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetAllValues : public RPCMethod
{
public:
	RPCGetAllValues()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetAllSystemVariables : public RPCMethod
{
public:
	RPCGetAllSystemVariables()
	{
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetConfigParameter : public RPCMethod
{
public:
	RPCGetConfigParameter()
	{
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tString, BaseLib::VariableType::tString });
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString });
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetDeviceDescription : public RPCMethod
{
public:
	RPCGetDeviceDescription()
	{
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetDeviceInfo : public RPCMethod
{
public:
	RPCGetDeviceInfo()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tArray});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetEvent : public RPCMethod
{
public:
	RPCGetEvent()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetInstallMode : public RPCMethod
{
public:
	RPCGetInstallMode()
	{
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetKeyMismatchDevice : public RPCMethod
{
public:
	RPCGetKeyMismatchDevice()
	{
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetLinkInfo : public RPCMethod
{
public:
	RPCGetLinkInfo()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetLinkPeers : public RPCMethod
{
public:
	RPCGetLinkPeers()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetLinks : public RPCMethod
{
public:
	RPCGetLinks()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetMetadata : public RPCMethod
{
public:
	RPCGetMetadata()
	{
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetName : public RPCMethod
{
public:
	RPCGetName()
	{
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetPairingMethods : public RPCMethod
{
public:
	RPCGetPairingMethods()
	{
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetParamsetDescription : public RPCMethod
{
public:
	RPCGetParamsetDescription()
	{
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetParamsetId : public RPCMethod
{
public:
	RPCGetParamsetId()
	{
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetParamset : public RPCMethod
{
public:
	RPCGetParamset()
	{
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetPeerId : public RPCMethod
{
public:
	RPCGetPeerId()
	{
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetServiceMessages : public RPCMethod
{
public:
	RPCGetServiceMessages()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetSystemVariable : public RPCMethod
{
public:
	RPCGetSystemVariable()
	{
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetUpdateStatus : public RPCMethod
{
public:
	RPCGetUpdateStatus()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetValue : public RPCMethod
{
public:
	RPCGetValue()
	{
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tString, BaseLib::VariableType::tString });
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean });
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean });
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString });
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean });
		addSignature(BaseLib::VariableType::tVariant, std::vector<BaseLib::VariableType>{ BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tBoolean });
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCGetVersion : public RPCMethod
{
public:
	RPCGetVersion()
	{
		addSignature(BaseLib::VariableType::tString, std::vector<BaseLib::VariableType>{});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCInit : public RPCMethod
{
public:
	RPCInit()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
	}
	virtual ~RPCInit();

	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
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
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListClientServers : public RPCMethod
{
public:
	RPCListClientServers()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString}));
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListDevices : public RPCMethod
{
public:
	RPCListDevices()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListEvents : public RPCMethod
{
public:
	RPCListEvents()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger}));
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger}));
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString}));
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListFamilies : public RPCMethod
{
public:
	RPCListFamilies()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListInterfaces : public RPCMethod
{
public:
	RPCListInterfaces()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListKnownDeviceTypes : public RPCMethod
{
public:
	RPCListKnownDeviceTypes()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tBoolean, BaseLib::VariableType::tArray}));
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCListTeams : public RPCMethod
{
public:
	RPCListTeams()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCLogLevel : public RPCMethod
{
public:
	RPCLogLevel()
	{
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCPutParamset : public RPCMethod
{
public:
	RPCPutParamset()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tStruct});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tStruct});

	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCRemoveEvent : public RPCMethod
{
public:
	RPCRemoveEvent()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCRemoveLink : public RPCMethod
{
public:
	RPCRemoveLink()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCReportValueUsage : public RPCMethod
{
public:
	RPCReportValueUsage()
	{
		addSignature(BaseLib::VariableType::tArray, std::vector<BaseLib::VariableType>({BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger}));
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCRssiInfo : public RPCMethod
{
public:
	RPCRssiInfo()
	{
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>());
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCRunScript : public RPCMethod
{
public:
	RPCRunScript()
	{
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tStruct, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSearchDevices : public RPCMethod
{
public:
	RPCSearchDevices()
	{
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>());
		addSignature(BaseLib::VariableType::tInteger, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetLinkInfo : public RPCMethod
{
public:
	RPCSetLinkInfo()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetMetadata : public RPCMethod
{
public:
	RPCSetMetadata()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetId : public RPCMethod
{
public:
	RPCSetId()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetInstallMode : public RPCMethod
{
public:
	RPCSetInstallMode()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetInterface : public RPCMethod
{
public:
	RPCSetInterface()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetName : public RPCMethod
{
public:
	RPCSetName()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetSystemVariable : public RPCMethod
{
public:
	RPCSetSystemVariable()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetTeam : public RPCMethod
{
public:
	RPCSetTeam()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSetValue : public RPCMethod
{
public:
	RPCSetValue()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString, BaseLib::VariableType::tVariant});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tInteger, BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCSubscribePeers : public RPCMethod
{
public:
	RPCSubscribePeers()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCTriggerEvent : public RPCMethod
{
public:
	RPCTriggerEvent()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCTriggerRpcEvent : public RPCMethod
{
public:
	RPCTriggerRpcEvent()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCUnsubscribePeers : public RPCMethod
{
public:
	RPCUnsubscribePeers()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tArray});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCUpdateFirmware : public RPCMethod
{
public:
	RPCUpdateFirmware()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tInteger, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tString, BaseLib::VariableType::tBoolean});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tArray});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

class RPCWriteLog : public RPCMethod
{
public:
	RPCWriteLog()
	{
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tVoid, BaseLib::VariableType::tString});
		addSignature(BaseLib::VariableType::tVoid, std::vector<BaseLib::VariableType>{BaseLib::VariableType::tVoid, BaseLib::VariableType::tString, BaseLib::VariableType::tInteger});
	}
	BaseLib::PVariable invoke(BaseLib::PRpcClientInfo clientInfo, std::shared_ptr<std::vector<BaseLib::PVariable>> parameters);
};

}
#endif
