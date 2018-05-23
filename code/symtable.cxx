#include "symtable.hxx"
#include <boost/algorithm/string/predicate.hpp>
#include <cassert>
#include <cstddef>
#include <llvm/IR/DerivedTypes.h>
#include <stdexcept>
#include <utility>

#include <iostream>

namespace sym {

std::string_view Entry::name() const noexcept {
  return m_name;
}

Entry::Entry(std::string&& name) noexcept
: m_name{std::move(name)} {}

llvm::AllocaInst* Variable::value() const noexcept {
  return m_value;
}

void Variable::setValue(llvm::AllocaInst* value) noexcept {
  m_value = value;
}

Class* Variable::cls() const noexcept {
  return m_cls;
}

Variable::Variable(std::string&& name, Class* cls) noexcept
: Entry{std::move(name)}, m_cls{cls} {}

Class* Field::cls() const noexcept {
  return m_cls;
}

Field::Field(std::string&& name, Class* cls) noexcept
: Entry{std::move(name)}, m_cls{cls} {}

llvm::Function* Method::function() const noexcept {
  return m_function;
}

void Method::setFunction(llvm::Function* function) noexcept {
  m_function = function;
}

const std::vector<Class*>& Method::argumentClasses() const noexcept {
  return m_argument_classes;
}

Class* Method::returnClass() const noexcept {
  return m_return_class;
}

Method::Method(std::string&& name, std::vector<Class*>&& argument_classes, Class* return_class) noexcept
: Entry{std::move(name)}, m_function{nullptr}, m_argument_classes{std::move(argument_classes)}, m_return_class{return_class} {}

llvm::Type* Class::type() const noexcept {
  return m_type;
}

Class* Class::pointer() {
  if (!m_pointer) {
    std::string new_name{name()};
    new_name += '*';
    m_pointer = std::unique_ptr<Class>(new Class{std::move(new_name), type()->getPointerTo()});
  }
  return m_pointer.get();
}

Class::Class(std::string&& name, llvm::Type* type) noexcept
: Entry{std::move(name)}, m_type{type} {}

void Composite::setType(llvm::Type* type) noexcept {
  m_type = type;
}

std::vector<Field*>& Composite::fields() noexcept {
  return m_fields;
}

Composite::Composite(std::string&& name) noexcept
: Class{std::move(name), nullptr} {}

SymbolTable::SymbolTable() : m_scope{"."} {}

Entry* SymbolTable::lookup(std::string_view name) const {
  //std::cout << "looking up name " << name << " in scope " << m_scope << std::endl;
  //for (auto& item : m_table)
  //  std::cout << item.first << ": " << typeid(*item.second).name() << std::endl;
  assert(name.find('.') == std::string_view::npos);
  std::string lookup_name{m_scope};
  lookup_name += name;
  while (true) {
    auto iter = m_table.find(lookup_name);
    if (iter != m_table.end())
      return iter->second;
    auto name_index = lookup_name.find_last_of('.');
    if (name_index == 0)
      return nullptr;
    auto scope_index = lookup_name.find_last_of('.', name_index - 1);
    auto new_length = lookup_name.size() - (name_index - scope_index);
    for (auto i = scope_index + 1; i != new_length; ++i)
      lookup_name[i] = lookup_name[name_index + (i - scope_index)];
    lookup_name.erase(new_length);
  }
}

Entry* SymbolTable::lookup(std::string_view scope, std::string_view name) const {
  assert(name.find('.') == std::string_view::npos);
  std::string lookup_name{scope};
  lookup_name += '.';
  lookup_name += name;
  while (true) {
    auto iter = m_table.find(lookup_name);
    if (iter != m_table.end())
      return iter->second;
    auto name_index = lookup_name.find_last_of('.');
    if (name_index == 0)
      return nullptr;
    auto scope_index = lookup_name.find_last_of('.', name_index - 1);
    auto new_length = lookup_name.size() - (name_index - scope_index);
    for (auto i = scope_index + 1; i != new_length; ++i)
      lookup_name[i] = lookup_name[name_index + (i - scope_index)];
    lookup_name.erase(new_length);
  }
}

Composite* SymbolTable::getParent(Entry* entry) const {
  auto index = entry->name().find_last_of('.');
  assert(index != std::string_view::npos && index != 0);
  auto parent_name = entry->name().substr(0, index);
  auto iter = m_table.find(parent_name);
  assert(iter != m_table.end());
  auto result = dynamic_cast<Composite*>(iter->second);
  assert(result);
  return result;
}

void SymbolTable::pushScope(std::string_view name) {
  assert(name.find('.') == std::string_view::npos);
  m_scope += name;
  m_scope += '.';
}

void SymbolTable::popScope() {
  assert(m_scope != ".");
  m_scope.erase(m_scope.find_last_of('.', m_scope.size() - 2) + 1);
}

void SymbolTable::pushAnonymousScope() {
  m_scope += '.';
}

void SymbolTable::popAnonymousScope() {
  std::size_t i = 0;
  while (i != m_entries.size()) {
    if (boost::starts_with(m_entries[i]->name(), m_scope)) {
      m_table.erase(m_table.find(m_entries[i]->name()));
      m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(i));
    } else {
      ++i;
    }
  }
  popScope();
}

Variable* SymbolTable::createVariable(std::string_view name, Class* cls) {
  assert(name.find('.') == std::string_view::npos);
  std::string full_name{m_scope};
  full_name += name;
  std::unique_ptr<Variable> uptr{new Variable{std::move(full_name), cls}};
  auto ptr = uptr.get();
  m_entries.push_back(std::move(uptr));
  if (m_table[ptr->name()])
    throw std::runtime_error("name already taken");
  m_table[ptr->name()] = ptr;
  return ptr;
}

Field* SymbolTable::createField(std::string_view name, Class* cls) {
  assert(name.find('.') == std::string_view::npos);
  std::string full_name{m_scope};
  full_name += name;
  std::unique_ptr<Field> uptr{new Field{std::move(full_name), cls}};
  auto ptr = uptr.get();
  m_entries.push_back(std::move(uptr));
  if (m_table[ptr->name()])
    throw std::runtime_error("name already taken");
  m_table[ptr->name()] = ptr;
  return ptr;
}

Method* SymbolTable::createMethod(std::string_view name, std::vector<Class*>&& argument_classes, Class* return_class) {
  assert(name.find('.') == std::string_view::npos);
  std::string full_name{m_scope};
  full_name += name;
  std::unique_ptr<Method> uptr{new Method{std::move(full_name), std::move(argument_classes), return_class}};
  auto ptr = uptr.get();
  m_entries.push_back(std::move(uptr));
  if (m_table[ptr->name()])
    throw std::runtime_error("name already taken");
  m_table[ptr->name()] = ptr;
  return ptr;
}

Method* SymbolTable::createMethod(std::string scope, std::string_view name, std::vector<Class*>&& argument_classes, Class* return_class) {
  assert(name.find('.') == std::string_view::npos);
  std::string full_name{m_scope};
  full_name += name;
  std::unique_ptr<Method> uptr{new Method{std::move(full_name), std::move(argument_classes), return_class}};
  auto ptr = uptr.get();
  m_entries.push_back(std::move(uptr));
  if (m_table[ptr->name()])
    throw std::runtime_error("name already taken");
  m_table[ptr->name()] = ptr;
  return ptr;
}

Class* SymbolTable::createClass(std::string_view name, llvm::Type* type) {
  assert(name.find('.') == std::string_view::npos);
  std::string full_name{m_scope};
  full_name += name;
  std::unique_ptr<Class> uptr{new Class{std::move(full_name), type}};
  auto ptr = uptr.get();
  m_entries.push_back(std::move(uptr));
  if (m_table[ptr->name()])
    throw std::runtime_error("name already taken");
  m_table[ptr->name()] = ptr;
  return ptr;
}

Composite* SymbolTable::createComposite(std::string_view name) {
  assert(name.find('.') == std::string_view::npos);
  std::string full_name{m_scope};
  full_name += name;
  std::unique_ptr<Composite> uptr{new Composite{std::move(full_name)}};
  auto ptr = uptr.get();
  m_entries.push_back(std::move(uptr));
  if (m_table[ptr->name()])
    throw std::runtime_error("name already taken");
  m_table[ptr->name()] = ptr;
  return ptr;
}

void SymbolTable::forEachEntry(const std::function<void(Entry*)>& function) {
  for (auto& entry : m_entries)
    function(entry.get());
}

void SymbolTable::showSymbols() {
  auto show = [](Entry* entry){
    if (!entry)
      return "nullptr";
    else if (dynamic_cast<Variable*>(entry))
      return "variable";
    else if (dynamic_cast<Field*>(entry))
      return "field";
    else if (dynamic_cast<Method*>(entry))
      return "method";
    else if (dynamic_cast<Composite*>(entry))
      return "composite";
    else {
      assert(dynamic_cast<Class*>(entry));
      return "class";
    }
  };
  for (auto& [k, v] : m_table)
    std::cerr << k << ": " << show(v) << std::endl;
}

}
