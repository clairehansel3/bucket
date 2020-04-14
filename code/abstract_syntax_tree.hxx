// Copyright (C) 2019  Claire Hansel
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

#ifndef BUCKET_ABSTRACT_SYNTAX_TREE_HXX
#define BUCKET_ABSTRACT_SYNTAX_TREE_HXX

#include "miscellaneous.hxx"
#include <boost/noncopyable.hpp>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ast {

struct Node;
struct Program;
struct Global;
struct Class;
struct Method;
struct Field;
struct Statement;
struct Declaration;
struct If;
struct InfiniteLoop;
struct PreTestLoop;
struct Break;
struct Cycle;
struct Ret;
struct ExpressionStatement;
struct Expression;
struct Assignment;
struct Call;
struct Identifier;
struct Literal;
struct Real;
struct Integer;
struct Boolean;
struct String;
struct Character;

class Visitor {
public:
  virtual ~Visitor() = default;
  virtual void visit(Node*);
  virtual void visit(Program*);
  virtual void visit(Global*);
  virtual void visit(Class*);
  virtual void visit(Method*);
  virtual void visit(Field*);
  virtual void visit(Statement*);
  virtual void visit(Declaration*);
  virtual void visit(If*);
  virtual void visit(InfiniteLoop*);
  virtual void visit(PreTestLoop*);
  virtual void visit(Break*);
  virtual void visit(Cycle*);
  virtual void visit(Ret*);
  virtual void visit(ExpressionStatement*);
  virtual void visit(Expression*);
  virtual void visit(Assignment*);
  virtual void visit(Call*);
  virtual void visit(Identifier*);
  virtual void visit(Literal*);
  virtual void visit(Real*);
  virtual void visit(Integer*);
  virtual void visit(Boolean*);
  virtual void visit(String*);
  virtual void visit(Character*);
};

struct Node : private boost::noncopyable {
  virtual ~Node() = default;
  virtual void receive(Visitor*);
};

struct Program final : Node {
  std::vector<std::unique_ptr<Global>> globals;
  void receive(Visitor*) override;
};

struct Global : Node {
  void receive(Visitor*) override;
};

struct Class final : Global {
  std::string name;
  std::vector<std::unique_ptr<Global>> globals;
  void receive(Visitor*) override;
};

struct Method final : Global {
  std::string name;
  std::vector<std::pair<std::string, std::unique_ptr<Expression>>> arguments;
  std::unique_ptr<Expression> return_type;
  std::vector<std::unique_ptr<Statement>> statements;
  void receive(Visitor*) override;
};

struct Field final : Global {
  std::string name;
  std::unique_ptr<Expression> type;
  void receive(Visitor*) override;
};

struct Statement : Node {
  void receive(Visitor*) override;
};

struct Declaration final : Statement {
  std::string name;
  std::unique_ptr<Expression> type;
  void receive(Visitor*) override;
};

struct If final : Statement {
  std::unique_ptr<Expression> condition;
  std::vector<std::unique_ptr<Statement>> if_body;
  std::vector<std::pair<std::unique_ptr<Expression>,
    std::vector<std::unique_ptr<Statement>>>> elif_bodies;
  std::vector<std::unique_ptr<Statement>> else_body;
  void receive(Visitor*) override;
};

struct InfiniteLoop final : Statement {
  std::vector<std::unique_ptr<Statement>> body;
  void receive(Visitor*) override;
};

struct PreTestLoop final : Statement {
  std::unique_ptr<Expression> condition;
  std::vector<std::unique_ptr<Statement>> body;
  std::vector<std::unique_ptr<Statement>> else_body;
  void receive(Visitor*) override;
};

struct Break final : Statement {
  void receive(Visitor*) override;
};

struct Cycle final : Statement {
  void receive(Visitor*) override;
};

struct Ret final : Statement {
  std::unique_ptr<Expression> expression;
  void receive(Visitor*) override;
};

struct ExpressionStatement final : Statement {
  std::unique_ptr<Expression> expression;
  void receive(Visitor*) override;
};

struct Expression : Node {
  void receive(Visitor*) override;
};

struct Assignment final : Expression {
  std::unique_ptr<Expression> left, right;
  void receive(Visitor*) override;
};

struct Call final : Expression {
  std::unique_ptr<Expression> expression;
  std::string name;
  std::vector<std::unique_ptr<Expression>> arguments;
  void receive(Visitor*) override;
};

struct Identifier final : Expression {
  std::string value;
  void receive(Visitor*) override;
};

struct Literal : Expression {
  void receive(Visitor*) override;
};

struct Real final : Literal {
  double value;
  void receive(Visitor*) override;
};

struct Integer final : Literal {
  std::int64_t value;
  void receive(Visitor*) override;
};

struct Boolean final : Literal {
  bool value;
  void receive(Visitor*) override;
};

struct String final : Literal {
  std::string value;
  void receive(Visitor*) override;
};

struct Character final : Literal {
  std::uint32_t value;
  void receive(Visitor*) override;
};

namespace details {

template <typename To>
class Caster final : public Visitor {
public:
  To* result;
private:
  void visit(Node* ptr) override {cast(ptr);}
  void visit(Program* ptr) override {cast(ptr);}
  void visit(Global* ptr) override {cast(ptr);}
  void visit(Class* ptr) override {cast(ptr);}
  void visit(Method* ptr) override {cast(ptr);}
  void visit(Field* ptr) override {cast(ptr);}
  void visit(Statement* ptr) override {cast(ptr);}
  void visit(Declaration* ptr) override {cast(ptr);}
  void visit(If* ptr) override {cast(ptr);}
  void visit(InfiniteLoop* ptr) override {cast(ptr);}
  void visit(PreTestLoop* ptr) override {cast(ptr);}
  void visit(Break* ptr) override {cast(ptr);}
  void visit(Cycle* ptr) override {cast(ptr);}
  void visit(Ret* ptr) override {cast(ptr);}
  void visit(ExpressionStatement* ptr) override {cast(ptr);}
  void visit(Expression* ptr) override {cast(ptr);}
  void visit(Assignment* ptr) override {cast(ptr);}
  void visit(Call* ptr) override {cast(ptr);}
  void visit(Identifier* ptr) override {cast(ptr);}
  void visit(Literal* ptr) override {cast(ptr);}
  void visit(Real* ptr) override {cast(ptr);}
  void visit(Integer* ptr) override {cast(ptr);}
  void visit(Boolean* ptr) override {cast(ptr);}
  void visit(String* ptr) override {cast(ptr);}
  void visit(Character* ptr) override {cast(ptr);}
  template <typename From>
  void cast([[maybe_unused]] From* ptr) {
    if constexpr (std::is_base_of_v<To, From>)
      result = static_cast<To*>(ptr);
    else
      result = nullptr;
  }
};

}

template <typename ToPtr, typename FromPtr>
ToPtr ast_cast(FromPtr ptr)
// 'ast_cast' is exactly like 'dynamic_cast' except it is only for Node
// instances, it runs in constant time, and it only works for pointers.
{
  if (ptr) {
    static_assert(std::is_pointer_v<ToPtr>);
    static_assert(std::is_pointer_v<FromPtr>);
    using To   = std::remove_pointer_t<ToPtr>;
    using From = std::remove_pointer_t<FromPtr>;
    static_assert(std::is_base_of_v<Node, To>);
    static_assert(std::is_base_of_v<Node, From>);
    details::Caster<To> caster;
    const_cast<std::add_pointer_t<std::remove_const_t<From>>>(ptr)
      ->receive(&caster);
    #ifdef BUCKET_RTTI_ENABLED
    BUCKET_ASSERT(caster.result == dynamic_cast<ToPtr>(ptr));
    #endif
    return caster.result;
  }
  return nullptr;
}

template <typename NodeType, typename VisitorType>
void dispatch(NodeType* node, VisitorType* visitor)
{
  node->receive(visitor);
}

}

std::ostream& operator<< (std::ostream& stream, ast::Node& node);

#endif
