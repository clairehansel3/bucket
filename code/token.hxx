#ifndef BUCKET_TOKEN_HXX
#define BUCKET_TOKEN_HXX

#include "source.hxx"
#include <cstdint>
#include <ostream>
#include <string>
#include <variant>

enum class Keyword {
  End, If, Elif, Else, Do, For, Break, Cycle, Ret, And, Or, Not, Class, Method
};

enum class Symbol {
  OpenParenthesis, CloseParenthesis, OpenSquareBracket, CloseSquareBracket,
  Plus, Minus, Asterisk, Slash, Caret, PercentSign, SingleEquals, DoubleEquals,
  BangEquals, Greater, GreaterOrEqual, Lesser, LesserOrEqual, Period, Comma,
  Colon, Ampersand, Newline
};

const char* keywordToString(Keyword keyword) noexcept;

const char* symbolToString(Symbol symbol) noexcept;

class Token {
public:
  static Token endOfFile() noexcept;
  static Token identifier(SourceFile::Position begin, SourceFile::Position end, std::string&& identifier);
  static Token keyword(SourceFile::Position begin, SourceFile::Position end, Keyword keyword);
  static Token symbol(SourceFile::Position begin, SourceFile::Position end, Symbol symbol);
  static Token integerLiteral(SourceFile::Position begin, SourceFile::Position end, std::int64_t integer_literal);
  static Token booleanLiteral(SourceFile::Position begin, SourceFile::Position end, bool boolean_literal);
  static Token stringLiteral(SourceFile::Position begin, SourceFile::Position end, std::string&& string_literal);
  SourceFile::Position begin() const noexcept;
  SourceFile::Position end() const noexcept;
  bool isEndOfFile() const noexcept;
  std::string* getIdentifier() noexcept;
  Keyword* getKeyword() noexcept;
  Symbol* getSymbol() noexcept;
  std::int64_t* getIntegerLiteral() noexcept;
  bool* getBooleanLiteral() noexcept;
  std::string* getStringLiteral() noexcept;
  friend std::ostream& operator<<(std::ostream& stream, Token& token);
private:
  using VariantType = std::variant<
    std::monostate, // End Of File
    std::string,    // Identifier
    Keyword,        // Keyword
    Symbol,         // Symbol
    std::int64_t,   // Integer Literal
    bool,           // Boolean Literal
    std::string     // String Literal
  >;
  SourceFile::Position m_begin, m_end;
  VariantType m_value;
  Token() noexcept;
  Token(SourceFile::Position begin, SourceFile::Position end, VariantType&& variant) noexcept;
};

#endif
