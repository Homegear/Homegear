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

   project "output"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Output/*.h", "./Libraries/Output/*.cpp" }
 
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
		 
   project "helperfunctions"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/HelperFunctions/*.h", "./Libraries/HelperFunctions/*.cpp" }
 
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
	  
   project "physicaldevices"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/PhysicalDevices/*.h", "./Libraries/PhysicalDevices/*.cpp" }
 
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
	  
   project "types"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Types/*.h", "./Libraries/Types/*.cpp" }
 
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
		 
   project "logicaldevices"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/LogicalDevices/*.h", "./Libraries/LogicalDevices/*.cpp" }
 
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
		 
   project "threads"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Threads/*.h", "./Libraries/Threads/*.cpp" }
 
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

   project "filedescriptormanager"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/FileDescriptorManager/*.h", "./Libraries/FileDescriptorManager/*.cpp" }
 
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

   project "encoding"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Encoding/*.h", "./Libraries/Encoding/*.cpp" }
 
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

   project "settings"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Settings/*.h", "./Libraries/Settings/*.cpp" }
 
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
   
   project "metadata"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Metadata/*.h", "./Libraries/Metadata/*.cpp" }
 
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
		 
   project "bidcos"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Systems/HomeMaticBidCoS/*.h", "./Libraries/Systems/HomeMaticBidCoS/*.cpp" }
	  files { "./Libraries/Systems/HomeMaticBidCoS/Devices/*.h", "./Libraries/Systems/HomeMaticBidCoS/Devices/*.cpp" }
	  files { "./Libraries/Systems/HomeMaticBidCoS/PhysicalDevices/*.h", "./Libraries/Systems/HomeMaticBidCoS/PhysicalDevices/*.cpp" }
 
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
		 
   project "hmwired"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Systems/HomeMaticWired/*.h", "./Libraries/Systems/HomeMaticWired/*.cpp" }
	  files { "./Libraries/Systems/HomeMaticWired/Devices/*.h", "./Libraries/Systems/HomeMaticWired/Devices/*.cpp" }
	  files { "./Libraries/Systems/HomeMaticWired/PhysicalDevices/*.h", "./Libraries/Systems/HomeMaticWired/PhysicalDevices/*.cpp" }
 
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
		 
   project "insteon"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Systems/Insteon/*.h", "./Libraries/Systems/Insteon/*.cpp" }
	  files { "./Libraries/Systems/Insteon/Devices/*.h", "./Libraries/Systems/Insteon/Devices/*.cpp" }
	  files { "./Libraries/Systems/Insteon/PhysicalDevices/*.h", "./Libraries/Systems/Insteon/PhysicalDevices/*.cpp" }
 
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
   
   project "fs20"
      kind "StaticLib"
      language "C++"
      files { "./Libraries/Systems/HomeMaticWired/*.h", "./Libraries/Systems/FS20/*.cpp" }
	  files { "./Libraries/Systems/HomeMaticWired/Devices/*.h", "./Libraries/Systems/FS20/Devices/*.cpp" }
	  files { "./Libraries/Systems/HomeMaticWired/PhysicalDevices/*.h", "./Libraries/Systems/FS20/PhysicalDevices/*.cpp" }
 
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
	  files { "./Libraries/Systems/General/*.h", "./Libraries/Systems/General/*.cpp" }
      linkoptions { "-l bidcos", "-l hmwired", "-l insteon", "-l fs20", "-l rpc", "-l pthread", "-l sqlite3", "-l readline", "-l ssl", "-l output", "-l helperfunctions", "-l physicaldevices", "-l types", "-l logicaldevices", "-l threads", "-l database", "-l filedescriptormanager", "-l encoding", "-l user", "-l settings", "-l metadata", "-l cli", "-l events", "-l gd" }
 
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
