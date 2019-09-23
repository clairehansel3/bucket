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

#ifndef BUCKET_TOKEN_HXX
#define BUCKET_TOKEN_HXX

#include "source_file.hxx"
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>

enum class Keyword {
  End, If, Elif, Else, Do, For, Break, Cycle, Ret, And, Or, Not, Class, Method
};

enum class Symbol {
  OpenParenthesis, CloseParenthesis, OpenSquareBracket, CloseSquareBracket,
  Plus, Minus, Asterisk, Slash, Caret, PercentSign, ExclamationPoint,
  Equals, DoubleEquals, ExclamationPointEquals, Greater, GreaterOrEqual, Lesser,
  LesserOrEqual, Period, Comma, Colon, AtSymbol, Ampersand, Newline
};

std::string_view keyword2String(Keyword keyword);
// Converts a keyword to a string (e.g. Keyword::Method becomes "method")

std::string_view symbol2String(Symbol symbol);
// Converts a symbol to a string (e.g. Symbol::Ampersand becomes "&")

std::optional<Keyword> string2Keyword(std::string_view string);
// Converts a string to a keyword (e.g. "method" becomes Keyword::Method). If
// the string is not a keyword, a null optional is returned.

std::optional<Symbol> string2Symbol(std::string_view string);
// Converts a string to a keyword (e.g. "&" becomes Symbol::Ampersand). If
// the string is not a symbol, a null optional is returned.

class Token {
// A lexer token. A token is either a null token, an identifier token, a keyword
// token, a symbol token, an integer literal token, a real literal token, a
// string literal token, a character literal token, or a boolean literal token.

public:

  Token();
  // Default constructor which constructs a null token

  static Token createIdentifier(
    std::string value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createKeyword(
    Keyword value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createSymbol(
    Symbol value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createIntegerLiteral(
    std::int64_t value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createRealLiteral(
    double value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createStringLiteral(
    std::string value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createCharacterLiteral(
    std::uint32_t value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createBooleanLiteral(
    bool value,
    SourceFile::iterator begin,
    SourceFile::iterator end);
  static Token createEndOfFile(
    SourceFile::iterator begin,
    SourceFile::iterator end);
  // Factory methods for creating different kinds of tokens

  SourceFile::iterator begin() const;
  SourceFile::iterator end() const;
  // Returns an iterator to the source code corresponding to the beginning/end
  // of the token

  operator bool() const;
  // Returns false if the token is a null token and true if it is not

  std::optional<std::string> getIdentifier() const;
  std::optional<Keyword> getKeyword() const;
  std::optional<Symbol> getSymbol() const;
  std::optional<std::int64_t> getIntegerLiteral() const;
  std::optional<double> getRealLiteral() const;
  std::optional<std::string> getStringLiteral() const;
  std::optional<std::uint32_t> getCharacterLiteral() const;
  std::optional<bool> getBooleanLiteral() const;
  // getIdentifier() returns an optional which is either empty (if the token is
  // not an identifier token) or contains the identifier string. The other
  // methods work in the same way

  bool isEndOfFile() const;
  // Checks whether the token is an end of file token

private:

  std::variant<
    std::monostate, // Index 0: Null Token
    std::string,    // Index 1: Identifier Token
    Keyword,        // Index 2: Keyword Token
    Symbol,         // Index 3: Symbol Token
    std::int64_t,   // Index 4: Integer Literal Token
    double,         // Index 5: Real Literal Token
    std::string,    // Index 6: String Literal Token
    std::uint32_t,  // Index 7: Character Literal Token
    bool,           // Index 8: Boolean Literal Token
    std::monostate  // Index 9: End Of File Token
  > m_value;
  // Basically a token is a giant variant

  SourceFile::iterator m_begin, m_end;

  using ValueType = decltype(m_value);

  Token(
    ValueType&& value,
    SourceFile::iterator&& begin,
    SourceFile::iterator&& end
  );

};

std::ostream& operator<< (std::ostream& stream, const Token& token);
// Custom print function for the Token class

#endif
