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

#include "User.h"
#include "../GD/GD.h"
#include "../../Modules/Base/BaseLib.h"

std::vector<unsigned char> User::generatePBKDF2(const std::string& password, std::vector<unsigned char>& salt)
{
	std::vector<char> passwordBytes;
	passwordBytes.insert(passwordBytes.begin(), password.begin(), password.end());
	if(salt.empty())
	{
		std::default_random_engine generator;
		std::uniform_int_distribution<unsigned char> distribution(0, 255);
		auto randByte = std::bind(distribution, generator);
		for(uint32_t i = 0; i < 16; ++i) salt.push_back(randByte());
	}
	int32_t keySize = 20;
	std::vector<unsigned char> keyBytes(keySize);
	if(PKCS5_PBKDF2_HMAC_SHA1(&passwordBytes.at(0), passwordBytes.size(), &salt.at(0), salt.size(), 4000, keySize, &keyBytes.at(0)) != 0)
	{
		return keyBytes;
	}
	else return std::vector<unsigned char>();
}

bool User::verify(const std::string& userName, const std::string& password)
{
	try
	{
		DataColumnVector dataSelect;
		dataSelect.push_back(std::shared_ptr<DataColumn>(new DataColumn(userName)));
		DataTable rows = BaseLib::db.executeCommand("SELECT password, salt FROM users WHERE name=?", dataSelect);
		if(rows.empty() || rows.at(0).empty() || rows.at(0).size() != 2) return false;
		std::vector<unsigned char> salt;
		salt.insert(salt.begin(), rows.at(0).at(1)->binaryValue->begin(), rows.at(0).at(1)->binaryValue->end());
		std::vector<unsigned char> storedHash;
		storedHash.insert(storedHash.begin(), rows.at(0).at(0)->binaryValue->begin(), rows.at(0).at(0)->binaryValue->end());
		std::vector<unsigned char> hash = generatePBKDF2(password, salt);
		if(hash == storedHash) return true;
		return false;
	}
	catch(std::exception& ex)
	{
		Output::printError("Error verifying user credentials: " + std::string(ex.what()));
	}
	catch(...)
	{
		Output::printError("Unknown error verifying user credentials.");
	}
	return false;
}
