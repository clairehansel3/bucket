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
  if (argc < 3)
    throw std::runtime_error("not enough arguments");
  if (argc > 3)
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
    std::cout << BUCKET_BOLD "pass 2" BUCKET_BLACK << std::endl;
    SecondPass second_pass{symbol_table};
    ast::dispatch(program_ptr.get(), &second_pass);
    std::cout << BUCKET_BOLD "resolving classes" BUCKET_BLACK << std::endl;
    resolveClasses(symbol_table, llvm_context);
    std::cout << BUCKET_BOLD "resolving methods" BUCKET_BLACK << std::endl;
    resolveMethods(symbol_table, llvm_module);
    std::cout << BUCKET_BOLD "pass 3" BUCKET_BLACK << std::endl;
    ThirdPass third_pass{symbol_table, llvm_context};
    ast::dispatch(program_ptr.get(), &third_pass);
    std::cout << BUCKET_BOLD "finalizing" BUCKET_BLACK << std::endl;
    finalizeModule(llvm_context, llvm_module, symbol_table);
    writeModule(llvm_module, argv[2]);
  } catch (CompilerError& ce) {
    std::cerr << std::string(terminal_width, '-') << "\n" BUCKET_RED
      << ce.errorName() << ":" BUCKET_BLACK " " << ce.what()
      << std::string(terminal_width, '-') << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
