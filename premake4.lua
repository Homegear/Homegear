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
      linkoptions { "-l pthread", "-l sqlite3", "-l readline", "-l ssl" }
      defines "FORTIFY_SOURCE=2"

   configuration { "rpi", "gmake" }
      buildoptions { "-std=c++11" }
      linkoptions { "-l pthread", "-l sqlite3", "-l readline", "-l ssl", "-l crypto" }
      includedirs { "./ARM\ headers" }
      libdirs { "./ARM\ libraries" }

   -- A project defines one build target
   project "homegear"
      kind "ConsoleApp"
      language "C++"
      files { "*.h", "*.cpp" }
      files { "./HomeMaticBidCoS/*.h", "./HomeMaticBidCoS/*.cpp" }
      files { "./HomeMaticBidCoS/Devices/*.h", "./HomeMaticBidCoS/Devices/*.cpp" }
      files { "./HomeMaticWired/*.h", "./HomeMaticWired/*.cpp" }
      files { "./RPC/*.h", "./RPC/*.cpp" }
      files { "./CLI/*.h", "./CLI/*.cpp" }
      files { "./PhysicalDevices/*.h", "./PhysicalDevices/*.cpp" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "bin/Release"

      configuration "Profiling"
         defines { "NDEBUG" }
         flags { "Optimize", "Symbols" }
         targetdir "bin/Profiling"
         buildoptions { "-std=c++11", "-pg" }
         linkoptions { "-pg" }
