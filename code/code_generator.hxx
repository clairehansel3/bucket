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

#ifndef BUCKET_CODE_GENERATOR_HXX
#define BUCKET_CODE_GENERATOR_HXX

#include "abstract_syntax_tree.hxx"
#include "symbol_table.hxx"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <optional>
#include <string>

namespace llvm {
  class Value;
  class BasicBlock;
}

class CodeGenerator : public ast::Visitor {
// A class that walks the abstract syntax tree and generates LLVM IR.

public:

  CodeGenerator();

  void printIR(std::optional<std::string> output_path = std::nullopt);
  void printBC(std::optional<std::string> output_path = std::nullopt);
  // Prints the generated code to a file in either human readable LLVM IR or
  // LLVM IR bytecode. If no argument is supplied, the code is written to
  // standard output.

private:

  SymbolTable m_symbol_table;
  llvm::LLVMContext m_context;
  llvm::Module m_module;
  llvm::IRBuilder<> m_ir_builder;
  SymbolTable::Class* m_current_class;
  SymbolTable::Method* m_current_method;
  SymbolTable::Type* m_expression_type;
  llvm::Value* m_expression_value;
  llvm::BasicBlock* m_scope_entry_block;
  llvm::BasicBlock* m_loop_entry_block;
  llvm::BasicBlock* m_loop_merge_block;
  bool m_after_jump;

  void initializeBuiltins();
  void initializeClasses(ast::Program* ast_program);
  void initializeFieldsAndMethods(ast::Program* ast_program);
  void resolveClasses();
  void resolveMethods();
  void finalize();

  void visit(ast::Program*) override;
  void visit(ast::Class*) override;
  void visit(ast::Method*) override;
  void visit(ast::Field*) override;
  void visit(ast::Declaration*) override;
  void visit(ast::If*) override;
  void visit(ast::InfiniteLoop*) override;
  void visit(ast::PreTestLoop*) override;
  void visit(ast::Break*) override;
  void visit(ast::Cycle*) override;
  void visit(ast::Ret*) override;
  void visit(ast::ExpressionStatement*) override;
  void visit(ast::Assignment*) override;
  void visit(ast::Call*) override;
  void visit(ast::Identifier*) override;
  void visit(ast::Real*) override;
  void visit(ast::Integer*) override;
  void visit(ast::Boolean*) override;
  void visit(ast::String*) override;
  void visit(ast::Character*) override;

};

#endif
