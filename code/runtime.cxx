#include "runtime.hxx"
#include <iostream>

void foo() {
  std::cout << "Hello, World!" << std::endl;
}

void print_integer(std::int64_t value) {
  std::cout << value << std::endl;
}
