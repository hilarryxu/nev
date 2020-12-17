chromium_base_dir = path.getabsolute("../chromium_base")

newoption {
  trigger = "no-demo",
  description = "Not build demos"
}

newoption {
  trigger = "no-example",
  description = "Not build examples"
}

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
      "include",
      path.join(chromium_base_dir, "src"),
      "deps/libev",
    }
    libdirs {
      path.join(chromium_base_dir, "builddir"),
    }

    filter { "system:windows", "toolset:gcc" }
      defines { "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      buildoptions {
        "-std=c++14",
        "-fno-rtti",
      }


local function demo(prj_name, prj_files)
  if not prj_files then
    prj_files = "demos/" .. prj_name .. ".cc"
  end

  project(prj_name)
    kind "ConsoleApp"
    language "C++"

    files(prj_files)
    includedirs {
      "include",
      path.join(chromium_base_dir, "src"),
    }
    libdirs {
      "builddir",
      path.join(chromium_base_dir, "builddir"),
    }

    filter "system:windows"
      defines { "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      links { "nev", "chromium_base", "ws2_32", "winmm" }
      linkoptions { "-Wall -static -static-libgcc -static-libstdc++" }
end


function example_project(prj_name, prj_files)
  project(prj_name)
    kind "ConsoleApp"
    language "C++"

    files(prj_files)
    includedirs {
      _WORKING_DIR,
      path.join(_WORKING_DIR, "include"),
      path.join(chromium_base_dir, "src"),
    }
    libdirs {
      path.join(_WORKING_DIR, "builddir"),
      path.join(chromium_base_dir, "builddir"),
    }

    filter "system:windows"
      defines { "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
      links { "nev", "chromium_base", "ws2_32", "winmm" }
      linkoptions { "-Wall -static -static-libgcc -static-libstdc++" }
end


if not _OPTIONS["no-demo"] then
  demo("test1")
  demo("test2")
  demo("test3")
  -- demo("test4")
  demo("test5")
  demo("test6")
  demo("test7")
  demo("test8")
  demo("test9")
  demo("test10")
  demo("test11")
  demo("nev_echo")
  -- demo("test12")
  demo("test13")
  demo("nev_httpd")
end

if not _OPTIONS["no-example"] then
  include("examples")
end
