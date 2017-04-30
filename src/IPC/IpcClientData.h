/* Copyright 2013-2017 Sathya Laufer
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

#ifndef IPCCLIENTDATA_H_
#define IPCCLIENTDATA_H_

#include "IpcResponse.h"

#include <homegear-base/BaseLib.h>

namespace Ipc
{

class IpcClientData
{
private:
	std::string _secret;

	void init();
public:
	int32_t id = 0;
	bool closed = false;
	std::vector<char> buffer;
	std::unique_ptr<BaseLib::Rpc::BinaryRpc> binaryRpc;
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	std::mutex sendMutex;
	std::mutex waitMutex;
	std::mutex rpcResponsesMutex;
	std::map<int32_t, PIpcResponse> rpcResponses;
	std::condition_variable requestConditionVariable;

	IpcClientData();
	IpcClientData(std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor);
	virtual ~IpcClientData();
	std::string getSecret() { return _secret; };
};

typedef std::shared_ptr<IpcClientData> PIpcClientData;

}
#endif
