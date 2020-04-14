#ifndef BUCKET_RUN_COMPILER_HXX
#define BUCKET_RUN_COMPILER_HXX

#include <optional>
#include <string>

void run_compiler(
  std::string input_path,
  std::optional<std::string> output_path_optional,
  bool read,
  bool lex,
  bool parse,
  bool ir,
  bool bc,
  bool asmb,
  bool obj,
  bool exec
);

#endif
