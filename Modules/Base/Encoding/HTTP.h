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

#ifndef HTTP_H_
#define HTTP_H_

#include "../Exception.h"

#include <iostream>
#include <string>
#include <map>
#include <cstring>
#include <memory>
#include <vector>

namespace BaseLib
{
class HTTPException : public BaseLib::Exception
{
public:
	HTTPException(std::string message) : BaseLib::Exception(message) {}
};

class HTTP
{
public:
	struct Type
	{
		enum Enum { none, request, response };
	};
	struct TransferEncoding
	{
		enum Enum { none = 0, chunked = 1, compress = 2, deflate = 4, gzip = 8, identity = 16 };
	};
	struct Connection
	{
		enum Enum { none, keepAlive, close };
	};
	struct Header
	{
		bool parsed = false;
		int32_t responseCode = -1;
		uint32_t contentLength = 0;
		std::string host;
		std::string contentType;
		TransferEncoding::Enum transferEncoding = TransferEncoding::Enum::none;
		Connection::Enum connection = Connection::Enum::none;
		std::string authorization;
	};

	HTTP();
	virtual ~HTTP() {}

	Type::Enum getType() { return _type; }
	bool headerIsFinished() { return _header.parsed; }
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
	 * Parses HTTP data from a buffer.
	 *
	 * @param buffer The buffer to parse
	 * @param bufferLength The size of the buffer
	 * @param checkForChunkedXML Optional. Only works for XML-like content (content needs to start with '<'). Needed when TransferEncoding is not set to chunked.
	 */
	void process(char* buffer, int32_t bufferLength, bool checkForChunkedXML = false);
	bool dataProcessed() { return _dataProcessed; }
private:
	bool _dataProcessed = false;
	bool _crlf = true;
	Header _header;
	Type::Enum _type = Type::Enum::none;
	std::shared_ptr<std::vector<char>> _content;
	std::shared_ptr<std::vector<char>> _chunk;
	bool _finished = false;
	int32_t _chunkSize = -1;
	int32_t _endChunkSizeBytes = -1;
	std::string _partialChunkSize;

	void processHeader(char** buffer, int32_t& bufferLength);
	void processHeaderField(char* name, uint32_t nameSize, char* value, uint32_t valueSize);
	void processContent(char* buffer, int32_t bufferLength);
	void processChunkedContent(char* buffer, int32_t bufferLength);
	void readChunkSize(char** buffer, int32_t& bufferLength);

	int32_t strnaicmp(char const *a, char const *b, uint32_t size);
};
}
#endif
