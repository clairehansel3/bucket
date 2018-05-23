#include "symtable.hxx"
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <llvm/IR/DerivedTypes.h>
#include <stdexcept>
#include <utility>

namespace sym {

std::string_view Entry::fullname() const noexcept {
  return m_fullname;
}

Entry::Entry(std::string&& fullname) noexcept
: m_fullname{std::move(fullname)} {}

llvm::AllocaInst* Variable::value() const noexcept {
  return m_value;
}

void Variable::setValue(llvm::AllocaInst* value) noexcept {
  m_value = value;
}

Class* Variable::classof() const noexcept {
  return m_classof;
}

Variable::Variable(std::string&& fullname, Class* classof) noexcept
: Entry{std::move(fullname)}, m_value{nullptr}, m_classof{classof} {}

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

Method::Method(std::string&& fullname, std::vector<Class*>&& argument_classes, Class* return_class) noexcept
: Entry{std::move(fullname)}, m_function{nullptr}, m_argument_classes{std::move(argument_classes)}, m_return_class{return_class} {}

Class* Field::classof() const noexcept {
  return m_classof;
}

Field::Field(std::string&& fullname, Class* classof) noexcept
: Entry{std::move(fullname)}, m_classof{classof} {}

llvm::Type* Class::type() const noexcept {
  return m_type;
}

Class::Class(std::string&& fullname, llvm::Type* type) noexcept
: Entry{std::move(fullname)}, m_type{type} {}

void CompositeClass::setType(llvm::Type* type) noexcept {
  assert(type);
  m_type = type;
}

std::vector<Field*>& CompositeClass::fields() noexcept {
  return m_fields;
}

CompositeClass::CompositeClass(std::string&& fullname) noexcept
: Class{std::move(fullname), nullptr} {}

SymbolTable::SymbolTable() : m_active_scope{"."} {}

SymbolTable::iterator SymbolTable::begin() {
  return iterator(m_entry_map.begin());
}

SymbolTable::iterator SymbolTable::end() {
  return iterator(m_entry_map.end());
}

Entry* SymbolTable::lookup(std::string_view shortname) const {
  std::string_view scope{m_active_scope.data(), m_active_scope.size() - 1};
  return lookupInScope(scope, shortname);
}

Entry* SymbolTable::lookupKnown(std::string_view shortname) const {
  std::string lookup_name{m_active_scope};
  lookup_name += shortname;
  auto iter = m_entry_map.find(lookup_name);
  assert(iter != m_entry_map.end());
  return iter->second.get();
}

Entry* SymbolTable::lookupInScope(std::string_view scope, std::string_view shortname) const {
  assert(shortname.find('.') == std::string_view::npos);
  std::string lookup_name{scope};
  lookup_name += '.';
  lookup_name += shortname;
  while (true) {
    auto iter = m_entry_map.find(lookup_name);
    if (iter != m_entry_map.end())
      return iter->second.get();
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

Entry* SymbolTable::lookupExact(std::string_view fullname) const {
  assert(fullname.length() != 0 && fullname[0] == '.');
  auto iter = m_entry_map.find(fullname);
  return iter == m_entry_map.end() ? nullptr : iter->second.get();
}

CompositeClass* SymbolTable::getParentClass(Entry* entry) const {
  auto index = entry->fullname().find_last_of('.');
  assert(index != std::string_view::npos && index != 0);
  auto parent_name = entry->fullname().substr(0, index);
  auto iter = m_entry_map.find(parent_name);
  assert(iter != m_entry_map.end());
  auto result = sym_cast<CompositeClass*>(iter->second.get());
  assert(result);
  return result;
}

Class* SymbolTable::getReferenceClass(Class* cls) {
  std::string reference_class_name{cls->fullname()};
  reference_class_name += '*';
  if (auto reference_class_pointer = lookupExact(reference_class_name)) {
    assert(sym_cast<Class*>(reference_class_pointer));
    return static_cast<Class*>(reference_class_pointer);
  }
  auto reference_class_unique_pointer = std::unique_ptr<Class>(new Class{std::move(reference_class_name), cls->type()->getPointerTo()});
  auto reference_class_pointer = reference_class_unique_pointer.get();
  assert(m_entry_map.find(reference_class_pointer->fullname()) == m_entry_map.end());
  m_entry_map[reference_class_pointer->fullname()] = std::move(reference_class_unique_pointer);
  return reference_class_pointer;
}


void SymbolTable::pushScope(std::string_view scopename) {
  assert(scopename.find('.') == std::string_view::npos);
  m_active_scope += scopename;
  m_active_scope += '.';
}

void SymbolTable::popScope() {
  assert(m_active_scope != ".");
  m_active_scope.erase(m_active_scope.find_last_of('.', m_active_scope.size() - 2) + 1);
}

void SymbolTable::pushAnonymousScope() {
  m_active_scope += '.';
}

void SymbolTable::popAnonymousScope() {
  std::vector<Entry*> entries_to_delete;
  for (auto entry : *this)
    if (boost::starts_with(entry->fullname(), m_active_scope))
      entries_to_delete.push_back(entry);
  for (auto entry : entries_to_delete)
    m_entry_map.erase(m_entry_map.find(entry->fullname()));
  popScope();
}

Variable* SymbolTable::createVariable(std::string_view shortname, Class* classof) {
  assert(shortname.find('.') == std::string_view::npos);
  std::string fullname{m_active_scope};
  fullname += shortname;
  auto variable_unique_pointer = std::unique_ptr<Variable>(new Variable{std::move(fullname), classof});
  auto variable_pointer = variable_unique_pointer.get();
  if (m_entry_map.find(variable_pointer->fullname()) != m_entry_map.end())
    throw std::runtime_error("name already taken");
  m_entry_map[variable_pointer->fullname()] = std::move(variable_unique_pointer);
  return variable_pointer;
}

Method* SymbolTable::createMethod(std::string_view shortname, std::vector<Class*>&& argument_classes, Class* return_class) {
  assert(shortname.find('.') == std::string_view::npos);
  std::string fullname{m_active_scope};
  fullname += shortname;
  auto method_unique_pointer = std::unique_ptr<Method>(new Method{std::move(fullname), std::move(argument_classes), return_class});
  auto method_pointer = method_unique_pointer.get();
  if (m_entry_map.find(method_pointer->fullname()) != m_entry_map.end())
    throw std::runtime_error("name already taken");
  m_entry_map[method_pointer->fullname()] = std::move(method_unique_pointer);
  return method_pointer;
}

Field* SymbolTable::createField(std::string_view shortname, Class* classof) {
  assert(shortname.find('.') == std::string_view::npos);
  std::string fullname{m_active_scope};
  fullname += shortname;
  auto field_unique_pointer = std::unique_ptr<Field>(new Field{std::move(fullname), classof});
  auto field_pointer = field_unique_pointer.get();
  if (m_entry_map.find(field_pointer->fullname()) != m_entry_map.end())
    throw std::runtime_error("name already taken");
  m_entry_map[field_pointer->fullname()] = std::move(field_unique_pointer);
  return field_pointer;
}

Class* SymbolTable::createClass(std::string_view shortname, llvm::Type* type) {
  assert(shortname.find('.') == std::string_view::npos);
  std::string fullname{m_active_scope};
  fullname += shortname;
  auto class_unique_pointer = std::unique_ptr<Class>(new Class{std::move(fullname), type});
  auto class_pointer = class_unique_pointer.get();
  if (m_entry_map.find(class_pointer->fullname()) != m_entry_map.end())
    throw std::runtime_error("name already taken");
  m_entry_map[class_pointer->fullname()] = std::move(class_unique_pointer);
  return class_pointer;
}

CompositeClass* SymbolTable::createCompositeClass(std::string_view shortname) {
  assert(shortname.find('.') == std::string_view::npos);
  std::string fullname{m_active_scope};
  fullname += shortname;
  auto composite_class_unique_pointer = std::unique_ptr<CompositeClass>(new CompositeClass{std::move(fullname)});
  auto composite_class_pointer = composite_class_unique_pointer.get();
  if (m_entry_map.find(composite_class_pointer->fullname()) != m_entry_map.end())
    throw std::runtime_error("name already taken");
  m_entry_map[composite_class_pointer->fullname()] = std::move(composite_class_unique_pointer);
  return composite_class_pointer;
}

void SymbolTable::print() {
  auto entryKind = [](Entry* entry) {
    assert(entry);
    if (sym_cast<Variable*>(entry))
      return "variable";
    if (sym_cast<Method*>(entry))
      return "method";
    if (sym_cast<Field*>(entry))
      return "field";
    if (sym_cast<CompositeClass*>(entry))
      return "composite class";
    assert(sym_cast<Class*>(entry));
    return "class";
  };
  for (auto entry : *this)
    std::cout << entry->fullname() << " | " << entryKind(entry) << '\n';
}

}
