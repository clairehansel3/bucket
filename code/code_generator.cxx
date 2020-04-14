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

#include "code_generator.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/bimap.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/polymorphic_cast.hpp>
#include <initializer_list>
#include <llvm/ADT/StringRef.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

CodeGenerator::CodeGenerator()
: m_symbol_table{},
  m_context{},
  m_module{"bucket-llvm-module", m_context},
  m_ir_builder{m_context}
{}

void CodeGenerator::printIR(std::optional<std::string> output_path)
{
  std::string name = output_path ? *output_path : std::string("-");
  std::error_code error_code;
  llvm::raw_fd_ostream ostream{llvm::StringRef(name.data(), name.size()), error_code};
  m_module.print(ostream, nullptr);
  if (error_code)
    throw make_error<CodeGeneratorError>("unable to print IR to file: ", error_code.message());
}

void CodeGenerator::printBC(std::optional<std::string> output_path)
{
  std::string name = output_path ? *output_path : std::string("-");
  std::error_code error_code;
  llvm::raw_fd_ostream ostream{llvm::StringRef(name.data(), name.size()), error_code};
  llvm::WriteBitcodeToFile(m_module, ostream);
  if (error_code)
    throw make_error<CodeGeneratorError>("unable to print IR to file: ", error_code.message());
}

void CodeGenerator::initializeBuiltins()
{
  // function to help create built-in methods
  auto method = [this](
    SymbolTable::Type* type,
    std::string_view name,
    std::initializer_list<SymbolTable::Type*> args,
    SymbolTable::Type* return_type,
    std::string_view link_name
  ) {
    std::vector<SymbolTable::Type*> sym_args{args};
    std::vector<llvm::Type*> llvm_args{sym_args.size() + 1};
    llvm_args[0] = m_symbol_table.getPointerType(type)->m_llvm_type;
    std::transform(sym_args.begin(), sym_args.end(), llvm_args.begin() + 1,
      [](SymbolTable::Type* typ){return typ->m_llvm_type;});
    auto method_entry = m_symbol_table.createMethod(name, std::move(sym_args),
      return_type);
    method_entry->m_llvm_function = llvm::Function::Create(llvm::FunctionType::get(
      return_type->m_llvm_type, llvm_args, false),
      llvm::Function::ExternalLinkage, llvm::StringRef(link_name.data(),
      link_name.size()), m_module);
  };

  // function to help create built-in system calls
  auto syscall = [this](
    std::string_view name,
    std::initializer_list<SymbolTable::Type*> args,
    SymbolTable::Type* return_type,
    std::string_view link_name
  ) {
    std::vector<SymbolTable::Type*> sym_args{args};
    std::vector<llvm::Type*> llvm_args{sym_args.size()};
    std::transform(sym_args.begin(), sym_args.end(), llvm_args.begin(),
      [](SymbolTable::Type* typ){return typ->m_llvm_type;});
    auto method_entry = m_symbol_table.createMethod(name, std::move(sym_args),
      return_type);
    method_entry->m_llvm_function = llvm::Function::Create(llvm::FunctionType::get(
      return_type->m_llvm_type, llvm_args, false),
      llvm::Function::ExternalLinkage, llvm::StringRef(link_name.data(),
      link_name.size()), m_module);
  };

  // create built-in types
  auto bool_type = m_symbol_table.createType("bool", m_ir_builder.getInt1Ty());
  auto int_type = m_symbol_table.createType("int", m_ir_builder.getInt64Ty());
  auto real_type = m_symbol_table.createType("real", m_ir_builder.getDoubleTy());
  auto byte_type = m_symbol_table.createType("byte", m_ir_builder.getInt8Ty());
  auto nil_type = m_symbol_table.createType("nil", m_ir_builder.getVoidTy());
  [[maybe_unused]] auto system_type = m_symbol_table.createType("system", nullptr);

  // create built-in methods
  m_symbol_table.pushScope("bool");
  method(bool_type, "__and__", {bool_type}, bool_type, "bucket_bool_and");
  method(bool_type, "__or__", {bool_type}, bool_type, "bucket_bool_or");
  method(bool_type, "__not__", {}, bool_type, "bucket_bool_not");
  method(bool_type, "print", {}, nil_type, "bucket_bool_print");
  m_symbol_table.popScope();

  m_symbol_table.pushScope("int");
  method(int_type, "__add__", {int_type}, int_type, "bucket_int_add");
  method(int_type, "__sub__", {int_type}, int_type, "bucket_int_sub");
  method(int_type, "__mul__", {int_type}, int_type, "bucket_int_mul");
  method(int_type, "__div__", {int_type}, int_type, "bucket_int_div");
  method(int_type, "__mod__", {int_type}, int_type, "bucket_int_mod");
  method(int_type, "__lt__", {int_type}, bool_type, "bucket_int_lt");
  method(int_type, "__le__", {int_type}, bool_type, "bucket_int_le");
  method(int_type, "__eq__", {int_type}, bool_type, "bucket_int_eq");
  method(int_type, "__gt__", {int_type}, bool_type, "bucket_int_ge");
  method(int_type, "__ge__", {int_type}, bool_type, "bucket_int_gt");
  method(int_type, "print", {}, nil_type, "bucket_int_print");
  m_symbol_table.popScope();

  m_symbol_table.pushScope("real");
  method(real_type, "__add__", {real_type}, real_type, "bucket_real_add");
  method(real_type, "__sub__", {real_type}, real_type, "bucket_real_sub");
  method(real_type, "__mul__", {real_type}, real_type, "bucket_real_mul");
  method(real_type, "__div__", {real_type}, real_type, "bucket_real_div");
  method(real_type, "__lt__", {real_type}, bool_type, "bucket_real_lt");
  method(real_type, "__le__", {real_type}, bool_type, "bucket_real_le");
  method(real_type, "__eq__", {real_type}, bool_type, "bucket_real_eq");
  method(real_type, "__gt__", {real_type}, bool_type, "bucket_real_ge");
  method(real_type, "__ge__", {real_type}, bool_type, "bucket_real_gt");
  method(real_type, "print", {}, nil_type, "bucket_real_print");
  m_symbol_table.popScope();

  m_symbol_table.pushScope("byte");
  method(byte_type, "__and__", {byte_type}, byte_type, "bucket_byte_and");
  method(byte_type, "__or__", {byte_type}, byte_type, "bucket_byte_or");
  method(byte_type, "__xor__", {byte_type}, byte_type, "bucket_byte_xor");
  method(byte_type, "__not__", {}, byte_type, "bucket_byte_not");
  method(byte_type, "__lshift__", {int_type}, byte_type, "bucket_byte_lshift");
  method(byte_type, "__rshift__", {int_type}, byte_type, "bucket_byte_rshift");
  method(byte_type, "print", {}, nil_type, "bucket_byte_print");
  m_symbol_table.popScope();

  m_symbol_table.pushScope("system");
  syscall("test", {int_type}, int_type, "bucket_system_test");
  m_symbol_table.popScope();
}

namespace {

class InitializeClassesPass : public ast::Visitor {
public:
  InitializeClassesPass(SymbolTable& symbol_table);
private:
  SymbolTable& m_symbol_table;
  void visit(ast::Program*) override;
  void visit(ast::Class*) override;
  void visit(ast::Method*) override;
  void visit(ast::Field*) override;
};

InitializeClassesPass::InitializeClassesPass(SymbolTable& symbol_table)
: m_symbol_table{symbol_table}
{}

void InitializeClassesPass::visit(ast::Program* ast_program)
{
  for (auto& ast_global : ast_program->globals)
    ast::dispatch(ast_global.get(), this);
}

void InitializeClassesPass::visit(ast::Class* ast_class)
{
  m_symbol_table.createClass(ast_class->name);
  m_symbol_table.pushScope(ast_class->name);
  for (auto& ast_global : ast_class->globals)
    ast::dispatch(ast_global.get(), this);
  m_symbol_table.popScope();
}

void InitializeClassesPass::visit(ast::Method*)
{}

void InitializeClassesPass::visit(ast::Field*)
{}

}

void CodeGenerator::initializeClasses(ast::Program* ast_program)
{
  InitializeClassesPass pass{m_symbol_table};
  ast::dispatch(ast_program, &pass);
}

namespace {

class InitializeFieldsAndMethodsPass : public ast::Visitor {
public:
  InitializeFieldsAndMethodsPass(SymbolTable& symbol_table);
private:
  SymbolTable& m_symbol_table;
  SymbolTable::Class* m_current_class;
  void visit(ast::Program*) override;
  void visit(ast::Class*) override;
  void visit(ast::Method*) override;
  void visit(ast::Field*) override;
};

InitializeFieldsAndMethodsPass::InitializeFieldsAndMethodsPass(
  SymbolTable& symbol_table)
: m_symbol_table{symbol_table},
  m_current_class{nullptr}
{}

void InitializeFieldsAndMethodsPass::visit(ast::Program* ast_program)
{
  for (auto& ast_global : ast_program->globals)
    ast::dispatch(ast_global.get(), this);
}

void InitializeFieldsAndMethodsPass::visit(ast::Class* ast_class)
{
  auto old_current_class = m_current_class;
  m_current_class = m_symbol_table.gotoName<SymbolTable::Class*>(
    ast_class->name
  );
  m_symbol_table.pushScope(ast_class->name);
  for (auto& ast_global : ast_class->globals)
    ast::dispatch(ast_global.get(), this);
  m_symbol_table.popScope();
  m_current_class = old_current_class;
}

void InitializeFieldsAndMethodsPass::visit(ast::Method* ast_method)
{
  std::vector<SymbolTable::Type*> argument_types{ast_method->arguments.size()};
  std::transform(ast_method->arguments.begin(), ast_method->arguments.end(),
    argument_types.begin(), [this](auto& argument){
      return m_symbol_table.resolveType(argument.second.get());
  });
  auto return_type = m_symbol_table.resolveType(ast_method->return_type.get());
  m_symbol_table.createMethod(ast_method->name, std::move(argument_types),
  return_type);
}

void InitializeFieldsAndMethodsPass::visit(ast::Field* ast_field)
{
  m_current_class->m_fields.push_back(m_symbol_table.createField(
    ast_field->name, m_symbol_table.resolveType(ast_field->type.get())
  ));
}

}

void CodeGenerator::initializeFieldsAndMethods(ast::Program* ast_program)
{
  InitializeFieldsAndMethodsPass pass{m_symbol_table};
  ast::dispatch(ast_program, &pass);
}

void CodeGenerator::resolveClasses()
{
  using Graph = boost::adjacency_list<>;
  using BiMap = boost::bimap<SymbolTable::Class*, Graph::vertex_descriptor>;
  Graph graph;
  BiMap table;
  for (auto entry : m_symbol_table)
    if (auto sym_class = SymbolTable::sym_cast<SymbolTable::Class*>(entry))
      table.insert(BiMap::value_type(sym_class, boost::add_vertex(graph)));
  for (auto entry : m_symbol_table)
    if (auto field = SymbolTable::sym_cast<SymbolTable::Field*>(entry))
      if (auto field_class = SymbolTable::sym_cast<SymbolTable::Class*>(
        field->m_type))
        boost::add_edge(table.left.find(
          m_symbol_table.gotoPath<SymbolTable::Class*>(field->parent()))->second,
              table.left.find(field_class)->second, graph);
  std::vector<Graph::vertex_descriptor> result;
  try {
    boost::topological_sort(graph, std::back_inserter(result));
  } catch (boost::not_a_dag&) {
    throw make_error<CodeGeneratorError>("cyclic class dependencies detected");
  }
  for (auto iter : result) {
    auto sym_class = table.right.find(iter)->second;
    if (sym_class->m_fields.size()) {
      std::vector<llvm::Type*> llvm_types{sym_class->m_fields.size()};
      BUCKET_ASSERT(std::all_of(sym_class->m_fields.begin(),
        sym_class->m_fields.end(), [](bool b){return b;}));
      std::transform(sym_class->m_fields.begin(), sym_class->m_fields.end(),
        llvm_types.begin(), [](SymbolTable::Field* field){
          return field->m_type->m_llvm_type;});
      BUCKET_ASSERT(std::all_of(llvm_types.begin(), llvm_types.end(),
        [](bool b){return b;}));
      sym_class->m_llvm_type = llvm::StructType::create(m_context, llvm_types);
    }
  }
}

void CodeGenerator::resolveMethods()
{
  // we have to be careful to avoid iterator invalidation here
  std::vector<SymbolTable::Entry*> symbol_table_entries{m_symbol_table.begin(),
    m_symbol_table.end()};
  for (auto entry_ptr : symbol_table_entries) {
    if (auto method_ptr = SymbolTable::sym_cast<SymbolTable::Method*>(entry_ptr)
        ) {
      if (!method_ptr->m_llvm_function) {
        auto method_type = m_symbol_table.gotoPath<SymbolTable::Class*>(method_ptr->parent());
        std::vector<llvm::Type*> argument_types;
        if (method_type->m_llvm_type) {
          // non empty type
          argument_types.resize(method_ptr->m_argument_types.size() + 1);
          argument_types[0] = m_symbol_table.getPointerType(method_type)->m_llvm_type;
          std::transform(method_ptr->m_argument_types.begin(),
            method_ptr->m_argument_types.end(), argument_types.begin() + 1,
            [](SymbolTable::Type* type){return type->m_llvm_type;});
        }
        else {
          // empty type
          argument_types.resize(method_ptr->m_argument_types.size());
          std::transform(method_ptr->m_argument_types.begin(),
            method_ptr->m_argument_types.end(), argument_types.begin(),
            [](SymbolTable::Type* type){return type->m_llvm_type;});
        }
        auto function_type = llvm::FunctionType::get(
          method_ptr->m_return_type->m_llvm_type, argument_types, false);
        method_ptr->m_llvm_function = llvm::Function::Create(function_type,
          llvm::Function::ExternalLinkage, llvm::StringRef(
            method_ptr->path().data(), method_ptr->path().size()),
              m_module);
      }
    }
  }
}

void CodeGenerator::finalize()
{
  auto actual_main = llvm::Function::Create(
    llvm::FunctionType::get(m_ir_builder.getInt32Ty(), {m_ir_builder.getInt32Ty(),
      m_ir_builder.getInt8Ty()->getPointerTo()->getPointerTo()}, false),
      llvm::Function::ExternalLinkage, "main", &m_module
  );
  auto module_main = m_symbol_table.gotoPath<SymbolTable::Method*>("/main/main");
  BUCKET_ASSERT(module_main->m_return_type ==
    m_symbol_table.gotoPath<SymbolTable::Type*>("/bool"));
  BUCKET_ASSERT(module_main->m_argument_types.size() == 0);
  m_ir_builder.SetInsertPoint(llvm::BasicBlock::Create(m_context,
      "$entry", actual_main));
  auto call_inst = m_ir_builder.CreateCall(module_main->m_llvm_function);
  auto then_bb = llvm::BasicBlock::Create(m_context, "$then", actual_main);
  auto else_bb = llvm::BasicBlock::Create(m_context, "$else", actual_main);
  m_ir_builder.CreateCondBr(call_inst, then_bb, else_bb);
  m_ir_builder.SetInsertPoint(then_bb);
  m_ir_builder.CreateRet(llvm::ConstantInt::getSigned(
    llvm::Type::getInt32Ty(m_context), 0));
  m_ir_builder.SetInsertPoint(else_bb);
  m_ir_builder.CreateRet(llvm::ConstantInt::getSigned(
    llvm::Type::getInt32Ty(m_context), 1));

  // Verify the function
  std::string error_message;
  llvm::raw_string_ostream stream{error_message};
  if (llvm::verifyModule(m_module, &stream)) {
    #ifdef BUCKET_DEBUG
    throw make_error<CodeGeneratorError>("<internal compiler error>");
    #else
    throw make_error<CodeGeneratorError>("failed to verify llvm module:\n",
      stream.str()
    );
    #endif
  }
}

void CodeGenerator::visit(ast::Program* ast_program)
{
  initializeBuiltins();
  initializeClasses(ast_program);
  initializeFieldsAndMethods(ast_program);
  resolveClasses();
  resolveMethods();
  for (auto& ast_global : ast_program->globals)
    ast::dispatch(ast_global.get(), this);
  finalize();
}

void CodeGenerator::visit(ast::Class* ast_class)
{
  // update the current class variable
  auto old_class = m_current_class;
  m_current_class = m_symbol_table.gotoName<SymbolTable::Class*>(
    ast_class->name
  );

  // enter the scope of the class
  m_symbol_table.pushScope(ast_class->name);

  // visit everything in the class
  for (auto& ast_global : ast_class->globals)
    ast::dispatch(ast_global.get(), this);

  // exit the scope of the class
  m_symbol_table.popScope();

  // reset the current class variable to its previous value
  m_current_class = old_class;
}

void CodeGenerator::visit(ast::Method* ast_method)
{
  // set the current method
  m_current_method = m_symbol_table.gotoName<SymbolTable::Method*>(
    ast_method->name
  );
  m_after_jump = false;

  // enter an anonymous scope
  m_symbol_table.pushScope();

  // Create entry block and set IR builder to insert there
  m_scope_entry_block = llvm::BasicBlock::Create(
    m_context, "$entry", m_current_method->m_llvm_function
  );
  m_ir_builder.SetInsertPoint(m_scope_entry_block);

  // Define variables corresponding to the function arguments in both the symbol
  // table and the llvm function and generate store instructions.
  {
    auto llvm_arg_iter = m_current_method->m_llvm_function->arg_begin();
    auto llvm_arg_end = m_current_method->m_llvm_function->arg_end();
    auto ast_arg_iter = ast_method->arguments.begin();
    [[maybe_unused]] auto ast_arg_end = ast_method->arguments.end();
    auto sym_arg_iter = m_current_method->m_argument_types.begin();
    [[maybe_unused]] auto sym_arg_end = m_current_method->m_argument_types.end();

    // If the method is defined on a non empty class, it takes pointer to that
    // class as a first argument which it assigns to the variable 'this'.
    if (m_current_class->m_fields.size() > 0) {
      auto this_variable = m_symbol_table.createVariable(
        "this", m_symbol_table.getPointerType(m_current_class)
      );
      this_variable->m_llvm_value = m_ir_builder.CreateAlloca(
        this_variable->m_type->m_llvm_type, 0, nullptr, llvm::StringRef(
          this_variable->path().data(), this_variable->path().size()
      ));
      m_ir_builder.CreateStore(llvm_arg_iter, this_variable->m_llvm_value);
      ++llvm_arg_iter;
    }

    // Create variables corresponding to the remaining arguments.
    while (llvm_arg_iter != llvm_arg_end) {
      BUCKET_ASSERT(ast_arg_iter != ast_arg_end);
      BUCKET_ASSERT(sym_arg_iter != sym_arg_end);
      auto arg_variable = m_symbol_table.createVariable(
        ast_arg_iter->first, *sym_arg_iter
      );
      arg_variable->m_llvm_value = m_ir_builder.CreateAlloca(
        arg_variable->m_type->m_llvm_type, 0, nullptr, llvm::StringRef(
          arg_variable->path().data(), arg_variable->path().size()
      ));
      m_ir_builder.CreateStore(llvm_arg_iter, arg_variable->m_llvm_value);
      ++llvm_arg_iter;
      ++ast_arg_iter;
      ++sym_arg_iter;
    }
  }

  // Visit statements and generate code
  for (auto& ast_statement : ast_method->statements)
    ast::dispatch(ast_statement.get(), this);

  // Check that the function has returned
  if (!m_after_jump) {
    if (m_current_method->m_return_type !=
        m_symbol_table.gotoPath<SymbolTable::Type*>("/nil")) {
      throw make_error<CodeGeneratorError>(
        "method '", m_current_method->name(), "' in class '",
        m_current_class->path(), "' reaches end of code without returning"
      );
    }
    m_ir_builder.CreateRetVoid();
  }

  // Verify the function
  std::string error_message;
  llvm::raw_string_ostream stream{error_message};
  if (llvm::verifyFunction(*m_current_method->m_llvm_function, &stream)) {
    #ifdef BUCKET_DEBUG
    throw make_error<CodeGeneratorError>("<internal compiler error>");
    #else
    throw make_error<CodeGeneratorError>("failed to verify llvm function:\n",
      stream.str()
    );
    #endif
  }

  // exit the scope
  m_symbol_table.popScope();
}

void CodeGenerator::visit(ast::Field*)
{}

void CodeGenerator::visit(ast::Declaration* ast_declaration)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");
  auto variable = m_symbol_table.createVariable(
    ast_declaration->name, m_symbol_table.resolveType(ast_declaration->type.get())
  );
  auto old_insertion_point = m_ir_builder.saveIP();
  m_ir_builder.SetInsertPoint(m_scope_entry_block,
    m_scope_entry_block->begin()
  );
  variable->m_llvm_value = m_ir_builder.CreateAlloca(
    variable->m_type->m_llvm_type, 0, nullptr, llvm::StringRef(
      variable->path().data(), variable->path().size()
  ));
  m_ir_builder.restoreIP(old_insertion_point);
}

void CodeGenerator::visit(ast::If* ast_if)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");

  //
  auto merge_block = llvm::BasicBlock::Create(m_context, "$merge",
    m_current_method->m_llvm_function
  );

  // Save the old scope entry block
  auto old_scope_entry_block = m_scope_entry_block;

  // define a function that generates code for a single conditional (i.e. for a
  // single if or elif)
  auto generateCodeForConditional = [this, merge_block](
    std::unique_ptr<ast::Expression>& ast_condition,
    std::vector<std::unique_ptr<ast::Statement>>& ast_body
  )
  {
    // generate code to evaluate the condition
    ast::dispatch(ast_condition.get(), this);

    // ensure condition is a runtime variable as opposed to a type
    if (!m_expression_value)
      throw make_error<CodeGeneratorError>("condition in if statement must be a"
        " runtime value");

    // ensure condition expression is a boolean
    if (m_expression_type != m_symbol_table.gotoPath<SymbolTable::Type*>("/bool"))
      throw make_error<CodeGeneratorError>("condition in if statement must be a "
        "boolean, not an expression of type '",
        m_expression_type->path(), '\'');

    // Create labels for when the condition is true and false
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(m_context,
      "$then", m_current_method->m_llvm_function
    );
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(m_context,
      "$else", m_current_method->m_llvm_function
    );

    // Create a branch instruction
    m_ir_builder.CreateCondBr(m_expression_value, then_block, else_block);

    m_scope_entry_block = then_block;

    // Start generating code for the conditional body
    m_ir_builder.SetInsertPoint(then_block);

    m_symbol_table.pushScope();
    for (auto& ast_statement : ast_body)
      ast::dispatch(ast_statement.get(), this);
    m_symbol_table.popScope();

    if (!m_after_jump)
      m_ir_builder.CreateBr(merge_block);
    m_after_jump = false;

    // Set the insert point to the else block
    m_ir_builder.SetInsertPoint(else_block);
    m_scope_entry_block = else_block;
  };

  // Generate code for the main conditional and the elifs
  generateCodeForConditional(ast_if->condition, ast_if->if_body);
  for (auto& elif : ast_if->elif_bodies)
    generateCodeForConditional(elif.first, elif.second);

  // generate code for the else body
  m_symbol_table.pushScope();
  for (auto& ast_statement : ast_if->else_body)
    ast::dispatch(ast_statement.get(), this);
  m_symbol_table.popScope();
  if (!m_after_jump)
    m_ir_builder.CreateBr(merge_block);
  m_after_jump = false;
  m_ir_builder.CreateBr(merge_block);

  // Set the insert point to after the merge block and reset scope entry block
  m_ir_builder.SetInsertPoint(merge_block);
  m_scope_entry_block = old_scope_entry_block;
}

void CodeGenerator::visit(ast::InfiniteLoop* ast_infinite_loop)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");

  auto old_loop_entry_block = m_loop_entry_block;
  auto old_loop_merge_block = m_loop_merge_block;
  auto old_scope_entry_block = m_scope_entry_block;

  m_loop_entry_block = llvm::BasicBlock::Create(m_context, "$loop_entry",
    m_current_method->m_llvm_function
  );
  m_loop_merge_block = llvm::BasicBlock::Create(m_context, "$loop_merge",
    m_current_method->m_llvm_function
  );
  m_scope_entry_block = m_loop_entry_block;

  m_ir_builder.CreateBr(m_loop_entry_block);
  m_ir_builder.SetInsertPoint(m_loop_entry_block);
  m_symbol_table.pushScope();
  for (auto& ast_statement : ast_infinite_loop->body)
    ast::dispatch(ast_statement.get(), this);
  m_symbol_table.popScope();
  m_after_jump = false;
  m_ir_builder.SetInsertPoint(m_loop_merge_block);
  m_loop_merge_block = old_loop_merge_block;
  m_loop_entry_block = old_loop_entry_block;
  m_scope_entry_block = old_scope_entry_block;
}

void CodeGenerator::visit(ast::PreTestLoop* ast_pre_test_loop) {
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");

  auto old_loop_entry_block = m_loop_entry_block;
  auto old_loop_merge_block = m_loop_merge_block;
  auto old_scope_entry_block = m_scope_entry_block;

  m_loop_entry_block = llvm::BasicBlock::Create(m_context, "$loop_entry",
    m_current_method->m_llvm_function
  );
  m_loop_merge_block = llvm::BasicBlock::Create(m_context, "$merge",
    m_current_method->m_llvm_function
  );
  m_scope_entry_block = m_loop_entry_block;

  m_ir_builder.CreateBr(m_loop_entry_block);
  m_ir_builder.SetInsertPoint(m_loop_entry_block);
  m_symbol_table.pushScope();

  ast::dispatch(ast_pre_test_loop->condition.get(), this);

  // ensure condition is a runtime variable as opposed to a type
  if (!m_expression_value)
    throw make_error<CodeGeneratorError>("condition in loop must be a runtime va"
      "lue"
    );

  // ensure condition expression is a boolean
  if (m_expression_type != m_symbol_table.gotoPath<SymbolTable::Type*>("/bool"))
    throw make_error<CodeGeneratorError>("condition in loop must be a boolean, n"
      "ot an expression of type '", m_expression_type->path(), '\''
    );

  auto loop_condition_true_block = llvm::BasicBlock::Create(
    m_context, "$loop_condition_true", m_current_method->m_llvm_function
  );

  auto loop_else_block = llvm::BasicBlock::Create(m_context, "$loop_else",
    m_current_method->m_llvm_function
  );

  m_ir_builder.CreateCondBr(m_expression_value, loop_condition_true_block,
    loop_else_block
  );

  m_ir_builder.SetInsertPoint(loop_condition_true_block);
  for (auto& ast_statement : ast_pre_test_loop->body)
    ast::dispatch(ast_statement.get(), this);
  if (!m_after_jump)
    m_ir_builder.CreateBr(m_loop_entry_block);

  m_after_jump = false;
  m_symbol_table.popScope();
  m_symbol_table.pushScope();
  auto pre_test_loop_merge_block = m_loop_merge_block; // so break statements in
    //the else work
  m_loop_merge_block = old_loop_merge_block;
  m_loop_entry_block = old_loop_entry_block;
  m_scope_entry_block = loop_else_block;
  m_ir_builder.SetInsertPoint(loop_else_block);
  for (auto& ast_statement : ast_pre_test_loop->else_body)
    ast::dispatch(ast_statement.get(), this);
  if (!m_after_jump)
    m_ir_builder.CreateBr(pre_test_loop_merge_block);
  m_symbol_table.popScope();
  m_after_jump = false;
  m_ir_builder.SetInsertPoint(pre_test_loop_merge_block);
  m_scope_entry_block = old_scope_entry_block;
}

void CodeGenerator::visit(ast::Break*)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");

  if (!m_loop_merge_block)
    throw make_error<CodeGeneratorError>("break statement occurs outside of a l"
      "oop"
  );
  m_ir_builder.CreateBr(m_loop_merge_block);
  m_after_jump = true;
}

void CodeGenerator::visit(ast::Cycle*)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");

  if (!m_loop_entry_block)
    throw make_error<CodeGeneratorError>("break statement occurs outside of a l"
      "oop"
  );
  m_ir_builder.CreateBr(m_loop_entry_block);
  m_after_jump = true;
}

void CodeGenerator::visit(ast::Ret* ast_ret)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code appears after return, break, or "
      "cycle");
  ast::dispatch(ast_ret->expression.get(), this);
  if (!m_expression_value)
    throw make_error<CodeGeneratorError>("return type must be a runtime value");
  if (m_expression_type != m_current_method->m_return_type)
    throw make_error<CodeGeneratorError>("value returned does not match method "
      "return type");
  m_ir_builder.CreateRet(m_expression_value);
  m_after_jump = true;
}

void CodeGenerator::visit(ast::ExpressionStatement* ast_expression_statement)
{
  if (m_after_jump)
    throw make_error<CodeGeneratorError>("code after function returns");
  ast::dispatch(ast_expression_statement->expression.get(), this);
}

void CodeGenerator::visit(ast::Assignment* ast_assignment)
{
  // Get name of lhs variable
  std::string lhs;
  {
    auto lhs_identifier = ast::ast_cast<ast::Identifier*>(
      ast_assignment->left.get());
    if (!lhs_identifier)
      throw make_error<CodeGeneratorError>("left hand side of assignment must b"
        "e an identifier");
    lhs = lhs_identifier->value;
  }

  // Visit rhs
  ast::dispatch(ast_assignment, this);

  if (!m_expression_value)
    throw make_error<CodeGeneratorError>("right hand side of assignment must be"
      " a runtime value");

  SymbolTable::Variable* lhs_variable;
  if (auto entry = m_symbol_table.lookup(lhs)) {
    // lhs is already defined
    lhs_variable = SymbolTable::sym_cast<SymbolTable::Variable*>(entry);
    if (!lhs_variable)
      throw make_error<CodeGeneratorError>("left hand side of assignment is not"
        " a variable");
    if (lhs_variable->m_type != m_expression_type)
      throw make_error<CodeGeneratorError>("type mismatch");
  }
  else {
    lhs_variable = m_symbol_table.createVariable(lhs, m_expression_type);

    auto old_insertion_point = m_ir_builder.saveIP();
    m_ir_builder.SetInsertPoint(m_scope_entry_block,
      m_scope_entry_block->begin()
    );
    lhs_variable->m_llvm_value = m_ir_builder.CreateAlloca(
      lhs_variable->m_type->m_llvm_type, 0, nullptr, llvm::StringRef(
        lhs_variable->path().data(), lhs_variable->path().size()
    ));
    m_ir_builder.restoreIP(old_insertion_point);
  }
  m_ir_builder.CreateStore(m_expression_value, lhs_variable->m_llvm_value);
}

void CodeGenerator::visit(ast::Call* ast_call)
{
  // resolve object being called
  ast::dispatch(ast_call->expression.get(), this);

  // obtain the method being called
  SymbolTable::Method* method;
  {
    auto scope = concatenate(m_expression_type->path(), '/');
    auto entry = m_symbol_table.lookup(scope, ast_call->name);
    if (!entry)
      throw make_error<CodeGeneratorError>("method '", ast_call->name, "' does "
        "not exist on type '", m_expression_type->path(), '\'');
    if (!(method = SymbolTable::sym_cast<SymbolTable::Method*>(entry)))
      throw make_error<CodeGeneratorError>('\'', ast_call->name, "' in type '",
        m_expression_type->path(), "' is not a method");
  }
  std::vector<llvm::Value*> arguments;
  std::vector<llvm::Value*>::iterator arguments_iter;
  if (m_expression_value) {
    arguments.resize(method->m_argument_types.size() + 1);
    auto old_insertion_point = m_ir_builder.saveIP();
    m_ir_builder.SetInsertPoint(m_scope_entry_block, m_scope_entry_block->begin());
    auto alloca_inst = m_ir_builder.CreateAlloca(m_expression_type->m_llvm_type,
      0, nullptr, "temp");
    m_ir_builder.CreateStore(m_expression_value, alloca_inst);
    arguments[0] = alloca_inst;
    m_ir_builder.restoreIP(old_insertion_point);
    if (method->m_argument_types.size() != ast_call->arguments.size())
      throw make_error<CodeGeneratorError>("argument count mismatch when callin"
        "g method '", ast_call->name, "' on type '", m_expression_type->path(),
        '\'');
    arguments_iter = arguments.begin() + 1;
  }
  else {
    arguments.resize(method->m_argument_types.size());
    arguments_iter = arguments.begin();
  }
  auto argument_class_iter = method->m_argument_types.begin();
  auto argument_expression_iter = ast_call->arguments.begin();
  while (arguments_iter != arguments.end()) {
    ast::dispatch((*argument_expression_iter).get(), this);
    if (!m_expression_value)
      throw std::runtime_error("cannot pass a class to a method");
    if (m_expression_type != *argument_class_iter)
      throw std::runtime_error("argument class mismatch");
    *arguments_iter = m_expression_value;
    ++argument_class_iter;
    ++argument_expression_iter;
    ++arguments_iter;
    }
  m_expression_value = m_ir_builder.CreateCall(method->m_llvm_function, arguments);
  m_expression_type = method->m_return_type;
}

void CodeGenerator::visit(ast::Identifier* ast_identifier)
{
  auto entry = m_symbol_table.lookup(ast_identifier->value);
  if (!entry)
    throw make_error<CodeGeneratorError>("unknown identifier '", ast_identifier->value, "'");
  if (auto type = SymbolTable::sym_cast<SymbolTable::Type*>(entry)) {
    m_expression_value = nullptr;
    m_expression_type = type;
  }
  else if (auto value = SymbolTable::sym_cast<SymbolTable::Variable*>(entry)) {
    m_expression_type = value->m_type;
    m_expression_value = m_ir_builder.CreateLoad(value->m_llvm_value);
  }
  else {
    throw make_error<CodeGeneratorError>("identifier '", ast_identifier->value,
      "' refers to something other than a type or a variable");
  }
}

void CodeGenerator::visit(ast::Real* ast_real)
{
  m_expression_type = m_symbol_table.gotoPath<SymbolTable::Type*>("/real");
  m_expression_value = llvm::ConstantFP::get(m_expression_type->m_llvm_type, ast_real->value);
}

void CodeGenerator::visit(ast::Integer* ast_integer)
{
  m_expression_type = m_symbol_table.gotoPath<SymbolTable::Type*>("/int");
  m_expression_value = llvm::ConstantInt::getSigned(m_expression_type->m_llvm_type, ast_integer->value);
}

void CodeGenerator::visit(ast::Boolean* ast_bool)
{
  m_expression_type = m_symbol_table.gotoPath<SymbolTable::Type*>("/bool");
  m_expression_value = ast_bool->value ? llvm::ConstantInt::getTrue(m_context) : llvm::ConstantInt::getFalse(m_context);
}

void CodeGenerator::visit(ast::String*)
{
  throw make_error<CodeGeneratorError>("strings are not yet implemented");
}

void CodeGenerator::visit(ast::Character*)
{
  throw make_error<CodeGeneratorError>("characters are not yet implemented");
}
