#ifndef BUCKET_CODE_GENERATOR_SETUP_HXX
#define BUCKET_CODE_GENERATOR_SETUP_HXX

#include "symtable.hxx"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace ast {

struct Class;
struct Expression;

}

class CodeGeneratorSetup {
public:
  CodeGeneratorSetup();
  void setup(ast::Class* cls);
  void showSymbols();
  llvm::Module& module();
protected:
  llvm::LLVMContext m_context;
  llvm::Module m_module;
  sym::SymbolTable m_symbol_table;
  sym::Class* m_bool = nullptr;
  sym::Class* m_int = nullptr;
  sym::Class* m_void = nullptr;
private:
  sym::Class* lookupClass(const ast::Expression* expression) const;
  void initializeBuiltins();
  void initializeClasses(const ast::Class* cls);
  void initializeFieldsAndMethods(const ast::Class* cls);
  void resolveComposites();
  void resolveComposite(sym::CompositeClass* cls);
  void resolveMethods();
};

#endif
