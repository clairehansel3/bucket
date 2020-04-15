#include "run_compiler.hxx"
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
  try {
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
      ("asm", "compiles the input into assembly")
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
        variables_map.count("asm"),
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
