// Copyright (C) 2020  Claire Hansel
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "abstract_syntax_tree.hxx"
#include "comterpreter.hxx"
#include "source_file.hxx"
#include "lexer.hxx"
#include "miscellaneous.hxx"
#include "parser.hxx"
#include "tree_transform.hxx"
#include <boost/program_options.hpp>
#include <boost/version.hpp>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace po = boost::program_options;

static void main3(std::string input_path,
  std::optional<std::string> output_path_optional, bool read, bool lex,
  bool parse, bool transform, bool compile)
{
  if (!(read || lex || parse || transform))
    compile = true;

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
      utf8::unchecked::append(character,
        std::ostream_iterator<char>(*output_stream_ptr)
      );
      #endif
    }
  }

  if (!(lex || parse || transform || compile))
    return;

  Lexer lexer{source_file};

  if (lex)
    for (auto token : lexer)
      *output_stream_ptr << token;

  if (!(parse || transform || compile))
    return;

  Parser parser{lexer};
  auto ast_program = parser.parse();

  if (parse)
    *output_stream_ptr << *ast_program;

  if (!(transform || compile))
    return;

  ElifRemover tree_transform;
  ast::dispatch(tree_transform, *ast_program);

  if (transform)
    *output_stream_ptr << *ast_program;

  if (!compile)
    return;

  Comterpreter comterpreter{};
  ast::dispatch(comterpreter, *ast_program);
}

static void main2(int argc, char* argv[])
{
  po::options_description options_description{"options"};
  options_description.add_options()
    ("help", "print help message")
    ("version", "print version info")
    ("license", "print license info")
    ("in", po::value<std::string>(), "sets the input file")
    ("out", po::value<std::string>(), "sets the output file")
    ("read", "reads the input file")
    ("lex", "prints a list of tokens")
    ("parse", "prints the original abstract syntax tree")
    ("transform", "prints the transformed abstract syntax tree")
    ("compile", "compiles the input into readable llvm ir")
  ;
  po::positional_options_description positional_options_description;
  positional_options_description
    .add("in", 1)
    .add("out", 2);
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
    std::cout << argv[0] << " [options] <in> [<out>]\n" << options_description
              << std::endl;
  }
  else if (variables_map.count("version")) {
    std::cout <<
      "bucket version pre-\xCE\xB1lpha 0.0.0\n"
    #if defined(__TIME__) && defined(__DATE__)
      "compiled at " __TIME__ " on " __DATE__ "\n"
    #endif
    #ifdef __clang__
      "clang " __clang_version__ "\n"
    #endif
      "boost " BOOST_LIB_VERSION "\n"
      "llvm " LLVM_VERSION_STRING
    << std::endl;
  }
  else if (variables_map.count("license")) {
    if (
      std::system("curl https://www.gnu.org/licenses/gpl-3.0.txt 2> /dev/null")
    ) {
      exception("network", "unable to download license from www.gnu.org");
    }
  }
  else {
    if (!variables_map.count("in"))
      exception("command line", "you must specify an input file");
    std::string input_path = variables_map["in"].as<std::string>();
    std::optional<std::string> output_path_optional;
    if (variables_map.count("out")) {
      output_path_optional = variables_map["out"].as<std::string>();
    }
    main3(
      std::move(input_path),
      std::move(output_path_optional),
      variables_map.count("read"),
      variables_map.count("lex"),
      variables_map.count("parse"),
      variables_map.count("transform"),
      variables_map.count("compile")
    );
  }
}

int main(int argc, char* argv[])
{
  try {
    std::cin.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::cout.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::cerr.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::clog.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    main2(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
