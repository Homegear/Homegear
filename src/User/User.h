/* Copyright 2013-2020 Homegear GmbH
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

#ifndef USER_H_
#define USER_H_

#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <map>

#include <gcrypt.h>
#include <homegear-base/Variable.h>

namespace Homegear
{

class User
{
public:
	struct UserInfo
	{
		std::string name;
		std::vector<uint64_t> groups;
		BaseLib::PVariable metadata;
	};

	User() {}

	virtual ~User() {}

	static std::vector<unsigned char> generateWHIRLPOOL(const std::string& password, std::vector<unsigned char>& salt);

	static uint64_t getId(const std::string& userName);

	static std::vector<uint64_t> getGroups(const std::string& userName);

	static bool verify(const std::string& userName, const std::string& password);

	static bool exists(const std::string& userName);

	static bool create(const std::string& userName, const std::string& password, const std::vector<uint64_t>& groups, const BaseLib::PVariable metadata = BaseLib::PVariable());

	static bool update(const std::string& userName, const std::string& password);

	static bool update(const std::string& userName, const std::vector<uint64_t>& groups);

	static bool update(const std::string& userName, const std::string& password, const std::vector<uint64_t>& groups);

	static bool remove(const std::string& userName);

	static BaseLib::PVariable getMetadata(const std::string& userName);

	static bool setMetadata(const std::string& userName, BaseLib::PVariable metadata);

    static void deleteData(const std::string& userName, const std::string& component, const std::string& key);

    static BaseLib::PVariable getData(const std::string& userName, const std::string& component, const std::string& key);

    static bool setData(const std::string& userName,const std::string& component, const std::string& key, const BaseLib::PVariable& value);

	static bool getAll(std::map<uint64_t, UserInfo>& users);

	// {{{ OAuth
	static bool oauthCreate(const std::string& userName, const std::string& privateKey, const std::string& publicKey, int32_t tokenLifetime, int32_t refreshTokenLifetime, std::string& newKey, std::string& newRefreshKey);

	static bool oauthVerify(const std::string& key, const std::string& privateKey, const std::string& publicKey, std::string& userName);

	static bool oauthRefresh(const std::string& refreshKey, const std::string& privateKey, const std::string& publicKey, int32_t tokenLifetime, int32_t refreshTokenLifetime, std::string& newKey, std::string& newRefreshKey, std::string& userName);
	// }}}

private:
	static std::string generateOauthKey(const std::string& userName, const std::string& privateKey, const std::string& publicKey, std::string type, int32_t lifetime);
};

}

#endif
