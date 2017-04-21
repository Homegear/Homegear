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

#include "User.h"
#include "../GD/GD.h"
#include <homegear-base/BaseLib.h>

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

uint64_t User::getID(const std::string& userName)
{
	uint64_t userID = GD::bl->db->getUserID(userName);
	return userID;
}

bool User::exists(const std::string& userName)
{
	return GD::bl->db->userNameExists(userName);
}

bool User::remove(const std::string& userName)
{
	try
	{
		uint64_t userID = GD::bl->db->getUserID(userName);
		if(userID == 0) return false;

		if(GD::bl->db->deleteUser(userID)) return true;
		return false;
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

bool User::create(const std::string& userName, const std::string& password)
{
	try
	{
		if(exists(userName)) return false;

		std::vector<uint8_t> salt;
		std::vector<uint8_t> passwordHash = generateWHIRLPOOL(password, salt);

		if(GD::bl->db->createUser(userName, passwordHash, salt)) return true;
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
		uint64_t userID = GD::bl->db->getUserID(userName);
		if(userID == 0) return false;

		std::vector<uint8_t> salt;
		std::vector<uint8_t> passwordHash = User::generateWHIRLPOOL(password, salt);

		if(GD::bl->db->updateUser(userID, passwordHash, salt)) return true;
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

bool User::getAll(std::map<uint64_t, std::string>& users)
{
	try
	{
		users.clear();
		std::shared_ptr<BaseLib::Database::DataTable> rows = GD::bl->db->getUsers();
		if(rows->size() == 0) return true;

		for(BaseLib::Database::DataTable::const_iterator i = rows->begin(); i != rows->end(); ++i)
		{
			users[i->second.at(0)->intValue] = i->second.at(1)->textValue;
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
