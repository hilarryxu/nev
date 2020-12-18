project "nev_thrift_echo_server"
  kind "ConsoleApp"
  language "C++"

  files {
    "echo_server.cc",
    "gen-cpp/echo_constants.cpp",
    "gen-cpp/echo_types.cpp",
    "gen-cpp/Echo.cpp",
  }
  includedirs {
    _WORKING_DIR,
    path.join(_WORKING_DIR, "include"),
    path.join(_WORKING_DIR, "src"),
    path.join(chromium_base_dir, "src"),
    "gen-cpp",
  }
  libdirs {
    path.join(_WORKING_DIR, "builddir"),
    path.join(chromium_base_dir, "builddir"),
  }

  filter "system:windows"
    defines { "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
    links { "nev_thrift", "thrift", "nev", "chromium_base", "ws2_32", "winmm" }
    linkoptions { "-Wall -static -static-libgcc -static-libstdc++" }
