#include "lexer.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <utility>

static bool isDigit(std::uint32_t character)
{
  switch (character) {
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
      return true;
    default:
      return false;
  }
}

static bool isLetter(std::uint32_t character)
{
  switch (character) {
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
      return true;

    default:
      return false;
  }
}

static void lex(SourceFile& source_file, Token& token, SourceFile::iterator iter, SourceFile::iterator end)
{
  if (iter == end) {
    token = Token::createEndOfFile(SourceFile::iterator_range(end, end));
    return;
  }
  switch (*iter) {

    case '\t':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
    {
      do
        ++iter;
      while (iter != end && (*iter == '\t' || *iter == '\v' || *iter == '\f'
             || *iter == '\r' || *iter == ' '));
      lex(source_file, token, iter, end);
      return;
    }

    case '\n':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Newline, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '(':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::OpenParenthesis, SourceFile::iterator_range(begin, iter));
      return;
    }
    case ')':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::CloseParenthesis, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '[':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::OpenSquareBracket, SourceFile::iterator_range(begin, iter));
      return;
    }
    case ']':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::CloseSquareBracket, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '+':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Plus, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '-':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Minus, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '*':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Asterisk, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '^':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Caret, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '%':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::PercentSign, SourceFile::iterator_range(begin, iter));
      return;
    }
    case ',':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Comma, SourceFile::iterator_range(begin, iter));
      return;
    }
    case ':':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Colon, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '@':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::AtSymbol, SourceFile::iterator_range(begin, iter));
      return;
    }
    case '&':
    {
      auto begin = iter;
      ++iter;
      token = Token::createSymbol(Symbol::Ampersand, SourceFile::iterator_range(begin, iter));
      return;
    }

    case '=':
    {
      auto begin = iter;
      ++iter;
      if (iter != end && *iter == '=') {
        token = Token::createSymbol(Symbol::DoubleEquals, SourceFile::iterator_range(begin, ++iter));
      }
      else {
        token = Token::createSymbol(Symbol::Equals, SourceFile::iterator_range(begin, iter));
      }
      return;
    }

    case '!':
    {
      auto begin = iter;
      ++iter;
      if (iter != end && *iter == '=') {
        token = Token::createSymbol(Symbol::ExclamationPointEquals, SourceFile::iterator_range(begin, ++iter));
      }
      else {
        token = Token::createSymbol(Symbol::ExclamationPoint, SourceFile::iterator_range(begin, iter));
      }
      return;
    }

    case '>':
    {
      auto begin = iter;
      ++iter;
      if (iter != end && *iter == '=') {
        token = Token::createSymbol(Symbol::GreaterOrEqual, SourceFile::iterator_range(begin, ++iter));
      }
      else {
        token = Token::createSymbol(Symbol::Greater, SourceFile::iterator_range(begin, iter));
      }
      return;
    }

    case '<':
    {
      auto begin = iter;
      ++iter;
      if (iter != end && *iter == '=') {
        token = Token::createSymbol(Symbol::LesserOrEqual, SourceFile::iterator_range(begin, ++iter));
      }
      else {
        token = Token::createSymbol(Symbol::Lesser, SourceFile::iterator_range(begin, iter));
      }
      return;
    }

    case '/':
    {
      auto begin = iter;
      ++iter;
      if (iter != end) {
        if (*iter == '/') {
          do {
            ++iter;
          } while (iter != end && *iter != '\n');
          lex(source_file, token, iter, end);
          return;
        }
        else if (*iter == '*') {
          unsigned long long depth = 1;
          while (true) {
            ++iter;
            if (iter == end) {
              throw make_runtime_error("end of file while inside multiline comment of depth ", depth);
            }
            else if (*iter == '*') {
              ++iter;
              if (iter == end) {
                throw make_runtime_error("end of file while inside multiline comment of depth ", depth);
              }
              else if (*iter == '/') {
                --depth;
                if (depth == 0) {
                  ++iter;
                  lex(source_file, token, iter, end);
                  return;
                }
              }
            }
            else if (*iter == '/') {
              ++iter;
              if (iter == end) {
                throw make_runtime_error("end of file while inside multiline comment of depth ", depth);
              }
              else if (*iter == '*') {
                ++depth;
                // theoretically this could overflow here if you had more than
                // std::numeric_limits<decltype(depth)>::max() nested comments
              }
            }
          }
        }
        else {
          token = Token::createSymbol(Symbol::Slash, SourceFile::iterator_range(begin, iter));
          return;
        }
      }
      else {
        token = Token::createSymbol(Symbol::Slash, SourceFile::iterator_range(begin, iter));
        return;
      }
    }

    case '\"':
    {
      auto begin = iter;
      ++iter;
      std::string string;
      while (iter != end && *iter != '"') {
        if (*iter == '\\') {
          auto start_of_escape_sequence = iter;
          ++iter;
          if (iter == end) {
            throw make_runtime_error("end of file while inside string");
          }
          switch (*iter) {
            case 'a':
              string += '\a';
              break;
            case 'b':
              string += '\b';
              break;
            case 'f':
              string += '\f';
              break;
            case 'n':
              string += '\n';
              break;
            case 'r':
              string += '\r';
              break;
            case 't':
              string += '\t';
              break;
            case 'v':
              string += '\v';
              break;
            case '\\':
              string += '\\';
              break;
            case '\'':
              string += '\'';
              break;
            case '"':
              string += '"';
              break;
            default:
              throw make_runtime_error("invalid escape sequence", '\n', source_file.highlight(SourceFile::iterator_range(start_of_escape_sequence, iter)));
          }
        }
        else {
          utf8::append(*iter, std::back_inserter(string));
        }
        ++iter;
      }
      if (iter == end) {
        throw make_runtime_error("end of file while inside string");
      }
      ++iter;
      std::string utf8string;
      token = Token::createStringLiteral(std::move(utf8string), SourceFile::iterator_range(begin, ++iter));
      return;
    }

    case '\'':
    {
      auto begin = iter;
      std::uint32_t character;
      ++iter;
      if (iter == end) {
        throw make_runtime_error("end of file in character literal");
      }
      else if (*iter == '\'') {
        throw make_runtime_error("empty character literal", '\n', source_file.highlight(SourceFile::iterator_range(begin, ++iter)));
      }
      else if (*iter == '\\') {
        ++iter;
        if (iter == end) {
          throw make_runtime_error("end of file in character literal");
        }
        switch (*iter) {
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
            throw make_runtime_error("invalid escape sequence", '\n', source_file.highlight(SourceFile::iterator_range(begin, ++iter)));
        }
      }
      else {
        character = *iter;
      }
      ++iter;
      if (iter == end) {
        throw make_runtime_error("end of file in character literal");
      }
      else if (*iter != '\'') {
        throw make_runtime_error("extra character in character literal", '\n', source_file.highlight(iter));
      }
      ++iter;
      token = Token::createCharacterLiteral(character, SourceFile::iterator_range(begin, iter));
      return;
    }

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
    {
      auto begin = iter;
      std::string string;
      do {
        utf8::append(*iter, std::back_inserter(string));
        ++iter;
      } while (iter != end && (isDigit(*iter) || isLetter(*iter) || *iter == '_'));
      static std::map<std::string, Keyword> keyword_dictionary{
        {"end", Keyword::End},
        {"if", Keyword::If},
        {"elif", Keyword::Elif},
        {"else", Keyword::Else},
        {"do", Keyword::Do},
        {"for", Keyword::For},
        {"break", Keyword::Break},
        {"cycle", Keyword::Cycle},
        {"ret", Keyword::Ret},
        {"and", Keyword::And},
        {"or", Keyword::Or},
        {"not", Keyword::Not},
        {"class", Keyword::Class},
        {"method", Keyword::Method}
      };
      auto it = keyword_dictionary.find(string);
      if (it != keyword_dictionary.end()) {
        token = Token::createKeyword(it->second, SourceFile::iterator_range(begin, iter));
      }
      else if (string == "true") {
        token = Token::createBooleanLiteral(true, SourceFile::iterator_range(begin, iter));
      }
      else if (string == "false") {
        token = Token::createBooleanLiteral(false, SourceFile::iterator_range(begin, iter));
      }
      else {
        token = Token::createIdentifier(string, SourceFile::iterator_range(begin, iter));
      }
      return;
    }

    case '.':
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
    {
      auto begin = iter;
      std::string number;
      if (*iter == '.') {
        // found the character .
        ++iter;
        if (iter == end || !isDigit(*iter)) {
          // just the character .
          token = Token::createSymbol(Symbol::Period, SourceFile::iterator_range(begin, iter));
          return;
        }
        // digits after the .
        number += '.';
        do {
          number += *iter;
          ++iter;
        } while (iter != end && isDigit(*iter));
      }
      else {
        // read numbers
        do {
          number += *iter;
          ++iter;
        } while (iter != end && isDigit(*iter));
        if (iter != end && *iter == '.') {
          // . found, read numbers after .
          do {
            number += *iter;
            ++iter;
          } while (iter != end && isDigit(*iter));
        }
        else if (iter == end || *iter != 'e' || *iter != 'E') {
          // no . found. check for 'e' or 'E' and if those are not there the
          // number must be an integer
          if (iter != end && (isLetter(*iter) || *iter == '_')) {
            throw make_runtime_error("letter appears in number literal", '\n', source_file.highlight(iter));
          }
          token = Token::createIntegerLiteral(boost::numeric_cast<std::int64_t>(std::stoll(number)), SourceFile::iterator_range(begin, iter));
          return;
        }
      }
      // the number must be a float
      if (iter != end && (*iter == 'e' || *iter == 'E')) {
        number += *iter;
        ++iter;
        if (iter != end && (*iter == '+' | *iter == '-')) {
          number += *iter;
          ++iter;
        }
        if (iter == end) {
          throw make_runtime_error("end of file in real literal");
        }
        else if (!isDigit(*iter)) {
          throw make_runtime_error("expected number in real literal exponent");
        }
        do {
          number += *iter;
          ++iter;
        } while (iter != end && isDigit(*iter));
      }
      if (iter != end && (isLetter(*iter) || *iter == '_')) {
        throw make_runtime_error("letter appears in number literal", '\n', source_file.highlight(iter));
      }
      token = Token::createRealLiteral(std::stod(number), SourceFile::iterator_range(begin, iter));
      return;
    }

    default:
      throw make_runtime_error("unknown");
  }
}

Lexer::LexerIterator::LexerIterator(SourceFile& source_file)
: m_source_file{source_file},
  m_token{std::make_unique<Token>()},
  m_end{}
{}

Lexer::LexerIterator::LexerIterator(SourceFile& source_file, SourceFile::iterator iter, SourceFile::iterator end)
: m_source_file{source_file},
  m_token{std::make_unique<Token>()},
  m_end{end}
{
  lex(m_source_file, *m_token, iter, end);
}

void Lexer::LexerIterator::increment()
{
  lex(m_source_file, *m_token, m_token->end(), m_end);
}

bool Lexer::LexerIterator::equal(LexerIterator const& other) const
{
  return m_token->begin() == other.m_token->begin();
}

Token& Lexer::LexerIterator::dereference() const
{
  return *m_token;
}

Lexer::Lexer(const char* path)
: m_source_file{path}
{}

Lexer::iterator Lexer::begin()
{
  return iterator(m_source_file, m_source_file.begin(), m_source_file.end());
}

Lexer::iterator Lexer::end()
{
  return iterator(m_source_file, m_source_file.end(), m_source_file.end());
}

void Lexer::highlight(std::ostream& stream, Token token)
{
  m_source_file.highlight(stream, SourceFile::iterator_range(token.begin(), token.end()));
}

void Lexer::highlight(std::ostream& stream, std::forward_list<Token> tokens)
{
  auto function = [](Token& token){
    return std::make_pair(token.begin(), token.end());
  };
  SourceFile::iterator_range_list ranges{
    boost::make_transform_iterator(tokens.begin(), function),
    boost::make_transform_iterator(tokens.end(), function)
  };
  m_source_file.highlight(stream, ranges);
}
