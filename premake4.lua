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
        cppflags = "-MMD -D_GLIBCXX_USE_NANOSLEEP"
    }
}

-- A solution contains projects, and defines the available configurations
solution "Homegear"
   configurations { "Debug", "Release" }
 
   configuration { "linux", "gmake" }
      buildoptions { "-std=c++11" }
      linkoptions { "-l pthread", "-l sqlite3" }

   configuration { "rpi", "gmake" }
      buildoptions { "-std=c++11" }
      linkoptions { "-l pthread", "-l sqlite3" }
      includedirs { "./ARM\ headers" }
      libdirs { "./ARM\ libraries" }

   -- A project defines one build target
   project "Homegear"
      kind "ConsoleApp"
      language "C++"
      files { "*.h", "*.cpp" }
      files { "./Devices/*.h", "./Devices/*.cpp" }
      files { "./RPC/*.h", "./RPC/*.cpp" }
 
      configuration "Debug"
         defines { "DEBUG" }
         flags { "Symbols" }
         targetdir "bin/Debug"
 
      configuration "Release"
         defines { "NDEBUG" }
         flags { "Optimize" }
         targetdir "bin/Release"
