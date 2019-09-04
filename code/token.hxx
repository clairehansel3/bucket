#ifndef BUCKET_TOKEN_HXX
#define BUCKET_TOKEN_HXX

#include "source_file.hxx"
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
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

class Token {
// A lexer token. A token is either a null token, an identifier token, a keyword
// token, a symbol token, an integer literal token, a real literal token, a
// string literal token, a character literal token, or a boolean literal token.

public:

  Token();

  static Token createIdentifier(std::string value, SourceFile::iterator_range range);

  static Token createKeyword(Keyword value, SourceFile::iterator_range range);

  static Token createSymbol(Symbol value, SourceFile::iterator_range range);

  static Token createIntegerLiteral(std::int64_t value, SourceFile::iterator_range range);

  static Token createRealLiteral(double value, SourceFile::iterator_range range);

  static Token createStringLiteral(std::string value, SourceFile::iterator_range range);

  static Token createCharacterLiteral(std::uint32_t value, SourceFile::iterator_range range);

  static Token createBooleanLiteral(bool value, SourceFile::iterator_range range);

  static Token createEndOfFile(SourceFile::iterator_range range);

  SourceFile::iterator begin() const;

  SourceFile::iterator end() const;

  operator bool() const;

  std::optional<std::string> getIdentifier() const;

  std::optional<Keyword> getKeyword() const;

  std::optional<Symbol> getSymbol() const;

  std::optional<std::int64_t> getIntegerLiteral() const;

  std::optional<double> getRealLiteral() const;

  std::optional<std::string> getStringLiteral() const;

  std::optional<std::uint32_t> getCharacterLiteral() const;

  std::optional<bool> getBooleanLiteral() const;

  bool isEndOfFile() const;

  friend std::ostream& operator<< (std::ostream& out, const Token& token);

private:

  std::variant<
    std::monostate,
    std::string,
    Keyword,
    Symbol,
    std::int64_t,
    double,
    std::string,
    std::uint32_t,
    bool,
    std::monostate
  > m_value;

  SourceFile::iterator m_begin, m_end;

  using ValueType = decltype(m_value);

  Token(ValueType&& value, SourceFile::iterator_range&& range);

};

#endif
