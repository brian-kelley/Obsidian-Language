set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DONYX_TESTING")

set(ONYX_SRC "${PROJECT_SOURCE_DIR}/src")

add_executable(Driver GeneralTestDriver.cpp ../src/Utils.cpp)
add_executable(LexFuzz LexerFuzzing.cpp ../src/Utils.cpp)
add_executable(UtilUnitTests UtilUnitTests.cpp ../src/Utils.cpp)

function(createTest name)
  configure_file("${name}.os" "${CMAKE_CURRENT_BINARY_DIR}/${name}.os" COPYONLY)
  configure_file("${name}.gold" "${CMAKE_CURRENT_BINARY_DIR}/${name}.gold" COPYONLY)
  add_test(${name} Driver ${name})
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
createTest("Assertions")
createTest("ShadowingErrorParam")
createTest("ShadowingErrorLocal")
createTest("ShadowingModuleOK")
createTest("Structs")
createTest("Tuples")
createTest("Overloading")
createTest("Using")
createTest("Casting")
createTest("UnionConversion")
createTest("FuncPatternMatching")

add_test(LexFuzzAll LexFuzz "--all")
add_test(LexFuzzASCII LexFuzz "--standard")
add_test(UtilUnitTests UtilUnitTests)

