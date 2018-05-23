#ifndef BUCKET_SYMBOL_TABLE_HXX
#define BUCKET_SYMBOL_TABLE_HXX

#include <boost/core/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace llvm {

class Function;
class Type;
class AllocaInst;

}

namespace sym {

class Entry;
class Variable;
class Field;
class Method;
class Class;
class Composite;

class Entry : private boost::noncopyable {
public:
  virtual ~Entry() = default;
  std::string_view name() const noexcept;
protected:
  Entry(std::string&& name) noexcept;
private:
  const std::string m_name;
};

class Variable final : public Entry {
friend class SymbolTable;
public:
  llvm::AllocaInst* value() const noexcept;
  void setValue(llvm::AllocaInst* value) noexcept;
  Class* cls() const noexcept;
private:
  llvm::AllocaInst* m_value;
  Class* const m_cls;
  Variable(std::string&& name, Class* cls) noexcept;
};

class Field final : public Entry {
friend class SymbolTable;
public:
  Class* cls() const noexcept;
private:
  Class* const m_cls;
  Field(std::string&& name, Class* cls) noexcept;
};

class Method final : public Entry {
friend class SymbolTable;
public:
  llvm::Function* function() const noexcept;
  void setFunction(llvm::Function* function) noexcept;
  const std::vector<Class*>& argumentClasses() const noexcept;
  Class* returnClass() const noexcept;
private:
  llvm::Function* m_function;
  const std::vector<Class*> m_argument_classes;
  Class* const m_return_class;
  Method(std::string&& name, std::vector<Class*>&& argument_classes, Class* return_class) noexcept;
};

class Class : public Entry {
friend class SymbolTable;
public:
  llvm::Type* type() const noexcept;
  Class* pointer();
protected:
  llvm::Type* m_type;
  std::unique_ptr<Class> m_pointer;
  Class(std::string&& name, llvm::Type* type) noexcept;
};

class Composite final : public Class {
friend class SymbolTable;
public:
  void setType(llvm::Type* type) noexcept;
  std::vector<Field*>& fields() noexcept;
private:
  std::vector<Field*> m_fields;
  Composite(std::string&& name) noexcept;
};

class SymbolTable {
public:
  SymbolTable();
  Entry* lookup(std::string_view name) const;
  Entry* lookup(std::string_view scope, std::string_view name) const;
  Composite* getParent(Entry* entry) const;
  void pushScope(std::string_view name);
  void popScope();
  void pushAnonymousScope();
  void popAnonymousScope();
  Variable* createVariable(std::string_view name, Class* cls);
  Field* createField(std::string_view name, Class* cls);
  Method* createMethod(std::string_view name, std::vector<Class*>&& argument_classes, Class* return_class);
  Method* createMethod(std::string scope, std::string_view name, std::vector<Class*>&& argument_classes, Class* return_class);
  Class* createClass(std::string_view name, llvm::Type* type);
  Composite* createComposite(std::string_view name);
  void forEachEntry(const std::function<void(Entry*)>& function);
  void showSymbols();
private:
  std::unordered_map<std::string_view, Entry*> m_table;
  std::string m_scope;
  std::vector<std::unique_ptr<Entry>> m_entries;
};

}

#endif
