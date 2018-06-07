#ifndef BUCKET_SYMTABLE_HXX
#define BUCKET_SYMTABLE_HXX

#include <boost/iterator/iterator_adaptor.hpp>
#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace llvm {

class AllocaInst;
class Function;
class Type;

}

namespace sym {

class Entry;
class Variable;
class Method;
class Field;
class Class;
class CompositeClass;

class Visitor {
public:
  virtual ~Visitor() = default;
  virtual void visit(Entry*)          {assert(false);}
  virtual void visit(Variable*)       {assert(false);}
  virtual void visit(Method*)         {assert(false);}
  virtual void visit(Field*)          {assert(false);}
  virtual void visit(Class*)          {assert(false);}
  virtual void visit(CompositeClass*) {assert(false);}
};

class Entry {
public:
  Entry(const Entry&) = delete;
  Entry(Entry&&) = delete;
  Entry& operator=(const Entry&) = delete;
  Entry& operator=(Entry&&) = delete;
  virtual ~Entry() = default;
  std::string_view fullname() const noexcept;
  virtual void receive(Visitor* visitor) {visitor->visit(this);}
protected:
  explicit Entry(std::string&& fullname) noexcept;
private:
  const std::string m_fullname;
};

class Variable final : public Entry {
friend class SymbolTable;
public:
  llvm::AllocaInst* value() const noexcept;
  void setValue(llvm::AllocaInst* value) noexcept;
  Class* classof() const noexcept;
  void receive(Visitor* visitor) override {visitor->visit(this);}
private:
  llvm::AllocaInst* m_value;
  Class* const m_classof;
  Variable(std::string&& fullname, Class* classof) noexcept;
};

class Method final : public Entry {
friend class SymbolTable;
public:
  llvm::Function* function() const noexcept;
  void setFunction(llvm::Function* function) noexcept;
  const std::vector<Class*>& argumentClasses() const noexcept;
  Class* returnClass() const noexcept;
  void receive(Visitor* visitor) override {visitor->visit(this);}
private:
  llvm::Function* m_function;
  const std::vector<Class*> m_argument_classes;
  Class* const m_return_class;
  Method(std::string&& fullname, std::vector<Class*>&& argument_classes, Class* return_class) noexcept;
};

class Field final : public Entry {
friend class SymbolTable;
public:
  Class* classof() const noexcept;
private:
  Class* const m_classof;
  Field(std::string&& fullname, Class* classof) noexcept;
};

class Class : public Entry {
friend class SymbolTable;
public:
  llvm::Type* type() const noexcept;
  void receive(Visitor* visitor) override {visitor->visit(this);}
protected:
  llvm::Type* m_type;
  Class(std::string&& fullname, llvm::Type* type) noexcept;
};

class CompositeClass final : public Class {
friend class SymbolTable;
public:
  void setType(llvm::Type* type) noexcept;
  std::vector<Field*>& fields() noexcept;
  void receive(Visitor* visitor) override {visitor->visit(this);}
private:
  std::vector<Field*> m_fields;
  CompositeClass(std::string&& fullname) noexcept;
};

namespace details {

template <typename To>
class Caster final : public Visitor {
public:
  To* result;
private:
  void visit(Entry* pointer)          {cast(pointer);}
  void visit(Variable* pointer)       {cast(pointer);}
  void visit(Method* pointer)         {cast(pointer);}
  void visit(Field* pointer)          {cast(pointer);}
  void visit(Class* pointer)          {cast(pointer);}
  void visit(CompositeClass* pointer) {cast(pointer);}
  template <typename From>
  void cast([[maybe_unused]] From* pointer) {
    if constexpr (std::is_base_of_v<To, From>)
      result = static_cast<To*>(pointer);
    else
      result = nullptr;
  }
};

}

template <typename ToPointer, typename FromPointer>
ToPointer sym_cast(FromPointer pointer) {
  assert(pointer);
  static_assert(std::is_pointer_v<ToPointer>);
  static_assert(std::is_pointer_v<FromPointer>);
  using To   = std::remove_pointer_t<ToPointer>;
  using From = std::remove_pointer_t<FromPointer>;
  static_assert(std::is_base_of_v<Entry, To>);
  static_assert(std::is_base_of_v<Entry, From>);
  details::Caster<To> caster;
  const_cast<std::add_pointer_t<std::remove_const_t<From>>>(pointer)->receive(&caster);
  assert(caster.result == dynamic_cast<ToPointer>(pointer));
  return caster.result;
}

class SymbolTable {
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
public:
  SymbolTable();
  iterator begin();
  iterator end();
  Entry* lookup(std::string_view shortname) const;
  Entry* lookupKnown(std::string_view name) const;
  Entry* lookupInScope(std::string_view scope, std::string_view shortname) const;
  Entry* lookupExact(std::string_view fullname) const;
  CompositeClass* getParentClass(Entry* entry) const;
  Class* getReferenceClass(Class* cls);
  void pushScope(std::string_view scopename);
  void popScope();
  void pushAnonymousScope();
  void popAnonymousScope();
  Variable* createVariable(std::string_view shortname, Class* classof);
  Method* createMethod(std::string_view shortname, std::vector<Class*>&& argument_classes, Class* return_class);
  Field* createField(std::string_view shortname, Class* classof);
  Class* createClass(std::string_view shortname, llvm::Type* type);
  CompositeClass* createCompositeClass(std::string_view shortname);
  void print();
private:
  std::unordered_map<std::string_view, std::unique_ptr<Entry>> m_entry_map;
  std::string m_active_scope;
};

}

#endif
