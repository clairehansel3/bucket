#include "abstract_syntax_tree.hxx"
#include "code_generator.hxx"
#include "lexer.hxx"
#include "miscellaneous.hxx"
#include "parser.hxx"
#include "source_file.hxx"
#include "symbol_table.hxx"
#include <cstdlib>
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <stdexcept>

constexpr auto terminal_width = 80;

int main(int argc, char* argv[]) {
  std::ios_base::sync_with_stdio(false);
  std::cin.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cout.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cerr.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::clog.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  if (argc < 2)
    throw std::runtime_error("no file specified");
  if (argc > 2)
    throw std::runtime_error("extra arguments");
  try {
    SourceFile source_file{argv[1]};
    Lexer lexer{source_file};
    Parser parser{lexer};
    auto program_ptr = parser.parse();
    SymbolTable symbol_table{};
    llvm::LLVMContext llvm_context{};
    llvm::Module llvm_module{"bucket_llvm_module", llvm_context};
    std::cout << BUCKET_BOLD "initializing builtins" BUCKET_BLACK << std::endl;
    initializeBuiltins(symbol_table, llvm_context, llvm_module);
    std::cout << BUCKET_BOLD "pass 1" BUCKET_BLACK << std::endl;
    FirstPass first_pass{symbol_table};
    ast::dispatch(program_ptr.get(), &first_pass);
    symbol_table.print();
  } catch (CompilerError& ce) {
    std::cerr << std::string(terminal_width, '-') << "\n" BUCKET_RED
      << ce.errorName() << ":" BUCKET_BLACK " " << ce.what()
      << std::string(terminal_width, '-') << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}