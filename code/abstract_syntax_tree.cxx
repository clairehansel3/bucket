#include "abstract_syntax_tree.hxx"
#include "miscellaneous.hxx"

using namespace ast;

void Visitor::visit(Node*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Program*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Global*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Class*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Method*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Field*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Statement*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Declaration*) {BUCKET_UNREACHABLE();}
void Visitor::visit(If*) {BUCKET_UNREACHABLE();}
void Visitor::visit(InfiniteLoop*) {BUCKET_UNREACHABLE();}
void Visitor::visit(PreTestLoop*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Break*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Cycle*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Ret*) {BUCKET_UNREACHABLE();}
void Visitor::visit(ExpressionStatement*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Expression*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Assignment*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Call*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Identifier*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Literal*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Real*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Integer*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Boolean*) {BUCKET_UNREACHABLE();}
void Visitor::visit(String*) {BUCKET_UNREACHABLE();}
void Visitor::visit(Character*) {BUCKET_UNREACHABLE();}

void Node::receive(Visitor* ptr) {ptr->visit(this);}
void Program::receive(Visitor* ptr) {ptr->visit(this);}
void Global::receive(Visitor* ptr) {ptr->visit(this);}
void Class::receive(Visitor* ptr) {ptr->visit(this);}
void Method::receive(Visitor* ptr) {ptr->visit(this);}
void Field::receive(Visitor* ptr) {ptr->visit(this);}
void Statement::receive(Visitor* ptr) {ptr->visit(this);}
void Declaration::receive(Visitor* ptr) {ptr->visit(this);}
void If::receive(Visitor* ptr) {ptr->visit(this);}
void InfiniteLoop::receive(Visitor* ptr) {ptr->visit(this);}
void PreTestLoop::receive(Visitor* ptr) {ptr->visit(this);}
void Break::receive(Visitor* ptr) {ptr->visit(this);}
void Cycle::receive(Visitor* ptr) {ptr->visit(this);}
void Ret::receive(Visitor* ptr) {ptr->visit(this);}
void ExpressionStatement::receive(Visitor* ptr) {ptr->visit(this);}
void Expression::receive(Visitor* ptr) {ptr->visit(this);}
void Assignment::receive(Visitor* ptr) {ptr->visit(this);}
void Call::receive(Visitor* ptr) {ptr->visit(this);}
void Identifier::receive(Visitor* ptr) {ptr->visit(this);}
void Literal::receive(Visitor* ptr) {ptr->visit(this);}
void Real::receive(Visitor* ptr) {ptr->visit(this);}
void Integer::receive(Visitor* ptr) {ptr->visit(this);}
void Boolean::receive(Visitor* ptr) {ptr->visit(this);}
void String::receive(Visitor* ptr) {ptr->visit(this);}
void Character::receive(Visitor* ptr) {ptr->visit(this);}

namespace {

class Printer final : public ast::Visitor {
public:
  explicit Printer(std::ostream& stream)
  : m_stream{stream}//,
  //  m_indent_level{0}
  {}
private:
  std::ostream& m_stream;
  //unsigned long long m_indent_level;
  void visit(Program*) override;
  void visit(Class*) override;
  void visit(Method*) override;
  void visit(Field*) override;
  void visit(Declaration*) override;
  void visit(If*) override;
  void visit(InfiniteLoop*) override;
  void visit(PreTestLoop*) override;
  void visit(Break*) override;
  void visit(Cycle*) override;
  void visit(Ret*) override;
  void visit(ExpressionStatement*) override;
  void visit(Assignment*) override;
  void visit(Call*) override;
  void visit(Identifier*) override;
  void visit(Real*) override;
  void visit(Integer*) override;
  void visit(Boolean*) override;
  void visit(String*) override;
  void visit(Character*) override;
};

void Printer::visit(Program* program_ptr)
{
  for (auto& global : program_ptr->globals)
    dispatch(global.get(), this);
}

void Printer::visit(Class* class_ptr)
{
  m_stream << "class " << class_ptr->name << '\n';
  for (auto& global : class_ptr->globals)
    dispatch(global.get(), this);
  m_stream << "end class " << class_ptr->name << '\n';
}

void Printer::visit(Method* method_ptr)
{
  m_stream << "method " << method_ptr->name << '(';
  auto iter = method_ptr->arguments.begin();
  if (iter != method_ptr->arguments.end()) {
    m_stream << iter->first << " : " << *iter->second;
    ++iter;
    while (iter != method_ptr->arguments.end()) {
      m_stream << ", " << iter->first << " : " << *iter->second;
      ++iter;
    }
  }
  m_stream << ") : " << *method_ptr->return_type << '\n';
  //++m_indent_level;
  for (auto& statement : method_ptr->statements)
    dispatch(statement.get(), this);
  //--m_indent_level;
  m_stream << "end\n";
}

void Printer::visit(Field* field_ptr)
{
  m_stream << field_ptr->name << " : ";
  dispatch(field_ptr->type.get(), this);
  m_stream << '\n';
}

void Printer::visit(Declaration* declaration_ptr)
{
  m_stream << declaration_ptr->name << " : ";
  dispatch(declaration_ptr->type.get(), this);
  m_stream << '\n';
}

void Printer::visit(If* if_ptr)
{
  m_stream << "if ";
  dispatch(if_ptr->condition.get(), this);
  m_stream << '\n';
  //++m_indent_level;
  for (auto& statement : if_ptr->if_body)
    dispatch(statement.get(), this);
  for (auto& elif_body : if_ptr->elif_bodies) {
    m_stream << "elif ";
    dispatch(elif_body.first.get(), this);
    m_stream << '\n';
    for (auto& statement : elif_body.second)
      dispatch(statement.get(), this);
  }
  if (if_ptr->else_body.size()) {
    m_stream << "else\n";
    for (auto& statement : if_ptr->else_body)
      dispatch(statement.get(), this);
    m_stream << '\n';
  }
  m_stream << "end\n";
}

void Printer::visit(InfiniteLoop* infinite_loop_ptr)
{
  m_stream << "do\n";
  for (auto& statement : infinite_loop_ptr->body)
    dispatch(statement.get(), this);
  m_stream << "end\n";
}

void Printer::visit(PreTestLoop* pre_test_loop_ptr)
{
  m_stream << "for ";
  dispatch(pre_test_loop_ptr->condition.get(), this);
  m_stream << '\n';
  for (auto& statement : pre_test_loop_ptr->body)
    dispatch(statement.get(), this);
  if (pre_test_loop_ptr->else_body.size()) {
    m_stream << "else\n";
    for (auto& statement : pre_test_loop_ptr->else_body)
      dispatch(statement.get(), this);
  }
  m_stream << "end\n";
}

void Printer::visit(Break*)
{
  m_stream << "break\n";
}

void Printer::visit(Cycle*)
{
  m_stream << "cycle\n";
}

void Printer::visit(Ret* ret_ptr)
{
  m_stream << "ret";
  if (ret_ptr->expression) {
    m_stream << ' ';
    dispatch(ret_ptr->expression.get(), this);
  }
  m_stream << '\n';
}

void Printer::visit(ExpressionStatement* expression_statement_ptr)
{
  dispatch(expression_statement_ptr->expression.get(), this);
  m_stream << '\n';
}

void Printer::visit(Assignment* assignment_ptr)
{
  dispatch(assignment_ptr->left.get(), this);
  m_stream << " = ";
  dispatch(assignment_ptr->right.get(), this);
  m_stream << '\n';
}

void Printer::visit(Call* call_ptr)
{
  dispatch(call_ptr->expression.get(), this);
  m_stream << '.' << call_ptr->name << '(';
  auto iter = call_ptr->arguments.begin();
  if (iter != call_ptr->arguments.end()) {
    dispatch((*iter).get(), this);
    ++iter;
    while (iter != call_ptr->arguments.end()) {
      m_stream << ", ";
      dispatch((*iter).get(), this);
      ++iter;
    }
  }
  m_stream << ')';
}

void Printer::visit(Identifier* identifier_ptr)
{
  m_stream << identifier_ptr->value;
}

void Printer::visit(Real* real_ptr)
{
  m_stream << real_ptr->value;
}

void Printer::visit(Integer* integer_ptr)
{
  m_stream << integer_ptr->value;
}

void Printer::visit(Boolean* boolean_ptr)
{
  m_stream << (boolean_ptr->value ? "true" : "false");
}

void Printer::visit(String* string_ptr)
{
  m_stream << '"' << string_ptr->value << '"';
}

void Printer::visit(Character* character_ptr)
{
  m_stream << "'\\U+" << character_ptr->value << '\'';

}

}

std::ostream& operator<< (std::ostream& stream, Node& node)
{
  Printer printer{stream};
  dispatch(&node, &printer);
  return stream;
}
