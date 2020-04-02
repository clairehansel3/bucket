#include "lexer.hxx"
#include "source_file.hxx"
#include "abstract_syntax_tree.hxx"
#include "code_generator.hxx"
#include "miscellaneous.hxx"
#include "parser.hxx"
#include "symbol_table.hxx"
#include <boost/program_options.hpp>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <utf8cpp/utf8.h>

namespace po = boost::program_options;

static void run_compiler(
  std::string input_path,
  std::optional<std::string> output_path_optional,
  bool read,
  bool lex,
  bool parse,
  bool ir,
  bool bc,
  bool obj,
  bool exec
)
{
  if (!(read || lex || parse || ir || bc || obj || exec))
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
      #if BUCKET_DEBUG
      utf8::append(character, std::ostream_iterator<char>(*output_stream_ptr));
      #else
      utf8::unchecked::append(character, std::ostream_iterator<char>(*output_stream_ptr));
      #endif
    }
  }

  if (!(lex || parse || ir || bc || obj || exec))
    return;

  Lexer lexer{source_file};

  if (lex)
    for (auto token : lexer)
      *output_stream_ptr << token;

  if (!(parse || ir || bc || obj || exec))
    return;

  Parser parser{lexer};
  auto ast_program = parser.parse();

  if (parse)
    *output_stream_ptr << *ast_program;

  if (!(ir || bc || obj || exec))
    return;

  SymbolTable symbol_table{};
  llvm::LLVMContext llvm_context{};
  llvm::Module llvm_module{"bucket", llvm_context};
  initializeBuiltins(symbol_table, llvm_context, llvm_module);
  FirstPass first_pass{symbol_table};
  ast::dispatch(ast_program.get(), &first_pass);
  SecondPass second_pass{symbol_table};
  ast::dispatch(ast_program.get(), &second_pass);
  resolveClasses(symbol_table, llvm_context);
  resolveMethods(symbol_table, llvm_module);
  ThirdPass third_pass{symbol_table, llvm_context};
  ast::dispatch(ast_program.get(), &third_pass);
  finalizeModule(llvm_context, llvm_module, symbol_table);

  if (ir)
    printModuleIR(llvm_module, output_path_optional);

  if (!(bc || obj || exec))
    return;

  if (bc)
    printModuleBC(llvm_module, output_path_optional);

  if (!(obj || exec))
    return;

  if (obj)
    printModuleOBJ(llvm_module, output_path_optional);

  if (exec) {
    auto tn = std::tmpnam(nullptr);
    printModuleOBJ(llvm_module, std::string(tn));
    BUCKET_ASSERT(std::getenv("BUCKET_RUNTIME_LIB"));
    if (output_path_optional) {
      std::system(concatenate("ld ", tn, " ", std::getenv("BUCKET_RUNTIME_LIB"), " -lSystem -o ", *output_path_optional).c_str());
    }
    else {
      auto tn2 = std::tmpnam(nullptr);
      std::system(concatenate("ld ", tn, " ", std::getenv("BUCKET_RUNTIME_LIB"), " -lSystem -o ", tn2).c_str());
      std::system(concatenate("cat ", tn2).c_str());
    }
  }
}

int main(int argc, char* argv[])
{
  try {
    std::ios_base::sync_with_stdio(false);
    std::cin.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::cout.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::cerr.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::clog.exceptions(std::ios_base::badbit | std::ios_base::failbit);

    po::options_description options_description{"options"};
    options_description.add_options()
      ("help", "print help message")
      ("version", "print version info")
      ("input-file", po::value<std::string>(), "sets the input file")
      ("output-file", po::value<std::string>(), "sets the output file")
      ("read", "reads the input file")
      ("lex", "turns the input into a list of tokens")
      ("parse", "turns the input into an abstract syntax tree")
      ("ir", "compiles the input into readable llvm ir")
      ("bc", "compiles the input into llvm bitcode")
      ("obj", "compiles the input into an object file")
      ("exec", "compiles and links the input into an executable")
    ;

    po::positional_options_description positional_options_description;
    positional_options_description
      .add("input-file", 1)
      .add("output-file", 2);
    po::variables_map variables_map;
    po::store(
      po::command_line_parser(argc, argv)
        .options(options_description)
        .positional(positional_options_description)
        .run(),
      variables_map
    );
    po::notify(variables_map);
    if (variables_map.count("help")) {
      std::cout << options_description << std::endl;
    }
    else if (variables_map.count("version")) {
      std::cout << "bucket version 0.0.0" << std::endl;
    }
    else {
      if (!variables_map.count("input-file")) {
        throw std::runtime_error("must specify an input file");
      }
      std::string input_path = variables_map["input-file"].as<std::string>();
      std::optional<std::string> output_path_optional;
      if (variables_map.count("output-file")) {
        output_path_optional = variables_map["output-file"].as<std::string>();
      }
      run_compiler(
        input_path,
        output_path_optional,
        variables_map.count("read"),
        variables_map.count("lex"),
        variables_map.count("parse"),
        variables_map.count("ir"),
        variables_map.count("bc"),
        variables_map.count("obj"),
        variables_map.count("exec")
      );
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
