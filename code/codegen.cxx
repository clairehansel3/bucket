#include "codegen.hxx"
#include <algorithm>
#include <boost/bimap.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
#include <llvm/ADT/StringRef.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <iterator>
#include <system_error>

CodeGenerator::CodeGenerator()
: m_llvm_module{"program", m_llvm_context}, m_llvm_ir_builder{m_llvm_context} {}

void CodeGenerator::generate(ast::Class* syntax_tree, std::string_view bitcode_file) {
  assert(syntax_tree);
  initializeBuiltins();
  initializeClasses(syntax_tree);
  initializeFieldsAndMethods(syntax_tree);
  resolveCompositeClasses();
  resolveMethods();
  syntax_tree->receive(this);
  finalize();
  std::error_code code;
  llvm::raw_fd_ostream stream{llvm::StringRef(bitcode_file.data(), bitcode_file.size()), code, llvm::sys::fs::F_None};
  llvm::WriteBitcodeToFile(m_llvm_module, stream);
  stream.flush();
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

void CodeGenerator::declareBuiltinMethod(sym::Class* cls, std::string_view name, std::initializer_list<sym::Class*> args, sym::Class* return_class, std::string_view link_name) {
  std::vector<sym::Class*> sym_args{args};
  std::vector<llvm::Type*> llvm_args{sym_args.size() + 1};
  llvm_args[0] = m_symbol_table.getReferenceClass(cls)->type();
  std::transform(sym_args.begin(), sym_args.end(), llvm_args.begin() + 1, [](sym::Class* cls2){return cls2->type();});
  auto method = m_symbol_table.createMethod(name, std::move(sym_args), return_class);
  method->setFunction(llvm::Function::Create(llvm::FunctionType::get(return_class->type(), llvm_args, false),
    llvm::Function::ExternalLinkage, llvm::StringRef(link_name.data(), link_name.size()), &m_llvm_module));
}

void CodeGenerator::initializeBuiltins() {
  [[maybe_unused]] auto void_class = m_symbol_table.createClass("void", llvm::Type::getVoidTy(m_llvm_context));
  auto bool_class = m_symbol_table.createClass("bool", llvm::IntegerType::getInt1Ty(m_llvm_context));
  auto int_class  = m_symbol_table.createClass("int", llvm::IntegerType::getInt64Ty(m_llvm_context));
  m_symbol_table.pushScope("bool");
  declareBuiltinMethod(bool_class, "__and__", {bool_class}, bool_class, "bucket_bool_and");
  declareBuiltinMethod(bool_class, "__or__", {bool_class}, bool_class, "bucket_bool_or");
  declareBuiltinMethod(bool_class, "__not__", {}, bool_class, "bucket_bool_not");
  m_symbol_table.popScope();
  m_symbol_table.pushScope("int");
  declareBuiltinMethod(int_class, "__add__", {int_class}, int_class, "bucket_int_add");
  declareBuiltinMethod(int_class, "__sub__", {int_class}, int_class, "bucket_int_sub");
  declareBuiltinMethod(int_class, "__mul__", {int_class}, int_class, "bucket_int_mul");
  declareBuiltinMethod(int_class, "__div__", {int_class}, int_class, "bucket_int_div");
  declareBuiltinMethod(int_class, "__mod__", {int_class}, int_class, "bucket_int_mod");
  declareBuiltinMethod(int_class, "__lt__", {int_class}, bool_class, "bucket_int_lt");
  declareBuiltinMethod(int_class, "__le__", {int_class}, bool_class, "bucket_int_le");
  declareBuiltinMethod(int_class, "__eq__", {int_class}, bool_class, "bucket_int_eq");
  declareBuiltinMethod(int_class, "__ge__", {int_class}, bool_class, "bucket_int_ge");
  declareBuiltinMethod(int_class, "__gt__", {int_class}, bool_class, "bucket_int_gt");
  m_symbol_table.popScope();
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
      std::transform(method->args.begin(), method->args.end(), argument_classes.begin(), [this](auto& arg){return lookupClass(arg.second.get());});
      auto return_class = method->return_class ? lookupClass(method->return_class.get()) : boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".void"));
      m_symbol_table.createMethod(method->name, std::move(argument_classes), return_class);
    }
  }
  m_symbol_table.popScope();
}

void CodeGenerator::resolveCompositeClasses() {
  using Graph = boost::adjacency_list<>;
  using BiMap = boost::bimap<sym::CompositeClass*, Graph::vertex_descriptor>;
  Graph graph;
  BiMap table;
  for (auto entry : m_symbol_table)
    if (auto composite = sym::sym_cast<sym::CompositeClass*>(entry))
      table.insert(BiMap::value_type(composite, boost::add_vertex(graph)));
  for (auto entry : m_symbol_table)
    if (auto field = sym::sym_cast<sym::Field*>(entry))
      if (auto field_classof_composite = sym::sym_cast<sym::CompositeClass*>(field->classof()))
        boost::add_edge(table.left.find(m_symbol_table.getParentClass(field))->second, table.left.find(field_classof_composite)->second, graph);
  std::vector<Graph::vertex_descriptor> result;
  try {
    boost::topological_sort(graph, std::back_inserter(result));
  } catch (boost::not_a_dag&) {
    throw std::runtime_error("cyclic class dependencies detected");
  }
  for (auto iter = result.rbegin(); iter != result.rend(); ++iter)
    resolveCompositeClass(table.right.find(*iter)->second);
}

void CodeGenerator::resolveCompositeClass(sym::CompositeClass* composite_class) {
  std::vector<llvm::Type*> llvm_types{composite_class->fields().size()};
  assert(std::all_of(composite_class->fields().begin(), composite_class->fields().end(), [](sym::Field* field){return field;}));
  std::transform(composite_class->fields().begin(), composite_class->fields().end(), llvm_types.begin(), [](sym::Field* field){return field->classof()->type();});
  composite_class->setType(llvm::StructType::create(m_llvm_context, llvm_types));
}

void CodeGenerator::resolveMethods() {
  // For each entry in the symbol table
  for (auto entry : m_symbol_table) {
    // If the entry is a method
    if (auto method = sym::sym_cast<sym::Method*>(entry)) {
      // Skip builtin functions which already have an associated llvm function.
      if (!method->function()) {
        std::vector<llvm::Type*> argument_types{method->argumentClasses().size() + 1};
        argument_types[0] = m_symbol_table.getReferenceClass(m_symbol_table.getParentClass(method))->type();
        std::transform(method->argumentClasses().begin(), method->argumentClasses().end(),
          argument_types.begin() + 1, [](sym::Class* cls){return cls->type();});
        auto function_type = llvm::FunctionType::get(method->returnClass()->type(), argument_types, false);
        method->setFunction(llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, llvm::StringRef(method->fullname().data(), method->fullname().size()), &m_llvm_module));
      }
    }
  }
}

sym::Variable* CodeGenerator::createAllocaVariable(std::string_view name, sym::Class* cls) {
  auto variable = m_symbol_table.createVariable(name, cls);
  llvm::IRBuilder<> temporary_ir_builder{&m_current_method->function()->getEntryBlock(), m_current_method->function()->getEntryBlock().begin()};
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
  // Set CodeGenerator member variables
  m_current_method = boost::polymorphic_pointer_downcast<sym::Method>(m_symbol_table.lookupKnown(method->name));
  m_method_returned = false;

  // Create entry block and set IR builder to insert there
  m_llvm_ir_builder.SetInsertPoint(llvm::BasicBlock::Create(m_llvm_context, "$entry", m_current_method->function()));

  // Push an anonymous scope to hold variables defined in the method
  m_symbol_table.pushAnonymousScope();

  // Create variables corresponding to the method's arguments
  auto llvm_arg_iter = m_current_method->function()->arg_begin();
  auto llvm_arg_end = m_current_method->function()->arg_end();
  auto ast_arg_iter = method->args.begin();
  auto ast_arg_end = method->args.end();
  auto sym_arg_iter = m_current_method->argumentClasses().begin();
  auto sym_arg_end = m_current_method->argumentClasses().end();
  auto this_variable = createAllocaVariable("this", m_symbol_table.getReferenceClass(m_symbol_table.getParentClass(m_current_method)));
  m_llvm_ir_builder.CreateStore(llvm_arg_iter, this_variable->value());
  ++llvm_arg_iter;
  while (llvm_arg_iter != llvm_arg_end) {
    assert(ast_arg_iter != ast_arg_end);
    assert(sym_arg_iter != sym_arg_end);
    auto arg_variable = createAllocaVariable(ast_arg_iter->first, *sym_arg_iter);
    m_llvm_ir_builder.CreateStore(llvm_arg_iter, arg_variable->value());
    ++llvm_arg_iter;
    ++ast_arg_iter;
    ++sym_arg_iter;
  }

  // Visit statements and generate code
  for (auto& statement : method->body)
    statement->receive(this);

  // Check that the function has returned
    if (!m_method_returned) {
    if (m_current_method->returnClass() != boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".void")))
      throw std::runtime_error("method does not return");
    m_llvm_ir_builder.CreateRetVoid();
  }

  // Verify the function
  llvm::verifyFunction(*m_current_method->function());

  // Clear variables and exit the scope
  m_symbol_table.popAnonymousScope();
}

void CodeGenerator::visit(ast::Field*) {}


void CodeGenerator::visit(ast::If* conditional) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");

  // Create Merge Block
  llvm::BasicBlock* merge_bb = llvm::BasicBlock::Create(m_llvm_context, "$merge", m_current_method->function());

  auto generate = [this, merge_bb](std::unique_ptr<ast::Expression>& condition, std::vector<std::unique_ptr<ast::Statement>>& body){

    // Visit the condition
    condition->receive(this);

    if (!m_current_value)
      throw std::runtime_error("condition must be a runtime value");

    // Ensure that the condition is a boolean
    if (m_current_value_class != boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".bool")))
      throw std::runtime_error("condition is not a boolean");

    // Create labels for when the condition is true and false
    llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(m_llvm_context, "$then", m_current_method->function());
    llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(m_llvm_context, "$else", m_current_method->function());

    // Create a branch instruction
    m_llvm_ir_builder.CreateCondBr(m_current_value, then_bb, else_bb);

    // Start generating code for the conditional body
    m_llvm_ir_builder.SetInsertPoint(then_bb);
    m_symbol_table.pushAnonymousScope();
    for (auto& statement : body)
      statement->receive(this);
    m_symbol_table.popAnonymousScope();

    // Jump to merge point
    m_llvm_ir_builder.CreateBr(merge_bb);

    // Set the insert point to the else block
    m_llvm_ir_builder.SetInsertPoint(else_bb);
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
  m_llvm_ir_builder.CreateBr(merge_bb);

  // Set the insert point to after the merge block
  m_llvm_ir_builder.SetInsertPoint(merge_bb);
}

void CodeGenerator::visit(ast::Loop* loop) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");

  // Create Labels
  llvm::BasicBlock* loop_bb = llvm::BasicBlock::Create(m_llvm_context, "$loop", m_current_method->function());
  llvm::BasicBlock* done_bb = llvm::BasicBlock::Create(m_llvm_context, "$done", m_current_method->function());

  // Push loop exit block
  m_loop_blocks.emplace(done_bb, loop_bb);

  // Create Branch
  m_llvm_ir_builder.CreateBr(loop_bb);

  // Set insert point
  m_llvm_ir_builder.SetInsertPoint(loop_bb);

  // Generate Code
  m_symbol_table.pushAnonymousScope();
  for (auto& statement : loop->body)
    statement->receive(this);
  m_symbol_table.popAnonymousScope();

  // Create Branch
  m_llvm_ir_builder.CreateBr(loop_bb);

  // Pop loop exit block
  m_loop_blocks.pop();

  m_llvm_ir_builder.SetInsertPoint(done_bb);
}

void CodeGenerator::visit(ast::Break*) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");

  // Check if we are in a loop in the first place
  if (m_loop_blocks.empty())
    throw std::runtime_error("break outside of a loop");

  m_llvm_ir_builder.CreateBr(m_loop_blocks.top().first);
}

void CodeGenerator::visit(ast::Cycle*) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");

  // Check if we are in a loop in the first place
  if (m_loop_blocks.empty())
    throw std::runtime_error("break outside of a loop");

  m_llvm_ir_builder.CreateBr(m_loop_blocks.top().second);
}

void CodeGenerator::visit(ast::Ret* ret) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");

  ret->value->receive(this);
  if (!m_current_value)
    throw std::runtime_error("return type must be a runtime value");
  if (m_current_value_class != m_current_method->returnClass())
    throw std::runtime_error("return types don't match");
  m_llvm_ir_builder.CreateRet(m_current_value);
  m_method_returned = true;
}


void CodeGenerator::visit(ast::ExpressionStatement* expression_statement) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");
  expression_statement->value->receive(this);
}

void CodeGenerator::visit(ast::Assignment* assignment) {
  // Check if function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");
  auto lhs = ast::ast_cast<ast::Identifier*>(assignment->left.get());
  if (!lhs)
    throw std::runtime_error("invalid lvalue");
  assignment->right->receive(this);
  if (!m_current_value)
    throw std::runtime_error("rvalue must be a runtime value");
  auto result = m_symbol_table.lookup(lhs->value);
  sym::Variable* variable;
  if (result) {
    variable = sym::sym_cast<sym::Variable*>(result);
    if (!variable) {
      throw std::runtime_error("assigning to something other than a variable");
    }
    if (variable->classof() != m_current_value_class)
      throw std::runtime_error("assignment type error");
  }
  else
    variable = createAllocaVariable(lhs->value, m_current_value_class);
  m_llvm_ir_builder.CreateStore(m_current_value, variable->value());
}


void CodeGenerator::visit(ast::Call* call) {
  // First we check if the function has already returned
  if (m_method_returned)
    throw std::runtime_error("code after function returned");

  // Now we resolve the object being called
  call->object->receive(this);

  // Next we check if the call is to a special method
  call->object->receive(this);
  if (call->name == "dereference") {
    if (call->args.size())
      throw std::runtime_error("cannot pass arguments to dereference");
    assert(false);
  }
  if (call->name == "addressof") {
    if (call->args.size())
      throw std::runtime_error("cannot pass arguments to dereference");
    assert(false);
  }

  // Now we lookup the method in the object's class
  auto found = m_symbol_table.lookupInScope(m_current_value_class->fullname(), call->name);
  if (!found)
    throw std::runtime_error("unknown method");
  auto method = sym::sym_cast<sym::Method*>(found);
  if (!method)
    throw std::runtime_error("not a method");
  std::vector<llvm::Value*> arguments{method->argumentClasses().size() + 1};
  if (m_current_value) {
    llvm::IRBuilder<> temporary_ir_builder{&m_current_method->function()->getEntryBlock(), m_current_method->function()->getEntryBlock().begin()};
    auto alloca_inst = temporary_ir_builder.CreateAlloca(m_current_value_class->type(), 0, nullptr, "temp");
    m_llvm_ir_builder.CreateStore(m_current_value, alloca_inst);
    arguments[0] = alloca_inst;
  }
  else
    arguments[0] = llvm::ConstantPointerNull::get(static_cast<llvm::PointerType*>(m_symbol_table.getReferenceClass(m_symbol_table.getParentClass(method))->type()));
  if (method->argumentClasses().size() != call->args.size())
    throw std::runtime_error("argument count mismatch");
  auto argument_class_iter = method->argumentClasses().begin();
  auto argument_expression_iter = call->args.begin();
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
  m_current_value = m_llvm_ir_builder.CreateCall(method->function(), arguments);
  m_current_value_class = method->returnClass();
  return;
}

void CodeGenerator::visit(ast::Identifier* identifier) {
  auto entry = m_symbol_table.lookup(identifier->value);
  if (!entry)
    throw std::runtime_error("unknown identifier");
  if (auto variable = sym::sym_cast<sym::Variable*>(entry)) {
    m_current_value_class = variable->classof();
    m_current_value = m_llvm_ir_builder.CreateLoad(variable->value());
  }
  else if (auto cls = sym::sym_cast<sym::Class*>(entry)) {
    m_current_value_class = cls;
    m_current_value = nullptr;
  }
  else if (auto field = sym::sym_cast<sym::Field*>(entry)) {
    assert(false);
  }
  else {
    assert(sym::sym_cast<sym::Method*>(entry));
    throw std::runtime_error("identifier refers to method");
  }
}

void CodeGenerator::visit(ast::Integer* integer) {
  auto int_class = boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".int"));
  m_current_value_class = int_class;
  m_current_value = llvm::ConstantInt::getSigned(int_class->type(), integer->value);
}

void CodeGenerator::visit(ast::Bool* boolean) {
  m_current_value_class = boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".bool"));
  m_current_value = boolean->value ? llvm::ConstantInt::getTrue(m_llvm_context) : llvm::ConstantInt::getFalse(m_llvm_context);
}


void CodeGenerator::finalize() {
  auto actual_main = llvm::Function::Create(
    llvm::FunctionType::get(llvm::Type::getInt32Ty(m_llvm_context), {llvm::Type::getInt32Ty(m_llvm_context), llvm::Type::getInt8Ty(m_llvm_context)->getPointerTo()->getPointerTo()}, false),
    llvm::Function::ExternalLinkage, "main", &m_llvm_module
  );
  auto module_cls = boost::polymorphic_pointer_downcast<sym::CompositeClass>(m_symbol_table.lookupKnown("__module__"));
  auto module_main = boost::polymorphic_pointer_downcast<sym::Method>(m_symbol_table.lookupKnown("__module__.main"));
  assert(module_main->returnClass() == boost::polymorphic_pointer_downcast<sym::Class>(m_symbol_table.lookupExact(".bool")));
  assert(module_main->argumentClasses().size() == 0);
  m_llvm_ir_builder.SetInsertPoint(llvm::BasicBlock::Create(m_llvm_context, "$entry", actual_main));
  auto alloca_inst = m_llvm_ir_builder.CreateAlloca(module_cls->type(), 0, nullptr, "main");
  std::vector<llvm::Value*> args;
  args.push_back(alloca_inst);
  auto call_inst = m_llvm_ir_builder.CreateCall(module_main->function(), args);
  llvm::BasicBlock* then_bb = llvm::BasicBlock::Create(m_llvm_context, "$then", actual_main);
  llvm::BasicBlock* else_bb = llvm::BasicBlock::Create(m_llvm_context, "$else", actual_main);
  m_llvm_ir_builder.CreateCondBr(call_inst, then_bb, else_bb);
  m_llvm_ir_builder.SetInsertPoint(then_bb);
  m_llvm_ir_builder.CreateRet(llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(m_llvm_context), 0));
  m_llvm_ir_builder.SetInsertPoint(else_bb);
  m_llvm_ir_builder.CreateRet(llvm::ConstantInt::getSigned(llvm::Type::getInt32Ty(m_llvm_context), 1));
  llvm::verifyModule(m_llvm_module, &llvm::errs());
}




/*



void CodeGenerator::print() {
  llvm::PrintModulePass pass;
  llvm::AnalysisManager<llvm::Module> manager;
  pass.run(m_llvm_module, manager);
}






*/
