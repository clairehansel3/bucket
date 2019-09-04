#include "lexer.hxx"
#include "miscellaneous.hxx"
#include <algorithm>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iterator>
#include <string>
#include <sstream>
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

static Token lex(SourceFile& source_file, SourceFile::iterator iter)
// This is the function where the lexing actually takes place. It starts lexing
// at 'iter' in the source code and returns a token.
{
  SourceFile::iterator begin = iter;
  SourceFile::iterator end = source_file.end();

  if (iter == end)
    return Token::createEndOfFile(end, end);

  // basically this function is one giant switch statement
  switch (*iter) {

    // Whitespace (excluding newline)
    case '\t':
    case '\v':
    case '\f':
    case '\r':
    case ' ':
    {
      // keep iterating through the whitespace until a non whitespace character
      // (or the end of the file) is found.
      do
        ++iter;
      while (iter != end && (*iter == '\t' || *iter == '\v' || *iter == '\f'
             || *iter == '\r' || *iter == ' '));
      // Recursively restart the whole lexing process at the point after the
      // whitespace.
      return lex(source_file, iter);
    }

    // 'Simple' Symbol
    case '\n':
    {
      ++iter;
      return Token::createSymbol(Symbol::Newline, begin, iter);
    }
    case '(':
    {
      ++iter;
      return Token::createSymbol(Symbol::OpenParenthesis, begin, iter);
    }
    case ')':
    {
      ++iter;
      return Token::createSymbol(Symbol::CloseParenthesis, begin, iter);
    }
    case '[':
    {
      ++iter;
      return Token::createSymbol(Symbol::OpenSquareBracket, begin, iter);
    }
    case ']':
    {
      ++iter;
      return Token::createSymbol(Symbol::CloseSquareBracket, begin, iter);
    }
    case '+':
    {
      ++iter;
      return Token::createSymbol(Symbol::Plus, begin, iter);
    }
    case '-':
    {
      ++iter;
      return Token::createSymbol(Symbol::Minus, begin, iter);
    }
    case '*':
    {
      ++iter;
      return Token::createSymbol(Symbol::Asterisk, begin, iter);
    }
    case '^':
    {
      ++iter;
      return Token::createSymbol(Symbol::Caret, begin, iter);
    }
    case '%':
    {
      ++iter;
      return Token::createSymbol(Symbol::PercentSign, begin, iter);
    }
    case ',':
    {
      ++iter;
      return Token::createSymbol(Symbol::Comma, begin, iter);
    }
    case ':':
    {
      ++iter;
      return Token::createSymbol(Symbol::Colon, begin, iter);
    }
    case '@':
    {
      ++iter;
      return Token::createSymbol(Symbol::AtSymbol, begin, iter);
    }
    case '&':
    {
      ++iter;
      return Token::createSymbol(Symbol::Ampersand, begin, iter);
    }

    // 'Complex' symbol
    case '=':
    {
      Symbol symbol;
      ++iter;
      if (iter != end && *iter == '=') {
        symbol = Symbol::DoubleEquals;
        ++iter;
      }
      else {
        symbol = Symbol::Equals;
      }
      return Token::createSymbol(Symbol::Equals, begin, iter);
    }
    case '!':
    {
      Symbol symbol;
      ++iter;
      if (iter != end && *iter == '=') {
        symbol = Symbol::ExclamationPointEquals;
        ++iter;
      }
      else {
        symbol = Symbol::ExclamationPoint;
      }
      return Token::createSymbol(Symbol::Equals, begin, iter);
    }
    case '>':
    {
      Symbol symbol;
      ++iter;
      if (iter != end && *iter == '=') {
        symbol = Symbol::GreaterOrEqual;
        ++iter;
      }
      else {
        symbol = Symbol::Greater;
      }
      return Token::createSymbol(Symbol::Equals, begin, iter);
    }
    case '<':
    {
      Symbol symbol;
      ++iter;
      if (iter != end && *iter == '=') {
        symbol = Symbol::LesserOrEqual;
        ++iter;
      }
      else {
        symbol = Symbol::Lesser;
      }
      return Token::createSymbol(Symbol::Equals, begin, iter);
    }

    // Comment or Slash Symbol
    case '/':
    {
      ++iter;
      if (iter != end) {
        if (*iter == '/') {
          // Single line comment
          do
            ++iter;
          while (iter != end && *iter != '\n');
          return lex(source_file, iter);
        }
        else if (*iter == '*') {
          // multiline comment
          unsigned long long depth = 1;
          while (true) {
            ++iter;
            if (iter == end) {
              auto begin_plus_one = begin;
              ++++begin_plus_one;
              throw make_error<LexerError>("file ends inside multiline comment "
                "(depth ", depth, ") which starts here:\n",
                source_file.highlight(begin, begin_plus_one));
            }
            else if (*iter == '*') {
              ++iter;
              if (iter == end) {
                auto begin_plus_one = begin;
                ++++begin_plus_one;
                throw make_error<LexerError>("file ends inside multiline commen"
                  "t (depth ", depth, ") which starts here:\n",
                  source_file.highlight(begin, begin_plus_one));
              }
              else if (*iter == '/') {
                --depth;
                if (depth == 0) {
                  ++iter;
                  return lex(source_file, iter);
                }
              }
            }
            else if (*iter == '/') {
              ++iter;
              if (iter == end) {
                auto begin_plus_one = begin;
                ++++begin_plus_one;
                throw make_error<LexerError>("file ends inside multiline commen"
                  "t (depth ", depth, ") which starts here:\n",
                  source_file.highlight(begin, begin_plus_one));
              }
              else if (*iter == '*') {
                // theoretically this could overflow here if you had more than
                // std::numeric_limits<decltype(depth)>::max() nested comments
                ++depth;
              }
            }
          }
        }
      }
      return Token::createSymbol(Symbol::Slash, begin, iter);
    }

    // String Literal
    case '\"':
    {
      std::string string;
      ++iter;
      while (iter != end && *iter != '"') {
        if (*iter == '\\') {
          auto start_of_escape_sequence = iter;
          ++iter;
          if (iter == end)
            throw make_error<LexerError>("file ends while inside string which s"
              "tarts here:\n", source_file.highlight(begin));
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
              {
                std::string character;
                utf8::append(*iter, std::back_inserter(character));
                ++iter;
                throw make_error<LexerError>("invalid escape sequence '\\",
                  character, "':\n", source_file.highlight(
                    start_of_escape_sequence, iter));
              }
          }
        }
        else {
          utf8::append(*iter, std::back_inserter(string));
        }
        ++iter;
      }
      if (iter == end)
        throw make_error<LexerError>("file ends while inside string which start"
          "s here:\n", source_file.highlight(begin));
      ++iter;
      return Token::createStringLiteral(std::move(string), begin, iter);
    }

    case '\'':
    {
      std::uint32_t character;
      ++iter;
      if (iter == end)
        throw make_error<LexerError>("file ends while inside character literal "
          "which starts here:\n", source_file.highlight(begin));
      else if (*iter == '\'') {
        ++iter;
        throw make_error<LexerError>("empty character literal:\n",
          source_file.highlight(begin, iter));
      }
      else if (*iter == '\\') {
        ++iter;
        if (iter == end)
          throw make_error<LexerError>("file ends while inside character litera"
            "l which starts here:\n", source_file.highlight(begin));
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
          {
            std::string character;
            auto start_of_escape_sequence = begin;
            ++start_of_escape_sequence;
            utf8::append(*iter, std::back_inserter(character));
            ++iter;
            throw make_error<LexerError>("invalid escape sequence '\\",
              character, "':\n", source_file.highlight(
                start_of_escape_sequence, iter));
          }
        }
      }
      else {
        character = *iter;
      }
      ++iter;
      if (iter == end)
        throw make_error<LexerError>("file ends while inside character literal "
          "which starts here:\n", source_file.highlight(begin));
      else if (*iter != '\'')
        throw make_error<LexerError>("extra character appears in character lite"
          "ral:\n", source_file.highlight(iter));
      ++iter;
      return Token::createCharacterLiteral(character, begin, iter);
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
      std::string string;
      do {
        utf8::append(*iter, std::back_inserter(string));
        ++iter;
      } while (iter != end
               && (isDigit(*iter) || isLetter(*iter) || *iter == '_'));
      if (auto keyword = string2Keyword(string))
        return Token::createKeyword(*keyword, begin, iter);
      else if (string == "true")
        return Token::createBooleanLiteral(true, begin, iter);
      else if (string == "false")
        return Token::createBooleanLiteral(false, begin, iter);
      else
        return Token::createIdentifier(std::move(string), begin, iter);
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
      std::string number;
      if (*iter == '.') {
        // found the character .
        ++iter;
        if (iter == end || !isDigit(*iter)) {
          // just the character .
          return Token::createSymbol(Symbol::Period, begin, iter);
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
          if (iter != end && (isLetter(*iter) || *iter == '_'))
            throw make_error<LexerError>("letter appears in number literal:\n",
              source_file.highlight(iter));
          return Token::createIntegerLiteral(
            boost::numeric_cast<std::int64_t>(std::stoll(number)),
            begin, iter
          );
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
        if (iter == end)
          throw make_error<LexerError>("file ends in real literal starting here"
            ":\n", source_file.highlight(begin));
        else if (!isDigit(*iter))
          throw make_error<LexerError>("expected digit in real literal exponent"
            ":\n", source_file.highlight(iter));
        do {
          number += *iter;
          ++iter;
        } while (iter != end && isDigit(*iter));
      }
      if (iter != end && (isLetter(*iter) || *iter == '_'))
        throw make_error<LexerError>("letter appears in number literal:\n",
          source_file.highlight(iter));
      return Token::createRealLiteral(std::stod(number), begin, iter);
    }

    default:
    {
      std::string character;
      utf8::append(*iter, std::back_inserter(character));
      std::stringstream character_hex;
      character_hex << std::hex << std::setfill('0') << std::setw(5)<< *iter;
      throw make_error<LexerError>("unidentified character '", character, "' (",
        "U+", character_hex.str(), ") in source code:\n",
        source_file.highlight(iter));
    }
  }
}

Lexer::LexerIterator::LexerIterator()
: m_source_file{nullptr},
  m_token{}
{}

Lexer::LexerIterator::LexerIterator(
  SourceFile& source_file,
  SourceFile::iterator iter)
: m_source_file{&source_file},
  m_token{}
{
  m_token = lex(*m_source_file, iter);
}

void Lexer::LexerIterator::increment()
{
  m_token = lex(*m_source_file, m_token.end());
}

bool Lexer::LexerIterator::equal(LexerIterator const& other) const
{
  if (m_token.begin() == other.m_token.begin())
    assert(m_token == other.m_token);
  return m_token.begin() == other.m_token.begin();
}

Token Lexer::LexerIterator::dereference() const
{
  return m_token;
}

Lexer::Lexer(const char* path)
: m_source_file{path}
{}

Lexer::iterator Lexer::begin()
{
  return iterator(m_source_file, m_source_file.begin());
}

Lexer::iterator Lexer::end()
{
  return iterator(m_source_file, m_source_file.end());
}

void Lexer::highlight(std::ostream& stream, Token token)
{
  m_source_file.highlight(stream, token.begin(), token.end());
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

std::string Lexer::highlight(Token token)
{
  std::stringstream ss;
  highlight(ss, std::move(token));
  return ss.str();
}

std::string Lexer::highlight(std::forward_list<Token> tokens)
{
  std::stringstream ss;
  highlight(ss, std::move(tokens));
  return ss.str();
}
