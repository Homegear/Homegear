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

#ifndef MQTTSETTINGS_H_
#define MQTTSETTINGS_H_

#include <homegear-base/BaseLib.h>

#include <memory>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

class MqttSettings
{
public:
    MqttSettings();
    virtual ~MqttSettings() {}
    void load(std::string filename);

    bool enabled() { return _enabled; }
    int32_t processingThreadCount() { return _processingThreadCount; }
    std::string brokerHostname() { return _brokerHostname; }
    std::string brokerPort() { return _brokerPort; }
    std::string clientName() { return _clientName; }
    std::string prefix() { return _prefix; }
    std::string homegearId() { return _homegearId; }
    std::string username() { return _username; }
    std::string password() { return _password; }

    bool retain() { return _retain; }
    bool plainTopic() { return _plainTopic; }
    bool jsonTopic() { return _jsonTopic; }
    bool jsonobjTopic() { return _jsonobjTopic; }
    bool enableSSL() { return _enableSSL; }
    std::string caFile() { return _caFile; }
    bool verifyCertificate() { return _verifyCertificate; }
    std::string certPath() { return _certPath; }
    std::string keyPath() { return _keyPath; }

    //{{{ BMX
        bool bmxTopic() { return _bmxTopic; }
        std::string bmxPrefix() { return _bmxPrefix; }
        std::string bmxHostname() { return _bmxHostname; }
        std::string bmxPort() { return _bmxPort; }
        std::string bmxOrgId() { return _bmxOrgId; }
        std::string bmxDevTypeId() { return _bmxDevTypeId; }
        std::string bmxGwTypeId() { return _bmxGwTypeId; }
        std::string bmxDeviceId() { return _bmxDeviceId; }
        std::string bmxUsername() { return _bmxUsername; }
        std::string bmxToken() { return _bmxToken; }
    //}}}
private:
    bool _enabled = false;
    int32_t _processingThreadCount = 5;
    std::string _brokerHostname;
    std::string _brokerPort;
    std::string _clientName;
    std::string _prefix;
    std::string _homegearId;
    std::string _username;
    std::string _password;
    bool _retain = true;
    bool _plainTopic = true;
    bool _jsonTopic = true;
    bool _jsonobjTopic = true;
    bool _enableSSL = false;
    std::string _caFile;
    bool _verifyCertificate = true;
    std::string _certPath;
    std::string _keyPath;

    //{{{ BMX
        bool _bmxTopic = false;
        std::string _bmxPrefix;
        std::string _bmxHostname;
        std::string _bmxPort;
        std::string _bmxOrgId;
        std::string _bmxDevTypeId;
        std::string _bmxGwTypeId;
        std::string _bmxDeviceId;
        std::string _bmxUsername;
        std::string _bmxToken;
    //}}}

    void reset();
};
#endif
