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

#include "objects.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/polymorphic_pointer_cast.hpp>
#include <cassert>
#include <llvm/IR/DerivedTypes.h>
#include <mutex>
#include <utility>

namespace obj
{

//------------------------------------------------------------------------------
//----Object--------------------------------------------------------------------
//------------------------------------------------------------------------------

Class* Object::getClass()
{
  std::unique_lock<std::mutex> lock{mutex};
  #ifdef BUCKET_DEBUG
  assert(classof_is_set);
  #endif
  return classof;
}

void Object::setClass(Class* newclassof)
{
  std::unique_lock<std::mutex> lock{mutex};
  #ifdef BUCKET_DEBUG
  assert(!classof_is_set);
  classof_is_set = true;
  #endif
  classof = newclassof;
}

Object* Object::getAttribute(const std::string& name)
{
  std::unique_lock<std::mutex> lock{mutex};
  auto result = attributes.find(name);
  return result == attributes.end() ? nullptr : result->second;
}

void Object::setAttribute(std::string name, Object* object)
{
  std::unique_lock<std::mutex> lock{mutex};
  assert(object);
  attributes[std::move(name)] = object;
}

void Object::delAttribute(const std::string& name)
{
  std::unique_lock<std::mutex> lock{mutex};
  auto result = attributes.find(name);
  assert(result != attributes.end());
  attributes.erase(result);
}

void Object::trace()
{
  if (classof)
    classof->mark();
  for (auto& iter : attributes)
    iter.second->mark();
}

//------------------------------------------------------------------------------
//----Class---------------------------------------------------------------------
//------------------------------------------------------------------------------

Class* Class::getSuperclass()
{
  std::unique_lock<std::mutex> lock{mutex};
  #ifdef BUCKET_DEBUG
  assert(superclass_is_set);
  #endif
  return superclass;
}

void Class::setSuperclass(Class* new_superclass)
{
  std::unique_lock<std::mutex> lock{mutex};
  #ifdef BUCKET_DEBUG
  assert(!superclass_is_set);
  superclass_is_set = true;
  #endif
  superclass = new_superclass;
}

Object* Class::getMethod(const std::string& name)
{
  std::unique_lock<std::mutex> lock{mutex};
  auto result = methods.find(name);
  return result == methods.end() ? nullptr : result->second;
}

void Class::setMethod(std::string name, Method* object)
{
  std::unique_lock<std::mutex> lock{mutex};
  assert(object);
  methods[std::move(name)] = object;
}

void Class::delMethod(const std::string& name)
{
  std::unique_lock<std::mutex> lock{mutex};
  auto result = methods.find(name);
  assert(result != methods.end());
  methods.erase(result);
}

void Class::trace()
{
  Object::trace();
  if (superclass)
    superclass->mark();
  for (auto& iter : methods)
    iter.second->mark();
}

//------------------------------------------------------------------------------
//----CustomClass---------------------------------------------------------------
//------------------------------------------------------------------------------

void CustomClass::setAttribute(std::string name, Object* object)
{
  if (is_locked && cast<Field*>(object))
    exception("interpreter", "you cannot add a field to a class after it is cre"
      "ated");
  Class::setAttribute(std::move(name), object);
}

void CustomClass::delAttribute(const std::string& name)
{
  if (is_locked && cast<Field*>(getAttribute(name)))
    exception("interpreter", "you cannot delete a field from a class after it i"
      "s created");
  Class::delAttribute(std::move(name));
}

void CustomClass::lockAndResolveType(llvm::LLVMContext& context)
{
  std::unique_lock<std::mutex> lock{mutex};
  assert(!is_locked);
  assert(!llvm);
  std::vector<llvm::Type*> fields;
  auto isRTimeField = [](std::pair<std::string, Object*> pair) -> bool {
    auto field = cast<Field*>(pair.second);
    return field && field->getClass()->llvm;
  };
  auto iter = attributes.begin();
  while (true) {
    iter = std::find_if(iter, attributes.end(), isRTimeField);
    if (iter == attributes.end())
      break;
    fields.push_back(
      boost::polymorphic_pointer_downcast<Field>(iter->second)->getClass()->llvm
    );
  }
  if (fields.size())
    llvm = llvm::StructType::create(context, fields);
  is_locked = true;
}

//------------------------------------------------------------------------------
//----BuiltinRTimeMethod--------------------------------------------------------
//------------------------------------------------------------------------------

void BuiltinRTimeMethod::trace()
{
  std::unique_lock<std::mutex> lock{mutex};
  for (auto argument : signature)
    argument->mark();
}

//------------------------------------------------------------------------------
//----TemplateMethod------------------------------------------------------------
//------------------------------------------------------------------------------


void TemplateMethod::trace()
{
  std::unique_lock<std::mutex> lock{mutex};
  for (auto const& cache_entry : cache)
    for (auto const& argument : cache_entry.first)
      argument.second->mark();
}

}
