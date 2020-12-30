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

#include "GD/GD.h"
#include "CLI/CliClient.h"
#include "ScriptEngine/ScriptEngineClient.h"
#include "Node-BLUE/NodeBlueClient.h"
#include "UPnP/UPnP.h"
#include "MQTT/Mqtt.h"
#include "Nodejs/Nodejs.h"
#include <homegear-base/BaseLib.h>
#include <homegear-base/Managers/ProcessManager.h>

#include <execinfo.h>
#include <signal.h>
#include <wait.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/prctl.h> //For function prctl
#ifdef BSDSYSTEM
#include <sys/sysctl.h> //For BSD systems
#endif
#include <malloc.h>

#include <cmath>
#include <vector>
#include <memory>
#include <algorithm>

#include <gcrypt.h>
#include <grp.h>

using namespace Homegear;

void startUp();

GCRY_THREAD_OPTION_PTHREAD_IMPL;

bool _startAsDaemon = false;
bool _nonInteractive = false;
std::shared_ptr<std::function<void(int32_t, std::string)>> _errorCallback;
std::thread _signalHandlerThread;
bool _stopHomegear = false;
int _signalNumber = -1;
std::mutex _stopHomegearMutex;
std::condition_variable _stopHomegearConditionVariable;

void exitHomegear(int exitCode) {
  if (GD::eventHandler) GD::eventHandler->dispose();
  if (GD::familyController) GD::familyController->disposeDeviceFamilies();
  if (GD::bl->hgdc) GD::bl->hgdc->stop();
  if (GD::bl->db) {
    //Finish database operations before closing modules, otherwise SEGFAULT
    GD::bl->db.reset();
  }
  if (GD::familyController) GD::familyController->dispose();
  if (GD::licensingController) GD::licensingController->dispose();
  exit(exitCode);
}

void loadSettings(bool hideOutput = false) {
#ifdef NODE_BLUE
  GD::bl->settings.load(GD::configPath + "main.conf", GD::executablePath, hideOutput, true);
#else
  GD::bl->settings.load(GD::configPath + "main.conf", GD::executablePath, hideOutput);
#endif
}

void bindRPCServers() {
  BaseLib::TcpSocket tcpSocket(GD::bl.get());
  // Bind all RPC servers listening on ports <= 1024
  for (int32_t i = 0; i < GD::serverInfo.count(); i++) {
    BaseLib::Rpc::PServerInfo settings = GD::serverInfo.get(i);
    if (settings->port > 1024) continue;
    std::string info = "Info: Binding XML RPC server " + settings->name + " listening on " + settings->interface + ":" + std::to_string(settings->port);
    if (settings->ssl) info += ", SSL enabled";
    else GD::bl->rpcPort = static_cast<uint32_t>(settings->port);
    if (settings->authType != BaseLib::Rpc::ServerInfo::Info::AuthType::none) info += ", authentication enabled";
    info += "...";
    GD::out.printInfo(info);
    int32_t listenPort = -1;
    settings->socketDescriptor = tcpSocket.bindAndReturnSocket(GD::bl->fileDescriptorManager, settings->interface, std::to_string(settings->port), 100, settings->address, listenPort);
    if (settings->socketDescriptor) GD::out.printInfo("Info: Server successfully bound.");
  }
}

void startRPCServers() {
  for (int32_t i = 0; i < GD::serverInfo.count(); i++) {
    BaseLib::Rpc::PServerInfo settings = GD::serverInfo.get(i);
    std::string info = "Starting XML RPC server " + settings->name + " listening on " + settings->interface + ":" + std::to_string(settings->port);
    if (settings->ssl) info += ", SSL enabled";
    else GD::bl->rpcPort = static_cast<uint32_t>(settings->port);
    if (settings->authType != BaseLib::Rpc::ServerInfo::Info::AuthType::none) info += ", authentication enabled";
    info += "...";
    GD::out.printInfo(info);
    if (!GD::rpcServers[i]) GD::rpcServers[i] = std::make_shared<Rpc::RpcServer>();
    GD::rpcServers[i]->start(settings);
  }
  if (GD::rpcServers.empty()) {
    GD::out.printCritical("Critical: No RPC servers are running. Terminating Homegear.");
    exitHomegear(1);
  }

}

void stopRPCServers(bool dispose) {
  GD::out.printInfo("(Shutdown) => Stopping RPC servers");
  for (auto i = GD::rpcServers.begin(); i != GD::rpcServers.end(); ++i) {
    i->second->stop();
    if (dispose) i->second->dispose();
  }
  GD::bl->rpcPort = 0;
  //Don't clear map!!! Server is still accessed i. e. by the event handler!
}

void terminateHomegear(int signalNumber) {
  try {
    GD::out.printMessage("(Shutdown) => Stopping Homegear (Signal: " + std::to_string(signalNumber) + ")");
    GD::bl->shuttingDown = true;

    if (GD::nodeBlueServer) GD::nodeBlueServer->homegearShuttingDown(); //Needs to be called before familyController->homegearShuttingDown()
    if (GD::ipcServer) GD::ipcServer->homegearShuttingDown();
    #ifndef NO_SCRIPTENGINE
    if (GD::scriptEngineServer) GD::scriptEngineServer->homegearShuttingDown(); //Needs to be called before familyController->homegearShuttingDown()
    #endif
    if (GD::familyController) GD::familyController->homegearShuttingDown();
    if (GD::bl->settings.enableUPnP()) {
      GD::out.printInfo("Stopping UPnP server...");
      GD::uPnP->stop();
    }
    #ifdef EVENTHANDLER
    GD::out.printInfo( "(Shutdown) => Stopping Event handler");
    if(GD::eventHandler) GD::eventHandler->dispose();
    #endif
    if (GD::mqtt && GD::mqtt->enabled()) {
      GD::out.printInfo("(Shutdown) => Stopping MQTT client");;
      GD::mqtt->stop();
    }
    stopRPCServers(true);
    GD::rpcServers.clear();
    GD::out.printInfo("(Shutdown) => Stopping RPC client");;
    if (GD::rpcClient) GD::rpcClient->dispose();
    GD::out.printInfo("(Shutdown) => Closing physical interfaces...");
    if (GD::familyController) GD::familyController->physicalInterfaceStopListening();
    if (GD::bl->hgdc) {
      GD::out.printInfo("(Shutdown) => Stopping Homegear Daisy Chain client...");
      if (GD::bl->hgdc) GD::bl->hgdc->stop();
    }
    if (GD::bl->settings.enableNodeBlue()) GD::out.printInfo("(Shutdown) => Stopping Node-BLUE server...");
    if (GD::nodeBlueServer) GD::nodeBlueServer->stop();
    //Stop after Node-BLUE server so that nodes connected over IPC are properly stopped
    GD::out.printInfo("(Shutdown) => Stopping IPC server...");
    if (GD::ipcServer) GD::ipcServer->stop();
    #ifndef NO_SCRIPTENGINE
    GD::out.printInfo("(Shutdown) => Stopping script engine server...");
    if (GD::scriptEngineServer) GD::scriptEngineServer->stop();
    #endif
    GD::out.printMessage("(Shutdown) => Saving device families");
    if (GD::familyController) GD::familyController->save(false);
    GD::out.printMessage("(Shutdown) => Disposing device families");
    if (GD::familyController) GD::familyController->disposeDeviceFamilies();
    if (GD::bl->hgdc) {
      GD::out.printMessage("(Shutdown) => Disposing Homegear Daisy Chain client...");
      GD::bl->hgdc.reset();
    }
    GD::out.printMessage("(Shutdown) => Disposing database");
    if (GD::bl->db) {
      //Finish database operations before closing modules, otherwise SEGFAULT
      GD::bl->db.reset();
    }
    GD::out.printMessage("(Shutdown) => Disposing family modules");
    GD::familyController->dispose();
    GD::out.printMessage("(Shutdown) => Disposing licensing modules");
    if (GD::licensingController) GD::licensingController->dispose();
    GD::bl->fileDescriptorManager.dispose();

    BaseLib::ProcessManager::stopSignalHandler(GD::bl->threadManager);

    GD::out.printMessage("(Shutdown) => Shutdown complete.");

    if (_startAsDaemon || _nonInteractive) {
      fclose(stdout);
      fclose(stderr);
    }
    gnutls_global_deinit();
    gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
    gcry_control(GCRYCTL_TERM_SECMEM);
    gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

    return;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  _exit(1);
}

void reloadHomegear() {
  try {
    GD::out.printCritical("Info: Backing up database...");
    GD::bl->db->hotBackup();
    if (!GD::bl->db->isOpen()) {
      GD::out.printCritical("Critical: Can't reopen database. Exiting...");
      _exit(1);
    }

    if (GD::bl->settings.changed()) {
      GD::out.printMessage("Settings were changed. Restarting services...");

      if (GD::bl->settings.enableUPnP()) {
        GD::out.printInfo("Stopping UPnP server");
        GD::uPnP->stop();
      }
      stopRPCServers(false);
      if (GD::mqtt->enabled()) {
        GD::out.printInfo("(Shutdown) => Stopping MQTT client");
        GD::mqtt->stop();
      }
      //Binding fails sometimes with "address is already in use" without waiting.
      std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      GD::out.printMessage("Reloading settings...");
      loadSettings();
      GD::clientSettings.load(GD::bl->settings.clientSettingsPath());
      GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
      startRPCServers();
      GD::mqtt->loadSettings();
      if (GD::mqtt->enabled()) {
        GD::out.printInfo("Starting MQTT client");;
        GD::mqtt->start();
      }
      if (GD::bl->settings.enableUPnP()) {
        GD::out.printInfo("Starting UPnP server");
        GD::uPnP->start();
      }
    }

    GD::out.printInfo("Reloading Node-BLUE server...");
    if (GD::nodeBlueServer) GD::nodeBlueServer->homegearReloading();
#ifndef NO_SCRIPTENGINE
    GD::out.printInfo("Reloading script engine server...");
    if (GD::scriptEngineServer) GD::scriptEngineServer->homegearReloading();
#endif
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void signalHandlerThread() {
  int signalNumber = -1;
  sigset_t set{};
  sigemptyset(&set);
  sigaddset(&set, SIGHUP);
  sigaddset(&set, SIGTERM);
  sigaddset(&set, SIGINT);
  sigaddset(&set, SIGABRT);
  sigaddset(&set, SIGSEGV);
  sigaddset(&set, SIGQUIT);
  sigaddset(&set, SIGILL);
  sigaddset(&set, SIGFPE);
  sigaddset(&set, SIGALRM);
  sigaddset(&set, SIGUSR1);
  sigaddset(&set, SIGUSR2);
  sigaddset(&set, SIGTSTP);
  sigaddset(&set, SIGTTIN);
  sigaddset(&set, SIGTTOU);

  while (!_stopHomegear) {
    try {
      if (sigwait(&set, &signalNumber) != 0) {
        GD::out.printError("Error calling sigwait. Killing myself.");
        kill(getpid(), SIGKILL);
      }
      if (GD::bl->settings.devLog()) GD::out.printDebug("Debug: Signal " + std::to_string(signalNumber) + " received in main.cpp.");
      if (signalNumber == SIGTERM || signalNumber == SIGINT) {
        std::unique_lock<std::mutex> stopHomegearGuard(_stopHomegearMutex);
        _stopHomegear = true;
        _signalNumber = signalNumber;
        stopHomegearGuard.unlock();
        _stopHomegearConditionVariable.notify_all();
        return;
      } else if (signalNumber == SIGHUP) {
        GD::out.printMessage("Info: SIGHUP received. Reloading...");

        reloadHomegear();

        //Reopen log files, important for logrotate
        if (_startAsDaemon || _nonInteractive) {
          if (!std::freopen((GD::bl->settings.logfilePath() + "homegear.log").c_str(), "a", stdout)) {
            GD::out.printError("Error: Could not redirect output to new log file.");
          }
          if (!std::freopen((GD::bl->settings.logfilePath() + "homegear.err").c_str(), "a", stderr)) {
            GD::out.printError("Error: Could not redirect errors to new log file.");
          }
        }

        GD::out.printInfo("Info: Reload complete.");
      } else {
        GD::out.printCritical("Signal " + std::to_string(signalNumber) + " received.");
        pthread_sigmask(SIG_SETMASK, &BaseLib::SharedObjects::defaultSignalMask, nullptr);
        kill(getpid(), signalNumber); //Raise same signal again using the default action.
      }
    }
    catch (const std::exception &ex) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }
  }
}

void errorCallback(int32_t level, std::string message) {
  if (GD::rpcClient) GD::rpcClient->broadcastError(level, message);
}

void getExecutablePath(int argc, char *argv[]) {
  std::array<char, PATH_MAX> path{};
  if (!getcwd(path.data(), path.size())) {
    std::cerr << "Could not get working directory." << std::endl;
    exit(1);
  }
  GD::workingDirectory = std::string(path.data());
#ifdef KERN_PROC //BSD system
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  size_t cb = sizeof(path);
  int result = sysctl(mib, 4, path, &cb, NULL, 0);
  if(result == -1)
  {
      std::cerr << "Could not get executable path." << std::endl;
      exit(1);
  }
  path[sizeof(path) - 1] = '\0';
  GD::executablePath = std::string(path);
  GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#else
  int length = readlink("/proc/self/exe", path.data(), path.size() - 1);
  if (length < 0) {
    std::cerr << "Could not get executable path." << std::endl;
    exit(1);
  }
  if ((unsigned)length > sizeof(path)) {
    std::cerr << "The path to the Homegear binary is in has more than 1024 characters." << std::endl;
    exit(1);
  }
  path[length] = '\0';
  GD::executablePath = std::string(path.data());
  GD::executablePath = GD::executablePath.substr(0, GD::executablePath.find_last_of("/") + 1);
#endif

  GD::executableFile = std::string(argc > 0 ? argv[0] : "homegear");
  BaseLib::HelperFunctions::trim(GD::executableFile);
  if (GD::executableFile.empty()) GD::executableFile = "homegear";
  std::pair<std::string, std::string> pathNamePair = BaseLib::HelperFunctions::splitLast(GD::executableFile, '/');
  if (!pathNamePair.second.empty()) GD::executableFile = pathNamePair.second;
}

void initGnuTls() {
  // {{{ Init Libgcrypt and GnuTLS
  gcry_error_t gcryResult;
  if ((gcryResult = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread)) != GPG_ERR_NO_ERROR) {
    GD::out.printCritical("Critical: Could not enable thread support for gcrypt.");
    exit(2);
  }

  if (!gcry_check_version(GCRYPT_VERSION)) {
    GD::out.printCritical("Critical: Wrong gcrypt version.");
    exit(2);
  }
  gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);
  if ((gcryResult = gcry_control(GCRYCTL_INIT_SECMEM, (int)GD::bl->settings.secureMemorySize(), 0)) != GPG_ERR_NO_ERROR) {
    GD::out.printCritical("Critical: Could not allocate secure memory. Error code is: " + std::to_string((int32_t)gcryResult));
    exit(2);
  }
  gcry_control(GCRYCTL_RESUME_SECMEM_WARN);
  gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

  int32_t gnutlsResult = 0;
  if ((gnutlsResult = gnutls_global_init()) != GNUTLS_E_SUCCESS) {
    GD::out.printCritical("Critical: Could not initialize GnuTLS: " + std::string(gnutls_strerror(gnutlsResult)));
    exit(2);
  }
  // }}}
}

void setLimits() {
  struct rlimit limits{};
  if (!GD::bl->settings.enableCoreDumps()) prctl(PR_SET_DUMPABLE, 0);
  else {
    //Set rlimit for core dumps
    getrlimit(RLIMIT_CORE, &limits);
    limits.rlim_cur = limits.rlim_max;
    GD::out.printInfo("Setting allowed core file size to \"" + std::to_string(limits.rlim_cur) + "\" for user with id " + std::to_string(getuid()) + " and group with id " + std::to_string(getgid()) + '.');
    setrlimit(RLIMIT_CORE, &limits);
    getrlimit(RLIMIT_CORE, &limits);
    GD::out.printInfo("Core file size now is \"" + std::to_string(limits.rlim_cur) + "\".");
  }
#ifdef RLIMIT_RTPRIO //Not existant on BSD systems
  getrlimit(RLIMIT_RTPRIO, &limits);
  limits.rlim_cur = limits.rlim_max;
  GD::out.printInfo("Setting maximum thread priority to \"" + std::to_string(limits.rlim_cur) + "\" for user with id " + std::to_string(getuid()) + " and group with id " + std::to_string(getgid()) + '.');
  setrlimit(RLIMIT_RTPRIO, &limits);
  getrlimit(RLIMIT_RTPRIO, &limits);
  GD::out.printInfo("Maximum thread priority now is \"" + std::to_string(limits.rlim_cur) + "\".");
#endif
}

void printHelp() {
#ifdef NODE_BLUE
  std::cout << "Usage: node-blue [OPTIONS]" << std::endl << std::endl;
  std::cout << "Option              Meaning" << std::endl;
  std::cout << "-h                  Show this help" << std::endl;
  std::cout << "-u                  Run as user" << std::endl;
  std::cout << "-g                  Run as group" << std::endl;
  std::cout << "-c <path>           Specify path to config directory" << std::endl;
  std::cout << "-d                  Run as daemon" << std::endl;
  std::cout << "-p <pid path>       Specify path to process id file" << std::endl;
  std::cout << "-r                  Connect to Node-BLUE on this machine" << std::endl;
  std::cout << "-e <command>        Execute CLI command" << std::endl;
  std::cout << "-n <script> <args>  Run Node.js script. \"-n\" can be ommited when filename ends with \".js\"." << std::endl;
  std::cout << "-l                  Checks the lifeticks of all components. Exit code \"0\" means everything is ok." << std::endl;
  std::cout << "-v                  Print program version" << std::endl;
#else
  std::cout << "Usage: homegear [OPTIONS]" << std::endl << std::endl;
  std::cout << "Option              Meaning" << std::endl;
  std::cout << "-h                  Show this help" << std::endl;
  std::cout << "-u                  Run as user" << std::endl;
  std::cout << "-g                  Run as group" << std::endl;
  std::cout << "-c <path>           Specify path to config directory" << std::endl;
  std::cout << "-d                  Run as daemon" << std::endl;
  std::cout << "-p <pid path>       Specify path to process id file" << std::endl;
  std::cout << "-s <user> <group>   Set GPIO settings and necessary permissions for all defined physical devices" << std::endl;
  std::cout << "-r                  Connect to Homegear on this machine" << std::endl;
  std::cout << "-e <command>        Execute CLI command" << std::endl;
  std::cout << "-n <script> <args>  Run Node.js script. \"-n\" can be ommited when filename ends with \".js\"." << std::endl;
  std::cout << "-o <input> <output> Convert old device description file into new format." << std::endl;
  std::cout << "-l                  Checks the lifeticks of all components. Exit code \"0\" means everything is ok." << std::endl;
  std::cout << "-i                  Generates and prints a new time UUID." << std::endl;
  std::cout << "-v                  Print program version" << std::endl;
#endif
}

void startDaemon() {
  try {
    pid_t pid, sid;
    pid = fork();
    if (pid < 0) {
      exitHomegear(1);
    }
    if (pid > 0) {
      exitHomegear(0);
    }

    //Set process permission
    umask(S_IWGRP | S_IWOTH);

    //Set child processe's id
    sid = setsid();
    if (sid < 0) {
      exitHomegear(1);
    }

    close(STDIN_FILENO);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  catch (...) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

void startUp() {
  try {
    if (GD::bl->settings.memoryDebugging()) mallopt(M_CHECK_ACTION, 3); //Print detailed error message, stack trace, and memory, and abort the program. See: http://man7.org/linux/man-pages/man3/mallopt.3.html

    //Use sigaction over signal because of different behavior in Linux and BSD
    struct sigaction sa{};
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, nullptr);

    setLimits();

    initGnuTls();

    if (_startAsDaemon || _nonInteractive) {
      if (!std::freopen((GD::bl->settings.logfilePath() + "homegear.log").c_str(), "a", stdout)) {
        GD::out.printError("Error: Could not redirect output to log file.");
      }
      if (!std::freopen((GD::bl->settings.logfilePath() + "homegear.err").c_str(), "a", stderr)) {
        GD::out.printError("Error: Could not redirect errors to log file.");
      }
    }

    GD::out.printMessage("Starting Homegear...");
    GD::out.printMessage(std::string("Homegear version: ") + GD::baseLibVersion);

    if (GD::bl->settings.maxTotalThreadCount() < 100) {
      GD::out.printMessage("Determining maximum thread count...");
      try {
        // {{{ Get maximum thread count
        std::string output;
        BaseLib::ProcessManager::exec(GD::executablePath + GD::executableFile + " -tc", GD::bl->fileDescriptorManager.getMax(), output);
        BaseLib::HelperFunctions::trim(output);
        if (BaseLib::Math::isNumber(output, false)) GD::bl->threadManager.setMaxThreadCount(BaseLib::Math::getNumber(output, false));
        // }}}
      }
      catch (const std::exception &ex) {
        GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
      }
    } else {
      GD::bl->threadManager.setMaxThreadCount(GD::bl->settings.maxTotalThreadCount());
    }
    GD::out.printMessage("Maximum thread count is: " + std::to_string(GD::bl->threadManager.getMaxThreadCount()));

    if (GD::bl->settings.waitForCorrectTime()) {
      while (BaseLib::HelperFunctions::getTime() < 1000000000000) {
        GD::out.printWarning("Warning: Time is in the past. Waiting for ntp to set the time...");
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
      }
    }
    GD::bl->setStartTime(BaseLib::HelperFunctions::getTime());

    if (!GD::bl->settings.waitForIp4OnInterface().empty()) {
      std::string ipAddress;
      while (ipAddress.empty()) {
        try {
          ipAddress = BaseLib::Net::getMyIpAddress(GD::bl->settings.waitForIp4OnInterface());
        }
        catch (const BaseLib::NetException &ex) {
          GD::out.printDebug("Debug: " + std::string(ex.what()));
        }
        if (ipAddress.empty()) {
          GD::out.printWarning("Warning: " + GD::bl->settings.waitForIp4OnInterface() + " has no IPv4 address assigned yet. Waiting...");
          std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
      }
    }

    if (!GD::bl->settings.waitForIp6OnInterface().empty()) {
      std::string ipAddress;
      while (ipAddress.empty()) {
        try {
          ipAddress = BaseLib::Net::getMyIp6Address(GD::bl->settings.waitForIp6OnInterface());
        }
        catch (const BaseLib::NetException &ex) {
          GD::out.printDebug("Debug: " + std::string(ex.what()));
        }
        if (ipAddress.empty()) {
          GD::out.printWarning("Warning: " + GD::bl->settings.waitForIp6OnInterface() + " has no IPv6 address assigned yet. Waiting...");
          std::this_thread::sleep_for(std::chrono::milliseconds(10000));
        }
      }
    }

    if (!GD::bl->io.directoryExists(GD::bl->settings.socketPath())) {
      if (!GD::bl->io.createDirectory(GD::bl->settings.socketPath(), S_IRWXU | S_IRWXG)) {
        GD::out.printCritical("Critical: Directory \"" + GD::bl->settings.socketPath() + "\" does not exist and cannot be created.");
        exit(1);
      }
      if (GD::bl->userId != 0 || GD::bl->groupId != 0) {
        if (chown(GD::bl->settings.socketPath().c_str(), GD::bl->userId, GD::bl->groupId) == -1) {
          GD::out.printCritical("Critical: Could not set permissions on directory \"" + GD::bl->settings.socketPath() + "\"");
          exit(1);
        }
      }
    }

    GD::bl->db->init();
    std::string databasePath = GD::bl->settings.databasePath();
    if (databasePath.empty()) databasePath = GD::bl->settings.dataPath();
    std::string databaseBackupPath = GD::bl->settings.databaseBackupPath();
    if (databaseBackupPath.empty()) databaseBackupPath = GD::bl->settings.dataPath();
    GD::bl->db->open(databasePath, "db.sql", GD::bl->settings.databaseSynchronous(), GD::bl->settings.databaseMemoryJournal(), GD::bl->settings.databaseWALJournal(), databaseBackupPath, "db.sql.bak");
    if (!GD::bl->db->isOpen()) exitHomegear(1);

    GD::out.printInfo("Initializing database...");
    if (GD::bl->db->convertDatabase()) exitHomegear(0);
    GD::bl->db->initializeDatabase();

    {
      bool runningAsUser = !GD::runAsUser.empty() && !GD::runAsGroup.empty();

      std::string currentPath = GD::bl->settings.dataPath();
      if (!currentPath.empty() && runningAsUser) {
        uid_t userId = GD::bl->hf.userId(GD::bl->settings.dataPathUser());
        gid_t groupId = GD::bl->hf.groupId(GD::bl->settings.dataPathGroup());
        if (((int32_t)userId) == -1 || ((int32_t)groupId) == -1) {
          userId = GD::bl->userId;
          groupId = GD::bl->groupId;
        }
        std::vector<std::string> files;
        try {
          files = GD::bl->io.getFiles(currentPath, false);
        }
        catch (const std::exception &ex) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        }
        catch (...) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        }
        for (std::vector<std::string>::iterator k = files.begin(); k != files.end(); ++k) {
          if ((*k).compare(0, 6, "db.sql") != 0) continue;
          std::string file = currentPath + *k;
          if (chown(file.c_str(), userId, groupId) == -1) GD::out.printError("Could not set owner on " + file);
          if (chmod(file.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printError("Could not set permissions on " + file);
        }
      }

      currentPath = GD::bl->settings.databasePath();
      if (!currentPath.empty() && runningAsUser) {
        uid_t userId = GD::bl->hf.userId(GD::bl->settings.dataPathUser());
        gid_t groupId = GD::bl->hf.groupId(GD::bl->settings.dataPathGroup());
        if (((int32_t)userId) == -1 || ((int32_t)groupId) == -1) {
          userId = GD::bl->userId;
          groupId = GD::bl->groupId;
        }
        std::vector<std::string> files;
        try {
          files = GD::bl->io.getFiles(currentPath, false);
        }
        catch (const std::exception &ex) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        }
        catch (...) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        }
        for (std::vector<std::string>::iterator k = files.begin(); k != files.end(); ++k) {
          std::string file = currentPath + *k;
          if (chown(file.c_str(), userId, groupId) == -1) GD::out.printError("Could not set owner on " + file);
          if (chmod(file.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printError("Could not set permissions on " + file);
        }
      }

      currentPath = GD::bl->settings.databaseBackupPath();
      if (!currentPath.empty() && runningAsUser) {
        uid_t userId = GD::bl->hf.userId(GD::bl->settings.dataPathUser());
        gid_t groupId = GD::bl->hf.groupId(GD::bl->settings.dataPathGroup());
        if (((int32_t)userId) == -1 || ((int32_t)groupId) == -1) {
          userId = GD::bl->userId;
          groupId = GD::bl->groupId;
        }
        std::vector<std::string> files;
        try {
          files = GD::bl->io.getFiles(currentPath, false);
        }
        catch (const std::exception &ex) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
        }
        catch (...) {
          GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
        }
        for (std::vector<std::string>::iterator k = files.begin(); k != files.end(); ++k) {
          std::string file = currentPath + *k;
          if (chown(file.c_str(), userId, groupId) == -1) GD::out.printError("Could not set owner on " + file);
          if (chmod(file.c_str(), GD::bl->settings.dataPathPermissions()) == -1) GD::out.printError("Could not set permissions on " + file);
        }
      }

      if (runningAsUser) {
        //Logs are created as root. So it is really important to set the permissions here.
        if (chown((GD::bl->settings.logfilePath() + "homegear.log").c_str(), GD::bl->userId, GD::bl->groupId) == -1) GD::out.printError("Could not set owner on file homegear.log");
        if (chown((GD::bl->settings.logfilePath() + "homegear.err").c_str(), GD::bl->userId, GD::bl->groupId) == -1) GD::out.printError("Could not set owner on file homegear.err");
      }
    }

    std::string homegearInstanceId;
    if (GD::bl->db->getHomegearVariableString(DatabaseController::HomegearVariables::uniqueid, homegearInstanceId)) {
      GD::out.printMessage("Homegear instance ID: " + homegearInstanceId);
    }

    for (int32_t i = 0; i < 60; i++) {
      try {
        bindRPCServers();
        break;
      }
      catch (const BaseLib::NetException &ex) {
        GD::out.printError("Error binding RPC servers (1): " + std::string(ex.what()) + " Retrying in 5 seconds...");
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        continue;
      }
      catch (const BaseLib::SocketOperationException &ex) {
        GD::out.printError("Error binding RPC servers (2): " + std::string(ex.what()) + " Retrying in 5 seconds...");
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        continue;
      }
    }

    GD::licensingController->loadModules();

    GD::out.printInfo("Initializing system variable controller...");
    GD::systemVariableController.reset(new SystemVariableController());

    GD::familyController->init();
    GD::familyController->loadModules();

    if (getuid() == 0 && !GD::runAsUser.empty() && !GD::runAsGroup.empty()) {
      if (GD::bl->userId == 0 || GD::bl->groupId == 0) {
        GD::out.printCritical("Could not drop privileges. User name or group name is not valid.");
        exitHomegear(1);
      }
      GD::out.printInfo("Info: Setting up physical interfaces and GPIOs...");
      if (GD::familyController) GD::familyController->physicalInterfaceSetup(GD::bl->userId, GD::bl->groupId, GD::bl->settings.setDevicePermissions());
      BaseLib::LowLevel::Gpio gpio(GD::bl.get(), GD::bl->settings.gpioPath());
      auto gpiosToExport = GD::bl->settings.exportGpios();
      gpio.setup(GD::bl->userId, GD::bl->groupId, GD::bl->settings.setDevicePermissions(), gpiosToExport);
      GD::out.printInfo("Info: Dropping privileges to user " + GD::runAsUser + " (" + std::to_string(GD::bl->userId) + ") and group " + GD::runAsGroup + " (" + std::to_string(GD::bl->groupId) + ")");

      int result = -1;
      std::vector<gid_t> supplementaryGroups(10);
      int numberOfGroups = 10;
      while (result == -1) {
        result = getgrouplist(GD::runAsUser.c_str(), 10000, supplementaryGroups.data(), &numberOfGroups);

        if (result == -1) supplementaryGroups.resize(numberOfGroups);
        else supplementaryGroups.resize(result);
      }

      if (setgid(GD::bl->groupId) != 0) {
        GD::out.printCritical("Critical: Could not drop group privileges: " + std::string(strerror(errno)));
        exitHomegear(1);
      }

      if (setgroups(supplementaryGroups.size(), supplementaryGroups.data()) != 0) {
        GD::out.printCritical("Critical: Could not set supplementary groups: " + std::string(strerror(errno)));
        exitHomegear(1);
      }

      if (setuid(GD::bl->userId) != 0) {
        GD::out.printCritical("Critical: Could not drop user privileges: " + std::string(strerror(errno)));
        exitHomegear(1);
      }

      //Core dumps are disabled by setuid. Enable them again.
      if (GD::bl->settings.enableCoreDumps()) prctl(PR_SET_DUMPABLE, 1);
    }

    if (getuid() == 0) {
      if (!GD::runAsUser.empty() && !GD::runAsGroup.empty()) {
        GD::out.printCritical("Critical: Homegear still has root privileges though privileges should have been dropped. Exiting Homegear as this is a security risk.");
        exit(1);
      } else GD::out.printWarning("Warning: Running as root. The authors of Homegear recommend running Homegear as user.");
    } else {
      if (setuid(0) != -1) {
        GD::out.printCritical("Critical: Regaining root privileges succeded. Exiting Homegear as this is a security risk.");
        exit(1);
      }
      GD::out.printInfo("Info: Homegear is (now) running as user with id " + std::to_string(getuid()) + " and group with id " + std::to_string(getgid()) + '.');
    }

    //Create PID file
    try {
      if (!GD::pidfilePath.empty()) {
        int32_t pidfile = open(GD::pidfilePath.c_str(), O_CREAT | O_RDWR, 0666);
        if (pidfile < 0) {
          GD::out.printError("Error: Cannot create pid file \"" + GD::pidfilePath + "\".");
        } else {
          int32_t rc = flock(pidfile, LOCK_EX | LOCK_NB);
          if (rc && errno == EWOULDBLOCK) {
            GD::out.printError("Error: Homegear is already running - Can't lock PID file.");
          }
          std::string pid(std::to_string(getpid()));
          int32_t bytesWritten = write(pidfile, pid.c_str(), pid.size());
          if (bytesWritten <= 0) GD::out.printError("Error writing to PID file: " + std::string(strerror(errno)));
          close(pidfile);
        }
      }
    }
    catch (const std::exception &ex) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
    }
    catch (...) {
      GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__);
    }

    if (!GD::bl->io.directoryExists(GD::bl->settings.tempPath())) {
      if (!GD::bl->io.createDirectory(GD::bl->settings.tempPath(), S_IRWXU | S_IRWXG)) {
        GD::out.printCritical("Critical: Cannot create temp directory \"" + GD::bl->settings.tempPath());
        exit(1);
      }
    }
    std::vector<std::string> tempFiles = GD::bl->io.getFiles(GD::bl->settings.tempPath(), false);
    for (std::vector<std::string>::iterator i = tempFiles.begin(); i != tempFiles.end(); ++i) {
      if (!GD::bl->io.deleteFile(GD::bl->settings.tempPath() + *i)) {
        GD::out.printCritical("Critical: deleting temporary file \"" + GD::bl->settings.tempPath() + *i + "\": " + strerror(errno));
      }
    }
    std::string phpTempPath = GD::bl->settings.tempPath() + "/php/";
    if (GD::bl->io.directoryExists(phpTempPath)) {
      tempFiles = GD::bl->io.getFiles(phpTempPath, false);
      for (std::vector<std::string>::iterator i = tempFiles.begin(); i != tempFiles.end(); ++i) {
        if (!GD::bl->io.deleteFile(phpTempPath + *i)) {
          GD::out.printCritical("Critical: deleting temporary file \"" + phpTempPath + *i + "\": " + strerror(errno));
        }
      }
    }

    GD::bl->globalServiceMessages.load();

    GD::uiController = std::make_unique<UiController>();
    GD::uiController->load();

    GD::variableProfileManager = std::make_unique<VariableProfileManager>();

    GD::ipcLogger = std::make_unique<IpcLogger>();

#ifdef EVENTHANDLER
    GD::eventHandler.reset(new EventHandler());
#endif

    GD::nodeBlueServer = std::make_unique<NodeBlue::NodeBlueServer>();
    GD::ipcServer = std::make_unique<IpcServer>();

    if (!GD::bl->io.directoryExists(GD::bl->settings.tempPath() + "php")) {
      if (!GD::bl->io.createDirectory(GD::bl->settings.tempPath() + "php", S_IRWXU | S_IRWXG)) {
        GD::out.printCritical("Critical: Cannot create temp directory \"" + GD::bl->settings.tempPath() + "php");
        exitHomegear(1);
      }
    }
#ifndef NO_SCRIPTENGINE
    GD::out.printInfo("Starting script engine server...");
    GD::scriptEngineServer = std::make_unique<ScriptEngine::ScriptEngineServer>();
    if (!GD::scriptEngineServer->start()) {
      GD::out.printCritical("Critical: Cannot start script engine server. Exiting Homegear.");
      exitHomegear(1);
    }
#else
    GD::out.printInfo("Info: Homegear is compiled without script engine.");
#endif

    GD::out.printInfo("Initializing licensing controller...");
    GD::licensingController->init();

    GD::out.printInfo("Loading licensing controller data...");
    GD::licensingController->load();

    if (GD::bl->settings.enableHgdc()) {
      GD::out.printInfo("Initializing Homegear Daisy Chain client...");
      GD::bl->hgdc = std::make_shared<BaseLib::Hgdc>(GD::bl.get(), GD::bl->settings.hgdcPort());
      GD::out.printInfo("Starting Homegear Daisy Chain client...");
      GD::bl->hgdc->start();
    }

    GD::out.printInfo("Loading devices...");
    if (BaseLib::Io::fileExists(GD::configPath + "physicalinterfaces.conf")) GD::out.printWarning("Warning: File physicalinterfaces.conf exists in config directory. Interface configuration has been moved to " + GD::bl->settings.familyConfigPath());
    GD::familyController->load(); //Don't load before database is open!

    GD::out.printInfo("Initializing RPC client...");
    GD::rpcClient->init();

    startRPCServers();

    GD::mqtt->loadSettings(); //Needs database to be available
    if (GD::mqtt->enabled()) {
      GD::out.printInfo("Starting MQTT client...");
      GD::mqtt->start();
    }

#ifdef EVENTHANDLER
    GD::out.printInfo("Initializing event handler...");
    GD::eventHandler->init();
    GD::out.printInfo("Loading events...");
    GD::eventHandler->load();
#endif

    GD::out.printInfo("Start listening for packets...");
    GD::familyController->physicalInterfaceStartListening();
    if (!GD::familyController->physicalInterfaceIsOpen()) GD::out.printCritical("Critical: At least one of the communication modules could not be opened...");

    GD::out.printInfo("Starting IPC server...");
    if (!GD::ipcServer->start()) {
      GD::out.printCritical("Critical: Cannot start IPC server. Exiting Homegear.");
      exitHomegear(1);
    }

    if (GD::bl->settings.enableNodeBlue()) {
      GD::out.printInfo("Starting Node-BLUE server...");
      if (!GD::nodeBlueServer->start()) {
        GD::out.printCritical("Critical: Cannot start Node-BLUE server. Exiting Homegear.");
        exitHomegear(1);
      }
    }

    GD::out.printInfo("Starting variable profile manager...");
    GD::variableProfileManager->load();

    BaseLib::ProcessManager::startSignalHandler(GD::bl->threadManager);
    GD::bl->threadManager.start(_signalHandlerThread, true, &signalHandlerThread);

    GD::out.printMessage("Startup complete. Waiting for physical interfaces to connect.");

    //Wait for all interfaces to connect before setting booting to false
    {
      uint32_t maxWait = GD::bl->settings.maxWaitForPhysicalInterfaces();
      if (maxWait < 1) maxWait = 1;
      for (int32_t i = 0; i < (signed)maxWait; i++) {
        if (GD::bl->debugLevel >= 4 && i % 10 == 0) GD::out.printInfo("Info: Waiting for physical interfaces to connect (" + std::to_string(i) + " of 180s" + ").");
        if (GD::familyController->physicalInterfaceIsOpen()) {
          GD::out.printMessage("All physical interfaces are connected now.");
          break;
        }
        if (i == 299) GD::out.printError("Error: At least one physical interface is not connected.");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      }
    }

    if (GD::bl->settings.enableUPnP()) {
      GD::out.printInfo("Starting UPnP server...");
      GD::uPnP->start();
    }

    GD::bl->booting = false;
    GD::familyController->homegearStarted();

    if (BaseLib::Io::fileExists(GD::bl->settings.workingDirectory() + "core")) {
      GD::out.printError("Error: A core file exists in Homegear's working directory (\"" + GD::bl->settings.workingDirectory() + "core"
                             + "\"). Please send this file to the Homegear team including information about your system (Linux distribution, CPU architecture), the Homegear version, the current log files and information what might've caused the error.");
    }

    while (!_stopHomegear) {
      std::unique_lock<std::mutex> stopHomegearGuard(_stopHomegearMutex);
      _stopHomegearConditionVariable.wait(stopHomegearGuard);
    }

    terminateHomegear(_signalNumber);
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
}

int main(int argc, char *argv[]) {
  try {
    getExecutablePath(argc, argv);
    _errorCallback.reset(new std::function<void(int32_t, std::string)>(errorCallback));
    GD::bl = std::make_unique<BaseLib::SharedObjects>();

    if (BaseLib::Io::directoryExists(GD::executablePath + "config")) GD::configPath = GD::executablePath + "config/";
    else if (BaseLib::Io::directoryExists(GD::executablePath + "cfg")) GD::configPath = GD::executablePath + "cfg/";
#ifdef NODE_BLUE
      else GD::configPath = "/etc/node-blue/";
#else
    else GD::configPath = "/etc/homegear/";
#endif

    if (std::string(GD::baseLibVersion) != GD::bl->version()) {
      GD::out.printCritical(std::string("Base library has wrong version. Expected version ") + GD::baseLibVersion + " but got version " + GD::bl->version());
      exit(1);
    }

    if (std::string(GD::nodeLibVersion) != Flows::INode::version()) {
      GD::out.printCritical(std::string("Node library has wrong version. Expected version ") + GD::nodeLibVersion + " but got version " + Flows::INode::version());
      exit(1);
    }

    if (std::string(GD::ipcLibVersion) != Ipc::IIpcClient::version()) {
      GD::out.printCritical(std::string("IPC library has wrong version. Expected version ") + GD::ipcLibVersion + " but got version " + Ipc::IIpcClient::version());
      exit(1);
    }

    for (int32_t i = 1; i < argc; i++) {
      std::string arg(argv[i]);
      if (arg == "-h" || arg == "--help") {
        printHelp();
        exit(0);
      } else if (arg == "-c") {
        if (i + 1 < argc) {
          std::string configPath = std::string(argv[i + 1]);
          if (!configPath.empty()) GD::configPath = configPath;
          if (GD::configPath[GD::configPath.size() - 1] != '/') GD::configPath.push_back('/');
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-p") {
        if (i + 1 < argc) {
          GD::pidfilePath = std::string(argv[i + 1]);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-u") {
        if (i + 1 < argc) {
          auto user = std::string(argv[i + 1]);
          if (user != "root") GD::runAsUser = user;
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-g") {
        if (i + 1 < argc) {
          auto group = std::string(argv[i + 1]);
          if (group != "root") GD::runAsGroup = group;
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-s") {
        if (i + 2 < argc) {
          if (getuid() != 0) {
            std::cout << "Please run Homegear as root to set the device permissions." << std::endl;
            exit(1);
          }
          loadSettings();
          GD::bl->debugLevel = 3; //Only output warnings.
          GD::licensingController.reset(new LicensingController());
          GD::familyController.reset(new FamilyController());
          GD::licensingController->loadModules();
          GD::familyController->loadModules();
          uid_t userId = GD::bl->hf.userId(std::string(argv[i + 1]));
          gid_t groupId = GD::bl->hf.groupId(std::string(argv[i + 2]));
          GD::out.printDebug("Debug: User ID set to " + std::to_string(userId) + " group ID set to " + std::to_string(groupId));
          if ((signed)userId == -1 || (signed)groupId == -1) {
            GD::out.printCritical("Could not setup physical devices. Username or group name is not valid.");
            GD::familyController->dispose();
            GD::licensingController->dispose();
            exit(1);
          }
          GD::familyController->physicalInterfaceSetup(userId, groupId, GD::bl->settings.setDevicePermissions());
          BaseLib::LowLevel::Gpio gpio(GD::bl.get(), GD::bl->settings.gpioPath());
          auto gpiosToExport = GD::bl->settings.exportGpios();
          gpio.setup(userId, groupId, GD::bl->settings.setDevicePermissions(), gpiosToExport);
          GD::familyController->dispose();
          GD::licensingController->dispose();
          exit(0);
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-o") {
        if (i + 2 < argc) {
          loadSettings();
          std::string inputFile(argv[i + 1]);
          std::string outputFile(argv[i + 2]);
          BaseLib::DeviceDescription::Devices devices(GD::bl.get(), nullptr, 0);
          std::shared_ptr<HomegearDevice> device = devices.loadFile(inputFile);
          if (!device) exit(1);
          device->save(outputFile);
          exit(0);
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "--title") {
        if (i + 1 < argc) {
          auto processTitle = std::string(argv[i + 1]);
          auto maxTitleSize = strlen(argv[0]); //We can't exceed this size. Assigning a newly allocated string won't make the title show in ps and top.
          strncpy(argv[0], processTitle.data(), maxTitleSize);
          getExecutablePath(argc, argv);
          i++;
        } else {
          printHelp();
          exit(1);
        }
      } else if (arg == "-d") {
        _startAsDaemon = true;
      } else if (arg == "-r") {
        loadSettings();
        CliClient cliClient(GD::bl->settings.socketPath() + "homegearIPC.sock");
        std::string command;
        int32_t exitCode = cliClient.terminal(command);
        exit(exitCode);
      }
#ifndef NO_SCRIPTENGINE
      else if (arg == "-rse") {
        loadSettings();
        initGnuTls();
        setLimits();
        BaseLib::ProcessManager::startSignalHandler(GD::bl->threadManager);
        GD::licensingController.reset(new LicensingController());
        GD::licensingController->loadModules();
        GD::licensingController->init();
        GD::licensingController->load();
        ScriptEngine::ScriptEngineClient scriptEngineClient;
        scriptEngineClient.start();
        GD::licensingController->dispose();
        BaseLib::ProcessManager::stopSignalHandler(GD::bl->threadManager);
        exit(0);
      }
#endif
      else if (arg == "-n") {
        if (i + 1 < argc) {
          int newArgc = argc - i;
          char *newArgv[newArgc];
          newArgv[0] = argv[0];
          for (int j = i + 1; j < argc; j++) {
            newArgv[j - i] = argv[j];
          }
          auto exitCode = Nodejs::run(newArgc, newArgv);
          exit(exitCode);
        } else {
          std::cerr << "Invalid number of arguments." << std::endl;
          exit(1);
        }
      } else if (arg.size() > 3 && arg.compare(arg.size() - 3, 3, ".js") == 0 && BaseLib::Io::fileExists(arg)) {
        int newArgc = (argc - i) + 1;
        char *newArgv[newArgc];
        newArgv[0] = argv[0];
        for (int j = i; j < argc; j++) {
          newArgv[(j - i) + 1] = argv[j];
        }
        auto exitCode = Nodejs::run(newArgc, newArgv);
        exit(exitCode);
      } else if (arg == "-rl") {
        loadSettings();
        initGnuTls();
        setLimits();
        BaseLib::ProcessManager::startSignalHandler(GD::bl->threadManager);
        GD::licensingController = std::make_unique<LicensingController>();
        GD::licensingController->loadModules();
        GD::licensingController->init();
        GD::licensingController->load();
        NodeBlue::NodeBlueClient nodeBlueClient;
        nodeBlueClient.start();
        GD::licensingController->dispose();
        BaseLib::ProcessManager::stopSignalHandler(GD::bl->threadManager);
        exit(0);
      } else if (arg == "-e") {
        loadSettings(true);
        GD::bl->debugLevel = 0; //Disable output messages
        std::stringstream command;
        if (i + 1 < argc) {
          command << std::string(argv[i + 1]);
        } else {
          printHelp();
          exit(1);
        }

        for (int32_t j = i + 2; j < argc; j++) {
          std::string element(argv[j]);
          if (element.find(' ') != std::string::npos) command << " \"" << element << "\"";
          else command << " " << argv[j];
        }

        CliClient cliClient(GD::bl->settings.socketPath() + "homegearIPC.sock");
        std::string commandString = command.str();
        int32_t exitCode = cliClient.terminal(commandString);
        exit(exitCode);
      } else if (arg == "-tc") {
        GD::bl->threadManager.testMaxThreadCount();
        std::cout << GD::bl->threadManager.getMaxThreadCount() << std::endl;
        exit(0);
      } else if (arg == "-l") {
        loadSettings();
        GD::bl->debugLevel = 0; //Only output warnings.
        std::string command = "lifetick";
        CliClient cliClient(GD::bl->settings.socketPath() + "homegearIPC.sock");
        int32_t exitCode = cliClient.terminal(command);
        exit(exitCode);
      } else if (arg == "-i") {
        loadSettings();
        GD::bl->debugLevel = 3; //Only output warnings.
        std::cout << BaseLib::HelperFunctions::getTimeUuid() << std::endl;
        exit(0);
      } else if (arg == "-pre") {
        loadSettings();
        GD::serverInfo.init(GD::bl.get());
        GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
        if (GD::runAsUser.empty()) GD::runAsUser = GD::bl->settings.runAsUser();
        if (GD::runAsGroup.empty()) GD::runAsGroup = GD::bl->settings.runAsGroup();
        if ((!GD::runAsUser.empty() && GD::runAsGroup.empty()) || (!GD::runAsGroup.empty() && GD::runAsUser.empty())) {
          GD::out.printCritical("Critical: You only provided a user OR a group for Homegear to run as. Please specify both.");
          exit(1);
        }
        if (GD::runAsUser.empty() || GD::runAsGroup.empty()) {
          GD::out.printInfo("Info: Not setting permissions as user and group are not specified.");
          exit(0);
        }
        uid_t userId = GD::bl->hf.userId(GD::runAsUser);
        gid_t groupId = GD::bl->hf.groupId(GD::runAsGroup);
        std::string currentPath;
        if (!GD::pidfilePath.empty() && GD::pidfilePath.find('/') != std::string::npos) {
          currentPath = GD::pidfilePath.substr(0, GD::pidfilePath.find_last_of('/'));
          if (!currentPath.empty()) {
            if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
            if (chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set owner on " << currentPath << std::endl;
            if (chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
          }
        }

        currentPath = GD::bl->settings.dataPath();
        if (!currentPath.empty() && currentPath != GD::executablePath) {
          uid_t localUserId = GD::bl->hf.userId(GD::bl->settings.dataPathUser());
          gid_t localGroupId = GD::bl->hf.groupId(GD::bl->settings.dataPathGroup());
          if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
            localUserId = userId;
            localGroupId = groupId;
          }
          if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, GD::bl->settings.dataPathPermissions());
          if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set owner on " << currentPath << std::endl;
          if (chmod(currentPath.c_str(), GD::bl->settings.dataPathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
          std::vector<std::string> subdirs = GD::bl->io.getDirectories(currentPath, true);
          for (std::vector<std::string>::iterator j = subdirs.begin(); j != subdirs.end(); ++j) {
            std::string subdir = currentPath + *j;
            if (subdir != GD::bl->settings.scriptPath() && subdir != GD::bl->settings.nodeBluePath() && subdir != GD::bl->settings.adminUiPath() && subdir != GD::bl->settings.uiPath() && subdir != GD::bl->settings.nodeBlueDataPath()
                && subdir != GD::bl->settings.socketPath() && subdir != GD::bl->settings.modulePath() && subdir != GD::bl->settings.logfilePath()) {
              if (chown(subdir.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set owner on " << subdir << std::endl;
            }
            std::vector<std::string> files = GD::bl->io.getFiles(subdir, false);
            for (std::vector<std::string>::iterator k = files.begin(); k != files.end(); ++k) {
              std::string file = subdir + *k;
              if (chown(file.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set owner on " << file << std::endl;
            }
          }
          for (std::vector<std::string>::iterator j = subdirs.begin(); j != subdirs.end(); ++j) {
            std::string subdir = currentPath + *j;
            if (subdir == GD::bl->settings.scriptPath() || subdir == GD::bl->settings.nodeBluePath() || subdir == GD::bl->settings.adminUiPath() || subdir == GD::bl->settings.uiPath() || subdir == GD::bl->settings.nodeBlueDataPath()
                || subdir == GD::bl->settings.socketPath() || subdir == GD::bl->settings.modulePath() || subdir == GD::bl->settings.logfilePath())
              continue;
            if (chmod(subdir.c_str(), GD::bl->settings.dataPathPermissions()) == -1) std::cerr << "Could not set permissions on " << subdir << std::endl;
          }
        }

        std::string databasePath = (GD::bl->settings.databasePath().empty() ? GD::bl->settings.dataPath() : GD::bl->settings.databasePath()) + "db.sql";
        if (BaseLib::Io::fileExists(databasePath)) {
          if (chmod(databasePath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP) == -1) std::cerr << "Could not set permissions on " << databasePath << std::endl;
        }

        currentPath = GD::bl->settings.scriptPath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        uid_t localUserId = GD::bl->hf.userId(GD::bl->settings.scriptPathUser());
        gid_t localGroupId = GD::bl->hf.groupId(GD::bl->settings.scriptPathGroup());
        if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
          localUserId = userId;
          localGroupId = groupId;
        }
        if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), GD::bl->settings.scriptPathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

        currentPath = GD::bl->settings.nodeBluePath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        localUserId = GD::bl->hf.userId(GD::bl->settings.nodeBluePathUser());
        localGroupId = GD::bl->hf.groupId(GD::bl->settings.nodeBluePathGroup());
        if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
          localUserId = userId;
          localGroupId = groupId;
        }
        if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), GD::bl->settings.nodeBluePathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

        currentPath = GD::bl->settings.nodeBluePath() + "nodes/";
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        localUserId = GD::bl->hf.userId(GD::bl->settings.nodeBluePathUser());
        localGroupId = GD::bl->hf.groupId(GD::bl->settings.nodeBluePathGroup());
        if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
          localUserId = userId;
          localGroupId = groupId;
        }
        if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), GD::bl->settings.nodeBluePathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

        currentPath = GD::bl->settings.nodeBlueDataPath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        localUserId = GD::bl->hf.userId(GD::bl->settings.nodeBluePathUser());
        localGroupId = GD::bl->hf.groupId(GD::bl->settings.nodeBluePathGroup());
        if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
          localUserId = userId;
          localGroupId = groupId;
        }
        if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), GD::bl->settings.nodeBluePathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

        currentPath = GD::bl->settings.adminUiPath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        localUserId = GD::bl->hf.userId(GD::bl->settings.adminUiPathUser());
        localGroupId = GD::bl->hf.groupId(GD::bl->settings.adminUiPathGroup());
        if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
          localUserId = userId;
          localGroupId = groupId;
        }
        if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), GD::bl->settings.adminUiPathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

        currentPath = GD::bl->settings.uiPath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
        localUserId = GD::bl->hf.userId(GD::bl->settings.uiPathUser());
        localGroupId = GD::bl->hf.groupId(GD::bl->settings.uiPathGroup());
        if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) {
          localUserId = userId;
          localGroupId = groupId;
        }
        if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), GD::bl->settings.uiPathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;

        if (GD::bl->settings.socketPath() != GD::bl->settings.dataPath() && GD::bl->settings.socketPath() != GD::executablePath) {
          currentPath = GD::bl->settings.socketPath();
          if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
          if (chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
          if (chmod(currentPath.c_str(), S_IRWXU | S_IRWXG) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        }

        if (GD::bl->settings.lockFilePath() != GD::bl->settings.dataPath() && GD::bl->settings.lockFilePath() != GD::executablePath) {
          uid_t localUserId = GD::bl->hf.userId(GD::bl->settings.lockFilePathUser());
          gid_t localGroupId = GD::bl->hf.groupId(GD::bl->settings.lockFilePathGroup());
          currentPath = GD::bl->settings.lockFilePath();
          if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRWXG);
          if (((int32_t)localUserId) != -1 && ((int32_t)localGroupId) != -1) { if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl; }
          if (GD::bl->settings.lockFilePathPermissions() != 0) { if (chmod(currentPath.c_str(), GD::bl->settings.lockFilePathPermissions()) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl; }
        }

        currentPath = GD::bl->settings.modulePath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP);
        if (chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        std::vector<std::string> files = GD::bl->io.getFiles(currentPath, false);
        for (std::vector<std::string>::iterator j = files.begin(); j != files.end(); ++j) {
          std::string file = currentPath + *j;
          if (chown(file.c_str(), userId, groupId) == -1) std::cerr << "Could not set owner on " << file << std::endl;
        }

        currentPath = GD::bl->settings.logfilePath();
        if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, S_IRWXU | S_IRGRP | S_IXGRP);
        if (chown(currentPath.c_str(), userId, groupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        if (chmod(currentPath.c_str(), S_IRWXU | S_IRGRP | S_IXGRP) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        files = GD::bl->io.getFiles(currentPath, false);
        for (std::vector<std::string>::iterator j = files.begin(); j != files.end(); ++j) {
          std::string file = currentPath + *j;
          if (chown(file.c_str(), userId, groupId) == -1) std::cerr << "Could not set owner on " << file << std::endl;
          if (chmod(file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP) == -1) std::cerr << "Could not set permissions on " << file << std::endl;
        }

        for (int32_t i = 0; i < GD::serverInfo.count(); i++) {
          BaseLib::Rpc::PServerInfo settings = GD::serverInfo.get(i);
          if (settings->contentPathUser.empty() || settings->contentPathGroup.empty()) continue;
          uid_t localUserId = GD::bl->hf.userId(settings->contentPathUser);
          gid_t localGroupId = GD::bl->hf.groupId(settings->contentPathGroup);
          if (((int32_t)localUserId) == -1 || ((int32_t)localGroupId) == -1) continue;
          currentPath = settings->contentPath;
          if (!BaseLib::Io::directoryExists(currentPath)) BaseLib::Io::createDirectory(currentPath, settings->contentPathPermissions);
          if (chown(currentPath.c_str(), localUserId, localGroupId) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
          if (chmod(currentPath.c_str(), settings->contentPathPermissions) == -1) std::cerr << "Could not set permissions on " << currentPath << std::endl;
        }

        exit(0);
      } else if (arg == "-v") {
#ifdef NODE_BLUE
        std::cout << "Node-BLUE version " << GD::baseLibVersion << std::endl;
#else
        std::cout << "Homegear version " << GD::baseLibVersion << std::endl;
#endif
        std::cout << "Copyright (c) 2013-2020 Homegear GmbH" << std::endl << std::endl;
        std::cout << "Required library versions:" << std::endl;
        std::cout << "  - libhomegear-base: " << GD::baseLibVersion << std::endl;
        std::cout << "  - libhomegear-node: " << GD::nodeLibVersion << std::endl;
        std::cout << "  - libhomegear-ipc:  " << GD::ipcLibVersion << std::endl << std::endl;
        std::cout << "Included open source software:" << std::endl;
        std::cout << "  - Node.js (license: MIT License, homepage: nodejs.org)" << std::endl;
        std::cout << "  - PHP (license: PHP License, homepage: www.php.net):" << std::endl;
        std::cout << "      This product includes PHP software, freely available from <http://www.php.net/software/>" << std::endl;
        std::cout << "      Copyright (c) 1999-2020 The PHP Group. All rights reserved." << std::endl << std::endl;

        exit(0);
      } else {
        printHelp();
        exit(1);
      }
    }

    if (!isatty(STDIN_FILENO)) _nonInteractive = true;

    {
      //Block the signals below during start up
      //Needs to be called after initialization of GD::bl as GD::bl reads the current (default) signal mask.
      sigset_t set{};
      sigemptyset(&set);
      sigaddset(&set, SIGHUP);
      sigaddset(&set, SIGTERM);
      sigaddset(&set, SIGINT);
      sigaddset(&set, SIGABRT);
      sigaddset(&set, SIGSEGV);
      sigaddset(&set, SIGQUIT);
      sigaddset(&set, SIGILL);
      sigaddset(&set, SIGFPE);
      sigaddset(&set, SIGALRM);
      sigaddset(&set, SIGUSR1);
      sigaddset(&set, SIGUSR2);
      sigaddset(&set, SIGTSTP);
      sigaddset(&set, SIGTTIN);
      sigaddset(&set, SIGTTOU);
      if (pthread_sigmask(SIG_BLOCK, &set, nullptr) < 0) {
        std::cerr << "SIG_BLOCK error." << std::endl;
        exit(1);
      }
    }

    // {{{ Load settings
    GD::out.printInfo("Loading settings from " + GD::configPath + "main.conf");
    loadSettings();
    if (GD::runAsUser.empty()) GD::runAsUser = GD::bl->settings.runAsUser();
    if (GD::runAsGroup.empty()) GD::runAsGroup = GD::bl->settings.runAsGroup();
    if ((!GD::runAsUser.empty() && GD::runAsGroup.empty()) || (!GD::runAsGroup.empty() && GD::runAsUser.empty())) {
      GD::out.printCritical("Critical: You only provided a user OR a group for Homegear to run as. Please specify both.");
      exitHomegear(1);
    }
    GD::bl->userId = GD::bl->hf.userId(GD::runAsUser);
    GD::bl->groupId = GD::bl->hf.groupId(GD::runAsGroup);
    if ((int32_t)GD::bl->userId == -1 || (int32_t)GD::bl->groupId == -1) {
      GD::bl->userId = 0;
      GD::bl->groupId = 0;
    }

    GD::out.printInfo("Loading RPC server settings from " + GD::bl->settings.serverSettingsPath());
    GD::serverInfo.init(GD::bl.get());
    GD::serverInfo.load(GD::bl->settings.serverSettingsPath());
    GD::out.printInfo("Loading RPC client settings from " + GD::bl->settings.clientSettingsPath());
    GD::clientSettings.load(GD::bl->settings.clientSettingsPath());
    GD::mqtt.reset(new Mqtt());
    // }}}

    if ((chdir(GD::bl->settings.workingDirectory().c_str())) < 0) {
      GD::out.printError("Could not change working directory to " + GD::bl->settings.workingDirectory() + ".");
      exitHomegear(1);
    }

    GD::licensingController.reset(new LicensingController());
    GD::familyController.reset(new FamilyController());
    GD::familyServer.reset(new FamilyServer());
    GD::bl->db.reset(new DatabaseController());
    GD::rpcClient.reset(new Rpc::Client());

    if (_startAsDaemon) startDaemon();
    startUp();

    GD::bl->threadManager.join(_signalHandlerThread);
    return 0;
  }
  catch (const std::exception &ex) {
    GD::out.printEx(__FILE__, __LINE__, __PRETTY_FUNCTION__, ex.what());
  }
  _exit(1);
}
