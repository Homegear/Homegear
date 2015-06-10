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

#ifndef SONOSPACKET_H_
#define SONOSPACKET_H_

#include "../Base/BaseLib.h"

#include <map>

namespace Sonos
{
class SonosPacket : public BaseLib::Systems::Packet
{
    public:
        SonosPacket();
        SonosPacket(std::string soap);
        SonosPacket(std::string& ip, std::string& path, std::string& soapAction, std::string& schema, std::string& functionName, std::shared_ptr<std::map<std::string, std::string>>& values);
        virtual ~SonosPacket();

        std::string ip() { return _ip; }
        std::string path() { return _path; }
        std::string soapAction() { return _soapAction; }
        std::string schema() { return _schema; }
        std::string functionName() { return _functionName; }
        std::shared_ptr<std::map<std::string, std::string>> values() { return _values; }

        void getSoapRequest(std::string& request);
    protected:
        std::string _ip;
        std::string _path;
        std::string _soapAction;
        std::string _schema;
        std::string _functionName;
        std::shared_ptr<std::map<std::string, std::string>> _values;
};

}
#endif
