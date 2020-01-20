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

#include "php_config_fixes.h"
#include "php_homegear_globals.h"

#ifndef NO_SCRIPTENGINE

#include "php_config_fixes.h"
#include "../GD/GD.h"

static pthread_key_t pthread_key;

pthread_key_t* php_homegear_get_pthread_key()
{
    return &pthread_key;
}

zend_homegear_globals* php_homegear_get_globals()
{
    zend_homegear_globals* data = (zend_homegear_globals*)pthread_getspecific(pthread_key);
    if(!data)
    {
        data = new zend_homegear_globals();
        if(!data || pthread_setspecific(pthread_key, data) != 0)
        {
            Homegear::GD::out.printCritical("Critical: Could not set PHP globals data - out of memory?.");
            if(data) delete data;
            data = nullptr;
            return data;
        }
        data->id = 0;
        data->webRequest = false;
        data->commandLine = false;
        data->cookiesParsed = false;
        data->peerId = 0;
        data->logLevel = -1;
    }
    return data;
}

#endif
