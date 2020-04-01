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

#ifndef BUCKET_SYMBOL_TABLE_HXX
#define BUCKET_SYMBOL_TABLE_HXX

#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/noncopyable.hpp>
#include <boost/polymorphic_pointer_cast.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include "miscellaneous.hxx"

#include <iostream>

namespace llvm {
  struct Function;
  struct Type;
  struct AllocaInst;
};

class SymbolTable {

public:

  class Entry;
  class Field;
  class Method;
  class Type;
  class Class;
  class Variable;

  class Visitor {
  public:
    virtual ~Visitor() = default;
    virtual void visit(Entry*);
    virtual void visit(Field*);
    virtual void visit(Method*);
    virtual void visit(Type*);
    virtual void visit(Class*);
    virtual void visit(Variable*);
  };

  class Entry : private boost::noncopyable {
  public:
    virtual ~Entry() = default;
    std::string_view path() const noexcept;
    std::string_view name() const noexcept;
    virtual void receive(Visitor* visitor);
  protected:
    explicit Entry(std::string path);
  private:
    const std::string m_path;
  };

  class Field final : public Entry {
  friend class SymbolTable;
  public:
    Type* const m_type;
    void receive(Visitor* visitor) override;
  private:
    Field(std::string path, Type* type) noexcept;
  };

  class Method final : public Entry {
  friend class SymbolTable;
  public:
    llvm::Function* m_llvm_function;
    const std::vector<Type*> m_argument_types;
    Type* const m_return_type;
    void receive(Visitor* visitor) override;
  private:
    Method(std::string path, std::vector<Type*> argument_types, Type* return_type) noexcept;
  };

  class Type : public Entry {
  friend class SymbolTable;
  public:
    llvm::Type* m_llvm_type;
    void receive(Visitor* visitor) override;
  protected:
    Type(std::string path, llvm::Type* llvm_type) noexcept;
  };

  class Class final : public Type {
  friend class SymbolTable;
  public:
    std::vector<Field*> m_fields;
    void receive(Visitor* visitor) override;
  private:
    Class(std::string path) noexcept;
  };

  class Variable final : public Entry {
  friend class SymbolTable;
  public:
    llvm::AllocaInst* m_llvm_value;
    Type* const m_type;
    void receive(Visitor* visitor) override;
  private:
    Variable(std::string path, Type* type) noexcept;
  };

private:

  template <typename To>
  class Caster final : public Visitor {
  public:
    To* result;
  private:
    void visit(Entry* ptr) override {cast(ptr);}
    void visit(Field* ptr) override {cast(ptr);}
    void visit(Method* ptr) override {cast(ptr);}
    void visit(Type* ptr) override {cast(ptr);}
    void visit(Class* ptr) override {cast(ptr);}
    void visit(Variable* ptr) override {cast(ptr);}
    template <typename From>
    void cast([[maybe_unused]] From* ptr) {
      if constexpr (std::is_base_of_v<To, From>)
        result = static_cast<To*>(ptr);
      else
        result = nullptr;
    }
  };

public:

  template <typename ToPtr, typename FromPtr>
  static ToPtr sym_cast(FromPtr ptr)
  {
    if (ptr) {
      static_assert(std::is_pointer_v<ToPtr>);
      static_assert(std::is_pointer_v<FromPtr>);
      using To   = std::remove_pointer_t<ToPtr>;
      using From = std::remove_pointer_t<FromPtr>;
      static_assert(std::is_base_of_v<Entry, To>);
      static_assert(std::is_base_of_v<Entry, From>);
      Caster<To> caster;
      const_cast<std::add_pointer_t<std::remove_const_t<From>>>(ptr)
        ->receive(&caster);
      BUCKET_ASSERT(caster.result == dynamic_cast<ToPtr>(ptr));
      return caster.result;
    }
    return nullptr;
  }

  SymbolTable();

  Type* getParentType(Entry* entry) const;

  // guide to lookup functions
  // lookupKnownPath : finds a known path and a known type
  // lookupKnownNameInCurrentScope : finds a known name in the current scope and a known type
  // lookup path finds a path
  // lookup find a name in the current scope
  // lookupInScope find a name in a given scope
  // scopes always end in '/'

  template <typename T>
  T* lookupKnownPath(std::string_view path) const
  {
    static_assert(!std::is_pointer_v<T>);
    auto iter = m_map.find(path);
    BUCKET_ASSERT(iter != m_map.end());
    T* result = boost::polymorphic_pointer_downcast<T>(iter->second.get());
    BUCKET_ASSERT(result);
    return result;
  }

  template <typename T>
  T* lookupKnownNameInCurrentScope(std::string_view name) const
  {
    return lookupKnownPath<T>(concatenate(m_scope, name));
  }

  Entry* lookupPath(std::string_view path) const;

  Entry* lookup(std::string_view name) const;

  Entry* lookupInScope(std::string_view scope, std::string_view name) const;

  void pushScope(std::string_view scope = "");

  void popScope();

  Field* createField(std::string_view name, Type* type);

  Method* createMethod(std::string_view name, std::vector<Type*> argument_types, Type* return_type);

  Type* createType(std::string_view name, llvm::Type* llvm_type);

  Class* createClass(std::string_view name);

  Variable* createVariable(std::string_view name, Type* type);

  Type* getReferenceType(Type* type);

  void print();

private:

  class iterator : public boost::iterator_adaptor<
    iterator,
    std::unordered_map<std::string_view, std::unique_ptr<Entry>>::iterator,
    Entry*,
    boost::forward_traversal_tag,
    Entry*
  > {
  public:
    iterator() : iterator::iterator_adaptor_() {}
    iterator(const iterator::iterator_adaptor_::base_type& p) : iterator::iterator_adaptor_(p) {}
  private:
    friend class boost::iterator_core_access;
    Entry* dereference() const {return this->base()->second.get();}
  };

  std::unordered_map<std::string_view, std::unique_ptr<Entry>> m_map;

  std::string m_scope;

public:

  iterator begin();

  iterator end();

};

#endif
