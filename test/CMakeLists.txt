set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DONYX_TESTING")

set(ONYX_SRC "${PROJECT_SOURCE_DIR}/src")

add_executable (onyx-tests
  tests.cpp
  ${ONYX_SRC}/Common.cpp
  ${ONYX_SRC}/SourceFile.cpp
  ${ONYX_SRC}/Token.cpp
  ${ONYX_SRC}/Lexer.cpp
  ${ONYX_SRC}/Parser.cpp
  ${ONYX_SRC}/AST.cpp
  ${ONYX_SRC}/Variable.cpp
  ${ONYX_SRC}/Expression.cpp
  ${ONYX_SRC}/TypeSystem.cpp
  ${ONYX_SRC}/Subroutine.cpp
  ${ONYX_SRC}/Scope.cpp
  ${ONYX_SRC}/Subroutine.cpp
  ${ONYX_SRC}/AstToIR.cpp
  ${ONYX_SRC}/AstInterpreter.cpp
  ${ONYX_SRC}/Dotfile.cpp
  ${ONYX_SRC}/BuiltIn.cpp
  ${ONYX_SRC}/Options.cpp
  ${ONYX_SRC}/AST_Output.cpp
  )

function(createTest name)
  configure_file("${name}.os" "${CMAKE_CURRENT_BINARY_DIR}/${name}.os" COPYONLY)
  configure_file("${name}.gold" "${CMAKE_CURRENT_BINARY_DIR}/${name}.gold" COPYONLY)
  add_test(${name} onyx-tests ${name})
endfunction(createTest)

createTest("HelloWorld")
createTest("Arithmetic")
createTest("Primes")
createTest("IterStringArray")
createTest("IterMultidim")
createTest("CStyleLoops")
createTest("RecursiveFibonacci")
createTest("Sierpinski")
createTest("Sorting")
createTest("Unions")
createTest("Switch")

