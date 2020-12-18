project "nev_thrift"
  kind "StaticLib"
  language "C++"

  files { "*.cc" }
  includedirs {
    _WORKING_DIR,
    path.join(_WORKING_DIR, "include"),
    path.join(_WORKING_DIR, "src"),
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
