#include "token.hxx"
#include <cassert>
#include <utility>

const char* keywordToString(Keyword keyword) noexcept {
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
  }
  assert(false);
  return nullptr;
}

const char* symbolToString(Symbol symbol) noexcept {
  switch (symbol) {
    case Symbol::OpenParenthesis:     return "(";
    case Symbol::CloseParenthesis:    return ")";
    case Symbol::OpenSquareBracket:   return "[";
    case Symbol::CloseSquareBracket:  return "]";
    case Symbol::Plus:                return "+";
    case Symbol::Minus:               return "-";
    case Symbol::Asterisk:            return "*";
    case Symbol::Slash:               return "/";
    case Symbol::Caret:               return "^";
    case Symbol::PercentSign:         return "%";
    case Symbol::SingleEquals:        return "=";
    case Symbol::DoubleEquals:        return "==";
    case Symbol::BangEquals:          return "!=";
    case Symbol::Greater:             return ">";
    case Symbol::GreaterOrEqual:      return ">=";
    case Symbol::Lesser:              return "<";
    case Symbol::LesserOrEqual:       return "<=";
    case Symbol::Period:              return ".";
    case Symbol::Comma:               return ",";
    case Symbol::Colon:               return ":";
    case Symbol::Ampersand:           return "&";
    case Symbol::Newline:             return "newline";
  }
  assert(false);
  return nullptr;
}

Token Token::endOfFile() noexcept {
  return Token();
}

Token Token::identifier(SourceFile::Position begin, SourceFile::Position end, std::string&& identifier) {
  return Token(begin, end, VariantType(std::in_place_index_t<1>(), std::move(identifier)));
}

Token Token::keyword(SourceFile::Position begin, SourceFile::Position end, Keyword keyword) {
  return Token(begin, end, VariantType(std::in_place_index_t<2>(), keyword));
}

Token Token::symbol(SourceFile::Position begin, SourceFile::Position end, Symbol symbol) {
  return Token(begin, end, VariantType(std::in_place_index_t<3>(), symbol));
}

Token Token::integerLiteral(SourceFile::Position begin, SourceFile::Position end, std::int64_t integer_literal) {
  return Token(begin, end, VariantType(std::in_place_index_t<4>(), integer_literal));
}

Token Token::booleanLiteral(SourceFile::Position begin, SourceFile::Position end, bool boolean_literal) {
  return Token(begin, end, VariantType(std::in_place_index_t<5>(), boolean_literal));
}

Token Token::stringLiteral(SourceFile::Position begin, SourceFile::Position end, std::string&& string_literal) {
  return Token(begin, end, VariantType(std::in_place_index_t<6>(), std::move(string_literal)));
}

SourceFile::Position Token::begin() const noexcept {
  return m_begin;
}

SourceFile::Position Token::end() const noexcept {
  return m_end;
}

bool Token::isEndOfFile() const noexcept {
  return std::holds_alternative<std::monostate>(m_value);
}

std::string* Token::getIdentifier() noexcept {
  return std::get_if<1>(&m_value);
}

Keyword* Token::getKeyword() noexcept {
  return std::get_if<2>(&m_value);
}

Symbol* Token::getSymbol() noexcept {
  return std::get_if<3>(&m_value);
}

std::int64_t* Token::getIntegerLiteral() noexcept {
  return std::get_if<4>(&m_value);
}

bool* Token::getBooleanLiteral() noexcept {
  return std::get_if<5>(&m_value);
}

std::string* Token::getStringLiteral() noexcept {
  return std::get_if<6>(&m_value);
}

std::ostream& operator<<(std::ostream& stream, Token& token) {
  if (token.isEndOfFile())
    stream << "<eof>";
  else if (auto identifier_ptr = token.getIdentifier())
    stream << "<id:" << *identifier_ptr << '>';
  else if (auto keyword_ptr = token.getKeyword())
    stream << '<' << keywordToString(*keyword_ptr) << '>';
  else if (auto symbol_ptr = token.getSymbol())
    stream << '<' << symbolToString(*symbol_ptr) << '>';
  else if (auto integer_literal_ptr = token.getIntegerLiteral())
    stream << "<int:" << *integer_literal_ptr << '>';
  else if (auto boolean_literal_ptr = token.getBooleanLiteral())
    stream << "<bool:" << (*boolean_literal_ptr ? "true" : "false") << '>';
  else if (auto string_literal_ptr = token.getStringLiteral())
    stream << "<str:\"" << *string_literal_ptr << "\">";
  else
    assert(false);
  return stream;
}

Token::Token() noexcept
: m_begin{nullptr},
  m_end{nullptr},
  m_value{} {}

Token::Token(SourceFile::Position begin, SourceFile::Position end, VariantType&& value) noexcept
: m_begin{begin},
  m_end{end},
  m_value{std::move(value)} {}
