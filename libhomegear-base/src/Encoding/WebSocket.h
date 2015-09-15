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

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include "../Exception.h"

#include <memory>
#include <vector>

namespace BaseLib
{
class WebSocketException : public BaseLib::Exception
{
public:
	WebSocketException(std::string message) : BaseLib::Exception(message) {}
};

class WebSocket
{
public:
	struct Header
	{
		struct Opcode
		{
			enum Enum { continuation, text, binary, reserved1, reserved2, reserved3, reserved4, reserved5, close, ping, pong, reserved6, reserved7, reserved8, reserved9, reserved10 };
		};
		bool close = false;
		bool parsed = false;
		uint64_t length = 0;
		bool fin = false;
		bool rsv1 = false;
		bool rsv2 = false;
		bool rsv3 = false;
		Opcode::Enum opcode = Opcode::close;
		bool hasMask = false;
		std::vector<char> maskingKey;
	};

	WebSocket();
	virtual ~WebSocket() {}

	bool isFinished() { return _finished; }

	/**
	 * This method sets _finished and terminates _content with a null character. Use it, when the header does not contain "Content-Length".
	 *
	 * @see isFinished()
	 * @see _finished
	 */
	void setFinished();

	std::shared_ptr<std::vector<char>> getContent() { return _content; }
	uint32_t getContentSize() { return _content->size(); }
	Header* getHeader() { return &_header; }
	void reset();

	/**
	 * Parses WebSocket data from a buffer.
	 *
	 * @param buffer The buffer to parse
	 * @param bufferLength The size of the buffer
	 */
	void process(char* buffer, int32_t bufferLength);

	bool dataProcessingStarted() { return _dataProcessingStarted; }

	/**
	 * Encodes a WebSocket packet.
	 *
	 * @param[in] data The data to encode
	 * @param[in] messageType The message type of the packet.
	 * @param[out] output The WebSocket packet
	 */
	static void encode(const std::vector<char>& data, Header::Opcode::Enum messageType, std::vector<char>& output);

	/**
	 * Encodes a WebSocket "close" packet.
	 *
	 * @param[out] output The WebSocket "close" packet.
	 */
	static void encodeClose(std::vector<char>& output);
private:
	Header _header;
	std::shared_ptr<std::vector<char>> _content;
	uint32_t _oldContentSize = 0;
	bool _finished = false;
	bool _dataProcessingStarted = false;

	void processHeader(char** buffer, int32_t& bufferLength);
	void processContent(char* buffer, int32_t bufferLength);
	void applyMask();

};
}
#endif
