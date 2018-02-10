cmake_minimum_required (VERSION 3.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang)
project (Onyx-Lang)
include_directories(src/variadic-variant)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Wshadow -ferror-limit=2")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -std=c++11 -Wall -Wshadow -ferror-limit=2")
add_executable (onyx
  src/Common.cpp
  src/Token.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/ParseTreeOutput.cpp
  src/Meta.cpp
  src/Variable.cpp
  src/Expression.cpp
  src/TypeSystem.cpp
  src/Subroutine.cpp
  src/Scope.cpp
  src/Subroutine.cpp
  src/C_Backend.cpp
  src/main.cpp
  src/Options.cpp
)

