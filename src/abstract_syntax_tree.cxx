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

#include "abstract_syntax_tree.hxx"
#include "miscellaneous.hxx"
#include <cassert>

namespace ast
{

namespace
{

class Printer final : public Visitor
{
public:
  explicit Printer(std::ostream& stream);
private:
  std::ostream& m_stream;
  unsigned m_indent_level;
  void visit(Class&) override;
  void visit(Method&) override;
  void visit(If&) override;
  void visit(InfiniteLoop&) override;
  void visit(PreTestLoop&) override;
  void visit(Break&) override;
  void visit(Cycle&) override;
  void visit(Ret&) override;
  void visit(Declaration&) override;
  void visit(ExpressionStatement&) override;
  void visit(Assignment&) override;
  void visit(Call&) override;
  void visit(Identifier&) override;
  void visit(Real&) override;
  void visit(Integer&) override;
  void visit(Boolean&) override;
  void visit(String&) override;
  void visit(Character&) override;
};

Printer::Printer(std::ostream& stream)
: m_stream{stream},
  m_indent_level{0}
{}

void Printer::visit(Class& ast_class)
{
  m_stream << std::string(m_indent_level, ' ') << "class " << ast_class.name
         << " {\n";
  ++m_indent_level;
  for (auto& ast_statement : ast_class.statements)
    dispatch(*this, *ast_statement);
  --m_indent_level;
  m_stream << std::string(m_indent_level, ' ') << "}\n";
}

void Printer::visit(Method& ast_method)
{
  m_stream << std::string(m_indent_level, ' ') <<
    (ast_method.is_simple_dispatch ? "simple method " : "method ")
    << ast_method.name << '(';
  auto iter = ast_method.arguments.begin();
  if (iter != ast_method.arguments.end()) {
    m_stream << iter->first << " : " << *iter->second;
    ++iter;
    while (iter != ast_method.arguments.end()) {
      m_stream << ", " << iter->first << " : " << *iter->second;
      ++iter;
    }
  }
  m_stream << ") : " << *ast_method.return_type << '\n';
  ++m_indent_level;
  for (auto& ast_statement : ast_method.statements)
    dispatch(*this, *ast_statement);
  --m_indent_level;
  m_stream << std::string(m_indent_level, ' ') << "}\n";
}

void Printer::visit(If& ast_if)
{
  m_stream << std::string(m_indent_level, ' ') << "if ";
  dispatch(*this, *ast_if.condition);
  m_stream << " { \n";
  ++m_indent_level;
  for (auto& ast_statement : ast_if.if_body)
    dispatch(*this, *ast_statement);
  for (auto& elif_body : ast_if.elif_bodies) {
    assert(m_indent_level > 0);
    --m_indent_level;
    m_stream << std::string(m_indent_level, ' ') << "elif ";
    dispatch(*this, *elif_body.first);
    m_stream << '\n';
    ++m_indent_level;
    for (auto& ast_statement : elif_body.second)
      dispatch(*this, *ast_statement);
  }
  if (ast_if.else_body.size()) {
    assert(m_indent_level > 0);
    --m_indent_level;
    m_stream << std::string(m_indent_level, ' ') << "else\n";
    ++m_indent_level;
    for (auto& statement : ast_if.else_body)
      dispatch(*this, *statement);
    m_stream << '\n';
  }
  --m_indent_level;
  m_stream << std::string(m_indent_level, ' ') << "}\n";
}

void Printer::visit(InfiniteLoop& ast_infinite_loop)
{
  m_stream << std::string(m_indent_level, ' ') << "do {\n";
  ++m_indent_level;
  for (auto& statement : ast_infinite_loop.body)
    dispatch(*this, *statement);
  --m_indent_level;
  m_stream << std::string(m_indent_level, ' ') << "}\n";
}

void Printer::visit(PreTestLoop& ast_pre_test_loop)
{
  m_stream << std::string(m_indent_level, ' ') << "for ";
  dispatch(*this, *ast_pre_test_loop.condition);
  m_stream << " {\n";
  ++m_indent_level;
  for (auto& statement : ast_pre_test_loop.body)
    dispatch(*this, *statement);
  --m_indent_level;
  m_stream << std::string(m_indent_level, ' ') << "}\n";
}

void Printer::visit(Break&)
{
  m_stream << std::string(m_indent_level, ' ') << "break\n";
}

void Printer::visit(Cycle&)
{
  m_stream << std::string(m_indent_level, ' ') << "cycle\n";
}

void Printer::visit(Ret& ast_ret)
{
  m_stream << std::string(m_indent_level, ' ') << "ret";
  if (ast_ret.expression) {
    m_stream << ' ';
    dispatch(*this, *ast_ret.expression);
  }
  m_stream << '\n';
}

void Printer::visit(Declaration& ast_declaration)
{
  m_stream << std::string(m_indent_level, ' ') << "declaration "
    << ast_declaration.name << " : ";
  dispatch(*this, *ast_declaration.expression);
  m_stream << '\n';
}

void Printer::visit(ExpressionStatement& ast_expression_statement)
{
  m_stream << std::string(m_indent_level, ' ');
  dispatch(*this, *ast_expression_statement.expression);
  m_stream << '\n';
}

void Printer::visit(Assignment& ast_assignment)
{
  dispatch(*this, *ast_assignment.left);
  m_stream << " = ";
  dispatch(*this, *ast_assignment.right);
}

void Printer::visit(Call& ast_call)
{
  dispatch(*this, *ast_call.expression.get());
  m_stream << '.' << ast_call.name << '(';
  auto iter = ast_call.arguments.begin();
  if (iter != ast_call.arguments.end()) {
    dispatch(*this, **iter);
    ++iter;
    while (iter != ast_call.arguments.end()) {
      m_stream << ", ";
      dispatch(*this, **iter);
      ++iter;
    }
  }
  m_stream << ')';
}

void Printer::visit(Identifier& ast_identifier)
{
  m_stream << ast_identifier.value;
}

void Printer::visit(Real& ast_real)
{
  m_stream << ast_real.value;
}

void Printer::visit(Integer& ast_integer)
{
  m_stream << ast_integer.value;
}

void Printer::visit(Boolean& ast_bool)
{
  m_stream << (ast_bool.value ? "true" : "false");
}

void Printer::visit(String& ast_string)
{
  m_stream << ast_string.value;
}

void Printer::visit(Character& ast_character)
{
  m_stream << "'\\U+" << ast_character.value << '\'';
}

}

}

std::ostream& operator<< (std::ostream& stream, ast::Node& node)
{
  ast::Printer printer{stream};
  ast::dispatch(printer, node);
  return stream;
}
