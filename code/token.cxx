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

#include "token.hxx"
#include "miscellaneous.hxx"
#include <map>

std::string_view keyword2String(Keyword keyword)
{
  switch (keyword) {
    case Keyword::End:    return "end";
    case Keyword::If:     return "if";
    case Keyword::Elif:   return "elif";
    case Keyword::Else:   return "else";
    case Keyword::Do:     return "do";
    case Keyword::For:    return "for";
    case Keyword::Break:  return "break";
    case Keyword::Cycle:  return "cycle";
    case Keyword::Ret:    return "ret";
    case Keyword::And:    return "and";
    case Keyword::Or:     return "or";
    case Keyword::Not:    return "not";
    case Keyword::Class:  return "class";
    case Keyword::Method: return "method";
    case Keyword::Decl:   return "decl";
  }
  BUCKET_UNREACHABLE();
}

std::string_view symbol2String(Symbol symbol)
{
  switch (symbol) {
    case Symbol::OpenParenthesis:         return "(";
    case Symbol::CloseParenthesis:        return ")";
    case Symbol::OpenSquareBracket:       return "[";
    case Symbol::CloseSquareBracket:      return "]";
    case Symbol::Plus:                    return "+";
    case Symbol::Minus:                   return "-";
    case Symbol::Asterisk:                return "*";
    case Symbol::Slash:                   return "/";
    case Symbol::Caret:                   return "^";
    case Symbol::PercentSign:             return "%";
    case Symbol::ExclamationPoint:        return "!";
    case Symbol::Equals:                  return "=";
    case Symbol::DoubleEquals:            return "==";
    case Symbol::ExclamationPointEquals:  return "!=";
    case Symbol::Greater:                 return ">";
    case Symbol::GreaterOrEqual:          return ">=";
    case Symbol::Lesser:                  return "<";
    case Symbol::LesserOrEqual:           return "<=";
    case Symbol::Period:                  return ".";
    case Symbol::Comma:                   return ",";
    case Symbol::Colon:                   return ":";
    case Symbol::AtSymbol:                return "@";
    case Symbol::Ampersand:               return "&";
    case Symbol::Newline:                 return "\n";
  }
  BUCKET_UNREACHABLE();
}

std::optional<Keyword> string2Keyword(std::string_view string)
{
  static std::map<std::string_view, Keyword> keyword_dictionary{
    {"end",     Keyword::End},
    {"if",      Keyword::If},
    {"elif",    Keyword::Elif},
    {"else",    Keyword::Else},
    {"do",      Keyword::Do},
    {"for",     Keyword::For},
    {"break",   Keyword::Break},
    {"cycle",   Keyword::Cycle},
    {"ret",     Keyword::Ret},
    {"and",     Keyword::And},
    {"or",      Keyword::Or},
    {"not",     Keyword::Not},
    {"class",   Keyword::Class},
    {"method",  Keyword::Method},
    {"decl",    Keyword::Decl}
  };
  auto iter = keyword_dictionary.find(string);
  if (iter != keyword_dictionary.end()) {
    // found a keyword
    return iter->second;
  }
  else {
    return std::nullopt;
  }
}

std::optional<Symbol> string2Symbol(std::string_view string)
{
  static std::map<std::string_view, Symbol> keyword_dictionary{
    {"(",   Symbol::OpenParenthesis},
    {")",   Symbol::CloseParenthesis},
    {"[",   Symbol::OpenSquareBracket},
    {"]",   Symbol::CloseSquareBracket},
    {"+",   Symbol::Plus},
    {"-",   Symbol::Minus},
    {"*",   Symbol::Asterisk},
    {"/",   Symbol::Slash},
    {"^",   Symbol::Caret},
    {"%",   Symbol::PercentSign},
    {"!",   Symbol::ExclamationPoint},
    {"=",   Symbol::Equals},
    {"==",  Symbol::DoubleEquals},
    {"!=",  Symbol::ExclamationPointEquals},
    {">",   Symbol::Greater},
    {">=",  Symbol::GreaterOrEqual},
    {"<",   Symbol::Lesser},
    {"<=",  Symbol::LesserOrEqual},
    {".",   Symbol::Period},
    {",",   Symbol::Comma},
    {":",   Symbol::Colon},
    {"@",   Symbol::AtSymbol},
    {"&",   Symbol::Ampersand},
    {"\n",  Symbol::Newline}
  };
  auto iter = keyword_dictionary.find(string);
  if (iter != keyword_dictionary.end()) {
    // found a symbol
    return iter->second;
  }
  else {
    return std::nullopt;
  }
}

Token::Token()
: m_value{ValueType(std::in_place_index<0>)},
  m_begin{},
  m_end{}
{}

Token Token::createIdentifier(
  std::string value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<1>, std::move(value)), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createKeyword(
  Keyword value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<2>, value), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createSymbol(
  Symbol value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<3>, value), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createIntegerLiteral(
  std::int64_t value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<4>, value), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createRealLiteral(
  double value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<5>, value), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createStringLiteral(
  std::string value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<6>, std::move(value)), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createCharacterLiteral(
  std::uint32_t value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<7>, value), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createBooleanLiteral(
  bool value,
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<8>, value), std::move(range_begin),
    std::move(range_end)
  );
}

Token Token::createEndOfFile(
  SourceFile::iterator range_begin,
  SourceFile::iterator range_end)
{
  return Token(
    ValueType(std::in_place_index<9>), std::move(range_begin),
    std::move(range_end)
  );
}

SourceFile::iterator Token::begin() const
{
  return m_begin;
}

SourceFile::iterator Token::end() const
{
  return m_end;
}

Token::operator bool() const
{
  return !static_cast<bool>(std::get_if<9>(&m_value));
}

std::optional<std::string> Token::getIdentifier() const
{
  const std::string* ptr = std::get_if<1>(&m_value);
  if (ptr)
    return std::optional<std::string>(*ptr);
  else
    return std::nullopt;
}

std::optional<Keyword> Token::getKeyword() const
{
  const Keyword* ptr = std::get_if<2>(&m_value);
  if (ptr)
    return std::optional<Keyword>(*ptr);
  else
    return std::nullopt;
}

std::optional<Symbol> Token::getSymbol() const
{
  const Symbol* ptr = std::get_if<3>(&m_value);
  if (ptr)
    return std::optional<Symbol>(*ptr);
  else
    return std::nullopt;
}

std::optional<std::int64_t> Token::getIntegerLiteral() const
{
  const std::int64_t* ptr = std::get_if<4>(&m_value);
  if (ptr)
    return std::optional<std::int64_t>(*ptr);
  else
    return std::nullopt;
}

std::optional<double> Token::getRealLiteral() const
{
  const double* ptr = std::get_if<5>(&m_value);
  if (ptr)
    return std::optional<double>(*ptr);
  else
    return std::nullopt;
}

std::optional<std::string> Token::getStringLiteral() const
{
  const std::string* ptr = std::get_if<6>(&m_value);
  if (ptr)
    return std::optional<std::string>(*ptr);
  else
    return std::nullopt;
}

std::optional<std::uint32_t> Token::getCharacterLiteral() const
{
  const std::uint32_t* ptr = std::get_if<7>(&m_value);
  if (ptr)
    return std::optional<std::uint32_t>(*ptr);
  else
    return std::nullopt;
}

std::optional<bool> Token::getBooleanLiteral() const
{
  const bool* ptr = std::get_if<8>(&m_value);
  if (ptr)
    return std::optional<bool>(*ptr);
  else
    return std::nullopt;
}

bool Token::isEndOfFile() const
{
  return static_cast<bool>(std::get_if<9>(&m_value));
}

Token::Token(
  ValueType&& value,
  SourceFile::iterator&& begin,
  SourceFile::iterator&& end)
: m_value{std::move(value)},
  m_begin{std::move(begin)},
  m_end{std::move(end)}
{}

std::ostream& operator<< (std::ostream& stream, const Token& token)
{
  if (auto value = token.getIdentifier()) {
    return stream << "<identifier(" << *value << ")> ";
  }
  else if (auto value = token.getKeyword()) {
    return stream << "<keyword(" << keyword2String(*value) << ")> ";
  }
  else if (auto value = token.getSymbol()) {
    return stream << "<symbol(" << symbol2String(*value) << ")> ";
  }
  else if (auto value = token.getIntegerLiteral()) {
    return stream << "<integer(" << *value << ")> ";
  }
  else if (auto value = token.getRealLiteral()) {
    return stream << "<real(" << *value << ")> ";
  }
  else if (auto value = token.getStringLiteral()) {
    return stream << "<string(" << *value << ")> ";
  }
  else if (auto value = token.getCharacterLiteral()) {
    return stream << "<character(" << *value << ")> ";
  }
  else if (auto value = token.getBooleanLiteral()) {
    return stream << "<boolean(" << (*value ? "true" : "false") << ")> ";
  }
  else if (token.isEndOfFile()) {
    return stream << "<eof> ";
  }
  BUCKET_UNREACHABLE();
}
