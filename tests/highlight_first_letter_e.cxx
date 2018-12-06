#include "source_file.hxx"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
  std::ios_base::sync_with_stdio(false);
  std::cin.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cout.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::cerr.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::clog.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  if (argc != 2)
    throw std::runtime_error("no file specified");
  SourceFile sourcefile{argv[1]};
  for (auto iter = sourcefile.begin(); iter != sourcefile.end(); ++iter) {
    if (*iter == 'e') {
      sourcefile.highlight(std::cout, iter);
      break;
    }
  }
}
