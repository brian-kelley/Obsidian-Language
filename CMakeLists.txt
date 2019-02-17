cmake_minimum_required (VERSION 3.0)
#set(CMAKE_C_COMPILER clang)
#set(CMAKE_CXX_COMPILER clang++)
project (Onyx-Lang)
include_directories(src/variadic-variant)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wshadow")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wshadow")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -std=c++11 -Wall -Wshadow -ferror-limit=2")
add_executable (onyx
  src/Common.cpp
  src/SourceFile.cpp
  src/Token.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/AST.cpp
  src/Variable.cpp
  src/Expression.cpp
  src/TypeSystem.cpp
  src/Subroutine.cpp
  src/Scope.cpp
  src/Subroutine.cpp
  src/IR.cpp
  src/Dataflow.cpp
  src/CallGraph.cpp
  src/ConstantProp.cpp
  src/JumpThreading.cpp
  src/DeadCodeElim.cpp
  src/CSElim.cpp
  src/Inlining.cpp
  src/Dotfile.cpp
  src/BuiltIn.cpp
  src/main.cpp
  src/Options.cpp
  src/AST_Output.cpp
  src/IRDebug.cpp
)

#src/C_Backend.cpp
