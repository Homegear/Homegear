/* Copyright 2013-2019 Homegear GmbH
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

#include "User.h"
#include "../GD/GD.h"
#include <homegear-base/Security/Sign.h>

namespace Homegear
{

std::vector<unsigned char> User::generateWHIRLPOOL(const std::string& password, std::vector<unsigned char>& salt)
{
    std::vector<char> passwordBytes;
    passwordBytes.insert(passwordBytes.begin(), password.begin(), password.end());
    if(salt.empty())
    {
        std::random_device rd;
        std::default_random_engine generator(rd());
        std::uniform_int_distribution<unsigned char> distribution(0, 255);
        auto randByte = std::bind(distribution, generator);
        for(uint32_t i = 0; i < 16; ++i) salt.push_back(randByte());
    }
    passwordBytes.insert(passwordBytes.end(), salt.begin(), salt.end());

    gcry_error_t result;
    gcry_md_hd_t stribogHandle = nullptr;
    if((result = gcry_md_open(&stribogHandle, GCRY_MD_WHIRLPOOL, 0)) != GPG_ERR_NO_ERROR)
    {
        GD::out.printError("Could not initialize WHIRLPOOL handle: " + BaseLib::Security::Gcrypt::getError(result));
        return std::vector<unsigned char>();
    }
    gcry_md_write(stribogHandle, &passwordBytes.at(0), passwordBytes.size());
    gcry_md_final(stribogHandle);
    uint8_t* digest = gcry_md_read(stribogHandle, GCRY_MD_WHIRLPOOL);
    if(!digest)
    {
        GD::out.printError("Could not generate WHIRLPOOL of password: " + BaseLib::Security::Gcrypt::getError(result));
        gcry_md_close(stribogHandle);
        return std::vector<unsigned char>();
    }
    std::vector<unsigned char> keyBytes(digest, digest + gcry_md_get_algo_dlen(GCRY_MD_WHIRLPOOL));
    gcry_md_close(stribogHandle);
    return keyBytes;
}

bool User::verify(const std::string& userName, const std::string& password)
{
    try
    {
        if(password.empty()) return false;
        std::shared_ptr<BaseLib::Database::DataTable> rows = GD::bl->db->getPassword(userName);
        if(rows->empty() || rows->at(0).empty() || rows->at(0).size() != 2) return false;
        std::vector<unsigned char> salt;
        salt.insert(salt.begin(), rows->at(0).at(1)->binaryValue->begin(), rows->at(0).at(1)->binaryValue->end());
        std::vector<unsigned char> storedHash;
        storedHash.insert(storedHash.begin(), rows->at(0).at(0)->binaryValue->begin(), rows->at(0).at(0)->binaryValue->end());
        std::vector<unsigned char> hash = generateWHIRLPOOL(password, salt);
        if(hash.empty()) return false;
        if(hash == storedHash) return true;
        return false;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error verifying user credentials: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error verifying user credentials.");
    }
    return false;
}

uint64_t User::getId(const std::string& userName)
{
    uint64_t userId = GD::bl->db->getUserId(userName);
    return userId;
}

std::vector<uint64_t> User::getGroups(const std::string& userName)
{
    uint64_t userId = GD::bl->db->getUserId(userName);
    return GD::bl->db->getUsersGroups(userId);
}

bool User::exists(const std::string& userName)
{
    return GD::bl->db->userNameExists(userName);
}

bool User::remove(const std::string& userName)
{
    try
    {
        uint64_t userId = GD::bl->db->getUserId(userName);
        if(userId == 0) return false;

        if(GD::bl->db->deleteUser(userId)) return true;
        return false;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error removing user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error removing user.");
    }
    return false;
}

bool User::create(const std::string& userName, const std::string& password, const std::vector<uint64_t>& groups, const BaseLib::PVariable metadata)
{
    try
    {
        if(exists(userName)) return false;

        std::vector<uint8_t> salt;
        std::vector<uint8_t> passwordHash = password.empty() ? std::vector<uint8_t>() : generateWHIRLPOOL(password, salt);

        if(GD::bl->db->createUser(userName, passwordHash, salt, groups))
        {
            if(metadata) setMetadata(userName, metadata);
            return true;
        }
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error creating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error creating user.");
    }
    return false;
}

bool User::update(const std::string& userName, const std::string& password)
{
    try
    {
        std::vector<uint64_t> groups;
        return update(userName, password, groups);
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error updating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error updating user.");
    }
    return false;
}

bool User::update(const std::string& userName, const std::vector<uint64_t>& groups)
{
    try
    {
        std::string password;
        return update(userName, password, groups);
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error updating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error updating user.");
    }
    return false;
}

bool User::update(const std::string& userName, const std::string& password, const std::vector<uint64_t>& groups)
{
    try
    {
        uint64_t userId = GD::bl->db->getUserId(userName);
        if(userId == 0) return false;

        std::vector<uint8_t> salt;
        std::vector<uint8_t> passwordHash = password.empty() ? std::vector<uint8_t>() : User::generateWHIRLPOOL(password, salt);

        if(GD::bl->db->updateUser(userId, passwordHash, salt, groups)) return true;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error updating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error updating user.");
    }
    return false;
}

bool User::getAll(std::map<uint64_t, UserInfo>& users)
{
    try
    {
        users.clear();

        BaseLib::Rpc::RpcDecoder decoder(GD::bl.get(), false, false);

        std::shared_ptr<BaseLib::Database::DataTable> rows = GD::bl->db->getUsers();
        if(rows->size() == 0) return true;

        for(BaseLib::Database::DataTable::const_iterator i = rows->begin(); i != rows->end(); ++i)
        {
            UserInfo info;

            info.name = i->second.at(1)->textValue;
            info.metadata = decoder.decodeResponse(*i->second.at(3)->binaryValue);
            auto groups = decoder.decodeResponse(*i->second.at(2)->binaryValue);
            info.groups.reserve(groups->arrayValue->size());
            for(auto& group : *groups->arrayValue)
            {
                info.groups.push_back(group->integerValue64);
            }

            users[i->second.at(0)->intValue] = std::move(info);
        }
        return true;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error creating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error creating user.");
    }
    return false;
}

BaseLib::PVariable User::getMetadata(const std::string& userName)
{
    try
    {
        uint64_t userId = GD::bl->db->getUserId(userName);
        if(userId == 0) return BaseLib::Variable::createError(-1, "Unknown user ID.");

        return GD::bl->db->getUserMetadata(userId);
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error updating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error updating user.");
    }
    return BaseLib::Variable::createError(-32500, "Unknown application error.");
}

bool User::setMetadata(const std::string& userName, BaseLib::PVariable metadata)
{
    try
    {
        uint64_t userId = GD::bl->db->getUserId(userName);
        if(userId == 0) return false;

        auto result = GD::bl->db->setUserMetadata(userId, metadata);

        if(!result->errorStruct) return true;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error updating user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error updating user.");
    }
    return false;
}

// {{{ OAuth
std::string User::generateOauthKey(const std::string& userName, const std::string& privateKey, const std::string& publicKey, std::string type, int32_t lifetime)
{
    uint64_t userId = GD::bl->db->getUserId(userName);
    int64_t keyIndex = 0;
    if(type == "access")
    {
        keyIndex = GD::bl->db->getUserKeyIndex1(userId);
        while(keyIndex == 0) keyIndex++;
        GD::bl->db->setUserKeyIndex1(userId, keyIndex);
    }
    else if(type == "refresh")
    {
        keyIndex = GD::bl->db->getUserKeyIndex2(userId);
        while(keyIndex == 0) keyIndex++;
        GD::bl->db->setUserKeyIndex2(userId, keyIndex);
    }

    BaseLib::Security::Sign sign(privateKey, publicKey);

    auto randomBytes = BaseLib::HelperFunctions::getRandomBytes(32);
    std::string tempKey;
    tempKey.reserve(256);
    BaseLib::Base64::encode(randomBytes, tempKey);
    tempKey.push_back(',');
    tempKey.append(type);
    tempKey.push_back(',');
    tempKey.append(userName);
    tempKey.push_back(',');
    tempKey.append(std::to_string(keyIndex));
    tempKey.push_back(',');
    tempKey.append(std::to_string(BaseLib::HelperFunctions::getTimeSeconds() + lifetime));
    std::vector<char> tempKey2(tempKey.begin(), tempKey.end());
    auto signature = sign.sign(tempKey2);
    std::string signatureString;
    BaseLib::Base64::encode(signature, signatureString);
    tempKey.push_back(',');
    tempKey.append(signatureString);
    std::string key;
    BaseLib::Base64::encode(tempKey, key);
    return key;
}

bool User::oauthCreate(const std::string& userName, const std::string& privateKey, const std::string& publicKey, int32_t tokenLifetime, int32_t refreshTokenLifetime, std::string& newKey, std::string& newRefreshKey)
{
    try
    {
        if(!GD::bl->db->userNameExists(userName)) return false;

        newKey = generateOauthKey(userName, privateKey, publicKey, "access", tokenLifetime);
        newRefreshKey = generateOauthKey(userName, privateKey, publicKey, "refresh", refreshTokenLifetime);

        return true;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error creating OAuth key for user: " + std::string(ex.what()));
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printError("Error creating OAuth key for user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error creating OAuth key for user.");
    }
    return false;
}

bool User::oauthVerify(const std::string& key, const std::string& privateKey, const std::string& publicKey, std::string& userName)
{
    try
    {
        std::string decodedKey;
        BaseLib::Base64::decode(key, decodedKey);
        auto keyParts = BaseLib::HelperFunctions::splitAll(decodedKey, ',');
        if(keyParts.size() < 6) return false;

        if(keyParts.at(1) != "access") return false;

        std::string tempKey;
        tempKey.reserve(decodedKey.size());
        tempKey.append(keyParts.at(0));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(1));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(2));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(3));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(4));
        std::vector<char> tempKeyData(tempKey.begin(), tempKey.end());

        std::vector<char> signatureData;
        BaseLib::Base64::decode(keyParts.at(5), signatureData);

        BaseLib::Security::Sign sign(privateKey, publicKey);
        bool signatureResult = sign.verify(tempKeyData, signatureData);
        if(!signatureResult) return false;

        userName = keyParts.at(2);

        uint64_t userId = GD::bl->db->getUserId(userName);
        if(userId == 0) return false;
        int64_t keyIndex = GD::bl->db->getUserKeyIndex1(userId);
        if(keyIndex == 0 || keyIndex != BaseLib::Math::getNumber64(keyParts.at(3))) return false;

        auto lifeEnds = BaseLib::Math::getNumber(keyParts.at(4));
        return lifeEnds - BaseLib::HelperFunctions::getTimeSeconds() > 0;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error verifying OAuth key of user: " + std::string(ex.what()));
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printError("Error verifying OAuth key of user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error verifying OAuth key of user.");
    }
    return false;
}

bool User::oauthRefresh(const std::string& refreshKey, const std::string& privateKey, const std::string& publicKey, int32_t tokenLifetime, int32_t refreshTokenLifetime, std::string& newKey, std::string& newRefreshKey, std::string& userName)
{
    try
    {
        std::string decodedKey;
        BaseLib::Base64::decode(refreshKey, decodedKey);
        auto keyParts = BaseLib::HelperFunctions::splitAll(decodedKey, ',');
        if(keyParts.size() < 6) return false;

        if(keyParts.at(1) != "refresh") return false;

        std::string tempKey;
        tempKey.reserve(decodedKey.size());
        tempKey.append(keyParts.at(0));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(1));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(2));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(3));
        tempKey.push_back(',');
        tempKey.append(keyParts.at(4));
        std::vector<char> tempKeyData(tempKey.begin(), tempKey.end());

        std::vector<char> signatureData;
        BaseLib::Base64::decode(keyParts.at(5), signatureData);

        BaseLib::Security::Sign sign(privateKey, publicKey);
        bool signatureResult = sign.verify(tempKeyData, signatureData);
        if(!signatureResult) return false;

        userName = keyParts.at(2);

        uint64_t userId = GD::bl->db->getUserId(userName);
        if(userId == 0) return false;
        int64_t keyIndex = GD::bl->db->getUserKeyIndex2(userId);
        if(keyIndex == 0 || keyIndex != BaseLib::Math::getNumber64(keyParts.at(3))) return false;

        auto lifeEnds = BaseLib::Math::getNumber(keyParts.at(4));
        if(lifeEnds - BaseLib::HelperFunctions::getTimeSeconds() < 0) return false;

        if(!GD::bl->db->userNameExists(userName)) return false;

        newKey = generateOauthKey(userName, privateKey, publicKey, "access", tokenLifetime);
        newRefreshKey = generateOauthKey(userName, privateKey, publicKey, "refresh", refreshTokenLifetime);

        return true;
    }
    catch(std::exception& ex)
    {
        GD::out.printError("Error refreshing OAuth key for user: " + std::string(ex.what()));
    }
    catch(BaseLib::Exception& ex)
    {
        GD::out.printError("Error refreshing OAuth key for user: " + std::string(ex.what()));
    }
    catch(...)
    {
        GD::out.printError("Unknown error refreshing OAuth key for user.");
    }
    return false;
}
// }}}

}