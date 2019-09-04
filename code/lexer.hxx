#ifndef BUCKET_LEXER_HXX
#define BUCKET_LEXER_HXX

#include <boost/iterator/iterator_facade.hpp>
#include <boost/noncopyable.hpp>
#include <forward_list>
#include <ostream>
#include <utility>
#include "source_file.hxx"
#include "token.hxx"

class Lexer : private boost::noncopyable {

private:

  class LexerIterator : public boost::iterator_facade<
    LexerIterator,
    Token,
    boost::forward_traversal_tag
  > {

  public:

    LexerIterator(SourceFile& source_file);

    LexerIterator(SourceFile& source_file, SourceFile::iterator iter, SourceFile::iterator end);

  private:

    friend class boost::iterator_core_access;

    void increment();

    bool equal(LexerIterator const& other) const;

    Token& dereference() const;

    SourceFile& m_source_file;

    std::unique_ptr<Token> m_token;

    const SourceFile::iterator m_end;

  };

public:

  using iterator = LexerIterator;
  using iterator_range = std::pair<iterator, iterator>;
  using iterator_range_list = std::forward_list<iterator_range>;

  explicit Lexer(const char* path);

  iterator begin();
  iterator end();

  void highlight(std::ostream& stream, Token token);
  void highlight(std::ostream& stream, std::forward_list<Token> tokens);

private:

  SourceFile m_source_file;

};

#endif
