-- create Makefile with "./premake4 --platform=rpi gmake" or "./premake4 gmake"

function newplatform(plf)
    local name = plf.name
    local description = plf.description
 
    -- Register new platform
    premake.platforms[name] = {
        cfgsuffix = "_"..name,
        iscrosscompiler = true
    }
 
    -- Allow use of new platform in --platfroms
    table.insert(premake.option.list["platform"].allowed, { name, description })
    table.insert(premake.fields.platforms.allowed, name)
 
    -- Add compiler support
    -- gcc
    premake.gcc.platforms[name] = plf.gcc
    --other compilers (?)
end

newplatform {
    name = "rpi",
    description = "Raspberry Pi",
    gcc = {
        cc = "arm-linux-gnueabihf-gcc",
        cxx = "arm-linux-gnueabihf-g++",
        cppflags = "-MMD -D_GLIBCXX_USE_NANOSLEEP -D_FORTIFY_SOURCE=2"
    }
}

-- A solution contains projects, and defines the available configurations
solution "homegear"
   configurations { "Debug", "Release", "Profiling" }
 
   configuration { "linux", "gmake" }
      buildoptions { "-std=c++11" }
      defines "FORTIFY_SOURCE=2"

   configuration { "rpi", "gmake" }
      buildoptions { "-std=c++11" }
      linkoptions { "-l crypto" }
      includedirs { "./ARM\ headers" }
      libdirs { "./ARM\ libraries" }

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
        "./Modules/Base/FileDescriptorManager/*.h", "./Modules/Base/FileDescriptorManager/*.cpp",
        "./Modules/Base/Encoding/*.h", "./Modules/Base/Encoding/*.cpp",
        "./Modules/Base/Database/*.h", "./Modules/Base/Database/*.cpp",
        "./Modules/Base/Sockets/*.h", "./Modules/Base/Sockets/*.cpp",
        "./Modules/Base/Threads/*.h", "./Modules/Base/Threads/*.cpp",
        "./Modules/Base/Settings/*.h", "./Modules/Base/Settings/*.cpp"
      }
      buildoptions { "-fPIC" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }

   project "homematicbidcos"
      kind "SharedLib"
      language "C++"
      files { "./Modules/HomeMaticBidCoS/*.h", "./Modules/HomeMaticBidCoS/*.cpp" }
      files { "./Modules/HomeMaticBidCoS/Devices/*.h", "./Modules/HomeMaticBidCoS/Devices/*.cpp" }
      files { "./Modules/HomeMaticBidCoS/PhysicalInterfaces/*.h", "./Modules/HomeMaticBidCoS/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
	   
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }

   project "homematicwired"
      kind "SharedLib"
      language "C++"
      files { "./Modules/HomeMaticWired/*.h", "./Modules/HomeMaticWired/*.cpp" }
      files { "./Modules/HomeMaticWired/Devices/*.h", "./Modules/HomeMaticWired/Devices/*.cpp" }
      files { "./Modules/HomeMaticWired/PhysicalInterfaces/*.h", "./Modules/HomeMaticWired/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
	   
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }

--[[
   project "max"
      kind "SharedLib"
      language "C++"
      files { "./Modules/MAX/*.h", "./Modules/MAX/*.cpp" }
      files { "./Modules/MAX/LogicalDevices/*.h", "./Modules/MAX/LogicalDevices/*.cpp" }
      files { "./Modules/MAX/PhysicalInterfaces/*.h", "./Modules/MAX/PhysicalInterfaces/*.cpp" }
      linkoptions { "-l pthread", "-l base" }
      
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
--]]
--[[
   project "insteon"
      kind "SharedLib"
      language "C++"
      files { "./Modules/Insteon/*.h", "./Modules/Insteon/*.cpp" }
      files { "./Modules/Insteon/Devices/*.h", "./Modules/Insteon/Devices/*.cpp" }
      files { "./Modules/Insteon/PhysicalDevices/*.h", "./Modules/Insteon/PhysicalDevices/*.cpp" }
      linkoptions { "-l base" }
	   
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
--]]

   project "user"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/User/*.h", "./Libraries/User/*.cpp" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
   
   project "rpc"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/RPC/*.h", "./Libraries/RPC/*.cpp" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
		 
   project "cli"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/CLI/*.h", "./Libraries/CLI/*.cpp" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
		 
   project "events"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Events/*.h", "./Libraries/Events/*.cpp" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
   
   project "database"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Database/*.h", "./Libraries/Database/*.cpp" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
		 
   project "gd"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/GD/*.h", "./Libraries/GD/*.cpp" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
   
   project "homegear"
      kind "ConsoleApp"
      language "C++"
      files { "*.h", "*.cpp" }
	  files { "./Libraries/Systems/*.h", "./Libraries/Systems/*.cpp" }
      linkoptions { "-l rpc", "-l dl", "-l pthread", "-l sqlite3", "-l readline", "-l ssl", "-l user", "-l cli", "-l events", "-l gd", "-l database", "-l base" }
 
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
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
