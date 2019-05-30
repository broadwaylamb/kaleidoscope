include(GoogleTest)
add_subdirectory(googletest)

macro(package_add_test TESTNAME)
    add_executable(${TESTNAME} ${ARGN})
    target_link_libraries(${TESTNAME} gtest gtest_main kaleidoscope)
    gtest_discover_tests(${TESTNAME})
endmacro()

package_add_test(LexerTests LexerTests.cpp)