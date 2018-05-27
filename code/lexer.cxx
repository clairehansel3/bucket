#include "lexer.hxx"
#include <boost/format.hpp>
#include <cassert>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

Lexer::Lexer(const char* path)
: m_source_file{path},
  m_token{Token::endOfFile()} {
  next();
}

Token& Lexer::token() noexcept {
  return m_token;
}

void Lexer::next() {
  switch (m_source_file.character()) {
    case -1:
      m_token = Token::endOfFile();
      return;

    case '\t':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
      lexSpace();
      return;

    case '\n':
      lexSymbol(Symbol::Newline);
      return;

    case '!': {
      auto begin = m_source_file.position();
      m_source_file.next();
      if (m_source_file.character() != '=')
        throw std::runtime_error("expected '=' after '!'");
      m_source_file.next();
      m_token = Token::symbol(begin, m_source_file.position(), Symbol::BangEquals);
      return;
    }

    case '"':
      lexStringLiteral();
      return;

    case '%':
      lexSymbol(Symbol::PercentSign);
      return;

    case '&':
      lexSymbol(Symbol::Ampersand);
      return;

    case '(':
      lexSymbol(Symbol::OpenParenthesis);
      return;

    case ')':
      lexSymbol(Symbol::CloseParenthesis);
      return;

    case '*':
      lexSymbol(Symbol::Asterisk);
      return;

    case '+':
      lexSymbol(Symbol::Plus);
      return;

    case ',':
      lexSymbol(Symbol::Comma);
      return;

    case '-':
      lexSymbol(Symbol::Minus);
      return;

    case '.':
      lexSymbol(Symbol::Period);
      return;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      lexNumber();
      return;

    case '/':
      lexSlash();
      return;

    case ':':
      lexSymbol(Symbol::Colon);
      return;

    case '<': {
      Symbol symbol;
      auto begin = m_source_file.position();
      m_source_file.next();
      if (m_source_file.character() == '=') {
        symbol = Symbol::LesserOrEqual;
        m_source_file.next();
      }
      else
        symbol = Symbol::Lesser;
      m_token = Token::symbol(begin, m_source_file.position(), symbol);
      return;
    }

    case '=': {
      Symbol symbol;
      auto begin = m_source_file.position();
      m_source_file.next();
      if (m_source_file.character() == '=') {
        symbol = Symbol::DoubleEquals;
        m_source_file.next();
      }
      else
        symbol = Symbol::SingleEquals;
      m_token = Token::symbol(begin, m_source_file.position(), symbol);
      return;
    }

    case '>': {
      Symbol symbol;
      auto begin = m_source_file.position();
      m_source_file.next();
      if (m_source_file.character() == '=') {
        symbol = Symbol::GreaterOrEqual;
        m_source_file.next();
      }
      else
        symbol = Symbol::Greater;
      m_token = Token::symbol(begin, m_source_file.position(), symbol);
      return;
    }

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_':
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      lexIdentifierOrKeyword();
      return;

    case '[':
      lexSymbol(Symbol::OpenSquareBracket);
      return;

    case ']':
      lexSymbol(Symbol::CloseSquareBracket);
      return;

    case '^':
      lexSymbol(Symbol::Caret);
      return;

    default:
      throw std::runtime_error("unexpected character");

  }
}

void Lexer::lexSpace() {
  do
    m_source_file.next();
  while (std::isspace(m_source_file.character()) && m_source_file.character() != '\n' && m_source_file.character() != -1);
  next();
  return;
}

void Lexer::lexSymbol(Symbol symbol) {
  auto begin = m_source_file.position();
  m_source_file.next();
  m_token = Token::symbol(begin, m_source_file.position(), symbol);
}

void Lexer::lexNumber() {
  auto begin = m_source_file.position();
  std::int64_t value = 0;
  constexpr auto max_value = std::numeric_limits<std::int64_t>::max();
  do {
    if ((value > max_value / 10) || (value * 10 > max_value - (m_source_file.character() - '0')))
      throw std::overflow_error((boost::format("integer literal exceeds %1%") % std::numeric_limits<std::int64_t>::max()).str());
    value = value * 10 + (m_source_file.character() - '0');
    m_source_file.next();
  } while (std::isdigit(m_source_file.character()));
  m_token = Token::integerLiteral(begin, m_source_file.position(), value);
}

void Lexer::lexSlash() {
  auto begin = m_source_file.position();
  m_source_file.next();
  if (m_source_file.character() == '/') {
    do
      m_source_file.next();
    while (m_source_file.character() != '\n');
    next();
  }
  else if (m_source_file.character() == '*') {
    m_source_file.next();
    unsigned long long depth = 1;
    do {
      m_source_file.next();
      if (m_source_file.character() == -1)
        throw std::runtime_error("block comment not closed");
      if (m_source_file.character() == '*') {
        m_source_file.next();
        if (m_source_file.character() == '/')
          depth--;
      } else if (m_source_file.character() == '/') {
        m_source_file.next();
        if (m_source_file.character() == '*') {
          if (depth == std::numeric_limits<decltype(depth)>::max())
            throw std::runtime_error("too many nested comments");
          ++depth;
        }
      }
    } while (depth > 0);
    m_source_file.next();
    next();
  }
  else
    m_token = Token::symbol(begin, m_source_file.position(), Symbol::Slash);
}

void Lexer::lexIdentifierOrKeyword() {
  auto begin = m_source_file.position();
  std::string word;
  do {
    word += static_cast<char>(m_source_file.character());
    m_source_file.next();
  } while (std::isalnum(m_source_file.character()) || m_source_file.character() == '_');
  Keyword keyword;
  if (word == "end")
    keyword = Keyword::End;
  else if (word == "if")
    keyword = Keyword::If;
  else if (word == "elif")
    keyword = Keyword::Elif;
  else if (word == "else")
    keyword = Keyword::Else;
  else if (word == "do")
    keyword = Keyword::Do;
  else if (word == "for")
    keyword = Keyword::For;
  else if (word == "break")
    keyword = Keyword::Break;
  else if (word == "cycle")
    keyword = Keyword::Cycle;
  else if (word == "ret")
    keyword = Keyword::Ret;
  else if (word == "and")
    keyword = Keyword::And;
  else if (word == "or")
    keyword = Keyword::Or;
  else if (word == "not")
    keyword = Keyword::Not;
  else if (word == "class")
    keyword = Keyword::Class;
  else if (word == "method")
    keyword = Keyword::Method;
  else if (word == "true") {
    m_token = Token::booleanLiteral(begin, m_source_file.position(), true);
    return;
  }
  else if (word == "false") {
    m_token = Token::booleanLiteral(begin, m_source_file.position(), false);
    return;
  }
  else {
    m_token = Token::identifier(begin, m_source_file.position(), std::move(word));
    return;
  }
  m_token = Token::keyword(begin, m_source_file.position(), keyword);
}

void Lexer::lexStringLiteral() {
  auto begin = m_source_file.position();
  assert(m_source_file.character() == '"');
  m_source_file.next();
  std::string s;
  while (m_source_file.character() != '"') {
    char character;
    if (m_source_file.character() == '\\') {
      m_source_file.next();
      switch (m_source_file.character()) {
        case 'a':
          character = '\a';
          break;
        case 'b':
          character = '\b';
          break;
        case 'f':
          character = '\f';
          break;
        case 'n':
          character = '\n';
          break;
        case 'r':
          character = '\r';
          break;
        case 't':
          character = '\t';
          break;
        case 'v':
          character = '\v';
          break;
        case '\\':
          character = '\\';
          break;
        case '\'':
          character = '\'';
          break;
        case '"':
          character = '"';
          break;
        default:
          throw std::runtime_error("invalid escape sequence");
      }
    }
    else if (m_source_file.character() == -1)
      throw std::runtime_error("string literal not closed");
    else
      character = m_source_file.character();
    s += character;
    m_source_file.next();
  }
  m_source_file.next();
  m_token = Token::stringLiteral(begin, m_source_file.position(), std::move(s));
}
