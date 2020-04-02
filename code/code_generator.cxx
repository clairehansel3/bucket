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

#include "code_generator.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/bimap.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
#include <initializer_list>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <string_view>
#include <utility>
#include <vector>

#include <llvm/Analysis/InstructionSimplify.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <iostream>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/StringRef.h>

/*
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
*/


SymbolTable::Type* resolveType(SymbolTable& symbol_table, const ast::Expression* expression_ptr)
{
  if (auto identifier_ptr = ast::ast_cast<const ast::Identifier*>(expression_ptr)) {
    auto entry = symbol_table.lookup(identifier_ptr->value);
    if (!entry)
      throw make_error<CodeGeneratorError>("undefined identifier '", identifier_ptr->value, '\'');
    auto type = SymbolTable::sym_cast<SymbolTable::Type*>(entry);
    if (!type)
      throw make_error<CodeGeneratorError>("identifier '", identifier_ptr->value, "' is not a type");
    return type;
  }
  else {
    throw make_error<CodeGeneratorError>("non identifier types not yet implemented");
  }
}

void initializeBuiltins(SymbolTable& symbol_table, llvm::LLVMContext& llvm_context, llvm::Module& llvm_module)
{
  auto registerBuiltinMethod = [&symbol_table, &llvm_module](
    SymbolTable::Type* type,
    std::string_view name,
    std::initializer_list<SymbolTable::Type*> args,
    SymbolTable::Type* return_class,
    std::string_view link_name
  ) {
    std::vector<SymbolTable::Type*> st_args{args};
    std::vector<llvm::Type*> llvm_args{st_args.size() + 1};
    llvm_args[0] = symbol_table.getReferenceType(type)->m_llvm_type;
    std::transform(st_args.begin(), st_args.end(), llvm_args.begin() + 1, [](SymbolTable::Type* typ){return typ->m_llvm_type;});
    auto method = symbol_table.createMethod(name, std::move(st_args), return_class);
    method->m_llvm_function = llvm::Function::Create(llvm::FunctionType::get(return_class->m_llvm_type, llvm_args, false),
      llvm::Function::ExternalLinkage, llvm::StringRef(link_name.data(), link_name.size()), llvm_module);
  };

  auto bool_class = symbol_table.createType("bool", llvm::IntegerType::getInt1Ty(llvm_context));
  auto int_class  = symbol_table.createType("int", llvm::IntegerType::getInt64Ty(llvm_context));
  [[maybe_unused]] auto nil_class = symbol_table.createType("nil", llvm::Type::getVoidTy(llvm_context));
  symbol_table.pushScope("bool");
  registerBuiltinMethod(bool_class, "__and__", {bool_class}, bool_class, "bucket_bool_and");
  registerBuiltinMethod(bool_class, "__or__", {bool_class}, bool_class, "bucket_bool_or");
  registerBuiltinMethod(bool_class, "__not__", {}, bool_class, "bucket_bool_not");
  symbol_table.popScope();
  symbol_table.pushScope("int");
  registerBuiltinMethod(int_class, "__add__", {int_class}, int_class, "bucket_int_add");
  registerBuiltinMethod(int_class, "__sub__", {int_class}, int_class, "bucket_int_sub");
  registerBuiltinMethod(int_class, "__mul__", {int_class}, int_class, "bucket_int_mul");
  registerBuiltinMethod(int_class, "__div__", {int_class}, int_class, "bucket_int_div");
  registerBuiltinMethod(int_class, "__mod__", {int_class}, int_class, "bucket_int_mod");
  registerBuiltinMethod(int_class, "__lt__", {int_class}, bool_class, "bucket_int_lt");
  registerBuiltinMethod(int_class, "__le__", {int_class}, bool_class, "bucket_int_le");
  registerBuiltinMethod(int_class, "__eq__", {int_class}, bool_class, "bucket_int_eq");
  registerBuiltinMethod(int_class, "__ge__", {int_class}, bool_class, "bucket_int_ge");
  registerBuiltinMethod(int_class, "__gt__", {int_class}, bool_class, "bucket_int_gt");
  registerBuiltinMethod(int_class, "print", {}, nil_class, "bucket_int_print");
  symbol_table.popScope();
  auto syscall_class = symbol_table.createType("syscall", nullptr);
  symbol_table.pushScope("syscall");
  auto registerSyscall = [&symbol_table, &llvm_module](
    std::string_view name,
    std::initializer_list<SymbolTable::Type*> args,
    SymbolTable::Type* return_class,
    std::string_view link_name
  ) {
    std::vector<SymbolTable::Type*> st_args{args};
    std::vector<llvm::Type*> llvm_args{st_args.size()};
    std::transform(st_args.begin(), st_args.end(), llvm_args.begin(), [](SymbolTable::Type* typ){return typ->m_llvm_type;});
    auto method = symbol_table.createMethod(name, std::move(st_args), return_class);
    method->m_llvm_function = llvm::Function::Create(llvm::FunctionType::get(return_class->m_llvm_type, llvm_args, false),
      llvm::Function::ExternalLinkage, llvm::StringRef(link_name.data(), link_name.size()), llvm_module);
  };
  symbol_table.popScope();
}

FirstPass::FirstPass(SymbolTable& symbol_table)
: m_symbol_table{symbol_table}
{}

void FirstPass::visit(ast::Program* program_ptr)
{
  for (auto& global_ptr : program_ptr->globals) {
    ast::dispatch(global_ptr.get(), this);
  }
}

void FirstPass::visit(ast::Global*) {}

void FirstPass::visit(ast::Class* class_ptr)
{
  //if (m_symbol_table.lookup)
  m_symbol_table.createClass(class_ptr->name);
  m_symbol_table.pushScope(class_ptr->name);
  for (auto& global_ptr : class_ptr->globals) {
    ast::dispatch(global_ptr.get(), this);
  }
  m_symbol_table.popScope();
}

void FirstPass::visit(ast::Method*)
{}

void FirstPass::visit(ast::Field* field_ptr)
{}

SecondPass::SecondPass(SymbolTable& symbol_table)
: m_symbol_table{symbol_table},
  m_st_class{nullptr}
{}

void SecondPass::visit(ast::Program* program_ptr)
{
  for (auto& global_ptr : program_ptr->globals)
    ast::dispatch(global_ptr.get(), this);
}

void SecondPass::visit(ast::Class* class_ptr)
{
  auto old_m_st_class = m_st_class;
  m_st_class = m_symbol_table.lookupKnownNameInCurrentScope<SymbolTable::Class>(class_ptr->name);
  m_symbol_table.pushScope(class_ptr->name);
  for (auto& global_ptr : class_ptr->globals)
    ast::dispatch(global_ptr.get(), this);
  m_symbol_table.popScope();
  m_st_class = old_m_st_class;
}

void SecondPass::visit(ast::Method* method_ptr)
{
  std::vector<SymbolTable::Type*> argument_types{method_ptr->arguments.size()};
  std::transform(method_ptr->arguments.begin(), method_ptr->arguments.end(),
    argument_types.begin(), [this](auto& argument){return resolveType(m_symbol_table, argument.second.get());});
  auto return_type = resolveType(m_symbol_table, method_ptr->return_type.get());
  m_symbol_table.createMethod(method_ptr->name, std::move(argument_types), return_type);
}

void SecondPass::visit(ast::Field* field_ptr)
{
  m_st_class->m_fields.push_back(m_symbol_table.createField(field_ptr->name, resolveType(m_symbol_table, field_ptr->type.get())));
}

void resolveClasses(SymbolTable& symbol_table, llvm::LLVMContext& llvm_context)
{
  using Graph = boost::adjacency_list<>;
  using BiMap = boost::bimap<SymbolTable::Class*, Graph::vertex_descriptor>;
  Graph graph;
  BiMap table;
  for (auto entry : symbol_table)
    if (auto st_class = SymbolTable::sym_cast<SymbolTable::Class*>(entry))
      table.insert(BiMap::value_type(st_class, boost::add_vertex(graph)));
  for (auto entry : symbol_table)
    if (auto field = SymbolTable::sym_cast<SymbolTable::Field*>(entry))
      if (auto field_class = SymbolTable::sym_cast<SymbolTable::Class*>(field->m_type))
        boost::add_edge(table.left.find(boost::polymorphic_pointer_downcast<SymbolTable::Class>(symbol_table.getParentType(field)))->second, table.left.find(field_class)->second, graph);
  std::vector<Graph::vertex_descriptor> result;
  try {
    boost::topological_sort(graph, std::back_inserter(result));
  } catch (boost::not_a_dag&) {
    throw make_error<CodeGeneratorError>("cyclic class dependencies detected");
  }
  for (auto iter : result) {
    auto st_class = table.right.find(iter)->second;
    if (st_class->m_fields.size()) {
      std::vector<llvm::Type*> llvm_types{st_class->m_fields.size()};
      assert(std::all_of(st_class->m_fields.begin(), st_class->m_fields.end(), [](bool b){return b;}));
      std::transform(st_class->m_fields.begin(), st_class->m_fields.end(), llvm_types.begin(), [](SymbolTable::Field* field){return field->m_type->m_llvm_type;});
      BUCKET_ASSERT(std::all_of(llvm_types.begin(), llvm_types.end(), [](bool b){return b;}));
      st_class->m_llvm_type = llvm::StructType::create(llvm_context, llvm_types);
    }
  }
}

void resolveMethods(SymbolTable& symbol_table, llvm::Module& llvm_module)
{
  // we have to be careful to avoid iterator invalidation here
  std::vector<SymbolTable::Entry*> symbol_table_entries{symbol_table.begin(), symbol_table.end()};
  for (auto entry_ptr : symbol_table_entries) {
    if (auto method_ptr = SymbolTable::sym_cast<SymbolTable::Method*>(entry_ptr)) {
      if (!method_ptr->m_llvm_function) {
        auto method_type = symbol_table.getParentType(method_ptr);
        std::vector<llvm::Type*> argument_types;
        if (method_type->m_llvm_type) {
          // non empty type
          argument_types.resize(method_ptr->m_argument_types.size() + 1);
          argument_types[0] = symbol_table.getReferenceType(method_type)->m_llvm_type;
          std::transform(method_ptr->m_argument_types.begin(), method_ptr->m_argument_types.end(), argument_types.begin() + 1, [](SymbolTable::Type* type){return type->m_llvm_type;});
        }
        else {
          // empty type
          argument_types.resize(method_ptr->m_argument_types.size());
          std::transform(method_ptr->m_argument_types.begin(), method_ptr->m_argument_types.end(), argument_types.begin(), [](SymbolTable::Type* type){return type->m_llvm_type;});
        }
        auto function_type = llvm::FunctionType::get(method_ptr->m_return_type->m_llvm_type, argument_types, false);
        method_ptr->m_llvm_function = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, llvm::StringRef(method_ptr->path().data(), method_ptr->path().size()), llvm_module);
      }
    }
  }
}

ThirdPass::ThirdPass(SymbolTable& symbol_table, llvm::LLVMContext& llvm_context)
: m_st_class{nullptr},
  m_symbol_table{symbol_table},
  m_llvm_context{llvm_context},
  m_llvm_ir_builder{llvm_context}
{}

void ThirdPass::visit(ast::Program* program_ptr)
{
  for (auto& global_ptr : program_ptr->globals)
    ast::dispatch(global_ptr.get(), this);
}

void ThirdPass::visit(ast::Class* class_ptr)
{
  auto old_m_st_class = m_st_class;
  m_st_class = m_symbol_table.lookupKnownNameInCurrentScope<SymbolTable::Class>(class_ptr->name);
  m_symbol_table.pushScope(class_ptr->name);
  for (auto& global_ptr : class_ptr->globals)
    ast::dispatch(global_ptr.get(), this);
  m_symbol_table.popScope();
  m_st_class = old_m_st_class;
}

void ThirdPass::visit(ast::Method* method_ptr)
{
  // Set member variables
  m_current_method = m_symbol_table.lookupKnownNameInCurrentScope<SymbolTable::Method>(method_ptr->name);
  m_method_returned = false;

  // Create entry block and set IR builder to insert there
  m_llvm_ir_builder.SetInsertPoint(llvm::BasicBlock::Create(m_llvm_context, "$entry", m_current_method->m_llvm_function));

  // Push an anonymous scope to hold variables defined in the method
  m_symbol_table.pushScope();

  // Create variables corresponding to the method's arguments
  auto llvm_arg_iter = m_current_method->m_llvm_function->arg_begin();
  auto llvm_arg_end = m_current_method->m_llvm_function->arg_end();
  auto ast_arg_iter = method_ptr->arguments.begin();
  auto ast_arg_end = method_ptr->arguments.end();
  auto sym_arg_iter = m_current_method->m_argument_types.begin();
  auto sym_arg_end = m_current_method->m_argument_types.end();
  if (m_symbol_table.getParentType(m_current_method)->m_llvm_type) {
    auto this_variable = createAllocaVariable("this", m_symbol_table.getReferenceType(m_symbol_table.getParentType(m_current_method)));
    m_llvm_ir_builder.CreateStore(llvm_arg_iter, this_variable->m_llvm_value);
    ++llvm_arg_iter;
  }
  while (llvm_arg_iter != llvm_arg_end) {
    assert(ast_arg_iter != ast_arg_end);
    assert(sym_arg_iter != sym_arg_end);
    auto arg_variable = createAllocaVariable(ast_arg_iter->first, *sym_arg_iter);
    m_llvm_ir_builder.CreateStore(llvm_arg_iter, arg_variable->m_llvm_value);
    ++llvm_arg_iter;
    ++ast_arg_iter;
    ++sym_arg_iter;
  }

  // Visit statements and generate code
  for (auto& statement_ptr : method_ptr->statements)
    ast::dispatch(statement_ptr.get(), this);

  // Check that the function has returned
  if (!m_method_returned) {
    if (m_current_method->m_return_type != m_symbol_table.lookupKnownPath<SymbolTable::Type>("/nil"))
      throw make_error<CodeGeneratorError>("method does not return");
    m_llvm_ir_builder.CreateRetVoid();
  }

  // Verify the function
  llvm::verifyFunction(*m_current_method->m_llvm_function);

  // Clear variables and exit the scope
  m_symbol_table.popScope();
}

void ThirdPass::visit(ast::Field*)
{}

void ThirdPass::visit(ast::Declaration* declaration_ptr)
{
  createAllocaVariable(declaration_ptr->name, resolveType(m_symbol_table, declaration_ptr->type.get()));
}

void ThirdPass::visit(ast::If* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::InfiniteLoop* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::PreTestLoop* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::Break* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::Cycle* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::Ret* ret_ptr)
{
  if (m_method_returned)
    throw std::runtime_error("code after function returned");
  ast::dispatch(ret_ptr->expression.get(), this);
  if (!m_current_value)
    throw std::runtime_error("return type must be a runtime value");
  if (m_current_value_class != m_current_method->m_return_type)
    throw std::runtime_error("return types don't match");
  m_llvm_ir_builder.CreateRet(m_current_value);
  m_method_returned = true;
}

void ThirdPass::visit(ast::ExpressionStatement* expression_statement_ptr)
{
  ast::dispatch(expression_statement_ptr->expression.get(), this);
}

void ThirdPass::visit(ast::Assignment* assignment_ptr)
{
  // Check if method has already returned
  if (m_method_returned)
    throw make_error<CodeGeneratorError>("code after method has returned");

  // Get name of lhs variable
  std::string lhs;
  {
    auto lhs_identifier = ast::ast_cast<ast::Identifier*>(assignment_ptr->left.get());
    if (!lhs_identifier)
      throw make_error<CodeGeneratorError>("left hand side of assignment statement must be an identifier");
    lhs = lhs_identifier->value;
  }

  // Visit rhs
  assignment_ptr->right->receive(this);

  SymbolTable::Variable* lhs_variable;
  if (auto entry = m_symbol_table.lookup(lhs)) {
    // lhs is already defined
    lhs_variable = SymbolTable::sym_cast<SymbolTable::Variable*>(entry);
    if (!lhs_variable)
      throw make_error<CodeGeneratorError>("left hand side of assignment statement is not a variable");
    if (lhs_variable->m_type != m_current_value_class)
      throw make_error<CodeGeneratorError>("type mismatch");
  }
  else {
    lhs_variable = createAllocaVariable(lhs, m_current_value_class);
  }
  m_llvm_ir_builder.CreateStore(m_current_value, lhs_variable->m_llvm_value);
}

void ThirdPass::visit(ast::Call* call_ptr)
{
  if (m_method_returned)
    throw make_error<CodeGeneratorError>("code after function returns");
  // resolve object being called
  ast::dispatch(call_ptr->expression.get(), this);
  // check for special functions
  if (call_ptr->name == "dereference") {
    BUCKET_UNREACHABLE();
  }
  else if (call_ptr->name == "addressof") {
    BUCKET_UNREACHABLE();
  }
  // find the method
  auto found = m_symbol_table.lookupInScope(concatenate(m_current_value_class->path(), '/'), call_ptr->name);
  if (!found)
    throw std::runtime_error(concatenate("unknown method '", call_ptr->name, '\''));
  auto method = SymbolTable::sym_cast<SymbolTable::Method*>(found);
  if (!method)
    throw std::runtime_error("not a method");

  std::vector<llvm::Value*> arguments;
  if (m_current_value_class->m_llvm_type) {
    BUCKET_ASSERT(m_current_value);
    arguments.resize(method->m_argument_types.size() + 1);
    llvm::IRBuilder<> temporary_ir_builder{&m_current_method->m_llvm_function->getEntryBlock(), m_current_method->m_llvm_function->getEntryBlock().begin()};
    auto alloca_inst = temporary_ir_builder.CreateAlloca(m_current_value_class->m_llvm_type, 0, nullptr, "temp");
    m_llvm_ir_builder.CreateStore(m_current_value, alloca_inst);
    arguments[0] = alloca_inst;
    if (method->m_argument_types.size() != call_ptr->arguments.size())
      throw std::runtime_error("argument count mismatch");
    auto argument_class_iter = method->m_argument_types.begin();
    auto argument_expression_iter = call_ptr->arguments.begin();
    auto argument_iter = arguments.begin() + 1;
    while (argument_iter != arguments.end()) {
      (*argument_expression_iter)->receive(this);
      if (!m_current_value)
        throw std::runtime_error("cannot pass a class to a method");
      if (m_current_value_class != *argument_class_iter)
        throw std::runtime_error("argument class mismatch");
      *argument_iter = m_current_value;
      ++argument_class_iter;
      ++argument_expression_iter;
      ++argument_iter;
    }
  }
  else {
    // call on namespace
    arguments.resize(method->m_argument_types.size());
    if (m_current_value)
      BUCKET_UNREACHABLE();
    if (method->m_argument_types.size() != call_ptr->arguments.size())
      throw std::runtime_error("argument count mismatch");
    auto argument_class_iter = method->m_argument_types.begin();
    auto argument_expression_iter = call_ptr->arguments.begin();
    auto argument_iter = arguments.begin();
    while (argument_iter != arguments.end()) {
      ast::dispatch(argument_expression_iter->get(), this);
      if (!m_current_value)
        throw std::runtime_error("cannot pass a class to a method");
      if (m_current_value_class != *argument_class_iter)
        throw std::runtime_error("argument class mismatch");
      *argument_iter = m_current_value;
      ++argument_class_iter;
      ++argument_expression_iter;
      ++argument_iter;
    }
  }
  m_current_value = m_llvm_ir_builder.CreateCall(method->m_llvm_function, arguments);
  m_current_value_class = method->m_return_type;
}

void ThirdPass::visit(ast::Identifier* identifier_ptr)
{
  auto entry = m_symbol_table.lookup(identifier_ptr->value);
  if (!entry)
    throw std::runtime_error("unknown identifier");
  if (auto type = SymbolTable::sym_cast<SymbolTable::Type*>(entry)) {
    m_current_value_class = type;
    m_current_value = nullptr;
  }
  else if (auto value = SymbolTable::sym_cast<SymbolTable::Variable*>(entry)) {
    m_current_value_class = value->m_type;
    m_current_value = m_llvm_ir_builder.CreateLoad(value->m_llvm_value);
  }
  else {
    BUCKET_UNREACHABLE();
  }
}

void ThirdPass::visit(ast::Real* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::Integer* int_ast_ptr)
{
  auto int_sym_type = m_symbol_table.lookupKnownPath<SymbolTable::Type>("/int");
  m_current_value_class = int_sym_type;
  m_current_value = llvm::ConstantInt::getSigned(int_sym_type->m_llvm_type, int_ast_ptr->value);
}

void ThirdPass::visit(ast::Boolean* boolean_ptr)
{
  m_current_value_class = m_symbol_table.lookupKnownPath<SymbolTable::Type>("/bool");
  m_current_value = boolean_ptr->value ? llvm::ConstantInt::getTrue(m_llvm_context) : llvm::ConstantInt::getFalse(m_llvm_context);
}

void ThirdPass::visit(ast::String* if_ptr)
{
  BUCKET_UNREACHABLE();
}

void ThirdPass::visit(ast::Character* if_ptr)
{
  BUCKET_UNREACHABLE();
}

SymbolTable::Variable* ThirdPass::createAllocaVariable(std::string_view name, SymbolTable::Type* type)
{
  auto variable = m_symbol_table.createVariable(name, type);
  llvm::IRBuilder<> temporary_ir_builder{&m_current_method->m_llvm_function->getEntryBlock(), m_current_method->m_llvm_function->getEntryBlock().begin()};
  variable->m_llvm_value = temporary_ir_builder.CreateAlloca(type->m_llvm_type, 0, nullptr, llvm::StringRef(variable->path().data(), variable->path().size()));
  return variable;
}

void finalizeModule(llvm::LLVMContext& llvm_context, llvm::Module& llvm_module, SymbolTable& symbol_table)
{
  llvm::IRBuilder<> llvm_ir_builder{llvm_context};
  auto actual_main = llvm::Function::Create(
    llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), {llvm::Type::getInt32Ty(llvm_context), llvm::Type::getInt8Ty(llvm_context)->getPointerTo()->getPointerTo()}, false),
    llvm::Function::ExternalLinkage, "main", &llvm_module
  );
  auto module_cls = symbol_table.lookupKnownPath<SymbolTable::Class>("/main");
  auto module_main = symbol_table.lookupKnownPath<SymbolTable::Method>("/main/main");
  assert(module_main->m_return_type == symbol_table.lookupKnownPath<SymbolTable::Type>("/bool"));
  assert(module_main->m_argument_types.size() == 0);
  llvm_ir_builder.SetInsertPoint(llvm::BasicBlock::Create(llvm_context, "$entry", actual_main));
  BUCKET_ASSERT(!module_cls->m_llvm_type);
  //auto alloca_inst = llvm_ir_builder.CreateAlloca(module_cls->m_llvm_type, 0, nullptr, "main");
  //std::vector<llvm::Value*> args;
  //args.push_back(alloca_inst);
  auto call_inst = llvm_ir_builder.CreateCall(module_main->m_llvm_function);
  llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(llvm_context, "$then", actual_main);
  llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(llvm_context, "$else", actual_main);
  llvm_ir_builder.CreateCondBr(call_inst, then_bb, else_bb);
  llvm_ir_builder.SetInsertPoint(then_bb);
  llvm_ir_builder.CreateRet(llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(llvm_context), 0));
  llvm_ir_builder.SetInsertPoint(else_bb);
  llvm_ir_builder.CreateRet(llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(llvm_context), 1));
  llvm::verifyModule(llvm_module, &llvm::errs());
  //llvm_module.dump();
}

void writeModule(llvm::Module& llvm_module, std::string_view output_file)
{
  std::error_code code;
  llvm::raw_fd_ostream stream{llvm::StringRef(output_file.data(), output_file.size()), code, llvm::sys::fs::F_None};
  llvm::WriteBitcodeToFile(llvm_module, stream);
  stream.flush();
}

#include <system_error>
void printModuleIR(llvm::Module& llvm_module, std::optional<std::string> output_path_optional)
{
  std::string name = output_path_optional ? *output_path_optional : std::string("-");
  std::error_code ec;
  llvm::raw_fd_ostream ostream{llvm::StringRef(name.data(), name.size()), ec};
  llvm_module.print(ostream, nullptr);
  //llvm::PrintModulePass pmp{ostream};
  //llvm::AnalysisManager am;
  //pmp.run(llvm_module, am);
}

void printModuleBC(llvm::Module& llvm_module, std::optional<std::string> output_path_optional)
{
  std::string name = output_path_optional ? *output_path_optional : std::string("-");
  std::error_code ec;
  llvm::raw_fd_ostream ostream{llvm::StringRef(name.data(), name.size()), ec};
  llvm::WriteBitcodeToFile(llvm_module, ostream);
}

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

void printModuleOBJ(llvm::Module& llvm_module, std::optional<std::string> output_path_optional)
{
  std::string name = output_path_optional ? *output_path_optional : std::string("-");
  std::error_code ec;
  llvm::raw_fd_ostream dest{llvm::StringRef(name.data(), name.size()), ec};

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  auto TargetTriple = llvm::sys::getDefaultTargetTriple();
  llvm_module.setTargetTriple(TargetTriple);

  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

  BUCKET_ASSERT(Target);

  auto CPU = "generic";
  auto Features = "";

  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>();
  auto TheTargetMachine =
      Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

  llvm_module.setDataLayout(TheTargetMachine->createDataLayout());

  llvm::legacy::PassManager pass;
  auto FileType = llvm::CGFT_ObjectFile;

  if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    assert(false);
  }

  pass.run(llvm_module);
  dest.flush();
}
