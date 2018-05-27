#include "ast.hxx"
#include "codegen.hxx"
#include "lexer.hxx"
#include "parser.hxx"
#include "source.hxx"
#include <boost/filesystem.hpp>
#ifdef BUCKET_DEBUG
#include <csignal>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Support/FileSystem.h>
#ifndef BUCKET_DEBUG
#include <llvm/Support/initLLVM.h>
#endif
#include <llvm/Support/raw_ostream.h>
#include <stdexcept>
#include <system_error>

namespace fs = boost::filesystem;

#define BUILD_DIRECTORY         "/Users/claire/bucket/.build"
#define CXX_COMPILER_PATH       "/Users/claire/Software/prefix/bin/clang++"
#define INSTALL_PREFIX          "/Users/claire/Software/prefix"

static void bucket(int argc, char** argv) {
  // Initialize LLVM
  #ifndef BUCKET_DEBUG
  llvm::InitLLVM X{argc, argv};
  #endif
  // Initialize iostream
  std::ios_base::sync_with_stdio(false);
  std::cin.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cout.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cerr.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::clog.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  // Get Command
  if (argc < 2)
    throw std::runtime_error("no command specified");
  const char* command = argv[1];
  // Execute Command
  if (!std::strcmp(command, "b") || !std::strcmp(command, "bp")) {
    // Build Production
    if (!fs::is_directory(BUILD_DIRECTORY))
      fs::create_directory(BUILD_DIRECTORY);
    std::system(
      "cd " BUILD_DIRECTORY " && "
      "cmake .. -DCMAKE_BUILD_TYPE=Production -DCMAKE_CXX_COMPILER=" CXX_COMPILER_PATH " && "
      "make install"
    );
  }
  else if (!std::strcmp(command, "bd")) {
    // Build Debug
    if (!fs::is_directory(BUILD_DIRECTORY))
      fs::create_directory(BUILD_DIRECTORY);
    std::system(
      "cd " BUILD_DIRECTORY " && "
      "cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=" CXX_COMPILER_PATH " && "
      "make install"
    );
  }
  else if (!std::strcmp(command, "rb") || !std::strcmp(command, "rbp")) {
    // Rebuild Production
    fs::remove_all(BUILD_DIRECTORY);
    fs::create_directory(BUILD_DIRECTORY);
    std::system(
      "cd " BUILD_DIRECTORY " && "
      "cmake .. -DCMAKE_BUILD_TYPE=Production -DCMAKE_CXX_COMPILER=" CXX_COMPILER_PATH " && "
      "make install"
    );
  }
  else if (!std::strcmp(command, "rbd")) {
    // Rebuild Debug
    fs::remove_all(BUILD_DIRECTORY);
    fs::create_directory(BUILD_DIRECTORY);
    std::system(
      "cd " BUILD_DIRECTORY " && "
      "cmake .. -DCMAKE_BUILD_TYPE=Production -DCMAKE_CXX_COMPILER=" CXX_COMPILER_PATH " && "
      "make install"
    );
  }
  else if (!std::strcmp(command, "cl")) {
    // Clean
    fs::remove_all(BUILD_DIRECTORY);
  }
  else if (!std::strcmp(command, "h")) {
    // Help
    std::cout << "Commands:\nb   build production\nbp  build production\nbd  build debug\n"
    "rb  rebuild production\nrbp rebuild production\nrbd rebuild debug\ncl  clean\nh   help\n"
    "r   read file\nl   lex file\np   parse file\nss  show symbols\nc   compile\n";
  }
  else if (!std::strcmp(command, "r")) {
    // Read
    if (argc < 3)
      throw std::runtime_error("no file specified");
    SourceFile source_file{argv[2]};
    while (source_file.character() != -1) {
      std::cout << static_cast<char>(source_file.character());
      source_file.next();
    }
  }
  else if (!std::strcmp(command, "l")) {
    // Lex
    if (argc < 3)
      throw std::runtime_error("no file specified");
    Lexer lexer{argv[2]};
    while (!lexer.token().isEndOfFile()) {
      std::cout << lexer.token();
      lexer.next();
    }
  }
  else if (!std::strcmp(command, "p")) {
    // Parse
    if (argc < 3)
      throw std::runtime_error("no file specified");
    Parser parser{argv[2]};
    auto program = parser.parse();
    ast::dump(program.get());
  }
  else if (!std::strcmp(command, "c")) {
    // Compile
    if (argc < 3)
      throw std::runtime_error("no file specified");
    Parser parser{argv[2]};
    auto program = parser.parse();
    CodeGenerator cg;
    cg.generate(program.get(), "result.bc");
    std::system(
      "llc -O3 result.bc -o result.s && "
      "as result.s -o result.o && "
      "ld result.o -lSystem -lbucketrt -macosx_version_min 10.13.0 -e _main -o program && "
      "rm result.bc result.s result.o"
    );
  }
  else
    throw std::runtime_error("unknown command");
}

int main(int argc, char** argv) noexcept {
  try {
    bucket(argc, argv);
    return EXIT_SUCCESS;
  } catch (std::exception& exception) {
    try {
      std::cerr << "bucket: error: " << exception.what() << '\n';
    } catch (...) {}
    return EXIT_FAILURE;
  }
}

#ifdef BUCKET_DEBUG

extern "C" void* __cxa_allocate_exception(std::size_t) noexcept {
  std::raise(SIGSEGV);
  return nullptr;
}

#endif
