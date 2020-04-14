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

#include "symbol_table.hxx"
#include "abstract_syntax_tree.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>
#include <utility>
#ifdef BUCKET_DEBUG
#include <iostream>
#endif

SymbolTable::SymbolTable()
: m_scope{'/'}
{}

void SymbolTable::pushScope(std::string_view name)
{
  BUCKET_ASSERT(name.find('/') == std::string_view::npos);
  m_scope += name;
  m_scope += '/';
}

void SymbolTable::popScope()
{
  BUCKET_ASSERT(std::count(m_scope.begin(), m_scope.end(), '/') >= 2);
  BUCKET_ASSERT(m_scope.size() != 1);
  if (m_scope[m_scope.size() - 1] == '/' && m_scope[m_scope.size() - 2] == '/')
  {
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

SymbolTable::Entry* SymbolTable::lookup(std::string_view name)
{
  return lookup(m_scope, name);

}

SymbolTable::Entry* SymbolTable::lookup(std::string_view scope,
  std::string_view name)
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

SymbolTable::Field* SymbolTable::createField(std::string_view name, Type* type)
{
  BUCKET_ASSERT(type);
  BUCKET_ASSERT(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Field> field_uptr{new Field(std::move(path), type)};
  auto field_ptr = field_uptr.get();
  if (m_map.find(field_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(field_ptr->path(), " already exists");
  m_map[field_ptr->path()] = std::move(field_uptr);
  return field_ptr;
}

SymbolTable::Method* SymbolTable::createMethod(std::string_view name,
  std::vector<Type*> argument_types, Type* return_type)
{
  BUCKET_ASSERT(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Method> method_uptr{new Method(std::move(path),
    std::move(argument_types), return_type)};
  auto method_ptr = method_uptr.get();
  if (m_map.find(method_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(method_ptr->path(), " already exists");
  m_map[method_ptr->path()] = std::move(method_uptr);
  return method_ptr;
}

SymbolTable::Type* SymbolTable::createType(std::string_view name,
  llvm::Type* llvm_type)
{
  //BUCKET_ASSERT(llvm_type);
  BUCKET_ASSERT(name.find('/') == std::string_view::npos);
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
  BUCKET_ASSERT(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Class> class_uptr{new Class(std::move(path))};
  auto class_ptr = class_uptr.get();
  if (m_map.find(class_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(class_ptr->path(), " already exists");
  m_map[class_ptr->path()] = std::move(class_uptr);
  return class_ptr;
}

SymbolTable::Variable* SymbolTable::createVariable(std::string_view name,
  Type* type)
{
  BUCKET_ASSERT(name.find('/') == std::string_view::npos);
  std::string path{m_scope};
  path += name;
  std::unique_ptr<Variable> variable_uptr{new Variable(std::move(path), type)};
  auto variable_ptr = variable_uptr.get();
  if (m_map.find(variable_ptr->path()) != m_map.end())
    throw make_error<CodeGeneratorError>(variable_ptr->path(),
      " already exists");
  m_map[variable_ptr->path()] = std::move(variable_uptr);
  return variable_ptr;
}

SymbolTable::Type* SymbolTable::getPointerType(Type* type)
{
  BUCKET_ASSERT(type->m_llvm_type);
  std::string reference_type_name{type->path()};
  reference_type_name += '*';
  auto iter = m_map.find(reference_type_name);
  if (iter != m_map.end())
    return boost::polymorphic_downcast<Type*>(iter->second.get());
  auto reference_class_unique_pointer = std::unique_ptr<Type>(new Type{
    std::move(reference_type_name), type->m_llvm_type->getPointerTo()});
  auto reference_class_pointer = reference_class_unique_pointer.get();
  BUCKET_ASSERT(m_map.find(reference_class_pointer->path()) == m_map.end());
  m_map[reference_class_pointer->path()] = std::move(
    reference_class_unique_pointer);
  return reference_class_pointer;
}

SymbolTable::Type* SymbolTable::resolveType(ast::Expression* ast_expression)
{
  if (auto identifier_ptr = ast::ast_cast<ast::Identifier*>(ast_expression)) {
    auto entry = lookup(identifier_ptr->value);
    if (!entry)
      throw make_error<CodeGeneratorError>("undefined identifier '", identifier_ptr->value, '\'');
    auto type = sym_cast<SymbolTable::Type*>(entry);
    if (!type)
      throw make_error<CodeGeneratorError>("identifier '", identifier_ptr->value, "' is not a type");
    return type;
  }
  else {
    throw make_error<CodeGeneratorError>("non identifier types not yet implemented");
  }
}

template <> SymbolTable::Entry* SymbolTable::gotoName<SymbolTable::Entry*>(
  std::string_view name)
{
  return gotoPath<Entry*>(concatenate(m_scope, name));
}

template <>  SymbolTable::Entry* SymbolTable::gotoPath<SymbolTable::Entry*>(
  std::string_view path)
{
  BUCKET_ASSERT(m_map.find(path) != m_map.end());
  return m_map[path].get();
}

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
  BUCKET_ASSERT(index != std::string_view::npos);
  BUCKET_ASSERT(index + 1 <= m_path.size());
  return std::string_view(m_path).substr(index + 1);
}

std::string_view SymbolTable::Entry::parent() const noexcept
{
  auto index = m_path.find_last_of('/');
  assert(index != std::string_view::npos && index != 0);
  return std::string_view(m_path).substr(0, index);
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
  BUCKET_ASSERT(m_type);
}

void SymbolTable::Method::receive(Visitor* visitor)
{
  visitor->visit(this);
}

SymbolTable::Method::Method(std::string path, std::vector<Type*> argument_types,
  Type* return_type) noexcept
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

SymbolTable::iterator SymbolTable::begin()
{
  return iterator(m_map.begin());
}

SymbolTable::iterator SymbolTable::end()
{
  return iterator(m_map.end());
}
