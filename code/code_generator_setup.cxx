#include "code_generator_setup.hxx"
#include "ast.hxx"
#include <boost/bimap.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/exception.hpp>
#include <boost/graph/topological_sort.hpp>
#include <cassert>
#include <iterator>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <stdexcept>

void CodeGeneratorSetup::showSymbols() {
  m_symbol_table.showSymbols();
}

CodeGeneratorSetup::CodeGeneratorSetup()
: m_module{"program", m_context} {}

llvm::Module& CodeGeneratorSetup::module() {
  return m_module;
}

void CodeGeneratorSetup::setup(ast::Class* cls) {
  initializeBuiltins();
  initializeClasses(cls);
  initializeFieldsAndMethods(cls);
  resolveComposites();
  resolveMethods();
  declareBuiltins();
}

sym::Class* CodeGeneratorSetup::lookupClass(const ast::Expression* expression) const {
  if (auto identifier = ast::ast_cast<const ast::Identifier*>(expression)) {
    auto entry = m_symbol_table.lookup(identifier->value);
    if (!entry)
      throw std::runtime_error("undefined symbol");
    auto cls = dynamic_cast<sym::Class*>(entry);
    if (!cls)
      throw std::runtime_error("not a class");
    return cls;
  }
  assert(false);
  return nullptr;
}

void CodeGeneratorSetup::initializeBuiltins() {
  m_bool = m_symbol_table.createClass("bool", llvm::IntegerType::getInt1Ty(m_context));
  m_int = m_symbol_table.createClass("int", llvm::IntegerType::getInt64Ty(m_context));
  m_void = m_symbol_table.createClass("void", llvm::Type::getVoidTy(m_context));
}

void CodeGeneratorSetup::initializeClasses(const ast::Class* cls) {
  m_symbol_table.createComposite(cls->name);
  m_symbol_table.pushScope(cls->name);
  for (auto& global : cls->body)
    if (auto inner_class = ast::ast_cast<const ast::Class*>(global.get()))
      initializeClasses(inner_class);
  m_symbol_table.popScope();
}

void CodeGeneratorSetup::initializeFieldsAndMethods(const ast::Class* cls) {
  auto composite = static_cast<sym::Composite*>(m_symbol_table.lookup(cls->name));
  m_symbol_table.pushScope(cls->name);
  for (auto& global : cls->body) {
    if (auto inner_class = ast::ast_cast<const ast::Class*>(global.get())) {
      initializeFieldsAndMethods(inner_class);
    } else if (auto field = ast::ast_cast<const ast::Field*>(global.get())) {
      composite->fields().push_back(m_symbol_table.createField(field->name, lookupClass(field->cls.get())));
    } else {
      auto method = ast::checked_ast_cast<const ast::Method*>(global.get());
      std::vector<sym::Class*> argument_classes{method->args.size()};
      for (std::vector<sym::Class*>::size_type i = 0; i != argument_classes.size(); ++i)
        argument_classes[i] = lookupClass(method->args[i].second.get());
      auto return_class = lookupClass(method->return_class.get());
      m_symbol_table.createMethod(method->name, std::move(argument_classes), return_class);
    }
  }
  m_symbol_table.popScope();
}

void CodeGeneratorSetup::resolveComposites() {
  using Graph = boost::adjacency_list<>;
  using BiMap = boost::bimap<sym::Composite*, Graph::vertex_descriptor>;
  Graph graph;
  BiMap table;
  m_symbol_table.forEachEntry([&graph, &table](sym::Entry* entry){
    if (auto composite = dynamic_cast<sym::Composite*>(entry))
      table.insert(BiMap::value_type(composite, boost::add_vertex(graph)));
  });
  m_symbol_table.forEachEntry([this, &graph, &table](sym::Entry* entry){
    if (auto field = dynamic_cast<sym::Field*>(entry)) {
      auto parent = m_symbol_table.getParent(field);
      if (auto field_composite = dynamic_cast<sym::Composite*>(field->cls()))
        boost::add_edge(table.left.find(parent)->second, table.left.find(field_composite)->second, graph);
    }
  });
  std::vector<Graph::vertex_descriptor> result;
  try {
    boost::topological_sort(graph, std::back_inserter(result));
  } catch (boost::not_a_dag&) {
    throw std::runtime_error("cyclic class dependencies detected");
  }
  for (auto iter = result.rbegin(); iter != result.rend(); ++iter)
    resolveComposite(table.right.find(*iter)->second);
}

void CodeGeneratorSetup::resolveComposite(sym::Composite* composite) {
  std::vector<llvm::Type*> llvm_types{composite->fields().size()};
  for (std::vector<llvm::Type*>::size_type i = 0; i != llvm_types.size(); ++i) {
    assert(composite->fields()[i]->cls()->type());
    llvm_types[i] = composite->fields()[i]->cls()->type();
  }
  composite->setType(llvm::StructType::create(m_context, llvm_types));
}

void CodeGeneratorSetup::resolveMethods() {
  m_symbol_table.forEachEntry([this](sym::Entry* entry){
    if (auto method = dynamic_cast<sym::Method*>(entry)) {
      std::vector<llvm::Type*> argument_types{method->argumentClasses().size() + 1};
      argument_types[0] = m_symbol_table.getParent(method)->type()->getPointerTo();
      for (std::vector<llvm::Type*>::size_type i = 0; i != method->argumentClasses().size(); ++i)
        argument_types[i + 1] = method->argumentClasses()[i]->type();
      auto function_type = llvm::FunctionType::get(method->returnClass()->type(), argument_types, false);
      method->setFunction(llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, llvm::StringRef(method->name().data(), method->name().size()), &m_module));
    }
  });
}

void CodeGeneratorSetup::declareBuiltins() {
  {
  std::vector<llvm::Type*> args;
  auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(m_context), args, false);
  auto f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "foo", &m_module);
  auto method = m_symbol_table.createMethod(".__module__.", "foo", std::vector<sym::Class*>(), m_void);
  method->setFunction(f);
  }
  {
  std::vector<llvm::Type*> args;
  std::vector<sym::Class*> argc;
  args.push_back(m_int->type());
  argc.push_back(m_int);
  auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(m_context), std::move(args), false);
  auto f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "print_integer", &m_module);
  auto method = m_symbol_table.createMethod(".__module__.", "print_integer", std::move(argc), m_void);
  method->setFunction(f);
  }
}
