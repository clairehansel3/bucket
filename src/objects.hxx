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

#ifndef BUCKET_OBJECTS_HXX
#define BUCKET_OBJECTS_HXX

#include "garbage_collector.hxx"
#include "polymorphism.hxx"
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ast {
  class Method;
}

namespace llvm {
  class Function;
  class LLVMContext;
  class Type;
}

namespace obj
{

BUCKET_POLYMORPHISM_INIT()

class Object;
class Real;
class Integer;
class Boolean;
class String;
class Character;
class Class;
class CustomClass;
class Field;
class Method;
class BuiltinCTimeMethod;
class BuiltinRTimeMethod;
class TemplateMethod;

//------------------------------------------------------------------------------
//----Object--------------------------------------------------------------------
//------------------------------------------------------------------------------

class Object : public GarbageCollectable
// Base class for all Bucket objects. An object has a class as well as some
// attributes.
{
  BUCKET_POLYMORPHISM_BASE_CLASS()
public:
  Class* getClass();
  void setClass(Class* new_classof);
  Object* getAttribute(const std::string& name);
  virtual void setAttribute(std::string name, Object* object);
  virtual void delAttribute(const std::string& name);
  // Note: setAttribute and delAttribute are virtual because it is overloaded in
  // CustomClass to prevent fields from being added or deleted after the class's
  // llvm type is resolved.
protected:
  void trace() override;
  std::map<std::string, Object*> attributes;
private:
  Class* classof;
  #ifdef BUCKET_DEBUG
  bool classof_is_set = false;
  #endif
};

//------------------------------------------------------------------------------
//----Real----------------------------------------------------------------------
//------------------------------------------------------------------------------

class Real final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  double value;
};

//------------------------------------------------------------------------------
//----Integer-------------------------------------------------------------------
//------------------------------------------------------------------------------

class Integer final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::int64_t value;
};

//------------------------------------------------------------------------------
//----Boolean-------------------------------------------------------------------
//------------------------------------------------------------------------------

class Boolean final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  bool value;
};

//------------------------------------------------------------------------------
//----String--------------------------------------------------------------------
//------------------------------------------------------------------------------

class String final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::string value;
};

//------------------------------------------------------------------------------
//----Character-----------------------------------------------------------------
//------------------------------------------------------------------------------

class Character final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::uint32_t value;
};

//------------------------------------------------------------------------------
//----Byte----------------------------------------------------------------------
//------------------------------------------------------------------------------

class Byte final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::uint8_t value;
};

//------------------------------------------------------------------------------
//----Bytes---------------------------------------------------------------------
//------------------------------------------------------------------------------

class Bytes final : public Object
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::vector<std::uint8_t> value;
};

//------------------------------------------------------------------------------
//----Class---------------------------------------------------------------------
//------------------------------------------------------------------------------

class Class : public Object
// Base class for all Bucket classes. Classes contain the information associated
// with objects as well as an LLVM type, a methods dictionary, and a superclass.
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  Class* getSuperclass();
  void setSuperclass(Class* new_superclass);
  Object* getMethod(const std::string& name);
  void setMethod(std::string name, Method* method);
  void delMethod(const std::string& name);
  llvm::Type* llvm = nullptr;
  // This is null if the object does not have an llvm type (for example a
  // purely compile time object).
protected:
  void trace() override;
private:
  std::map<std::string, Method*> methods;
  Class* superclass;
  #ifdef BUCKET_DEBUG
  bool superclass_is_set = false;
  #endif
};

//------------------------------------------------------------------------------
//----CustomClass---------------------------------------------------------------
//------------------------------------------------------------------------------

class CustomClass final : public Class
// A CustomClass is one created by a 'class' statement in the bucket file being
// compiled. The LLVM type is always either nullptr or a struct type.
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  void setAttribute(std::string name, Object* object) override;
  void delAttribute(const std::string& name) override;
  // This method is overrided to ensure that fields cannot be modified after
  // lockAndResolveType() is called.
  void lockAndResolveType(llvm::LLVMContext& context);
  // This method should be called once fields are done being added to the file.
  // Calling this method creates an LLVM type and prevents new fields from being
  // added.
private:
  bool is_locked;
};

//------------------------------------------------------------------------------
//----Field---------------------------------------------------------------------
//------------------------------------------------------------------------------

class Field final : public Object
// A field object represents a member variable in a custom class. A field does
// not have any special Bucket type, i.e. an integer field of a class and an
// integer variable in a method have the same 'getClass()' and are
// indistinguishable from within bucket, and whether or not something is a field
// is encoded in the C++ class.
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
};

//------------------------------------------------------------------------------
//----Method---------------------------------------------------------------------
//------------------------------------------------------------------------------

class Method : public Object
// A method represents a bucket function. This class should not be directly
// instantiated, instead a subclass should be instantiated.
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  bool is_simple_dispatch;
};

//------------------------------------------------------------------------------
//----BuiltinCTimeMethod--------------------------------------------------------
//------------------------------------------------------------------------------

class BuiltinCTimeMethod final : public Method
// A built-in compile time method written in C++. These are automatically
// generated by the compiler and should not be created from Bucket code. Be
// careful about garbage collection in this class because if 'function'
// internally references any objects, they are not considered to be referenced
// by the method.
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::function<Object*(std::vector<Object*>)> function;
};

//------------------------------------------------------------------------------
//----BuiltinRTimeMethod--------------------------------------------------------
//------------------------------------------------------------------------------

class BuiltinRTimeMethod final : public Method
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  llvm::Function* llvm;
protected:
  void trace() override;
private:
  std::vector<Class*> signature;
};

//------------------------------------------------------------------------------
//----TemplateMethod------------------------------------------------------------
//------------------------------------------------------------------------------

class TemplateMethod final : public Method
// A TemplateMethod is a method that is written in Bucket. It is created with
// the 'method' keyword in a Bucket file being compiled. Note that they hold a
// pointer to the AST so make sure not to free the AST until the comterpreter
// is deleted.
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  ast::Method* code;
protected:
  void trace() override;
private:
  std::map<std::vector<std::pair<bool, Object*>>, llvm::Function*> cache;
};

//------------------------------------------------------------------------------
//----Polymorphism Things-------------------------------------------------------
//------------------------------------------------------------------------------

BUCKET_POLYMORPHISM_SETUP_BASE_1(Object)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Real)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Integer)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Boolean)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(String)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Character)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Byte)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Bytes)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Class)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(CustomClass)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Field)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Method)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(BuiltinCTimeMethod)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(BuiltinRTimeMethod)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(TemplateMethod)
BUCKET_POLYMORPHISM_SETUP_BASE_2(Object)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Real)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Integer)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Boolean)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(String)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Character)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Byte)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Bytes)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Class)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(CustomClass)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Field)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Method)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(BuiltinCTimeMethod)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(BuiltinRTimeMethod)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(TemplateMethod)
BUCKET_POLYMORPHISM_SETUP_BASE_3(Object)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Real)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Integer)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Boolean)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(String)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Character)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Byte)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Bytes)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Class)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(CustomClass)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Field)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Method)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(BuiltinCTimeMethod)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(BuiltinRTimeMethod)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(TemplateMethod)

}

#endif
