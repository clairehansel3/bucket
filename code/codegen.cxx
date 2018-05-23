#include "codegen.hxx"
#include <boost/bimap.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <iterator>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>

CodeGenerator::CodeGenerator()
: m_module{"program", m_context}, m_builder{m_context} {}

void CodeGenerator::generate(ast::Class* cls) {
  setup(cls);
  cls->receive(this);
  finalize();
}

void CodeGenerator::print() {
  llvm::PrintModulePass pass;
  llvm::AnalysisManager<llvm::Module> manager;
  pass.run(m_module, manager);
}

void CodeGenerator::showSymbols() {
  m_symbol_table.print();
}

llvm::Module& CodeGenerator::module() {
  return m_module;
}

void CodeGenerator::setup(ast::Class* cls) {
  initializeBuiltins();
  initializeClasses(cls);
  initializeFieldsAndMethods(cls);
  resolveComposites();
  resolveMethods();
}

sym::Class* CodeGenerator::lookupClass(const ast::Expression* expression) const {
  if (auto identifier = ast::ast_cast<const ast::Identifier*>(expression)) {
    auto entry = m_symbol_table.lookup(identifier->value);
    if (!entry)
      throw std::runtime_error("undefined symbol");
    auto cls = sym::sym_cast<sym::Class*>(entry);
    if (!cls)
      throw std::runtime_error("not a class");
    return cls;
  }
  assert(false);
  return nullptr;
}

void CodeGenerator::initializeBuiltins() {
  m_bool = m_symbol_table.createClass("bool", llvm::IntegerType::getInt1Ty(m_context));
  m_int = m_symbol_table.createClass("int", llvm::IntegerType::getInt64Ty(m_context));
  m_void = m_symbol_table.createClass("void", llvm::Type::getVoidTy(m_context));
  {
    auto method = m_symbol_table.createMethod("foo", std::vector<sym::Class*>(), m_void);
    method->setFunction(llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(m_context), false), llvm::Function::ExternalLinkage, "foo", &m_module));
  }
  {
    std::vector<llvm::Type*> args;
    std::vector<sym::Class*> argc;
    args.push_back(m_int->type());
    argc.push_back(m_int);
    auto method = m_symbol_table.createMethod("print_integer", std::move(argc), m_void);
    method->setFunction(llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(m_context), std::move(args), false), llvm::Function::ExternalLinkage, "print_integer", &m_module));
  }
}

void CodeGenerator::initializeClasses(const ast::Class* cls) {
  m_symbol_table.createCompositeClass(cls->name);
  m_symbol_table.pushScope(cls->name);
  for (auto& global : cls->body)
    if (auto inner_class = ast::ast_cast<const ast::Class*>(global.get()))
      initializeClasses(inner_class);
  m_symbol_table.popScope();
}

void CodeGenerator::initializeFieldsAndMethods(const ast::Class* cls) {
  auto composite = boost::polymorphic_pointer_downcast<sym::CompositeClass>(m_symbol_table.lookupKnown(cls->name));
  m_symbol_table.pushScope(cls->name);
  for (auto& global : cls->body) {
    if (auto inner_class = ast::ast_cast<const ast::Class*>(global.get())) {
      initializeFieldsAndMethods(inner_class);
    } else if (auto field = ast::ast_cast<const ast::Field*>(global.get())) {
      composite->fields().push_back(m_symbol_table.createField(field->name, lookupClass(field->cls.get())));
    } else {
      auto method = boost::polymorphic_pointer_downcast<const ast::Method>(global.get());
      std::vector<sym::Class*> argument_classes{method->args.size()};
      for (std::vector<sym::Class*>::size_type i = 0; i != argument_classes.size(); ++i)
        argument_classes[i] = lookupClass(method->args[i].second.get());
      auto return_class = method->return_class ? lookupClass(method->return_class.get()) : boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".void"));
      m_symbol_table.createMethod(method->name, std::move(argument_classes), return_class);
    }
  }
  m_symbol_table.popScope();
}

void CodeGenerator::resolveComposites() {
  using Graph = boost::adjacency_list<>;
  using BiMap = boost::bimap<sym::CompositeClass*, Graph::vertex_descriptor>;
  Graph graph;
  BiMap table;
  for (auto entry : m_symbol_table)
    if (auto composite = sym::sym_cast<sym::CompositeClass*>(entry))
      table.insert(BiMap::value_type(composite, boost::add_vertex(graph)));
  for (auto entry : m_symbol_table) {
    if (auto field = sym::sym_cast<sym::Field*>(entry)) {
      auto parent = m_symbol_table.getParentClass(field);
      if (auto field_composite = sym::sym_cast<sym::CompositeClass*>(field->classof()))
        boost::add_edge(table.left.find(parent)->second, table.left.find(field_composite)->second, graph);
    }
  }
  std::vector<Graph::vertex_descriptor> result;
  try {
    boost::topological_sort(graph, std::back_inserter(result));
  } catch (boost::not_a_dag&) {
    throw std::runtime_error("cyclic class dependencies detected");
  }
  for (auto iter = result.rbegin(); iter != result.rend(); ++iter)
    resolveComposite(table.right.find(*iter)->second);
}

void CodeGenerator::resolveComposite(sym::CompositeClass* composite) {
  std::vector<llvm::Type*> llvm_types{composite->fields().size()};
  for (std::vector<llvm::Type*>::size_type i = 0; i != llvm_types.size(); ++i) {
    assert(composite->fields()[i]->classof()->type());
    llvm_types[i] = composite->fields()[i]->classof()->type();
  }
  composite->setType(llvm::StructType::create(m_context, llvm_types));
}

void CodeGenerator::resolveMethods() {
  for (auto entry : m_symbol_table) {
    if (auto method = sym::sym_cast<sym::Method*>(entry)) {
      if (!method->function()) {
        std::vector<llvm::Type*> argument_types{method->argumentClasses().size() + 1};
        argument_types[0] = m_symbol_table.getReferenceClass(m_symbol_table.getParentClass(method))->type();
        for (std::vector<llvm::Type*>::size_type i = 0; i != method->argumentClasses().size(); ++i)
          argument_types[i + 1] = method->argumentClasses()[i]->type();
        auto function_type = llvm::FunctionType::get(method->returnClass()->type(), argument_types, false);
        method->setFunction(llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, llvm::StringRef(method->fullname().data(), method->fullname().size()), &m_module));
      }
    }
  }
}

sym::Variable* CodeGenerator::createAllocaVariable(std::string_view name, sym::Class* cls) {
  auto variable = m_symbol_table.createVariable(name, cls);
  llvm::IRBuilder<> temporary_ir_builder{&m_method->function()->getEntryBlock(), m_method->function()->getEntryBlock().begin()};
  auto alloca_inst = temporary_ir_builder.CreateAlloca(cls->type(), 0, nullptr, llvm::StringRef(variable->fullname().data(), variable->fullname().size()));
  variable->setValue(alloca_inst);
  return variable;
}

void CodeGenerator::visit(ast::Class* cls) {
  m_symbol_table.pushScope(cls->name);
  for (auto& global : cls->body)
    global->receive(this);
  m_symbol_table.popScope();
}

void CodeGenerator::visit(ast::Method* method) {
  m_method = boost::polymorphic_pointer_downcast<sym::Method>(m_symbol_table.lookupKnown(method->name));
  assert(m_method);
  m_builder.SetInsertPoint(llvm::BasicBlock::Create(m_context, "$entry", m_method->function()));
  m_symbol_table.pushAnonymousScope();
  {
    auto llvm_arg_iter = m_method->function()->arg_begin();
    auto llvm_arg_end = m_method->function()->arg_end();
    auto ast_arg_iter = method->args.begin();
    auto ast_arg_end = method->args.end();
    auto arg_cls_iter = m_method->argumentClasses().begin();
    auto arg_cls_end = m_method->argumentClasses().end();

    auto this_variable = createAllocaVariable("this", m_symbol_table.getReferenceClass(m_symbol_table.getParentClass(m_method)));
    m_builder.CreateStore(llvm_arg_iter, this_variable->value());
    ++llvm_arg_iter;

    while (llvm_arg_iter != llvm_arg_end) {
      assert(ast_arg_iter != ast_arg_end);
      assert(arg_cls_iter != arg_cls_end);

      auto arg_variable = createAllocaVariable(ast_arg_iter->first, *arg_cls_iter);
      m_builder.CreateStore(llvm_arg_iter, arg_variable->value());

      ++llvm_arg_iter;
      ++ast_arg_iter;
      ++arg_cls_iter;
    }
  }
  // Generate Code
  for (auto& statement : method->body)
    statement->receive(this);

  // Make sure the function has returned
  if (!m_ret) {
    if (m_method->returnClass() != m_void)
      throw std::runtime_error("method does not return");
    m_builder.CreateRetVoid();
  }

  // Verify the function
  llvm::verifyFunction(*m_method->function());

  // Clear variables and exit the scope
  m_symbol_table.popAnonymousScope();
}

void CodeGenerator::visit(ast::Field*) {}

void CodeGenerator::visit(ast::If* conditional) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");

  // Create Merge Block
  llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(m_context, "$merge", m_method->function());

  auto generate = [this, merge_bb](std::unique_ptr<ast::Expression>& condition, std::vector<std::unique_ptr<ast::Statement>>& body){

    // Visit the condition
    condition->receive(this);

    // Ensure that the condition is a boolean
    if (m_expr_class != m_bool)
      throw std::runtime_error("condition is not a boolean");

    // Create labels for when the condition is true and false
    llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(m_context, "$then", m_method->function());
    llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(m_context, "$else", m_method->function());

    // Create a branch instruction
    m_builder.CreateCondBr(m_value, then_bb, else_bb);

    // Start generating code for the conditional body
    m_builder.SetInsertPoint(then_bb);
    m_symbol_table.pushAnonymousScope();
    for (auto& statement : body)
      statement->receive(this);
    m_symbol_table.popAnonymousScope();

    // Jump to merge point
    m_builder.CreateBr(merge_bb);

    // Set the insert point to the else block
    m_builder.SetInsertPoint(else_bb);
  };

  // Generate code for the main conditional and the elifs
  generate(conditional->condition, conditional->if_body);
  for (auto& elif : conditional->elif_bodies)
    generate(elif.first, elif.second);

  m_symbol_table.pushAnonymousScope();
  // Generate code for the else body
  for (auto& statement : conditional->else_body)
    statement->receive(this);
  m_symbol_table.popAnonymousScope();
  m_builder.CreateBr(merge_bb);

  // Set the insert point to after the merge block
  m_builder.SetInsertPoint(merge_bb);
}

void CodeGenerator::visit(ast::Loop* loop) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");

  // Create Labels
  llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(m_context, "$loop", m_method->function());
  llvm::BasicBlock* done_bb = llvm::BasicBlock::Create(m_context, "$done", m_method->function());

  // Push loop exit block
  m_loop_blocks.emplace(done_bb, loop_bb);

  // Create Branch
  m_builder.CreateBr(loop_bb);

  // Set insert point
  m_builder.SetInsertPoint(loop_bb);

  // Generate Code
  m_symbol_table.pushAnonymousScope();
  for (auto& statement : loop->body)
    statement->receive(this);
  m_symbol_table.popAnonymousScope();

  // Create Branch
  m_builder.CreateBr(loop_bb);

  // Pop loop exit block
  m_loop_blocks.pop();

  m_builder.SetInsertPoint(done_bb);
}

void CodeGenerator::visit(ast::Break*) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");

  // Check if we are in a loop in the first place
  if (m_loop_blocks.empty())
    throw std::runtime_error("break outside of a loop");

  m_builder.CreateBr(m_loop_blocks.top().first);
}

void CodeGenerator::visit(ast::Cycle*) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");

  // Check if we are in a loop in the first place
  if (m_loop_blocks.empty())
    throw std::runtime_error("break outside of a loop");

  m_builder.CreateBr(m_loop_blocks.top().second);
}

void CodeGenerator::visit(ast::Ret* ret) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");

  ret->value->receive(this);
  if (m_expr_class != m_method->returnClass())
    throw std::runtime_error("return types don't match");
  m_builder.CreateRet(m_value);
  m_ret = true;

}

void CodeGenerator::visit(ast::ExpressionStatement* expression_statement) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");
  expression_statement->value->receive(this);
}

void CodeGenerator::visit(ast::Assignment* assignment) {
  // Check if function has already returned
  if (m_ret)
    throw std::runtime_error("code after function returned");
  auto lhs = ast::ast_cast<ast::Identifier*>(assignment->left.get());
  if (!lhs)
    throw std::runtime_error("invalid lvalue");
  assignment->right->receive(this);
  auto result = m_symbol_table.lookup(lhs->value);
  sym::Variable* variable;
  if (result) {
    variable = sym::sym_cast<sym::Variable*>(result);
    if (!variable) {
      throw std::runtime_error("assigning to something other than a variable");
    }
    if (variable->classof() != m_expr_class)
      throw std::runtime_error("assignment type error");
  }
  else
    variable = createAllocaVariable(lhs->value, m_expr_class);
  m_builder.CreateStore(m_value, variable->value());
}

void CodeGenerator::visit(ast::Call* call) {
  m_symbol_table.print();
  call->object->receive(this);
  if (!m_value) {
    auto found = m_symbol_table.lookupInScope(m_expr_class->fullname(), call->name);
    if (!found)
      throw std::runtime_error("unknown method");
    auto method = sym::sym_cast<sym::Method*>(found);
    if (!method)
      throw std::runtime_error("not a method");
    if (method->argumentClasses().size() != call->args.size())
      throw std::runtime_error("argument number mismatch");
    std::vector<llvm::Value*> arguments{method->argumentClasses().size()};
    auto argument_class_iter = method->argumentClasses().begin();
    auto argument_expression_iter = call->args.begin();
    auto argument_iter = arguments.begin();
    while (argument_iter != arguments.end()) {
      (*argument_expression_iter)->receive(this);
      if (!m_value)
        throw std::runtime_error("cannot pass a class to a method");
      if (m_expr_class != *argument_class_iter)
        throw std::runtime_error("argument class mismatch");
      *argument_iter = m_value;
      ++argument_class_iter;
      ++argument_expression_iter;
      ++argument_iter;
    }
    m_value = m_builder.CreateCall(method->function(), arguments);
    m_expr_class = method->returnClass();
    return;
  }
  assert(false);
}

void CodeGenerator::visit(ast::Identifier* identifier) {
  std::cout << identifier->value << std::endl;
  auto entry = m_symbol_table.lookup(identifier->value);
  if (!entry)
    throw std::runtime_error("unknown identifier");
  if (auto variable = sym::sym_cast<sym::Variable*>(entry)) {
    m_expr_class = variable->classof();
    m_value = m_builder.CreateLoad(variable->value());
  }
  else if (auto cls = sym::sym_cast<sym::Class*>(entry)) {
    m_expr_class = cls;
    m_value = nullptr;
  }
  else if (auto field = sym::sym_cast<sym::Field*>(entry))
    throw std::runtime_error("identifier refers to field");
  else {
    assert(sym::sym_cast<sym::Method*>(entry));
    throw std::runtime_error("identifier refers to method");
  }
}

void CodeGenerator::visit(ast::Integer* integer) {
  m_expr_class = m_int;
  m_value = llvm::ConstantInt::getSigned(m_int->type(), integer->value);
}

void CodeGenerator::visit(ast::Bool* boolean) {
  m_expr_class = m_bool;
  m_value = boolean->value ? llvm::ConstantInt::getTrue(m_context) : llvm::ConstantInt::getFalse(m_context);
}

void CodeGenerator::finalize() {
  llvm::Function* actual_main;
  {
    std::vector<llvm::Type*> argument_types{2};
    argument_types[0] = llvm::Type::getInt32Ty(m_context);
    argument_types[1] = llvm::Type::getInt8Ty(m_context)->getPointerTo()->getPointerTo();
    auto function_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(m_context), argument_types, false);
    actual_main = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, "main", &m_module);
  }
  auto module_cls = boost::polymorphic_pointer_downcast<sym::CompositeClass>(m_symbol_table.lookupKnown("__module__"));
  auto module_main = boost::polymorphic_pointer_downcast<sym::Method>(m_symbol_table.lookupKnown("__module__.main"));
  assert(module_main->returnClass() == m_bool);
  assert(module_main->argumentClasses().size() == 0);
  m_builder.SetInsertPoint(llvm::BasicBlock::Create(m_context, "$entry", actual_main));
  auto alloca_inst = m_builder.CreateAlloca(module_cls->type(), 0, nullptr, "main");
  std::vector<llvm::Value*> args;
  args.push_back(alloca_inst);
  auto call_inst = m_builder.CreateCall(module_main->function(), args);
  llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(m_context, "$then", actual_main);
  llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(m_context, "$else", actual_main);
  m_builder.CreateCondBr(call_inst, then_bb, else_bb);
  m_builder.SetInsertPoint(then_bb);
  m_builder.CreateRet(llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(m_context), 0));
  m_builder.SetInsertPoint(else_bb);
  m_builder.CreateRet(llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(m_context), 1));
  llvm::verifyModule(m_module, &llvm::errs());
}
