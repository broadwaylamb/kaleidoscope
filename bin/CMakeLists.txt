add_executable(kaleidoscopec main.cpp)

add_subdirectory(../lib ../lib)

target_link_libraries(kaleidoscopec kaleidoscope)

