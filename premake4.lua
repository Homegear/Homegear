-- create Makefile with "./premake4 --platform=rpi gmake", "./premake4 --platform=armel gmake" or "./premake4 gmake"

function newplatform(plf)
    local name = plf.name
    local description = plf.description
    local crosscompiler

    -- Register new platform
    premake.platforms[name] = {
        cfgsuffix = "_"..name,
        iscrosscompiler = crosscompiler
    }

    -- Allow use of new platform in --platfroms
    table.insert(premake.option.list["platform"].allowed, { name, description })
    table.insert(premake.fields.platforms.allowed, name)

    -- Add compiler support
    -- gcc
    if plf.gcc then
        premake.gcc.platforms[name] = plf.gcc
    end
    --other compilers (?)
end

newplatform {
    name = "rpi",
    description = "Raspberry Pi",
    iscrosscompiler = true,
    gcc = {
        cc = "arm-linux-gnueabihf-gcc",
        cxx = "arm-linux-gnueabihf-g++",
        cppflags = "-MMD -D_GLIBCXX_USE_NANOSLEEP -D_FORTIFY_SOURCE=2"
    }
}

newplatform {
    name = "armel",
    description = "Compile without script engine and event handler, because std::future is not supported on armel.",
    -- Needs to be "true" to be able to differentiate between "native" and "armel"
    iscrosscompiler = true,
    -- "iscrosscompiler" does not work without providing gcc
    gcc = {
        cc = "gcc",
        cxx = "g++",
        cppflags = ""
    }
}

solution "homegear"
   configurations { "Release", "Debug", "Profiling" }

   configuration { "native", "linux", "gmake" }
      --GCRYPT_NO_DEPRECATED only works after modifying the header file. See: http://lists.gnupg.org/pipermail/gcrypt-devel/2011-September/001844.html
      defines
      {
         "FORTIFY_SOURCE=2",
         "GCRYPT_NO_DEPRECATED",
         "SCRIPTENGINE",
         "EVENTHANDLER"
         --"BIDCOSTICC1101",
         --"BIDCOSRTLSDRLAN",
      }
      linkoptions { "-Wl,-rpath=/lib/homegear", "-Wl,-rpath=/usr/lib/homegear" }
      libdirs { "/usr/lib/php5" }

   configuration { "rpi", "gmake" }
      includedirs { "./Includes/ARM\ headers" }
      libdirs { "./ARM\ libraries" }

   --armel does not support std::future which is used in script engine and event handler
   configuration { "armel", "gmake" }
      --GCRYPT_NO_DEPRECATED only works after modifying the header file. See: http://lists.gnupg.org/pipermail/gcrypt-devel/2011-September/001844.html
      defines
      {
         "FORTIFY_SOURCE=2",
         "GCRYPT_NO_DEPRECATED",
      }
      linkoptions { "-Wl,-rpath=/lib/homegear", "-Wl,-rpath=/usr/lib/homegear" }

   project "base"
      kind "StaticLib"
      language "C++"
      files
      { 
        "./Modules/Base/*h", "./Modules/Base/*.cpp",
        "./Modules/Base/HelperFunctions/*.h", "./Modules/Base/HelperFunctions/*.cpp",
        "./Modules/Base/Output/*.h", "./Modules/Base/Output/*.cpp",
        "./Modules/Base/RPC/*.h", "./Modules/Base/RPC/*.cpp",
        "./Modules/Base/Systems/*.h", "./Modules/Base/Systems/*.cpp",
        "./Modules/Base/Managers/*.h", "./Modules/Base/Managers/*.cpp",
        "./Modules/Base/Encoding/*.h", "./Modules/Base/Encoding/*.cpp", "./Modules/Base/Encoding/RapidXml/*.h", "./Modules/Base/Encoding/RapidJSON/*.h",
        "./Modules/Base/Database/*.h", "./Modules/Base/Database/*.cpp",
        "./Modules/Base/Sockets/*.h", "./Modules/Base/Sockets/*.cpp",
        "./Modules/Base/Threads/*.h", "./Modules/Base/Threads/*.cpp",
        "./Modules/Base/Settings/*.h", "./Modules/Base/Settings/*.cpp",
      }
      buildoptions { "-Wall", "-std=c++11", "-fPIC" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

--[[
   project "homematicbidcos"
      kind "SharedLib"
      language "C++"
      files { "./Modules/HomeMaticBidCoS/*.h", "./Modules/HomeMaticBidCoS/*.cpp" }
      files { "./Modules/HomeMaticBidCoS/Devices/*.h", "./Modules/HomeMaticBidCoS/Devices/*.cpp" }
      files { "./Modules/HomeMaticBidCoS/PhysicalInterfaces/*.h", "./Modules/HomeMaticBidCoS/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
      buildoptions { "-Wall", "-std=c++11" }
	   
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         libdirs { "./lib/Debug" }
         targetdir "./lib/Modules/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         libdirs { "./lib/Release" }
         targetdir "./lib/Modules/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         libdirs { "./lib/Profiling" }
         targetdir "./lib/Modules/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

   project "homematicwired"
      kind "SharedLib"
      language "C++"
      files { "./Modules/HomeMaticWired/*.h", "./Modules/HomeMaticWired/*.cpp" }
      files { "./Modules/HomeMaticWired/Devices/*.h", "./Modules/HomeMaticWired/Devices/*.cpp" }
      files { "./Modules/HomeMaticWired/PhysicalInterfaces/*.h", "./Modules/HomeMaticWired/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
      buildoptions { "-Wall", "-std=c++11" }
	   
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         libdirs { "./lib/Debug" }
         targetdir "./lib/Modules/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         libdirs { "./lib/Release" }
         targetdir "./lib/Modules/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         libdirs { "./lib/Profiling" }
         targetdir "./lib/Modules/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

   project "max"
      kind "SharedLib"
      language "C++"
      files { "./Modules/MAX/*.h", "./Modules/MAX/*.cpp" }
      files { "./Modules/MAX/LogicalDevices/*.h", "./Modules/MAX/LogicalDevices/*.cpp" }
      files { "./Modules/MAX/PhysicalInterfaces/*.h", "./Modules/MAX/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
      buildoptions { "-Wall", "-std=c++11" }
      
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         libdirs { "./lib/Debug" }
         targetdir "./lib/Modules/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         libdirs { "./lib/Release" }
         targetdir "./lib/Modules/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         libdirs { "./lib/Profiling" }
         targetdir "./lib/Modules/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

   project "insteon"
      kind "SharedLib"
      language "C++"
      files { "./Modules/Insteon/*.h", "./Modules/Insteon/*.cpp" }
      files { "./Modules/Insteon/LogicalDevices/*.h", "./Modules/Insteon/LogicalDevices/*.cpp" }
      files { "./Modules/Insteon/PhysicalInterfaces/*.h", "./Modules/Insteon/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l base" }
      buildoptions { "-Wall", "-std=c++11" }
	   
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         libdirs { "./lib/Debug" }
         targetdir "./lib/Modules/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         libdirs { "./lib/Release" }
         targetdir "./lib/Modules/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         libdirs { "./lib/Profiling" }
         targetdir "./lib/Modules/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

   project "philipshue"
      kind "SharedLib"
      language "C++"
      files { "./Modules/PhilipsHue/*.h", "./Modules/PhilipsHue/*.cpp" }
      files { "./Modules/PhilipsHue/LogicalDevices/*.h", "./Modules/PhilipsHue/LogicalDevices/*.cpp" }
      files { "./Modules/PhilipsHue/PhysicalInterfaces/*.h", "./Modules/PhilipsHue/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l base" }
      buildoptions { "-Wall", "-std=c++11" }
	  
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         libdirs { "./lib/Debug" }
         targetdir "./lib/Modules/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         libdirs { "./lib/Release" }
         targetdir "./lib/Modules/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         libdirs { "./lib/Profiling" }
         targetdir "./lib/Modules/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
--]]

   project "miscellaneous"
      kind "SharedLib"
      language "C++"
      files { "./Modules/Miscellaneous/*.h", "./Modules/Miscellaneous/*.cpp" }
      files { "./Modules/Miscellaneous/LogicalDevices/*.h", "./Modules/Miscellaneous/LogicalDevices/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
      buildoptions { "-Wall", "-std=c++11" }
      
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         libdirs { "./lib/Debug" }
         targetdir "./lib/Modules/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         libdirs { "./lib/Release" }
         targetdir "./lib/Modules/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         libdirs { "./lib/Profiling" }
         targetdir "./lib/Modules/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

   project "user"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/User/*.h", "./Libraries/User/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
   
   project "rpc"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/RPC/*.h", "./Libraries/RPC/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
		 
   project "cli"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/CLI/*.h", "./Libraries/CLI/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
		 
   project "events"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Events/*.h", "./Libraries/Events/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
   
   project "database"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Database/*.h", "./Libraries/Database/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
      
   project "scriptengine"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/ScriptEngine/*.h", "./Libraries/ScriptEngine/*.cpp" }
      includedirs
      {
         "/usr/include/php5",
         "/usr/include/php5/main",
         "/usr/include/php5/sapi",
         "/usr/include/php5/TSRM",
         "/usr/include/php5/Zend"
      }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

    project "upnp"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/UPnP/*.h", "./Libraries/UPnP/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

    project "paho.mqtt.c"
      kind "StaticLib"
      language "C"
      files { "./Libraries/MQTT/paho.mqtt.c/src/*.h", "./Libraries/MQTT/paho.mqtt.c/src/*.c" }
      buildoptions { "-Wall" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

    project "mqtt"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/MQTT/*.h", "./Libraries/MQTT/*.cpp" }
      includedirs { "./Libraries/MQTT/paho.mqtt.c/src" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }

    project "gd"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/GD/*.h", "./Libraries/GD/*.cpp" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "./lib/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "./lib/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "./lib/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
   
   project "homegear"
      kind "ConsoleApp"
      language "C++"
      files { "*.h", "*.cpp" }
      files { "./Libraries/Systems/*.h", "./Libraries/Systems/*.cpp" }
      linkoptions { "-l rpc", "-l dl", "-l pthread", "-l readline", "-l gcrypt", "-l gnutls", "-l user", "-l cli", "-l events", "-l gd", "-l upnp", "-l mqtt", "-l paho.mqtt.c", "-l database", "-l scriptengine", "-l base", "-l gpg-error", "-l sqlite3", "-l php5" }
      buildoptions { "-Wall", "-std=c++11" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
		 libdirs { "./lib/Debug" }
         targetdir "bin/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
		 libdirs { "./lib/Release" }
         targetdir "bin/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
		 libdirs { "./lib/Profiling" }
         targetdir "bin/Profiling"
         buildoptions { "-pg" }
         linkoptions { "-pg" }
