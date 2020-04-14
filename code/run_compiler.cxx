#include "run_compiler.hxx"
#include "lexer.hxx"
#include "source_file.hxx"
#include "abstract_syntax_tree.hxx"
#include "code_generator.hxx"
#include "miscellaneous.hxx"
#include "parser.hxx"
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utf8cpp/utf8.h>

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
)
{
  if (!(read || lex || parse || ir || bc || asmb || obj || exec))
    exec = true;

  std::ofstream output_file_stream;
  output_file_stream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
  std::ostream* output_stream_ptr;
  if (output_path_optional) {
    output_file_stream.open(*output_path_optional);
    output_stream_ptr = &output_file_stream;
  }
  else {
    output_stream_ptr = &std::cout;
  }


  SourceFile source_file{input_path.c_str()};

  if (read) {
    for (std::uint32_t character : source_file) {
      #ifdef BUCKET_DEBUG
      utf8::append(character, std::ostream_iterator<char>(*output_stream_ptr));
      #else
      utf8::unchecked::append(character, std::ostream_iterator<char>(*output_stream_ptr));
      #endif
    }
  }

  if (!(lex || parse || ir || bc || asmb || obj || exec))
    return;

  Lexer lexer{source_file};

  if (lex)
    for (auto token : lexer)
      *output_stream_ptr << token;

  if (!(parse || ir || bc || asmb || obj || exec))
    return;

  Parser parser{lexer};
  auto ast_program = parser.parse();

  if (parse)
    *output_stream_ptr << *ast_program;

  if (!(ir || bc || asmb || obj || exec))
    return;

  CodeGenerator code_generator{};
  ast::dispatch(ast_program.get(), &code_generator);

  if (ir)
    code_generator.printIR(output_path_optional);

  if (!(bc || asmb || obj || exec))
    return;

  if (bc)
    code_generator.printBC(output_path_optional);

  if (!(asmb || obj || exec))
    return;

  throw make_error<GeneralError>("use llc/lld to create object files and executables");
}
