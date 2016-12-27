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

#ifndef FLOWSCLIENTDATA_H_
#define FLOWSCLIENTDATA_H_

#include <homegear-base/BaseLib.h>

namespace Flows
{

class FlowsClientData
{
public:
	FlowsClientData();
	FlowsClientData(std::shared_ptr<BaseLib::FileDescriptor> clientFileDescriptor);
	virtual ~FlowsClientData();

	int32_t id = 0;
	pid_t pid = 0;
	bool closed = false;
	std::vector<char> buffer;
	std::unique_ptr<BaseLib::Rpc::BinaryRpc> binaryRpc;
	std::shared_ptr<BaseLib::FileDescriptor> fileDescriptor;
	std::mutex sendMutex;
	std::mutex waitMutex;
	std::mutex rpcResponsesMutex;
	std::map<int32_t, BaseLib::PPVariable> rpcResponses;
	std::condition_variable requestConditionVariable;
};

typedef std::shared_ptr<FlowsClientData> PFlowsClientData;

}
#endif
