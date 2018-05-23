#ifndef BUCKET_LEXER_HXX
#define BUCKET_LEXER_HXX

#include "source.hxx"
#include "token.hxx"

class Lexer {
public:
  Lexer(const char* path);
  Token& token() noexcept;
  void next();
private:
  SourceFile m_source_file;
  Token m_token;
  void lexSpace();
  void lexSymbol(Symbol symbol);
  void lexNumber();
  void lexSlash();
  void lexIdentifierOrKeyword();
};

#endif
