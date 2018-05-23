#ifndef BUCKET_PARSER_HXX
#define BUCKET_PARSER_HXX

#include "lexer.hxx"

namespace ast {

struct Node;
struct Global;
struct Class;
struct Method;
struct Field;
struct Statement;
struct If;
struct Loop;
struct Break;
struct Cycle;
struct Ret;
struct ExpressionStatement;
struct Expression;
struct Assignment;
struct Call;
struct Identifier;
struct Integer;
struct Bool;

}

class Parser {
public:
  explicit Parser(const char* path);
  std::unique_ptr<ast::Class> parse();
private:
  Lexer m_lexer;
  std::unique_ptr<ast::Global> parseGlobal();
  std::unique_ptr<ast::Class> parseClassDefinition();
  std::unique_ptr<ast::Method> parseMethodDefinition();
  std::unique_ptr<ast::Field> parseMemberVariable();
  std::unique_ptr<ast::Statement> parseStatement();
  std::unique_ptr<ast::If> parseIf();
  std::unique_ptr<ast::Loop> parseLoop();
  std::unique_ptr<ast::Expression> parseExpression();
  std::unique_ptr<ast::Expression> parseOrExpression();
  std::unique_ptr<ast::Expression> parseAndExpression();
  std::unique_ptr<ast::Expression> parseEqualityExpression();
  std::unique_ptr<ast::Expression> parseComparisonExpression();
  std::unique_ptr<ast::Expression> parseArithmeticExpression();
  std::unique_ptr<ast::Expression> parseTerm();
  std::unique_ptr<ast::Expression> parseFactor();
  std::unique_ptr<ast::Expression> parseExponent();
  std::unique_ptr<ast::Expression> parsePostfixExpression();
  bool parseMethodOrAccess(std::unique_ptr<ast::Expression>& expression);
  bool parseCall(std::unique_ptr<ast::Expression>& expression);
  bool parseIndex(std::unique_ptr<ast::Expression>& expression);
  std::unique_ptr<ast::Expression> parseSimpleExpression();
  std::unique_ptr<ast::Expression> parseIdentifier();
  std::unique_ptr<ast::Expression> parseLiteral();
  std::string getIdentifierString();
  std::string expectIdentifier();
  bool accept(Keyword keyword);
  bool accept(Symbol symbol);
  void expect(Keyword keyword);
  void expect(Symbol symbol);
};

#endif
