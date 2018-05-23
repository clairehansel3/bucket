#ifndef BUCKET_CODE_GENERATOR_HXX
#define BUCKET_CODE_GENERATOR_HXX

#include "ast.hxx"
#include "code_generator_setup.hxx"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <stack>

class CodeGenerator : private ast::Visitor, public CodeGeneratorSetup {
public:
  CodeGenerator();
  void generate(ast::Class* cls);
  void print();
private:
  llvm::IRBuilder<> m_builder;
  sym::Method* m_method = nullptr;
  sym::Class* m_expr_class = nullptr;
  llvm::Value* m_value = nullptr;
  std::stack<std::pair<llvm::BasicBlock*, llvm::BasicBlock*>> m_loop_blocks;
  bool m_ret = false;
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
