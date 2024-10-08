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

#include "Nodejs.h"
#include <node.h>
#include <uv.h>
#include <iostream>
#include <homegear-base/HelperFunctions/Io.h>

using node::CommonEnvironmentSetup;
using node::Environment;
using node::MultiIsolatePlatform;
using v8::Context;
using v8::HandleScope;
using v8::Isolate;
using v8::Locker;
using v8::MaybeLocal;
using v8::V8;
using v8::Value;

namespace Homegear {

int Nodejs::run(int argc, char** argv) {
  try {
    //Code from node_main.cc of Node.js source

    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    {
      struct sigaction act{};
      memset(&act, 0, sizeof(act));
      act.sa_handler = SIG_IGN;
      sigaction(SIGPIPE, &act, nullptr);
    }

    return node::Start(argc, argv);
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
  return 1;
}

}