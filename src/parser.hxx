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

#ifndef BUCKET_PARSER_HXX
#define BUCKET_PARSER_HXX

#include "abstract_syntax_tree.hxx"
#include "lexer.hxx"
#include <boost/noncopyable.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Parser : private boost::noncopyable {

public:

  Parser(Lexer& lexer);

  std::unique_ptr<ast::Class> parse();

private:

  std::vector<std::unique_ptr<ast::Statement>> parseGlobals();
  std::unique_ptr<ast::Statement> parseGlobal();
  std::unique_ptr<ast::Class> parseClass();
  std::unique_ptr<ast::Method> parseMethod();
  std::vector<std::unique_ptr<ast::Statement>> parseStatements();
  std::unique_ptr<ast::Statement> parseStatement();
  std::unique_ptr<ast::Declaration> parseDeclaration();
  std::unique_ptr<ast::If> parseIf();
  std::unique_ptr<ast::InfiniteLoop> parseInfiniteLoop();
  std::unique_ptr<ast::PreTestLoop> parsePreTestLoop();
  std::unique_ptr<ast::Break> parseBreak();
  std::unique_ptr<ast::Cycle> parseCycle();
  std::unique_ptr<ast::Ret> parseRet();
  std::unique_ptr<ast::ExpressionStatement> parseExpressionStatement();
  std::unique_ptr<ast::Expression> parseExpression();
  std::unique_ptr<ast::Expression> parseOrExpression();
  std::unique_ptr<ast::Expression> parseAndExpression();
  std::unique_ptr<ast::Expression> parseEqualityExpression();
  std::unique_ptr<ast::Expression> parseComparisonExpression();
  std::unique_ptr<ast::Expression> parseArithmeticExpression();
  std::unique_ptr<ast::Expression> parseTerm();
  std::unique_ptr<ast::Expression> parseFactor();
  std::unique_ptr<ast::Expression> parseExponent();
  std::unique_ptr<ast::Expression> parsePostfixExpression();
  std::unique_ptr<ast::Expression> parseSimpleExpression();
  std::unique_ptr<ast::Identifier> parseIdentifier();
  std::unique_ptr<ast::Real> parseRealLiteral();
  std::unique_ptr<ast::Integer> parseIntegerLiteral();
  std::unique_ptr<ast::String> parseStringLiteral();
  std::unique_ptr<ast::Character> parseCharacterLiteral();
  std::unique_ptr<ast::Boolean> parseBooleanLiteral();

  bool accept(Keyword);
  bool accept(Symbol);
  std::optional<std::string> acceptIdentifier();
  void expect(Keyword);
  void expect(Symbol);
  std::string expectIdentifier();

  Lexer& m_lexer;
  Lexer::iterator m_token_iter;

};

#endif
