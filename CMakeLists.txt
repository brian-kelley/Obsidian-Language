cmake_minimum_required (VERSION 3.0)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10)
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk)
project (Onyx-Lang)
include_directories(src/variadic-variant)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=gnu++11 -Wall -Wshadow")
add_executable (onyx
  src/main.cpp
  src/Utils.cpp
  src/Options.cpp
  src/Token.cpp
  src/MiddleEnd.cpp
  src/MiddleEndDebug.cpp
  src/Variable.cpp
  src/Expression.cpp
  src/TypeSystem.cpp
  src/Subroutine.cpp
  src/Scope.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/CGen.cpp
  src/AST_Printer.cpp
  src/Subroutine.cpp
)

