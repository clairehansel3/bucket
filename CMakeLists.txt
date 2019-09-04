cmake_minimum_required(VERSION 3.13.1 FATAL_ERROR)
project(bucket VERSION 0.0.0 LANGUAGES CXX)

find_package(Boost 1.66 COMPONENTS)

add_library(bucket code/source_file.cxx code/token.cxx code/lexer.cxx)
set_target_properties(bucket PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(bucket PRIVATE code)

include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(bucket PRIVATE ${Boost_LIBRARIES})

add_executable(highlight_letter_e tests/highlight_letter_e.cxx)
set_target_properties(highlight_letter_e PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(highlight_letter_e PRIVATE code)
target_link_libraries(highlight_letter_e bucket)

add_executable(highlight_first_letter_e tests/highlight_first_letter_e.cxx)
set_target_properties(highlight_first_letter_e PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(highlight_first_letter_e PRIVATE code)
target_link_libraries(highlight_first_letter_e bucket)

add_executable(highlight_between_first_two_es tests/highlight_between_first_two_es.cxx)
set_target_properties(highlight_between_first_two_es PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(highlight_between_first_two_es PRIVATE code)
target_link_libraries(highlight_between_first_two_es bucket)

add_executable(lex tests/lex.cxx)
set_target_properties(lex PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(lex PRIVATE code)
target_link_libraries(lex bucket)

if (CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DBUCKET_DEBUG)
endif()
