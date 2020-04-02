// Copyright (C) 2019  Claire Hansel
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

#include "symbol_table.hxx"
#include "abstract_syntax_tree.hxx"
#include <llvm/IR/IRBuilder.h>
#include <stack>
#include <string>
#include <utility>

namespace llvm {
  class LLVMContext;
  class Module;
  class Value;
  class BasicBlock;
}

SymbolTable::Type* resolveType(SymbolTable& symbol_table, const ast::Expression* expression_ptr);

void initializeBuiltins(SymbolTable& symbol_table, llvm::LLVMContext& llvm_context, llvm::Module& llvm_module);

class FirstPass final : public ast::Visitor {
public:
  FirstPass(SymbolTable& symbol_table);
private:
  SymbolTable& m_symbol_table;
  void visit(ast::Program*) override;
  void visit(ast::Global*) override;
  void visit(ast::Class*) override;
  void visit(ast::Method*) override;
  void visit(ast::Field*) override;
};

class SecondPass final : public ast::Visitor {
public:
  SecondPass(SymbolTable& symbol_table);
private:
  SymbolTable& m_symbol_table;
  SymbolTable::Class* m_st_class;
  void visit(ast::Program*) override;
  void visit(ast::Class*) override;
  void visit(ast::Method*) override;
  void visit(ast::Field*) override;
  SymbolTable::Type* lookupType(const ast::Expression* expr) const;
};

void resolveClasses(SymbolTable& symbol_table, llvm::LLVMContext& llvm_context);

void resolveMethods(SymbolTable& symbol_table, llvm::Module& llvm_module);

class ThirdPass final : public ast::Visitor {
public:
  ThirdPass(SymbolTable& symbol_table, llvm::LLVMContext& llvm_context);
private:
  SymbolTable::Class* m_st_class;
  SymbolTable& m_symbol_table;
  llvm::LLVMContext& m_llvm_context;
  llvm::IRBuilder<> m_llvm_ir_builder;
  SymbolTable::Method* m_current_method;
  bool m_method_returned;
  SymbolTable::Type* m_current_value_class;
  llvm::Value* m_current_value;
  std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> m_loop_blocks;
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

  SymbolTable::Variable* createAllocaVariable(std::string_view name, SymbolTable::Type* type);

};

void finalizeModule(llvm::LLVMContext& llvm_context, llvm::Module& llvm_module, SymbolTable& symbol_table);

void writeModule(llvm::Module& llvm_module, std::string_view output_file);

void printModuleIR(llvm::Module& llvm_module, std::optional<std::string> output_path_optional);

void printModuleBC(llvm::Module& llvm_module, std::optional<std::string> output_path_optional);

void printModuleOBJ(llvm::Module& llvm_module, std::optional<std::string> output_path_optional);

#endif
