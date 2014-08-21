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

#include "ScriptEngine.h"
#include "../GD/GD.h"

int Output_Consumer(const void *pOutput, unsigned int nOutputLen, void *pUserData /* Unused */)
{
  /*
   * Note that it's preferable to use the write() system call to display the output
   * rather than using the libc printf() which everybody now is extremely slow.
   */
  printf("%.*s",
      nOutputLen,
      (const char *)pOutput /* Not null terminated */
   );
    /* All done, VM output was redirected to STDOUT */
    return PH7_OK;
 }

ScriptEngine::ScriptEngine()
{
	try
	{
		
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

#define PHP_PROG "<?php "\
"echo 'Welcome guest'.PHP_EOL;"\
"echo 'Current system time is: '.date('Y-m-d H:i:s').PHP_EOL;"\
"echo 'and you are running '.php_uname();"\
"?>" 

void ScriptEngine::test()
{
	try
	{
		ph7 *pEngine; /* PH7 engine */
		ph7_vm *pVm; /* Compiled PHP program */
		int rc;
		/* Allocate a new PH7 engine instance */
		rc = ph7_init(&pEngine);
		if( rc != PH7_OK )
		{
			/*
	* If the supplied memory subsystem is so sick that we are unable
	* to allocate a tiny chunk of memory, there is no much we can do here.
	*/
			GD::out.printError("Error while allocating a new PH7 engine instance");
			ph7_lib_shutdown();
			return;
		}
		/* Compile the PHP test program defined above */
		rc = ph7_compile_v2(
		pEngine, /* PH7 engine */
		PHP_PROG, /* PHP test program */
		-1 /* Compute input length automatically*/,
		&pVm, /* OUT: Compiled PHP program */
		0 /* IN: Compile flags */
		);
		if( rc != PH7_OK )
		{
			if( rc == PH7_COMPILE_ERR ){
				const char *zErrLog;
				int nLen;
				/* Extract error log */
				ph7_config(pEngine,
				PH7_CONFIG_ERR_LOG,
				&zErrLog,
				&nLen
				);
				if( nLen > 0 ){
					/* zErrLog is null terminated */
					puts(zErrLog);
				}
			}
			/* Exit */
			GD::out.printError("Compile error");
			ph7_lib_shutdown();
			return;
		}
		/*
* Now we have our script compiled, it's time to configure our VM.
* We will install the output consumer callback defined above
* so that we can consume and redirect the VM output to STDOUT.
*/
		rc = ph7_vm_config(pVm,
		PH7_VM_CONFIG_OUTPUT,
		Output_Consumer, /* Output Consumer callback */
		0 /* Callback private data */
		);
		if( rc != PH7_OK )
		{
			GD::out.printError("Error while installing the VM output consumer callback");
			ph7_lib_shutdown();
			return;
		}
		/*
* And finally, execute our program. Note that your output (STDOUT in our case)
* should display the result.
*/
		ph7_vm_exec(pVm,0);
		/* All done, cleanup the mess left behind.
*/
		ph7_vm_release(pVm);
		ph7_release(pEngine);
	}
	catch(const std::exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(BaseLib::Exception& ex)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
	}
	catch(...)
	{
		GD::bl->out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}
