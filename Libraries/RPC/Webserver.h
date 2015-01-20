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

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include "../../Modules/Base/BaseLib.h"
#include "ServerSettings.h"

namespace RPC
{
	class WebServer {
		public:
			WebServer(std::shared_ptr<ServerSettings::Settings>& settings);
			virtual ~WebServer();

			void get(std::string path, std::vector<char>& request, std::vector<char>& content);
			void post(std::string path, std::vector<char>& request, std::vector<char>& content);
			void getError(int32_t code, std::string codeDescription, std::string longDescription, std::vector<char>& content, std::string headerSuffix = "");
		protected:
		private:
			BaseLib::Output _out;
			std::shared_ptr<ServerSettings::Settings> _settings;
			BaseLib::Math _math;

			std::string getHeader(uint32_t contentLength, std::string contentType, int32_t code, std::string codeDescription, std::string headerSuffix = "");
			std::string decodeURL(const std::string& url);
	};
}
#endif
