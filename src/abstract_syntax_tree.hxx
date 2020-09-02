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

#ifndef BUCKET_ABSTRACT_SYNTAX_TREE_HXX
#define BUCKET_ABSTRACT_SYNTAX_TREE_HXX

#include "polymorphism.hxx"
#include <boost/noncopyable.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ast
{

BUCKET_POLYMORPHISM_INIT()

class Node : private boost::noncopyable
{
  BUCKET_POLYMORPHISM_BASE_CLASS()
public:
  virtual ~Node() = default;
};

class Statement : public Node
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
};

class Expression : public Node
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
};

class Class final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::string name;
  std::vector<std::unique_ptr<Statement>> statements;
};

class Method final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::string name;
  std::vector<std::pair<std::string, std::unique_ptr<Expression>>> arguments;
  std::unique_ptr<Expression> return_type;
  std::vector<std::unique_ptr<Statement>> statements;
  bool is_simple_dispatch;
};

class If final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::unique_ptr<Expression> condition;
  std::vector<std::unique_ptr<Statement>> if_body;
  std::vector<std::pair<
    std::unique_ptr<Expression>,
    std::vector<std::unique_ptr<Statement>>
  >> elif_bodies;
  std::vector<std::unique_ptr<Statement>> else_body;
};

class InfiniteLoop final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::vector<std::unique_ptr<Statement>> body;
};

class PreTestLoop final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::unique_ptr<Expression> condition;
  std::vector<std::unique_ptr<Statement>> body;
  std::vector<std::unique_ptr<Statement>> else_body;
};

class Break final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
};

class Cycle final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
};

class Ret final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::unique_ptr<Expression> expression;
};

class Declaration : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::string name;
  std::unique_ptr<Expression> expression;
};

class ExpressionStatement final : public Statement
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::unique_ptr<Expression> expression;
};

class Assignment final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::unique_ptr<Expression> left, right;
};

class Call final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::unique_ptr<Expression> expression;
  std::string name;
  std::vector<std::unique_ptr<Expression>> arguments;
  bool is_invocation;
};

class Identifier final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::string value;
};

class Real final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  double value;
};

class Integer final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::int64_t value;
};

class Boolean final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  bool value;
};

class String final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::string value;
};

class Character final : public Expression
{
  BUCKET_POLYMORPHISM_DERIVED_CLASS()
public:
  std::uint32_t value;
};

BUCKET_POLYMORPHISM_SETUP_BASE_1(Node)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Statement)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Expression)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Class)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Method)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(If)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(InfiniteLoop)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(PreTestLoop)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Break)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Cycle)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Ret)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Declaration)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(ExpressionStatement)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Assignment)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Call)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Identifier)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Real)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Integer)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Boolean)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(String)
BUCKET_POLYMORPHISM_SETUP_DERIVED_1(Character)
BUCKET_POLYMORPHISM_SETUP_BASE_2(Node)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Statement)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Expression)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Class)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Method)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(If)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(InfiniteLoop)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(PreTestLoop)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Break)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Cycle)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Ret)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Declaration)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(ExpressionStatement)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Assignment)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Call)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Identifier)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Real)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Integer)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Boolean)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(String)
BUCKET_POLYMORPHISM_SETUP_DERIVED_2(Character)
BUCKET_POLYMORPHISM_SETUP_BASE_3(Node)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Statement)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Expression)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Class)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Method)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(If)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(InfiniteLoop)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(PreTestLoop)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Break)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Cycle)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Ret)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Declaration)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(ExpressionStatement)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Assignment)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Call)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Identifier)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Real)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Integer)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Boolean)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(String)
BUCKET_POLYMORPHISM_SETUP_DERIVED_3(Character)

}

std::ostream& operator<< (std::ostream& stream, ast::Node& node);

#endif
