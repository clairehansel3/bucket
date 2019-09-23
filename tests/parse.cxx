#include "abstract_syntax_tree.hxx"
#include "lexer.hxx"
#include "miscellaneous.hxx"
#include "parser.hxx"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

constexpr auto terminal_width = 80;

int main(int argc, char* argv[]) {
  std::ios_base::sync_with_stdio(false);
  std::cin.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cout.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cerr.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::clog.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  if (argc != 2)
    throw std::runtime_error("no file specified");
  try {
    Lexer lexer{argv[1]};
    Parser parser{lexer};
    auto program_ptr = parser.parse();
    std::cout << *program_ptr;
  } catch (CompilerError& ce) {
    std::cerr << std::string(terminal_width, '-') << "\n" BUCKET_RED
      << ce.errorName() << ":" BUCKET_BLACK " " << ce.what()
      << std::string(terminal_width, '-') << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
