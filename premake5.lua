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


local function demo(prj_name, prj_files)
  project(prj_name)
    kind "ConsoleApp"
    language "C++"

    files(prj_files)
    includedirs {
      "src",
      path.join(chromium_base_dir, "src"),
      "deps/libev",
    }
    libdirs {
      "builddir",
      path.join(chromium_base_dir, "builddir"),
    }

    filter "system:windows"
      links { "nev", "chromium_base", "ws2_32", "winmm" }
      linkoptions { "-Wall -static -static-libgcc -static-libstdc++" }
end


demo("client", { "demos/client.cc" })
demo("server", { "demos/server.cc" })
