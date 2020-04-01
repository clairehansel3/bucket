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

#ifndef BUCKET_LEXER_HXX
#define BUCKET_LEXER_HXX

#include "token.hxx"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/noncopyable.hpp>
#include <forward_list>
#include <ostream>
#include <utility>

class Lexer : private boost::noncopyable {
// Similar in design to SourceFile, Lexer is a noncopyable class which has an
// inner iterator class defined which is used to iterate through it. Note that
// each Lexer object contains an internal SourceFile object.

private:

  class LexerIterator : public boost::iterator_facade<
    LexerIterator,
    Token,
    boost::forward_traversal_tag,
    Token
    // The above line is required to allow dereference() to return a value
    // instead of a reference. https://stackoverflow.com/questions/10352308/
    // explains in further detail
  > {
  // LexerIterator instances are iterators which contain a Token instance.
  // dereferencing the LexerIterator returns a copy of the token and
  // incrementing the LexerIterator reads the source code to find the next
  // token, a process which may throw an exception if there is a syntax error in
  // the source code.

  public:

    LexerIterator();

    explicit LexerIterator(SourceFile& source_file, SourceFile::iterator iter);

  private:

    friend class boost::iterator_core_access;

    void increment();

    bool equal(LexerIterator const& other) const;

    Token dereference() const;

    SourceFile* const m_source_file;

    Token m_token;

  };

public:

  using iterator = LexerIterator;

  explicit Lexer(SourceFile& source_file);

  iterator begin();
  iterator end();
  // Returns an iterator to the first/last token. Note that the last token is
  // always an End Of File token.

  void highlight(std::ostream& stream, Token token);
  void highlight(std::ostream& stream, std::forward_list<Token> tokens);
  // Highlights a token or list of tokens in the source code and writes the
  // result to a stream.

  std::string highlight(Token token);
  std::string highlight(std::forward_list<Token> tokens);
  // Highlights a token or list of tokens and returns the result as a string.

private:

  SourceFile& m_source_file;

};

#endif
