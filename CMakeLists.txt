# Basics
cmake_minimum_required(VERSION 3.10.2 FATAL_ERROR)
project(bucket VERSION 0.0.0 LANGUAGES C CXX)

# Find Packages
find_package(LLVM 7.0.0 REQUIRED)
find_package(Boost 1.66 COMPONENTS filesystem REQUIRED)

# Setup bucketrt
add_library(bucketrt SHARED
  code/runtime.c
)

set_target_properties(bucketrt PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_include_directories(bucketrt PRIVATE code)

# Setup Target
add_executable(bucket
  #code/ast.cxx
  #code/codegen.cxx
  #code/lexer.cxx
  code/main.cxx
  #code/parser.cxx
  code/source.cxx
  #code/symtable.cxx
  #code/token.cxx
)

# Target Details
set_target_properties(bucket PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_definitions(bucket PRIVATE ${LLVM_DEFINITIONS})
target_include_directories(bucket PRIVATE code)
target_include_directories(bucket SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})
llvm_map_components_to_libnames(bucket_LLVM_LIBRARIES core bitwriter)
target_link_libraries(bucket PRIVATE ${bucket_LLVM_LIBRARIES} ${Boost_LIBRARIES} ${Boost_FILESYSTEM_LIBRARY})
if (CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DBUCKET_DEBUG)
endif()

# Compiler Flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 6.0.0)
  message(STATUS "Detected clang 6.0.0 or greater")
  if(CMAKE_BUILD_TYPE MATCHES Production)
    target_compile_options(bucket PRIVATE -pedantic -Weverything -Wno-shadow-field-in-constructor -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-weak-vtables -Ofast -flto -fstrict-vtable-pointers -ffast-math -fwhole-program-vtables -march=native -mtune=native)
    target_compile_options(bucketrt PRIVATE -pedantic -Weverything -Wno-shadow-field-in-constructor -Wno-padded -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-weak-vtables -Ofast -flto -fstrict-vtable-pointers -ffast-math -fwhole-program-vtables -march=native -mtune=native)
  elseif(CMAKE_BUILD_TYPE MATCHES Debug)
    target_compile_options(bucket PRIVATE -g)
    target_compile_options(bucketrt PRIVATE -g)
    set_target_properties(bucket PROPERTIES LINK_FLAGS "-fsanitize=undefined,address")
  endif()
endif()

# Install
install(TARGETS bucket DESTINATION bin)
install(TARGETS bucketrt DESTINATION lib)
