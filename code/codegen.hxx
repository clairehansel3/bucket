#ifndef BUCKET_CODEGEN_HXX
#define BUCKET_CODEGEN_HXX

#include "ast.hxx"
#include "symtable.hxx"
#include <initializer_list>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <stack>

class CodeGenerator : private ast::Visitor {
public:
  CodeGenerator();
  void generate(ast::Class* syntax_tree, std::string_view bitcode_file);
private:
  llvm::LLVMContext m_llvm_context;
  llvm::Module m_llvm_module;
  llvm::IRBuilder<> m_llvm_ir_builder;
  sym::SymbolTable m_symbol_table;
  sym::Method* m_current_method;
  bool m_method_returned;
  sym::Class* m_current_value_class;
  llvm::Value* m_current_value;
  std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> m_loop_blocks;
  sym::Class* lookupClass(const ast::Expression* expression) const;
  void declareBuiltinMethod(std::string_view name, std::initializer_list<sym::Class*> args, sym::Class* return_class);
  void initializeBuiltins();
  void initializeClasses(const ast::Class* cls);
  void initializeFieldsAndMethods(const ast::Class* cls);
  void resolveCompositeClasses();
  void resolveCompositeClass(sym::CompositeClass* cls);
  void resolveMethods();
  sym::Variable* createAllocaVariable(std::string_view name, sym::Class* cls);
  void visit(ast::Class*) override;
  void visit(ast::Method*) override;
  void visit(ast::Field*) override;
  void visit(ast::If*) override;
  void visit(ast::Loop*) override;
  void visit(ast::Break*) override;
  void visit(ast::Cycle*) override;
  void visit(ast::Ret*) override;
  void visit(ast::ExpressionStatement*) override;
  void visit(ast::Assignment*) override;
  void visit(ast::Call*) override;
  void visit(ast::Identifier*) override;
  void visit(ast::Integer*) override;
  void visit(ast::Bool*) override;
  void finalize();
};

#endif







/*

#ifndef BUCKET_CODE_GENERATOR_HXX
#define BUCKET_CODE_GENERATOR_HXX

#include "ast.hxx"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include "symtable.hxx"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <stack>

class CodeGenerator : private ast::Visitor {
public:
  CodeGenerator();
  void generate(ast::Class* cls);
  void print();
  void showSymbols();
  void setup(ast::Class* cls);
  llvm::Module& module();
private:
  llvm::LLVMContext m_context;
  llvm::Module m_module;
  sym::SymbolTable m_symbol_table;
  sym::Class* m_bool = nullptr;
  sym::Class* m_int = nullptr;
  sym::Class* m_void = nullptr;
  llvm::IRBuilder<> m_builder;
  sym::Method* m_method = nullptr;
  sym::Class* m_expr_class = nullptr;
  llvm::Value* m_value = nullptr;
  std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> m_loop_blocks;
  bool m_ret = false;
  sym::Class* lookupClass(const ast::Expression* expression) const;
  void initializeBuiltins();
  void initializeClasses(const ast::Class* cls);
  void initializeFieldsAndMethods(const ast::Class* cls);
  void resolveComposites();
  void resolveComposite(sym::CompositeClass* cls);
  void resolveMethods();
  sym::Variable* createAllocaVariable(std::string_view name, sym::Class* cls);
  virtual void visit(ast::Class*)               override;
  virtual void visit(ast::Method*)              override;
  virtual void visit(ast::Field*)               override;
  virtual void visit(ast::If*)                  override;
  virtual void visit(ast::Loop*)                override;
  virtual void visit(ast::Break*)               override;
  virtual void visit(ast::Cycle*)               override;
  virtual void visit(ast::Ret*)                 override;
  virtual void visit(ast::ExpressionStatement*) override;
  virtual void visit(ast::Assignment*)          override;
  virtual void visit(ast::Call*)                override;
  virtual void visit(ast::Identifier*)          override;
  virtual void visit(ast::Integer*)             override;
  virtual void visit(ast::Bool*)                override;
  void finalize();
};

#endif
*/
