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

#include "miscellaneous.hxx"
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/noncopyable.hpp>
#include <boost/polymorphic_cast.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ast {
  struct Expression;
}

namespace llvm {
  class AllocaInst;
  class Function;
  class Type;
}

class SymbolTable : private boost::noncopyable {
// The symbol table contains entries which are used during code generation.
// Entries are things like types and variables. Each entry has a 'path' which is
// sort of analogous to an os path: e.g. a variable 'foo' in a method 'bar' in
// a class 'baz' in a namespace 'boo' would have path '/boo/baz/bar//foo'. The
// extra '/' will be explained later. The symbol table contains two things: a
// hash table which allows an entry to be looked up if you know its path, and
// the symbol table's current scope, which is a string somewhat analgous to the
// current working directory in an operating system. Now to explain the extra
// '/': while some scopes are named (e.g. a class) some are not (e.g. a for
// loop) and so '/boo/baz/bar//foo' means 'foo' is in an unnamed scope inside
// '/boo/baz/bar'. Leaving an unnamed scope deletes everything in it.

public:

  class Entry;
  class Field;
  class Method;
  class Type;
  class Class;
  class Variable;

  SymbolTable();

  void pushScope(std::string_view name = "");
  void popScope();
  // Used to enter and exit scopes. Calling pushScope() with no arguments pushes
  // an unnamed scope. Note that exiting an unnamed scope will delete everything
  // in it.

  Entry* lookup(std::string_view name);
  Entry* lookup(std::string_view scope, std::string_view name);
  // Looks up a name either in the current scope or in some specified scope.

  template <typename T>
  T gotoName(std::string_view name)
  {
    static_assert(std::is_pointer_v<T>);
    static_assert(std::is_base_of_v<Entry, std::remove_pointer_t<T>>);
    return boost::polymorphic_downcast<T>(gotoName<Entry*>(name));
  }

  template <typename T>
  T gotoPath(std::string_view path)
  {
    static_assert(std::is_pointer_v<T>);
    static_assert(std::is_base_of_v<Entry, std::remove_pointer_t<T>>);
    return boost::polymorphic_downcast<T>(gotoPath<Entry*>(path));
  }

  template <> Entry* gotoName<Entry*>(std::string_view name);
  template <> Entry* gotoPath<Entry*>(std::string_view path);
  // Looks up either a name in the current scope or a path that is known to
  // exist and be an entry of type T. In debug mode these are checked through
  // assertions but in a normal build these are not. This is useful if you want
  // to look up '/bool' for example and you know it exists and is of class Type.

  Field* createField(std::string_view name, Type* type);
  Method* createMethod(std::string_view name, std::vector<Type*> argument_types,
    Type* return_type);
  Type* createType(std::string_view name, llvm::Type* llvm_type);
  Class* createClass(std::string_view name);
  Variable* createVariable(std::string_view name, Type* type);
  // These are the only way to create entries in the SymbolTable (well, except
  // for getReferenceType). They create an entry in the current scope.

  Type* getPointerType(Type* type);
  // Gets a pointer type from a type. If the pointer type already exists in the
  // symbol table, then it is returned. If it doesn't already exist, it is
  // created.

  Type* resolveType(ast::Expression* ast_expression);

  #ifdef BUCKET_DEBUG
  void print();
  #endif

private:

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

public:

  class Entry : private boost::noncopyable {
  // The base class for all symbol table entries. Contains a string containing
  // the entry's path.
  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType*, VisitorType*);
  friend class Visitor;
  public:
    virtual ~Entry() = default;
    std::string_view path() const noexcept;
    // returns the entry's full path
    std::string_view name() const noexcept;
    // returns the last part of the entry's path (e.g. /foo/bar/baz -> baz)
    std::string_view parent() const noexcept;
  protected:
    explicit Entry(std::string path);
  private:
    const std::string m_path;
    virtual void receive(Visitor* visitor);
  };

  class Field final : public Entry {
  // A member variable of a class
  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType*, VisitorType*);
  friend class SymbolTable;
  public:
    Type* const m_type;
  private:
    Field(std::string path, Type* type) noexcept;
    void receive(Visitor* visitor) override;
  };

  class Method final : public Entry {
  // Contains a list of argument types and a return type, as well as an
  // llvm::Function pointer. Note that the list of arguments does not include
  // the implicit this* argument given to non-empty classes. The arguments list
  // and the return type are set on initialization and cannot be changed but
  // the llvm::Function pointer can be changed after initialization with the
  // setter method.
  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType*, VisitorType*);
  friend class SymbolTable;
  public:
    llvm::Function* m_llvm_function;
    const std::vector<Type*> m_argument_types;
    Type* const m_return_type;
  private:
    Method(std::string path, std::vector<Type*> argument_types,
      Type* return_type) noexcept;
    void receive(Visitor* visitor) override;
  };

  class Type : public Entry {
  // Contains a llvm::Type pointer
  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType*, VisitorType*);
  friend class SymbolTable;
  public:
    llvm::Type* m_llvm_type;
  protected:
    Type(std::string path, llvm::Type* llvm_type) noexcept;
  private:
    void receive(Visitor* visitor) override;
  };

  class Class final : public Type {
  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType*, VisitorType*);
  friend class SymbolTable;
  public:
    std::vector<Field*> m_fields;
  private:
    Class(std::string path) noexcept;
    void receive(Visitor* visitor) override;
  };

  class Variable final : public Entry {
  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType*, VisitorType*);
  friend class SymbolTable;
  public:
    llvm::AllocaInst* m_llvm_value;
    Type* const m_type;
  private:
    Variable(std::string path, Type* type) noexcept;
    void receive(Visitor* visitor) override;
  };

  template <typename EntryType, typename VisitorType>
  friend void dispatch(EntryType* entry, VisitorType* visitor)
  {
    static_assert(std::is_base_of_v<Entry, EntryType>);
    static_assert(std::is_base_of_v<Visitor, VisitorType>);
    entry->receive(visitor);
  }

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
      dispatch(const_cast<std::add_pointer_t<std::remove_const_t<From>>>(ptr),
        &caster);
      #ifdef BUCKET_RTTI_ENABLED
      BUCKET_ASSERT(caster.result == dynamic_cast<ToPtr>(ptr));
      #endif
      return caster.result;
    }
    return nullptr;
  }

private:

    class iterator : public boost::iterator_adaptor<
      iterator,
      std::unordered_map<std::string_view, std::unique_ptr<Entry>>::iterator,
      Entry*,
      boost::forward_traversal_tag,
      Entry*
    > {
    public:
      iterator() : iterator::iterator_adaptor_{} {}
      iterator(const iterator::iterator_adaptor_::base_type& p)
      : iterator::iterator_adaptor_{p}
      {}
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
