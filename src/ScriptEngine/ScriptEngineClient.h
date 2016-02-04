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

#ifndef SCRIPTENGINECLIENT_H_
#define SCRIPTENGINECLIENT_H_

#include "homegear-base/BaseLib.h"
#include "../RPC/RPCMethod.h"

#include <thread>
#include <mutex>
#include <string>

class ScriptEngineClient : public BaseLib::IQueue {
public:
	ScriptEngineClient();
	virtual ~ScriptEngineClient();

	void start();
private:
	class QueueEntry : public BaseLib::IQueueEntry
	{
	public:
		QueueEntry() {}
		QueueEntry(std::vector<uint8_t> packet, bool isRequest) { this->packet = packet; this->isRequest = isRequest; }
		virtual ~QueueEntry() {}

		std::vector<uint8_t> packet;
		bool isRequest = false;
	};

	BaseLib::Output _out;
	std::string _socketPath;
	std::shared_ptr<BaseLib::FileDescriptor> _fileDescriptor;
	bool _closed = false;
	std::mutex _requestMutex;
	BaseLib::PVariable _rpcResponse;
	std::condition_variable _requestConditionVariable;
	std::shared_ptr<BaseLib::RpcClientInfo> _dummyClientInfo;
	std::map<std::string, std::shared_ptr<RPC::RPCMethod>> _rpcMethods;
	std::thread _registerClientThread;

	std::unique_ptr<BaseLib::RPC::RPCDecoder> _rpcDecoder;
	std::unique_ptr<BaseLib::RPC::RPCEncoder> _rpcEncoder;

	void cleanUp();
	void registerClient();
	BaseLib::PVariable sendGlobalRequest(std::string methodName, std::shared_ptr<std::list<BaseLib::PVariable>>& parameters);
	void sendResponse(BaseLib::PVariable& variable);

	void processQueueEntry(int32_t index, std::shared_ptr<BaseLib::IQueueEntry>& entry);
};
#endif
