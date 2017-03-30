cmake_minimum_required (VERSION 3.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk)
project (Obsidian-Lang)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Os -Wall")
add_executable (onyx
  src/main.cpp
  src/Utils.cpp
  src/Options.cpp
  src/Preprocess.cpp
  src/Token.cpp
  src/Scope.cpp
  src/Variable.cpp
  src/Type.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/CGen.cpp
)

