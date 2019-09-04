#include "token.hxx"
#include "miscellaneous.hxx"

Token::Token()
: m_value{ValueType(std::in_place_index<0>)}, m_begin{}, m_end{}
{}

Token Token::createIdentifier(std::string value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<1>, std::move(value)), std::move(range));
}

Token Token::createKeyword(Keyword value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<2>, std::move(value)), std::move(range));
}

Token Token::createSymbol(Symbol value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<3>, std::move(value)), std::move(range));
}

Token Token::createIntegerLiteral(std::int64_t value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<4>, std::move(value)), std::move(range));
}

Token Token::createRealLiteral(double value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<5>, std::move(value)), std::move(range));
}

Token Token::createStringLiteral(std::string value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<6>, std::move(value)), std::move(range));
}

Token Token::createCharacterLiteral(std::uint32_t value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<7>, std::move(value)), std::move(range));
}

Token Token::createBooleanLiteral(bool value, SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<8>, std::move(value)), std::move(range));
}

Token Token::createEndOfFile(SourceFile::iterator_range range)
{
  return Token(ValueType(std::in_place_index<9>), std::move(range));
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
  return m_value.index() == 0;
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

static const char* keywordToString(Keyword keyword)
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
  }
  BUCKET_UNREACHABLE();
}

static const char* symbolToString(Symbol symbol)
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
    case Symbol::Newline:                 return "newline";
  }
  BUCKET_UNREACHABLE();
}
std::ostream& operator<< (std::ostream& stream, const Token& token)
{
  if (auto value = token.getIdentifier()) {
    return stream << "<identifier(" << *value << ")> ";
  }
  else if (auto value = token.getKeyword()) {
    return stream << "<keyword(" << keywordToString(*value) << ")> ";
  }
  else if (auto value = token.getSymbol()) {
    return stream << "<symbol(" << symbolToString(*value) << ")> ";
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

Token::Token(ValueType&& value, SourceFile::iterator_range&& range)
: m_value{std::move(value)}, m_begin{std::move(range.first)}, m_end{std::move(range.second)}
{}
