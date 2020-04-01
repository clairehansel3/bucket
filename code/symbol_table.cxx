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

#include "symbol_table.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <cassert>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <utility>

void SymbolTable::Visitor::visit(Entry*) {BUCKET_UNREACHABLE();}
void SymbolTable::Visitor::visit(Field*) {BUCKET_UNREACHABLE();}
void SymbolTable::Visitor::visit(Method*) {BUCKET_UNREACHABLE();}
void SymbolTable::Visitor::visit(Type*) {BUCKET_UNREACHABLE();}
void SymbolTable::Visitor::visit(Class*) {BUCKET_UNREACHABLE();}
void SymbolTable::Visitor::visit(Variable*) {BUCKET_UNREACHABLE();}

std::string_view SymbolTable::Entry::path() const noexcept
{
  return m_path;
}

std::string_view SymbolTable::Entry::name() const noexcept
{
  auto index = m_path.find_last_of('/');
  assert(index != std::string_view::npos);
  assert(index + 1 <= m_path.size());
  return std::string_view(m_path).substr(index + 1);
}

void SymbolTable::Entry::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Entry::Entry(std::string path)
: m_path{std::move(path)}
{}

void SymbolTable::Field::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Field::Field(std::string path, Type* type) noexcept
: Entry{std::move(path)}, m_type{type}
{
  assert(m_type);
}

void SymbolTable::Method::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Method::Method(std::string path, std::vector<Type*> argument_types, Type* return_type) noexcept
: Entry{std::move(path)},
  m_llvm_function{nullptr},
  m_argument_types{std::move(argument_types)},
  m_return_type{return_type}
{}

void SymbolTable::Type::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Type::Type(std::string path, llvm::Type* llvm_type) noexcept
: Entry{std::move(path)}, m_llvm_type{llvm_type}
{}

void SymbolTable::Class::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Class::Class(std::string path) noexcept
: Type{std::move(path), nullptr}
{}

void SymbolTable::Variable::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Variable::Variable(std::string path, Type* type) noexcept
: Entry{std::move(path)},
  m_type{type}
{}

SymbolTable::SymbolTable()
: m_scope{'/'}
{}

SymbolTable::Type* SymbolTable::getParentType(Entry* entry) const {
  auto index = entry->path().find_last_of('/');
  assert(index != std::string_view::npos && index != 0);
  auto parent_name = entry->path().substr(0, index);
  return lookupKnownPath<Type>(parent_name);
}

SymbolTable::Entry* SymbolTable::lookupPath(std::string_view path) const
{
  auto iter = m_map.find(path);
  if (iter == m_map.end())
    return nullptr;
  return iter->second.get();
}

SymbolTable::Entry* SymbolTable::lookup(std::string_view name) const {
  return lookupInScope(m_scope, name);

}

SymbolTable::Entry* SymbolTable::lookupInScope(std::string_view scope, std::string_view name) const
{
  BUCKET_ASSERT(scope.size() != 0 && scope[scope.size() - 1] == '/');
  std::string lookup_name{scope};
  lookup_name += name;
  while (true) {
    auto iter = m_map.find(lookup_name);
    if (iter != m_map.end())
      return iter->second.get();
    auto name_index = lookup_name.find_last_of('/');
    if (name_index == 0)
      return nullptr;
    auto scope_index = lookup_name.find_last_of('/', name_index - 1);
    auto new_length = lookup_name.size() - (name_index - scope_index);
    for (auto i = scope_index + 1; i != new_length; ++i)
      lookup_name[i] = lookup_name[name_index + (i - scope_index)];
    lookup_name.erase(new_length);
  }
}

void SymbolTable::pushScope(std::string_view name)
{
  assert(name.find('/') == std::string_view::npos);
  m_scope += name;
  m_scope += '/';
}

void SymbolTable::popScope()
{
  assert(std::count(m_scope.begin(), m_scope.end(), '/') >= 2);
  assert(m_scope.size() != 1);
  if (m_scope[m_scope.size() - 1] == '/' && m_scope[m_scope.size() - 2] == '/') {
    std::vector<std::string_view> paths_to_delete;
    for (auto& item : m_map)
      if (boost::starts_with(item.first, m_scope))
        paths_to_delete.push_back(item.first);
    for (auto path : paths_to_delete)
      m_map.erase(m_map.find(path));
    m_scope.pop_back();
  }
  else {
    m_scope.erase(m_scope.find_last_of('/', m_scope.size() - 2) + 1);
  }
}

SymbolTable::Field* SymbolTable::createField(std::string_view name, Type* type)
{
  assert(type);
  assert(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Field> field_uptr{new Field(std::move(path), type)};
  auto field_ptr = field_uptr.get();
  if (m_map.find(field_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(field_ptr->path(), " already exists");
  m_map[field_ptr->path()] = std::move(field_uptr);
  return field_ptr;
}

SymbolTable::Method* SymbolTable::createMethod(std::string_view name, std::vector<Type*> argument_types, Type* return_type)
{
  assert(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Method> method_uptr{new Method(std::move(path), std::move(argument_types), return_type)};
  auto method_ptr = method_uptr.get();
  if (m_map.find(method_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(method_ptr->path(), " already exists");
  m_map[method_ptr->path()] = std::move(method_uptr);
  return method_ptr;
}

SymbolTable::Type* SymbolTable::createType(std::string_view name, llvm::Type* llvm_type)
{
  //assert(llvm_type);
  assert(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Type> type_uptr{new Type(std::move(path), llvm_type)};
  auto type_ptr = type_uptr.get();
  if (m_map.find(type_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(type_ptr->path(), " already exists");
  m_map[type_ptr->path()] = std::move(type_uptr);
  return type_ptr;
}

SymbolTable::Class* SymbolTable::createClass(std::string_view name)
{
  assert(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Class> class_uptr{new Class(std::move(path))};
  auto class_ptr = class_uptr.get();
  if (m_map.find(class_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(class_ptr->path(), " already exists");
  m_map[class_ptr->path()] = std::move(class_uptr);
  return class_ptr;
}

SymbolTable::Variable* SymbolTable::createVariable(std::string_view name, Type* type)
{
  assert(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Variable> variable_uptr{new Variable(std::move(path), type)};
  auto variable_ptr = variable_uptr.get();
  if (m_map.find(variable_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(variable_ptr->path(), " already exists");
  m_map[variable_ptr->path()] = std::move(variable_uptr);
  return variable_ptr;
}

SymbolTable::Type* SymbolTable::getReferenceType(Type* type)
{
  BUCKET_ASSERT(type->m_llvm_type);
  std::string reference_type_name{type->path()};
  reference_type_name += '*';
  if (auto reference_class_pointer = boost::polymorphic_pointer_downcast<Type>(lookupPath(reference_type_name)))
    return reference_class_pointer;
  auto reference_class_unique_pointer = std::unique_ptr<Type>(new Type{std::move(reference_type_name), type->m_llvm_type->getPointerTo()});
  auto reference_class_pointer = reference_class_unique_pointer.get();
  assert(m_map.find(reference_class_pointer->path()) == m_map.end());
  m_map[reference_class_pointer->path()] = std::move(reference_class_unique_pointer);
  return reference_class_pointer;
}

SymbolTable::iterator SymbolTable::begin()
{
  return iterator(m_map.begin());
}

SymbolTable::iterator SymbolTable::end()
{
  return iterator(m_map.end());
}

#include <iostream>

void SymbolTable::print()
{
  for (auto entry : *this) {
    std::cout << BUCKET_BOLD << entry->path() << BUCKET_BLACK "\n";
    if (auto field_ptr = sym_cast<Field*>(entry)) {
      std::cout << "field(" << field_ptr->m_type->path() << ")\n";
    }
    else if (auto method_ptr = sym_cast<Method*>(entry)) {
      std::cout << '<' << getParentType(method_ptr)->path() << ">." << method_ptr->name() << "(";
      for (std::size_t i = 0; i != method_ptr->m_argument_types.size(); ++i) {
        std::cout << method_ptr->m_argument_types[i]->path();
        if (i != method_ptr->m_argument_types.size() - 1)
          std::cout << ", ";
      }
      std::cout << ") -> " << method_ptr->m_return_type->name() << "\nllvm = ";
      std::cout.flush();
      if (method_ptr->m_llvm_function)
        method_ptr->m_llvm_function->dump();
      else
        std::cout << "[none]\n";
    }
    else if (auto class_ptr = sym_cast<Class*>(entry)) {
      std::cout << "class\nllvm = ";
      std::cout.flush();
      if (class_ptr->m_llvm_type)
        class_ptr->m_llvm_type->dump();
      else
        std::cout << "[none]\n";
    }
    else if (auto type_ptr = sym_cast<Type*>(entry)) {
      std::cout << "type\nllvm = ";
      std::cout.flush();
      if (type_ptr->m_llvm_type)
        type_ptr->m_llvm_type->dump();
      else
        std::cout << "[none]\n";
    }
    else {
      BUCKET_UNREACHABLE();
    }
  }
}
