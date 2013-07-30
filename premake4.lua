-- A solution contains projects, and defines the available configurations
solution "Homegear"
   configurations { "Debug", "Release" }
 
   configuration { "linux", "gmake" }
      buildoptions { "-std=c++11" }
      linkoptions { "-l pthread", "-l sqlite3", "-l ncurses" }

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
