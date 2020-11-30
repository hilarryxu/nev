local chromium_base_dir = "../chromium_base"

workspace "sln-nev"
  objdir "builddir/obj"
  targetdir "builddir"
  libdirs { "builddir" }

  configurations { "Debug", "Release" }

  configuration "Debug"
     defines { "DEBUG" }
     symbols "On"

  configuration "Release"
     defines { "NDEBUG" }
     optimize "On"

  project "nev"
    kind "StaticLib"
    language "C++"
    files {
      "deps/libev/ev_standalone.cc",
      "src/**.cc",
    }
    includedirs {
      "src",
      path.join(chromium_base_dir, "src"),
      "deps/libev",
    }
    libdirs {
      path.join(chromium_base_dir, "builddir"),
    }

    filter { "system:windows", "toolset:gcc" }
      defines { "CRT_MINGW", "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      buildoptions { "-std=c++14", "-fno-rtti" }

    filter "system:linux"
      removefiles {
        "src/nev/winsock_init.cc"
      }
      buildoptions { "-std=c++14", "-fno-rtti" }
