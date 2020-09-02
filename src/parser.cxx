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

#include "parser.hxx"
#include "miscellaneous.hxx"
#include <utility>

Parser::Parser(Lexer& lexer)
: m_lexer{lexer},
  m_token_iter{lexer.begin()}
{}

std::unique_ptr<ast::Class> Parser::parse()
{
  auto program_ptr = std::make_unique<ast::Class>();
  program_ptr->name = "#module";
  program_ptr->statements = parseGlobals();
  if (!m_token_iter->isEndOfFile())
    exception("parser", "unable to parse top level statement:\n",
              m_lexer.highlight(*m_token_iter));
  return program_ptr;
}

std::vector<std::unique_ptr<ast::Statement>> Parser::parseGlobals()
{
  std::vector<std::unique_ptr<ast::Statement>> result;
  while (true) {
    if (auto global_ptr = parseGlobal()) {
      result.push_back(std::move(global_ptr));
      continue;
    }
    if (accept(Symbol::Newline))
      continue;
    break;
  }
  return result;
}

std::unique_ptr<ast::Statement> Parser::parseGlobal()
{
  if (std::unique_ptr<ast::Statement> global_ptr;
    (global_ptr = parseClass())  ||
    (global_ptr = parseMethod()))
    return global_ptr;
  else
    return nullptr;
}

std::unique_ptr<ast::Class> Parser::parseClass()
{
  if (!accept(Keyword::Class))
    return nullptr;
  auto class_ptr = std::make_unique<ast::Class>();
  class_ptr->name = expectIdentifier();
  expect(Symbol::Newline);
  class_ptr->statements = parseGlobals();
  accept(Keyword::End);
  expect(Symbol::Newline);
  return class_ptr;
}

std::unique_ptr<ast::Method> Parser::parseMethod()
{
  bool is_simple_dispatch;
  if (accept(Keyword::Simple)) {
    expect(Keyword::Method);
    is_simple_dispatch = true;
  }
  else {
    if (!accept(Keyword::Method))
      return nullptr;
    is_simple_dispatch = false;
  }
  auto method_ptr = std::make_unique<ast::Method>();
  method_ptr->is_simple_dispatch = is_simple_dispatch;
  method_ptr->name = expectIdentifier();
  expect(Symbol::OpenParenthesis);
  if (auto first_argument_name = acceptIdentifier()) {
    expect(Symbol::Colon);
    auto first_argument_type = parseExpression();
    if (!first_argument_type)
      exception("parser", "unable to parse method argument type:\n",
                m_lexer.highlight(*m_token_iter));
    method_ptr->arguments.push_back(std::make_pair(
      std::move(*first_argument_name), std::move(first_argument_type)));
    while (accept(Symbol::Comma)) {
      auto argument_name = expectIdentifier();
      expect(Symbol::Colon);
      auto argument_type = parseExpression();
      if (!argument_type)
      exception("parser", "unable to parse method argument type:\n",
                m_lexer.highlight(*m_token_iter));
      method_ptr->arguments.push_back(std::make_pair(std::move(argument_name),
        std::move(argument_type)));
    }
  }
  expect(Symbol::CloseParenthesis);
  if (accept(Symbol::Colon)) {
    if (!(method_ptr->return_type = parseExpression()))
      exception("parser", "unable to parse method return type:\n",
                m_lexer.highlight(*m_token_iter));
  }
  else {
    auto id = std::make_unique<ast::Identifier>();
    id->value = "nil";
    method_ptr->return_type = std::move(id);
  }
  expect(Symbol::Newline);
  method_ptr->statements = parseStatements();
  expect(Keyword::End);
  expect(Symbol::Newline);
  return method_ptr;
}

std::vector<std::unique_ptr<ast::Statement>> Parser::parseStatements()
{
  std::vector<std::unique_ptr<ast::Statement>> result;
  while (true) {
    if (auto statement_ptr = parseStatement()) {
      result.push_back(std::move(statement_ptr));
      continue;
    }
    if (accept(Symbol::Newline)) {
      continue;
    }
    break;
  }
  return result;
}

std::unique_ptr<ast::Statement> Parser::parseStatement()
{
  if (std::unique_ptr<ast::Statement> statement_ptr;
    (statement_ptr = parseDeclaration()) ||
    (statement_ptr = parseIf()) ||
    (statement_ptr = parseInfiniteLoop()) ||
    (statement_ptr = parsePreTestLoop()) ||
    (statement_ptr = parseBreak()) ||
    (statement_ptr = parseCycle()) ||
    (statement_ptr = parseRet()) ||
    (statement_ptr = parseExpressionStatement()))
    return statement_ptr;
  else
    return nullptr;
}

std::unique_ptr<ast::Declaration> Parser::parseDeclaration()
{
  if (accept(Keyword::Decl)) {
    auto declaration_ptr = std::make_unique<ast::Declaration>();
    declaration_ptr->name = expectIdentifier();
    expect(Symbol::Colon);
    if (!(declaration_ptr->expression = parseExpression()))
      exception("parser", "unable to parse type of declaration:\n",
                m_lexer.highlight(*m_token_iter));
    expect(Symbol::Newline);
    return declaration_ptr;
  }
  else
    return nullptr;
}

std::unique_ptr<ast::If> Parser::parseIf()
{
  if (!accept(Keyword::If))
    return nullptr;
  auto if_ptr = std::make_unique<ast::If>();
  if (!(if_ptr->condition = parseExpression()))
    exception("parser", "unable to parse if statement condition:\n",
              m_lexer.highlight(*m_token_iter));
  expect(Symbol::Newline);
  if_ptr->if_body = parseStatements();
  while (accept(Keyword::Elif)) {
    auto elif_condition = parseExpression();
    if (!elif_condition)
      exception("parser", "unable to parse elif statement condition:\n",
                m_lexer.highlight(*m_token_iter));
    expect(Symbol::Newline);
    auto elif_body = parseStatements();
    if_ptr->elif_bodies.push_back(std::make_pair(std::move(elif_condition),
      std::move(elif_body)));
  }
  if (accept(Keyword::Else)) {
    expect(Symbol::Newline);
    if_ptr->else_body = parseStatements();
  }
  expect(Keyword::End);
  expect(Symbol::Newline);
  return if_ptr;
}

std::unique_ptr<ast::InfiniteLoop> Parser::parseInfiniteLoop()
{
  if (!accept(Keyword::Do))
    return nullptr;
  expect(Symbol::Newline);
  auto infinite_loop_ptr = std::make_unique<ast::InfiniteLoop>();
  infinite_loop_ptr->body = parseStatements();
  expect(Keyword::End);
  expect(Symbol::Newline);
  return infinite_loop_ptr;
}

std::unique_ptr<ast::PreTestLoop> Parser::parsePreTestLoop()
{
  if (!accept(Keyword::For))
    return nullptr;
  auto pre_test_loop_ptr = std::make_unique<ast::PreTestLoop>();
  if (!(pre_test_loop_ptr->condition = parseExpression()))
    exception("parser", "unable to parse for loop condition:\n",
              m_lexer.highlight(*m_token_iter));
  expect(Symbol::Newline);
  pre_test_loop_ptr->body = parseStatements();
  if (accept(Keyword::Else)) {
    expect(Symbol::Newline);
    pre_test_loop_ptr->else_body = parseStatements();
  }
  expect(Keyword::End);
  expect(Symbol::Newline);
  return pre_test_loop_ptr;
}

std::unique_ptr<ast::Break> Parser::parseBreak()
{
  if (!accept(Keyword::Break))
    return nullptr;
  expect(Symbol::Newline);
  return std::make_unique<ast::Break>();
}

std::unique_ptr<ast::Cycle> Parser::parseCycle()
{
  if (!accept(Keyword::Cycle))
    return nullptr;
  expect(Symbol::Newline);
  return std::make_unique<ast::Cycle>();
}

std::unique_ptr<ast::Ret> Parser::parseRet()
{
  if (!accept(Keyword::Ret))
    return nullptr;
  auto ret_ptr = std::make_unique<ast::Ret>();
  ret_ptr->expression = parseExpression();
  expect(Symbol::Newline);
  return ret_ptr;
}

std::unique_ptr<ast::ExpressionStatement> Parser::parseExpressionStatement()
{
  if (auto expression = parseExpression()) {
    expect(Symbol::Newline);
    auto expression_statement_ptr =
      std::make_unique<ast::ExpressionStatement>();
    expression_statement_ptr->expression = std::move(expression);
    return expression_statement_ptr;
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::Expression> Parser::parseExpression()
{
  if (auto or_expression = parseOrExpression()) {
    if (!accept(Symbol::Equals))
      return or_expression;
    auto assignment_ptr = std::make_unique<ast::Assignment>();
    assignment_ptr->left = std::move(or_expression);
    if (!(assignment_ptr->right = parseExpression()))
      exception("parser", "unable to parse right hand side of assignment expres"
                "sion:\n", m_lexer.highlight(*m_token_iter));
    return std::move(assignment_ptr);
  }
  else
    return nullptr;
}

std::unique_ptr<ast::Expression> Parser::parseOrExpression()
{
  if (auto and_expression = parseAndExpression()) {
    if (!accept(Keyword::Or))
      return and_expression;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->expression = std::move(and_expression);
    call_ptr->name = "__or__";
    auto argument = parseOrExpression();
    if (!argument)
      exception("parser", "unable to parse right hand side of 'or' expression:"
                "\n", m_lexer.highlight(*m_token_iter));
    call_ptr->arguments.push_back(std::move(argument));
    return std::move(call_ptr);
  }
  else
    return nullptr;
}

std::unique_ptr<ast::Expression> Parser::parseAndExpression()
{
  if (auto equality_expression = parseEqualityExpression()) {
    if (!accept(Keyword::And))
      return equality_expression;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->expression = std::move(equality_expression);
    call_ptr->name = "__and__";
    auto argument = parseAndExpression();
    if (!argument)
      exception("parser", "unable to parse right hand side of 'and' expression:"
                "\n", m_lexer.highlight(*m_token_iter));
    call_ptr->arguments.push_back(std::move(argument));
    return std::move(call_ptr);
  }
  else
    return nullptr;
}

std::unique_ptr<ast::Expression> Parser::parseEqualityExpression()
{
  if (auto comparison_expression = parseComparisonExpression()) {
    std::string name;
    std::string_view debug_name;
    if (accept(Symbol::ExclamationPointEquals)) {
      name = "__neq__";
      debug_name = "!=";
    }
    else if (accept(Symbol::DoubleEquals)) {
      name = "__eq__";
      debug_name = "==";
    }
    else
      return comparison_expression;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->expression = std::move(comparison_expression);
    call_ptr->name = std::move(name);
    auto argument = parseEqualityExpression();
    if (!argument)
      exception("parser", "unable to parse right hand side of '", debug_name,
                "' expression:\n", m_lexer.highlight(*m_token_iter));
    call_ptr->arguments.push_back(std::move(argument));
    return std::move(call_ptr);
  }
  else
    return nullptr;
}

std::unique_ptr<ast::Expression> Parser::parseComparisonExpression()
{
  if (auto arithmetic_expression = parseArithmeticExpression()) {
    std::string name;
    std::string_view debug_name;
    if (accept(Symbol::Greater)) {
      name = "__gt__";
      debug_name = ">";
    }
    else if (accept(Symbol::GreaterOrEqual)) {
      name = "__ge__";
      debug_name = ">=";
    }
    else if (accept(Symbol::Lesser)) {
      name = "__lt__";
      debug_name = "<";
    }
    else if (accept(Symbol::LesserOrEqual)) {
      name = "__le__";
      debug_name = "<=";
    }
    else
      return arithmetic_expression;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->expression = std::move(arithmetic_expression);
    call_ptr->name = std::move(name);
    auto argument = parseComparisonExpression();
    if (!argument)
      exception("parser", "unable to parse right hand side of '", debug_name,
                "' expression:\n", m_lexer.highlight(*m_token_iter));
    call_ptr->arguments.push_back(std::move(argument));
    return std::move(call_ptr);
  }
  else
    return nullptr;
}

std::unique_ptr<ast::Expression> Parser::parseArithmeticExpression()
{
  auto expression = parseTerm();
  if (!expression)
    return nullptr;
  std::string name;
  std::string_view debug_name;
  if (accept(Symbol::Plus)) {
    name = "__add__";
    debug_name = "+";
  }
  else if (accept(Symbol::Minus)) {
    name = "__sub__";
    debug_name = "-";
  }
  else
    return expression;
  auto call_ptr = std::make_unique<ast::Call>();
  call_ptr->expression = std::move(expression);
  call_ptr->name = std::move(name);
  auto argument = parseTerm();
  if (!argument)
    exception("parser", "unable to parse right hand side of '", debug_name,
              "' expression:\n", m_lexer.highlight(*m_token_iter));
  call_ptr->arguments.push_back(std::move(argument));
  while (true) {
    if (accept(Symbol::Plus)) {
      name = "__add__";
      debug_name = "+";
    }
    else if (accept(Symbol::Minus)) {
      name = "__sub__";
      debug_name = "-";
    }
    else
      return std::move(call_ptr);
    auto new_call_ptr = std::make_unique<ast::Call>();
    new_call_ptr->expression = std::move(call_ptr);
    new_call_ptr->name = std::move(name);
    auto next_argument = parseTerm();
    if (!next_argument)
      exception("parser", "unable to parse right hand side of '", debug_name,
                "' expression:\n", m_lexer.highlight(*m_token_iter));
    new_call_ptr->arguments.push_back(std::move(next_argument));
    call_ptr = std::move(new_call_ptr);
  }
}

std::unique_ptr<ast::Expression> Parser::parseTerm()
{
  auto expression = parseFactor();
  if (!expression)
    return nullptr;
  std::string name;
  std::string_view debug_name;
  if (accept(Symbol::Asterisk)) {
    name = "__mul__";
    debug_name = "*";
  }
  else if (accept(Symbol::Slash)) {
    name = "__div__";
    debug_name = "/";
  }
  else if (accept(Symbol::PercentSign)) {
    name = "__mod__";
    debug_name = "%";
  }
  else
    return expression;
  auto call_ptr = std::make_unique<ast::Call>();
  call_ptr->expression = std::move(expression);
  call_ptr->name = std::move(name);
  auto argument = parseFactor();
  if (!argument)
    exception("parser", "unable to parse right hand side of '", debug_name,
              "' expression:\n", m_lexer.highlight(*m_token_iter));
  call_ptr->arguments.push_back(std::move(argument));
  while (true) {
    if (accept(Symbol::Asterisk)) {
      name = "__mul__";
      debug_name = "*";
    }
    else if (accept(Symbol::Slash)) {
      name = "__div__";
      debug_name = "/";
    }
    else if (accept(Symbol::PercentSign)) {
      name = "__mod__";
      debug_name = "%";
    }
    else
      return std::move(call_ptr);
    auto new_call_ptr = std::make_unique<ast::Call>();
    new_call_ptr->expression = std::move(call_ptr);
    new_call_ptr->name = std::move(name);
    auto next_argument = parseFactor();
    if (!next_argument)
      exception("parser", "unable to parse right hand side of '", debug_name,
                "' expression:\n", m_lexer.highlight(*m_token_iter));
    new_call_ptr->arguments.push_back(std::move(next_argument));
    call_ptr = std::move(new_call_ptr);
  }
}

std::unique_ptr<ast::Expression> Parser::parseFactor()
{
  if (auto exponent = parseExponent())
    return exponent;
  std::string name;
  std::string_view debug_name;
  if (accept(Symbol::Plus)) {
    name = "__pos__";
    debug_name = "+";
  }
  else if (accept(Symbol::Minus)) {
    name = "__neg__";
    debug_name = "-";
  }
  else if (accept(Keyword::Not)) {
    name = "__not__";
    debug_name = "not";
  }
  else
    return nullptr;
  auto call_ptr = std::make_unique<ast::Call>();
  if (!(call_ptr->expression = parseFactor()))
    exception("parser", "unable to parse '", debug_name, "' expression:\n",
              m_lexer.highlight(*m_token_iter));
  call_ptr->name = std::move(name);
  return std::move(call_ptr);
}

std::unique_ptr<ast::Expression> Parser::parseExponent()
{
  auto postfix_expression = parsePostfixExpression();
  if (!postfix_expression)
    return nullptr;
  if (!accept(Symbol::Caret))
    return postfix_expression;
  auto call_ptr = std::make_unique<ast::Call>();
  call_ptr->expression = std::move(postfix_expression);
  auto exponent = parseFactor();
  if (!exponent)
    exception("parser", "unable to parse right hand side of '^' expression:\n",
              m_lexer.highlight(*m_token_iter));
  call_ptr->name = "__exp__";
  call_ptr->arguments.push_back(std::move(exponent));
  return std::move(call_ptr);
}

std::unique_ptr<ast::Expression> Parser::parsePostfixExpression()
{
  auto parseMethod = [this](std::unique_ptr<ast::Expression>& expression) {
    if (!accept(Symbol::Period))
      return false;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->name = expectIdentifier();
    if (accept(Symbol::OpenParenthesis)) {
      if (!accept(Symbol::CloseParenthesis)) {
        do {
          auto argument = parseExpression();
          if (!argument)
            exception("parser", "unable to parse method call argument:\n",
                      m_lexer.highlight(*m_token_iter));
          call_ptr->arguments.push_back(std::move(argument));
        } while (accept(Symbol::Comma));
        expect(Symbol::CloseParenthesis);
      }
    }
    call_ptr->expression = std::move(expression);
    expression = std::move(call_ptr);
    return true;
  };
  auto parseCall = [this](std::unique_ptr<ast::Expression>& expression) {
    if (!accept(Symbol::OpenParenthesis))
      return false;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->name = "__call__";
    if (!accept(Symbol::CloseParenthesis)) {
      do {
        auto argument = parseExpression();
        if (!argument)
          exception("parser", "unable to parse function call argument:\n",
                    m_lexer.highlight(*m_token_iter));
        call_ptr->arguments.push_back(std::move(argument));
      } while (accept(Symbol::Comma));
      expect(Symbol::CloseParenthesis);
    }
    call_ptr->expression = std::move(expression);
    expression = std::move(call_ptr);
    return true;
  };
  auto parseIndex = [this](std::unique_ptr<ast::Expression>& expression) {
    if (!accept(Symbol::OpenSquareBracket))
      return false;
    auto call_ptr = std::make_unique<ast::Call>();
    call_ptr->name = "__index__";
    if (!accept(Symbol::CloseSquareBracket)) {
      do {
        auto argument = parseExpression();
        if (!argument)
          exception("parser", "unable to parse object index argument:\n",
                    m_lexer.highlight(*m_token_iter));
        call_ptr->arguments.push_back(std::move(argument));
      } while (accept(Symbol::Comma));
      expect(Symbol::CloseSquareBracket);
    }
    call_ptr->expression = std::move(expression);
    expression = std::move(call_ptr);
    return true;
  };
  auto expression = parseSimpleExpression();
  while (true) {
    if (
      parseMethod(expression) ||
      parseCall(expression) ||
      parseIndex(expression)
    ) continue;
    return expression;
  }
}

std::unique_ptr<ast::Expression> Parser::parseSimpleExpression()
{
  if (accept(Symbol::OpenParenthesis)) {
    auto expression = parseExpression();
    if (!expression)
      exception("parser", "unable to parse expression in parentheses:\n",
                m_lexer.highlight(*m_token_iter));
    expect(Symbol::CloseParenthesis);
    return expression;
  }
  else if (auto identifier_ptr = parseIdentifier()) {
    return std::move(identifier_ptr);
  }
  else if (auto real_literal_ptr = parseRealLiteral()) {
    return std::move(real_literal_ptr);
  }
  else if (auto integer_literal_ptr = parseIntegerLiteral()) {
    return std::move(integer_literal_ptr);
  }
  else if (auto string_literal_ptr = parseStringLiteral()) {
    return std::move(string_literal_ptr);
  }
  else if (auto character_literal_ptr = parseCharacterLiteral()) {
    return std::move(character_literal_ptr);
  }
  else if (auto boolean_literal_ptr = parseBooleanLiteral()) {
    return std::move(boolean_literal_ptr);
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::Identifier> Parser::parseIdentifier()
{
  if (auto value = m_token_iter->getIdentifier()) {
    auto result = std::make_unique<ast::Identifier>();
    result->value = *value;
    ++m_token_iter;
    return result;
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::Real> Parser::parseRealLiteral()
{
  if (auto value = m_token_iter->getRealLiteral()) {
    auto result = std::make_unique<ast::Real>();
    result->value = *value;
    ++m_token_iter;
    return result;
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::Integer> Parser::parseIntegerLiteral()
{
  if (auto value = m_token_iter->getIntegerLiteral()) {
    auto result = std::make_unique<ast::Integer>();
    result->value = *value;
    ++m_token_iter;
    return result;
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::String> Parser::parseStringLiteral()
{
  if (auto value = m_token_iter->getStringLiteral()) {
    auto result = std::make_unique<ast::String>();
    result->value = *value;
    ++m_token_iter;
    return result;
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::Character> Parser::parseCharacterLiteral()
{
  if (auto value = m_token_iter->getCharacterLiteral()) {
    auto result = std::make_unique<ast::Character>();
    result->value = *value;
    ++m_token_iter;
    return result;
  }
  else {
    return nullptr;
  }
}

std::unique_ptr<ast::Boolean> Parser::parseBooleanLiteral()
{
  if (auto value = m_token_iter->getBooleanLiteral()) {
    auto result = std::make_unique<ast::Boolean>();
    result->value = *value;
    ++m_token_iter;
    return result;
  }
  return nullptr;
}

bool Parser::accept(Keyword keyword)
{
  if (auto current_keyword = m_token_iter->getKeyword(); current_keyword
      && *current_keyword == keyword) {
    ++m_token_iter;
    return true;
  }
  return false;
}

bool Parser::accept(Symbol symbol)
{
  if (auto current_symbol = m_token_iter->getSymbol(); current_symbol
      && *current_symbol == symbol) {
    ++m_token_iter;
    return true;
  }
  return false;
}

void Parser::expect(Keyword keyword)
{
  if (!accept(keyword))
    exception("parser", "expected keyword '", keyword2String(keyword),
      "':\n", m_lexer.highlight(*m_token_iter));
}

void Parser::expect(Symbol symbol)
{
  if (!accept(symbol))
    exception("parser", "expected symbol '", symbol2String(symbol),
      "':\n", m_lexer.highlight(*m_token_iter));
}

std::optional<std::string> Parser::acceptIdentifier()
{
  if (auto string = m_token_iter->getIdentifier()) {
    ++m_token_iter;
    return *string;
  }
  return std::nullopt;
}

std::string Parser::expectIdentifier()
{
  if (auto string = acceptIdentifier())
    return *string;
  exception("parser", "expected identifier:\n",
            m_lexer.highlight(*m_token_iter));
}
