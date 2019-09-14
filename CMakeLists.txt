cmake_minimum_required (VERSION 3.0)

include_directories(src)
include_directories(src/variadic-variant)

if (WIN32)
	set(CMAKE_C_COMPILER cl)
	set(CMAKE_CXX_COMPILER cl)
elseif (UNIX)
  #set(CMAKE_C_COMPILER clang)
  #set(CMAKE_CXX_COMPILER clang++)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wshadow")
endif()

enable_testing()

project (Onyx-Lang)

add_executable (onyx
  src/Common.cpp
  src/SourceFile.cpp src/Token.cpp
  src/Lexer.cpp
  src/Parser.cpp
  src/AST.cpp
  src/Variable.cpp
  src/Expression.cpp
  src/TypeSystem.cpp
  src/Subroutine.cpp
  src/Scope.cpp
  src/Subroutine.cpp
  src/AstToIR.cpp
  src/AstInterpreter.cpp
  src/Dotfile.cpp
  src/BuiltIn.cpp
  src/main.cpp
  src/Options.cpp
  src/AST_Output.cpp
)

#  src/Inlining.cpp
#  src/IRDebug.cpp
#  src/ConstantProp.cpp
#  src/JumpThreading.cpp
#  src/DeadCodeElim.cpp
#  src/Dataflow.cpp
#  src/CallGraph.cpp
#  src/Memory.cpp
#  src/CSElim.cpp
#  src/IR.cpp

add_subdirectory(test)
