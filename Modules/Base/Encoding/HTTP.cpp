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

#include "HTTP.h"
#include "../HelperFunctions/HelperFunctions.h"

namespace BaseLib
{
HTTP::HTTP()
{
	_content.reset(new std::vector<char>());
	_chunk.reset(new std::vector<char>());
}

void HTTP::process(char* buffer, int32_t bufferLength)
{
	if(bufferLength <= 0 || _finished) return;
	if(!_header.parsed) processHeader(&buffer, bufferLength);
	_dataProcessed = true;
	if(_header.transferEncoding & TransferEncoding::Enum::chunked) processChunkedContent(buffer, bufferLength); else processContent(buffer, bufferLength);
}

void HTTP::processHeader(char** buffer, int32_t& bufferLength)
{
	char* end = strstr(*buffer, "\r\n\r\n");
	uint32_t headerSize;
	int32_t crlfOffset = 2;
	if(!end || ((end + 3) - *buffer) + 1 > bufferLength)
	{
		end = strstr(*buffer, "\n\n");
		if(!end || ((end + 1) - *buffer) + 1 > bufferLength) throw HTTPException("Could not find HTTP header.");
		crlfOffset = 1;
		_crlf = false;
		headerSize = ((end + 1) - *buffer) + 1;
	}
	else headerSize = ((end + 3) - *buffer) + 1;

	bufferLength -= headerSize;

	if(strncmp(*buffer, "GET", 3) || strncmp(*buffer, "POST", 4)) _type = Type::Enum::request;
	else if(strncmp(*buffer, "HTTP/1.", 7))
	{
		_type = Type::Enum::response;
		_header.responseCode = strtol(*buffer + 9, NULL, 10);
		if(_header.responseCode != 200) throw HTTPException("Response code was: " + std::to_string(_header.responseCode));
	}
	else throw HTTPException("Unknown HTTP request method.");

	char* newlinePos;
	char* colonPos;
	newlinePos = strchr(*buffer, '\n');
	if(!newlinePos || newlinePos > end) throw HTTPException("Could not parse HTTP header.");
	*buffer = newlinePos + 1;

	while(*buffer < end)
	{
		newlinePos = (crlfOffset == 2) ? strchr(*buffer, '\r') : strchr(*buffer, '\n');
		if(!newlinePos || newlinePos > end) break;
		colonPos = strchr(*buffer, ':');
		if(!colonPos || colonPos > newlinePos) continue;

		if(colonPos < newlinePos - 2) processHeaderField(*buffer, (uint32_t)(colonPos - *buffer), colonPos + 2, (uint32_t)(newlinePos - colonPos - 2));
		*buffer = newlinePos + crlfOffset;
	}
	*buffer += crlfOffset;
	_header.parsed = true;
}

void HTTP::processHeaderField(char* name, uint32_t nameSize, char* value, uint32_t valueSize)
{
	if(nameSize == 0 || valueSize == 0 || !name || !value) return;
	if(!strnaicmp(name, "content-length", nameSize)) _header.contentLength = strtol(value, NULL, 10);
	else if(!strnaicmp(name, "host", nameSize))
	{
		_header.host = std::string(value, valueSize);
		HelperFunctions::toLower(_header.host);
	}
	else if(!strnaicmp(name, "content-type", nameSize))
	{
		_header.contentType = std::string(value, valueSize);
		HelperFunctions::toLower(_header.contentType);
	}
	else if(!strnaicmp(name, "transfer-encoding", nameSize) || !strnaicmp(name, "te", nameSize))
	{
		std::string s(value, valueSize);
		s = s.substr(0, s.find(';'));
		int32_t pos = 0;
		while ((pos = s.find(',')) != (signed)std::string::npos || !s.empty())
		{
		    std::string te = (pos == (signed)std::string::npos) ? s : s.substr(0, pos);
		    HelperFunctions::trim(BaseLib::HelperFunctions::toLower(te));
		    if(te == "chunked") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::chunked);
			else if(te == "compress") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::compress);
			else if(te == "deflate") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::deflate);
			else if(te == "gzip") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::gzip);
			else if(te == "identity") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::identity);
			else throw HTTPException("Unknown value for HTTP header \"Transfer-Encoding\": " + std::string(value, valueSize));
		    if(pos == (signed)std::string::npos) s.clear(); else s.erase(0, pos + 1);
		}
	}
	else if(!strnaicmp(name, "connection", nameSize))
	{
		std::string s(value, valueSize);
		s = s.substr(0, s.find(';'));
		int32_t pos = 0;
		while ((pos = s.find(',')) != (signed)std::string::npos || !s.empty())
		{
			std::string c = (pos == (signed)std::string::npos) ? s : s.substr(0, pos);
			HelperFunctions::trim(BaseLib::HelperFunctions::toLower(c));
			if(c == "keep-alive") _header.connection = Connection::Enum::keepAlive;
			else if(c == "close") _header.connection = Connection::Enum::close;
			else if(c == "te") {} //ignore
			else throw HTTPException("Unknown value for HTTP header \"Connection\": " + std::string(value, valueSize));
			if(pos == (signed)std::string::npos) s.clear(); else s.erase(0, pos + 1);
		}
	}
	else if(!strnaicmp(name, "authorization", nameSize)) _header.authorization = std::string(value, valueSize);
}

int32_t HTTP::strnaicmp(char const *a, char const *b, uint32_t size)
{
	if(size == 0) return 0;
	for (uint32_t pos = 0;; a++, b++, pos++)
	{
        int32_t d = tolower(*a) - *b;
        if (d != 0 || pos == size - 1) return d;
    }
	return 0;
}

void HTTP::reset()
{
	_header = Header();
	_content.reset(new std::vector<char>());
	_chunk.reset(new std::vector<char>());
	_type = Type::Enum::none;
	_finished = false;
	_dataProcessed = false;
}

void HTTP::setFinished()
{
	_finished = true;
	_content->push_back('\0');
}

void HTTP::processContent(char* buffer, int32_t bufferLength)
{
	if(_content->size() + bufferLength > 10485760) throw HTTPException("Data is larger than 10MiB.");
	if(_header.contentLength == 0) _content->insert(_content->end(), buffer, buffer + bufferLength);
	else
	{
		if(_content->size() + bufferLength > _header.contentLength) bufferLength -= (_content->size() + bufferLength) - _header.contentLength;
		_content->insert(_content->end(), buffer, buffer + bufferLength);
		if(_content->size() == _header.contentLength)
		{
			_finished = true;
			_content->push_back('\0');
		}
	}
}

void HTTP::processChunkedContent(char* buffer, int32_t bufferLength)
{
	while(true)
	{
		if(_content->size() + bufferLength > 10485760) throw HTTPException("Data is larger than 10MiB.");
		if(_chunkSize == -1)
		{
			readChunkSize(&buffer, bufferLength);
			if(_chunkSize == -1) break;
		}
		else
		{
			if(_chunkSize == 0)
			{
				_finished = true;
				_content->push_back('\0');
				break;
			}
			if(bufferLength <= 0) break;
			int32_t sizeToInsert = bufferLength;
			if((signed)_chunk->size() + sizeToInsert > _chunkSize) sizeToInsert -= (_chunk->size() + sizeToInsert) - _chunkSize;
			_chunk->insert(_chunk->end(), buffer, buffer + sizeToInsert);
			if((signed)_chunk->size() == _chunkSize)
			{
				_content->insert(_content->end(), _chunk->begin(), _chunk->end());
				_chunk.reset(new std::vector<char>());
				_chunkSize = -1;
			}
			bufferLength -= _crlf ? sizeToInsert + 2 : sizeToInsert + 1;
			if(bufferLength < 0)
			{
				sizeToInsert += bufferLength;
				bufferLength = 0;
			}
			buffer = _crlf ? buffer + sizeToInsert + 2 : buffer + sizeToInsert + 1;
		}
	}
}

void HTTP::readChunkSize(char** buffer, int32_t& bufferLength)
{
	char* newlinePos;
	if(_chunkSize == -1 && _endChunkSizeBytes == 0)
	{
		newlinePos = strchr(*buffer, '\n');
		if(_partialChunkSize.empty() && newlinePos == *buffer) newlinePos = strchr(*buffer + 1, '\n'); //\n is first character
		if(_partialChunkSize.empty() && newlinePos == *buffer + 1 && **buffer == '\r') newlinePos = strchr(*buffer + 2, '\n'); //\r is first character
		if(!newlinePos || newlinePos >= *buffer + bufferLength) throw BaseLib::Exception("Could not parse chunk size.");
		std::string chunkSize = _partialChunkSize + std::string(*buffer, newlinePos - *buffer);
		HelperFunctions::trim(_partialChunkSize);
		if(!HelperFunctions::isNumber(chunkSize)) throw BaseLib::Exception("Chunk size is no number.");
		_chunkSize = HelperFunctions::getNumber(chunkSize, true);
		_partialChunkSize = "";
		bufferLength -= (newlinePos + 1) - *buffer;
		*buffer = newlinePos + 1;
	}
	_endChunkSizeBytes = -1;
	if(_chunkSize > -1) return;

	newlinePos = strchr(*buffer, '\n');
	if(!newlinePos || newlinePos >= *buffer + bufferLength)
	{
		_endChunkSizeBytes = 0;
		char* semicolonPos;
		semicolonPos = strchr(*buffer, ';');
		if(!semicolonPos || semicolonPos >= *buffer + bufferLength)
		{
			_partialChunkSize = std::string(*buffer, bufferLength);
			if(_partialChunkSize.size() > 8) throw HTTPException("Could not parse chunk size.");
		}
		else
		{
			_chunkSize = strtol(*buffer, NULL, 16);
			if(_chunkSize < 0) throw HTTPException("Could not parse chunk size. Chunk size is negative.");
		}
	}
	else
	{
		_chunkSize = strtol(*buffer, NULL, 16);
		if(_chunkSize < 0) throw HTTPException("Could not parse chunk size. Chunk size is negative.");
		bufferLength -= (newlinePos + 1) - *buffer;
		if(bufferLength == -1)
		{
			bufferLength = 0;
			_endChunkSizeBytes = 1;
		}
		*buffer = newlinePos + 1;
	}
}
}
