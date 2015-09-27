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

#include "HTTP.h"
#include "../HelperFunctions/Math.h"
#include "../HelperFunctions/HelperFunctions.h"

namespace BaseLib
{

std::string HTTP::getMimeType(std::string extension)
{
	if(_extMimeTypeMap.find(extension) != _extMimeTypeMap.end()) return _extMimeTypeMap[extension];
	return "";
}

std::string HTTP::getStatusText(int32_t code)
{
	if(_statusCodeMap.find(code) != _statusCodeMap.end()) return _statusCodeMap[code];
	return "";
}

void HTTP::constructHeader(uint32_t contentLength, std::string contentType, int32_t code, std::string codeDescription, std::vector<std::string>& additionalHeaders, std::string& header)
{
	std::string additionalHeader;
	additionalHeader.reserve(1024);
	for(std::vector<std::string>::iterator i = additionalHeaders.begin(); i != additionalHeaders.end(); ++i)
	{
		BaseLib::HelperFunctions::trim(*i);
		if((*i).find("Location: ") == 0)
		{
			code = 301;
			codeDescription = "Moved Permanently";
		}
		if(!(*i).empty()) additionalHeader.append(*i + "\r\n");
	}

	header.reserve(1024);
	header.append("HTTP/1.1 " + std::to_string(code) + " " + codeDescription + "\r\n");
	header.append("Connection: close\r\n");
	if(!contentType.empty()) header.append("Content-Type: " + contentType + "\r\n");
	header.append(additionalHeader);
	header.append("Content-Length: ").append(std::to_string(contentLength)).append("\r\n\r\n");
}

HTTP::HTTP()
{
	_content.reset(new std::vector<char>());
	_chunk.reset(new std::vector<char>());
	_rawHeader.reset(new std::vector<char>());

	_extMimeTypeMap = {
		{"html", "text/html"},
		{"htm", "text/html"},
		{"js", "text/javascript"},
		{"css", "text/css"},
		{"gif", "image/gif"},
		{"jpg", "image/jpeg"},
		{"jpeg", "image/jpeg"},
		{"jpe", "image/jpeg"},
		{"pdf", "application/pdf"},
		{"png", "image/png"},
		{"svg", "image/svg+xml"},
		{"txt", "text/plain"},
		{"webm", "video/webm"},
		{"ogv", "video/ogg"},
		{"ogg", "video/ogg"},
		{"3gp", "video/3gpp"},
		{"apk", "application/vnd.android.package-archive"},
		{"avi", "video/x-msvideo"},
		{"bmp", "image/x-ms-bmp"},
		{"csv", "text/comma-separated-values"},
		{"doc", "application/msword"},
		{"docx", "application/msword"},
		{"flac", "audio/flac"},
		{"gz", "application/x-gzip"},
		{"gzip", "application/x-gzip"},
		{"ics", "text/calendar"},
		{"kml", "application/vnd.google-earth.kml+xml"},
		{"kmz", "application/vnd.google-earth.kmz"},
		{"m4a", "audio/mp4"},
		{"mp3", "audio/mpeg"},
		{"mp4", "video/mp4"},
		{"mpg", "video/mpeg"},
		{"mpeg", "video/mpeg"},
		{"mov", "video/quicktime"},
		{"odp", "application/vnd.oasis.opendocument.presentation"},
		{"ods", "application/vnd.oasis.opendocument.spreadsheet"},
		{"odt", "application/vnd.oasis.opendocument.text"},
		{"oga", "audio/ogg"},
		{"pptx", "application/vnd.ms-powerpoint"},
		{"pps", "application/vnd.ms-powerpoint"},
		{"qt", "video/quicktime"},
		{"swf", "application/x-shockwave-flash"},
		{"tar", "application/x-tar"},
		{"text", "text/plain"},
		{"tif", "image/tiff"},
		{"tiff", "image/tiff"},
		{"wav", "audio/wav"},
		{"wmv", "video/x-ms-wmv"},
		{"xls", "application/vnd.ms-excel"},
		{"xlsx", "application/vnd.ms-excel"},
		{"zip", "application/zip"},
		{"xml", "application/xml"},
		{"xsl", "application/xml"},
		{"xsd", "application/xml"},

		{"xhtml", "application/xhtml+xml"},
		{"json", "application/json"},
		{"dtd", "application/xml-dtd"},
		{"xslt", "application/xslt+xml"},
		{"java", "text/x-java-source,java"}
	};

	_statusCodeMap = {
		{100, "Continue"},
		{101, "Switching Protocols"},
		{200, "OK"},
		{201, "Created"},
		{202, "Accepted"},
		{203, "Non-Authoritative Information"},
		{204, "No Content"},
		{205, "Reset Content"},
		{206, "Partial Content"},
		{300, "Multiple Choices"},
		{301, "Moved Permanently"},
		{302, "Found"},
		{303, "See Other"},
		{304, "Not Modified"},
		{305, "Use Proxy"},
		{307, "Temporary Redirect"},
		{308, "Permanent Redirect"},
		{400, "Bad Request"},
		{401, "Unauthorized"},
		{402, "Payment Required"},
		{403, "Forbidden"},
		{404, "Not Found"},
		{405, "Method Not Allowed"},
		{406, "Not Acceptable"},
		{407, "Proxy Authentication Required"},
		{408, "Request Timeout"},
		{409, "Conflict"},
		{410, "Gone"},
		{411, "Length Required"},
		{412, "Precondition Failed"},
		{413, "Request Entity Too Large"},
		{414, "Request-URI Too Long"},
		{415, "Unsupported Media Type"},
		{416, "Requested Range Not Satisfiable"},
		{417, "Expectation Failed"},
		{426, "Upgrade Required"},
		{428, "Precondition Required"},
		{429, "Too Many Requests"},
		{431, "Request Header Fields Too Large"},
		{500, "Internal Server Error"},
		{501, "Not Implemented"},
		{502, "Bad Gateway"},
		{503, "Service Unavailable"},
		{504, "Gateway Timeout"},
		{505, "HTTP Version Not Supported"},
		{511, "Network Authentication Required"}
	};
}

HTTP::~HTTP()
{
	_extMimeTypeMap.clear();
	_statusCodeMap.clear();
}

void HTTP::process(char* buffer, int32_t bufferLength, bool checkForChunkedXML)
{
	if(bufferLength <= 0 || _finished) return;
	_headerProcessingStarted = true;
	if(!_header.parsed) processHeader(&buffer, bufferLength);
	if(!_header.parsed) return;
	if(_header.method == "GET" || _header.method == "M-SEARCH" || (_header.method == "NOTIFY" && _header.contentLength == 0) || (_contentLengthSet && _header.contentLength == 0))
	{
		_dataProcessingStarted = true;
		setFinished();
		return;
	}
	if(!_dataProcessingStarted)
	{
		if(checkForChunkedXML)
		{
			if(bufferLength + _partialChunkSize.length() < 8) //Not enough data.
			{
				_partialChunkSize.append(buffer, bufferLength);
				return;
			}
			std::string chunk = _partialChunkSize + std::string(buffer, bufferLength);
			int32_t pos = chunk.find('<');
			if(pos != (signed)std::string::npos && pos != 0)
			{
				if(BaseLib::Math::isNumber(BaseLib::HelperFunctions::trim(chunk), true)) _header.transferEncoding = BaseLib::HTTP::TransferEncoding::chunked;
			}
		}
		if(_header.contentLength > 10485760) throw HTTPException("Data is larger than 10MiB.");
		_content->reserve(_header.contentLength);
	}
	_dataProcessingStarted = true;
	if(_header.transferEncoding & TransferEncoding::Enum::chunked) processChunkedContent(buffer, bufferLength); else processContent(buffer, bufferLength);
}

void HTTP::processHeader(char** buffer, int32_t& bufferLength)
{
	char* end = strstr(*buffer, "\r\n\r\n");
	uint32_t headerSize = 0;
	int32_t crlfOffset = 2;
	if(!end || ((end + 3) - *buffer) + 1 > bufferLength)
	{
		end = strstr(*buffer, "\n\n");
		if(!end || ((end + 1) - *buffer) + 1 > bufferLength)
		{
			if(_rawHeader->size() > 2 && (
					(_rawHeader->back() == '\n' && **buffer == '\n') ||
					(_rawHeader->back() == '\r' && **buffer == '\n' && *(*buffer + 1) == '\r') ||
					(_rawHeader->at(_rawHeader->size() - 2) == '\r' && _rawHeader->back() == '\n' && **buffer == '\r') ||
					(_rawHeader->at(_rawHeader->size() - 2) == '\n' && _rawHeader->back() == '\r' && **buffer == '\n')
			))
			{
				//Special case: The two new lines are split between _rawHeader and buffer
				//Cases:
				//	For crlf:
				//		rawHeader = ...\n, buffer = \n...
				//	For lf:
				//		rawHeader = ...\r, buffer = \n\r\n...
				//		rawHeader = ...\r\n, buffer = \r\n...
				//		rawHeader = ...\r\n\r, buffer = \n...
				if(**buffer == '\n' && *(*buffer + 1) != '\r') //rawHeader = ...\r\n\r, buffer = \n... or rawHeader = ...\n, buffer = \n...
				{
					headerSize = 1;
					if(_rawHeader->back() == '\r') crlfOffset = 2;
					else crlfOffset = 1;
				}
				else if(**buffer == '\n' && *(*buffer + 1) == '\r' && *(*buffer + 2) == '\n') //rawHeader = ...\r, buffer = \n\r\n...
				{
					headerSize = 3;
					crlfOffset = 2;
				}
				else if(**buffer == '\r' && *(*buffer + 1) == '\n') //rawHeader = ...\r\n, buffer = \r\n...
				{
					headerSize = 2;
					crlfOffset = 2;
				}
			}
			else
			{
				_rawHeader->insert(_rawHeader->end(), *buffer, *buffer + bufferLength);
				return;
			}
		}
		else
		{
			crlfOffset = 1;
			_crlf = false;
			headerSize = ((end + 1) - *buffer) + 1;
		}
	}
	else headerSize = ((end + 3) - *buffer) + 1;

	_rawHeader->insert(_rawHeader->end(), *buffer, *buffer + headerSize);

	char* headerBuffer = &_rawHeader->at(0);
	end = &_rawHeader->at(0) + _rawHeader->size();
	bufferLength -= headerSize;
	*buffer += headerSize;

	if(!strncmp(headerBuffer, "HTTP/1.", 7))
	{
		_type = Type::Enum::response;
		_header.responseCode = strtol(headerBuffer + 9, NULL, 10);
		if(_header.responseCode != 200) throw HTTPException("Response code was: " + std::to_string(_header.responseCode));
	}
	else
	{
		char* endPos = (char*)memchr(headerBuffer, ' ', 10);
		if(!endPos) throw HTTPException("Your client sent a request that this server could not understand.");
		_type = Type::Enum::request;
		_header.method = std::string(headerBuffer, endPos);
	}

	char* newlinePos = nullptr;

	if(!_header.method.empty())
	{
		int32_t startPos = _header.method.size() + 1;
		newlinePos = (crlfOffset == 2) ? (char*)memchr(headerBuffer, '\r', end - headerBuffer) : (char*)memchr(headerBuffer, '\n', end - headerBuffer);
		if(!newlinePos || newlinePos > end) throw HTTPException("Could not parse HTTP header.");

		char* endPos = (char*)HelperFunctions::memrchr(headerBuffer + startPos, ' ', newlinePos - (headerBuffer + startPos));
		if(!endPos) throw HTTPException("Your client sent a request that this server could not understand.");

		_header.path = std::string(headerBuffer + startPos, (int32_t)(endPos - headerBuffer - startPos));
		int32_t pos = _header.path.find('?');
		if(pos != (signed)std::string::npos)
		{
			if((unsigned)pos + 1 < _header.path.size()) _header.args = _header.path.substr(pos + 1);
			_header.path = _header.path.substr(0, pos);
		}
		pos = _header.path.find(".php");
		if(pos != (signed)std::string::npos)
		{
			pos = _header.path.find('/', pos);
			if(pos != (signed)std::string::npos)
			{
				_header.pathInfo = _header.path.substr(pos);
				_header.path = _header.path.substr(0, pos);
			}
		}
		_header.path = decodeURL(_header.path);
		HelperFunctions::stringReplace(_header.path, "../", "");

		if(!strncmp(endPos + 1, "HTTP/1.1", 8)) _header.protocol = HTTP::Protocol::http11;
		else if(!strncmp(endPos + 1, "HTTP/1.0", 8)) _header.protocol = HTTP::Protocol::http10;
		else throw HTTPException("Your client is using a HTTP protocol version that this server cannot understand.");
	}

	char* colonPos = nullptr;
	newlinePos = (char*)memchr(headerBuffer, '\n', _rawHeader->size());
	if(!newlinePos || newlinePos > end) throw HTTPException("Could not parse HTTP header.");
	headerBuffer = newlinePos + 1;

	while(headerBuffer < end)
	{
		newlinePos = (crlfOffset == 2) ? (char*)memchr(headerBuffer, '\r', end - headerBuffer) : (char*)memchr(headerBuffer, '\n', end - headerBuffer);
		if(!newlinePos || newlinePos > end) break;
		colonPos = (char*)memchr(headerBuffer, ':', newlinePos - headerBuffer);
		if(!colonPos || colonPos > newlinePos)
		{
			headerBuffer = newlinePos + crlfOffset;
			continue;
		}

		if(colonPos < newlinePos - 1) processHeaderField(headerBuffer, (uint32_t)(colonPos - headerBuffer), colonPos + 1, (uint32_t)(newlinePos - colonPos - 1));
		headerBuffer = newlinePos + crlfOffset;
	}
	_header.parsed = true;
}

void HTTP::processHeaderField(char* name, uint32_t nameSize, char* value, uint32_t valueSize)
{
	if(nameSize == 0 || valueSize == 0 || !name || !value) return;
	while(*value == ' ' && valueSize > 0)
	{
		//Skip whitespace
		value++;
		valueSize--;
	}
	if(!strnaicmp(name, "content-length", nameSize))
	{
		//Ignore Content-Length when Transfer-Encoding is present. See: http://greenbytes.de/tech/webdav/rfc2616.html#rfc.section.4.4
		if(_header.transferEncoding == TransferEncoding::Enum::none)
		{
			_contentLengthSet = true;
			_header.contentLength = strtol(value, NULL, 10);
		}
	}
	else if(!strnaicmp(name, "host", nameSize))
	{
		_header.host = std::string(value, valueSize);
		HelperFunctions::toLower(_header.host);
		HelperFunctions::stringReplace(_header.host, "../", "");
	}
	else if(!strnaicmp(name, "content-type", nameSize))
	{
		_header.contentType = std::string(value, valueSize);
		HelperFunctions::toLower(_header.contentType);
		_header.contentType = HelperFunctions::split(_header.contentType, ';').first;
	}
	else if(!strnaicmp(name, "transfer-encoding", nameSize) || !strnaicmp(name, "te", nameSize))
	{
		if(_header.contentLength > 0) _header.contentLength = 0; //Ignore Content-Length when Transfer-Encoding is present. See: http://greenbytes.de/tech/webdav/rfc2616.html#rfc.section.4.4
		std::string s(value, valueSize);
		s = s.substr(0, s.find(';'));
		int32_t pos = 0;
		while ((pos = s.find(',')) != (signed)std::string::npos || !s.empty())
		{
		    std::string te = (pos == (signed)std::string::npos) ? s : s.substr(0, pos);
		    HelperFunctions::trim(BaseLib::HelperFunctions::toLower(te));
		    if(te == "chunked") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::chunked);
			else if(te == "compress") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::compress | TransferEncoding::Enum::chunked);
			else if(te == "deflate") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::deflate | TransferEncoding::Enum::chunked);
			else if(te == "gzip") _header.transferEncoding = (TransferEncoding::Enum)(_header.transferEncoding | TransferEncoding::Enum::gzip | TransferEncoding::Enum::chunked);
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
			if(c == "keep-alive") _header.connection = (Connection::Enum)(_header.connection | Connection::Enum::keepAlive);
			else if(c == "close") _header.connection = (Connection::Enum)(_header.connection | Connection::Enum::close);
			else if(c == "upgrade") _header.connection = (Connection::Enum)(_header.connection | Connection::Enum::upgrade);
			else if(c == "te") {} //ignore
			else throw HTTPException("Unknown value for HTTP header \"Connection\": " + std::string(value, valueSize));
			if(pos == (signed)std::string::npos) s.clear(); else s.erase(0, pos + 1);
		}
	}
	else if(!strnaicmp(name, "cookie", nameSize)) _header.cookie = std::string(value, valueSize);
	else if(!strnaicmp(name, "authorization", nameSize)) _header.authorization = std::string(value, valueSize);
	std::string lowercaseName(name, nameSize);
	HelperFunctions::toLower(lowercaseName);
	_header.fields[lowercaseName] = std::string(value, valueSize);
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
	_rawHeader.reset(new std::vector<char>());
	_chunk.reset(new std::vector<char>());
	_type = Type::Enum::none;
	_finished = false;
	_dataProcessingStarted = false;
	_headerProcessingStarted = false;
}

void HTTP::setFinished()
{
	if(_finished) return;
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
		if(_content->size() == _header.contentLength) setFinished();
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
				setFinished();
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
		if(!Math::isNumber(chunkSize)) throw BaseLib::Exception("Chunk size is no number.");
		_chunkSize = Math::getNumber(chunkSize, true);
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

std::string HTTP::encodeURL(const std::string& url)
{
	std::ostringstream encoded;
	encoded.fill('0');
	encoded << std::hex;

	for (std::string::const_iterator i = url.begin(); i != url.end(); ++i) {
		if (isalnum(*i) || *i == '-' || *i == '_' || *i == '.' || *i == '~') {
			encoded << *i;
			continue;
		}

		encoded << '%' << std::setw(2) << int((unsigned char)(*i));
	}

	return encoded.str();
}

std::string HTTP::decodeURL(const std::string& url)
{
	Math math;
	std::ostringstream decoded;
	char character;
	for(std::string::const_iterator i = url.begin(); i != url.end(); ++i)
	{
		if(*i == '%')
		{
			i++;
			if(i == url.end()) return decoded.str();
			character = (char)(math.getNumber(*i) << 4);
			i++;
			if(i == url.end()) return decoded.str();
			character += (char)math.getNumber(*i);
			decoded << character;
		}
		else decoded << *i;
	}
	return decoded.str();
}

size_t HTTP::readStream(char* buffer, size_t requestLength)
{
	size_t bytesRead = 0;
	if(_streamPos < _rawHeader->size())
	{
		size_t length = requestLength;
		if(_streamPos + length > _rawHeader->size()) length = _rawHeader->size() - _streamPos;
		memcpy(buffer, &_rawHeader->at(_streamPos), length);
		_streamPos += length;
		bytesRead += length;
		requestLength -= length;
	}
	if(_content->size() == 0) return bytesRead;
	size_t contentSize = _content->size() - 1; //Ignore trailing "0"
	if(requestLength > 0 && (_streamPos - _rawHeader->size()) < contentSize)
	{
		size_t length = requestLength;
		if((_streamPos - _rawHeader->size()) + length > contentSize) length = _content->size() - (_streamPos - _rawHeader->size());
		memcpy(buffer + bytesRead, &_content->at(_streamPos - _rawHeader->size()), length);
		_streamPos += length;
		bytesRead += length;
		requestLength -= length;
	}
	return bytesRead;
}

size_t HTTP::readContentStream(char* buffer, size_t requestLength)
{
	size_t bytesRead = 0;
	size_t contentSize = _content->size() - 1; //Ignore trailing "0"
	if(_contentStreamPos < contentSize)
	{
		size_t length = requestLength;
		if(_contentStreamPos + length > contentSize) length = contentSize - _contentStreamPos;
		memcpy(buffer, &_content->at(_contentStreamPos), length);
		_contentStreamPos += length;
		bytesRead += length;
		requestLength -= length;
	}
	return bytesRead;
}

size_t HTTP::readFirstContentLine(char* buffer, size_t requestLength)
{
	if(_content->size() == 0) return 0;
	if(_contentStreamPos >= _content->size() - 1) return 0;
	size_t bytesRead = 0;
	char* posTemp = (char*)memchr(&_content->at(_contentStreamPos), '\n', _content->size() - 1 - _contentStreamPos);
	int32_t newlinePos = 0;
	if(posTemp > 0) newlinePos = posTemp - &_content->at(0);
	if(newlinePos > 0 && _content->at(newlinePos - 1) == '\r') newlinePos--;
	else if(newlinePos <= 0) newlinePos = _content->size() - 1;
	if(_contentStreamPos < (unsigned)newlinePos)
	{
		size_t length = requestLength;
		if(_contentStreamPos + length > (unsigned)newlinePos) length = newlinePos - _contentStreamPos;
		memcpy(buffer, &_content->at(_contentStreamPos), length);
		_contentStreamPos += length;
		bytesRead += length;
		requestLength -= length;
	}
	return bytesRead;
}

}
