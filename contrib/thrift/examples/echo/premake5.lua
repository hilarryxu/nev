project "nev_thrift_echo_server"
  kind "ConsoleApp"
  language "C++"

  files {
    "echo_server.cc",
    "gen-cpp/*.cpp",
  }
  removefiles {
    "gen-cpp/*.skeleton.cpp",
  }
  includedirs {
    _WORKING_DIR,
    path.join(_WORKING_DIR, "include"),
    path.join(chromium_base_dir, "src"),
    "gen-cpp",
  }
  libdirs {
    path.join(_WORKING_DIR, "builddir"),
    path.join(chromium_base_dir, "builddir"),
  }

  prebuildcommands {
    'thrift --gen cpp echo.thrift',
    'thrift --gen py echo.thrift',
  }

  filter "system:windows"
    defines { "MINGW_HAS_SECURE_API", "_POSIX_C_SOURCE" }
    links { "nev_thrift", "thrift", "nev", "chromium_base", "ws2_32", "winmm" }
    linkoptions { "-Wall -static -static-libgcc -static-libstdc++" }
