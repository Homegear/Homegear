/* Copyright 2013-2015 Sathya Laufer
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

#include "RemoteRpcServer.h"
#include "../GD/GD.h"

namespace RPC
{

RemoteRpcServer::RemoteRpcServer(std::shared_ptr<RpcClient> client)
{
	_client = client;

	socket = std::shared_ptr<BaseLib::SocketOperations>(new BaseLib::SocketOperations(GD::bl.get()));
	knownDevices.reset(new std::set<uint64_t>());
	fileDescriptor = std::shared_ptr<BaseLib::FileDescriptor>(new BaseLib::FileDescriptor);
	path = "/RPC2";

	_stopMethodProcessingThread = false;
	_methodProcessingMessageAvailable = false;
	_methodBufferHead = 0;
	_methodBufferTail = 0;
	_methodProcessingThread = std::thread(&RemoteRpcServer::processMethods, this);
	BaseLib::Threads::setThreadPriority(GD::bl.get(), _methodProcessingThread.native_handle(), GD::bl->settings.rpcClientThreadPriority(), GD::bl->settings.rpcClientThreadPolicy());
}

RemoteRpcServer::~RemoteRpcServer()
{
	removed = true;
	if(_stopMethodProcessingThread) return;
	_stopMethodProcessingThread = true;
	_methodProcessingMessageAvailable = true;
	_methodProcessingConditionVariable.notify_one();
	if(_methodProcessingThread.joinable()) _methodProcessingThread.join();
	_client.reset();
}

void RemoteRpcServer::queueMethod(std::shared_ptr<std::pair<std::string, std::shared_ptr<std::list<BaseLib::PVariable>>>> method)
{
	try
	{
		if(removed) return;
		_methodBufferMutex.lock();
		int32_t tempHead = _methodBufferHead + 1;
		if(tempHead >= _methodBufferSize) tempHead = 0;
		if(tempHead == _methodBufferTail)
		{
			GD::out.printError("Error: More than " + std::to_string(_methodBufferSize) + " methods are queued to be sent to server " + address.first + ". Your packet processing is too slow. Dropping method.");
			_methodBufferMutex.unlock();
			return;
		}

		_methodBuffer[_methodBufferHead] = method;
		_methodBufferHead++;
		if(_methodBufferHead >= _methodBufferSize)
		{
			_methodBufferHead = 0;
		}
		_methodProcessingMessageAvailable = true;
		_methodBufferMutex.unlock();

		_methodProcessingConditionVariable.notify_one();
	}
	catch(const std::exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_methodBufferMutex.unlock();
	}
	catch(BaseLib::Exception& ex)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		_methodBufferMutex.unlock();
	}
	catch(...)
	{
		GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		_methodBufferMutex.unlock();
	}
}

void RemoteRpcServer::processMethods()
{
	while(!_stopMethodProcessingThread)
	{
		std::unique_lock<std::mutex> lock(_methodProcessingThreadMutex);
		try
		{
			_methodBufferMutex.lock();
			if(_methodBufferHead == _methodBufferTail) //Only lock, when there is really no packet to process. This check is necessary, because the check of the while loop condition is outside of the mutex
			{
				_methodBufferMutex.unlock();
				_methodProcessingConditionVariable.wait(lock, [&]{ return _methodProcessingMessageAvailable; });
			}
			else _methodBufferMutex.unlock();
			if(_stopMethodProcessingThread)
			{
				lock.unlock();
				return;
			}

			while(_methodBufferHead != _methodBufferTail)
			{
				_methodBufferMutex.lock();
				std::shared_ptr<std::pair<std::string, std::shared_ptr<std::list<BaseLib::PVariable>>>> message = _methodBuffer[_methodBufferTail];
				_methodBuffer[_methodBufferTail].reset();
				_methodBufferTail++;
				if(_methodBufferTail >= _methodBufferSize) _methodBufferTail = 0;
				if(_methodBufferHead == _methodBufferTail) _methodProcessingMessageAvailable = false; //Set here, because otherwise it might be set to "true" in publish and then set to false again after the while loop
				_methodBufferMutex.unlock();
				if(!removed) _client->invokeBroadcast(this, message->first, message->second);
			}
		}
		catch(const std::exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(const BaseLib::Exception& ex)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
		}
		catch(...)
		{
			GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		lock.unlock();
	}
}

}
