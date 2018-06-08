#include "ast.hxx"
#include <iostream>

namespace ast {

namespace {

class Printer final : public Visitor {
private:
  void visit(Class* ptr)               override;
  void visit(Method* ptr)              override;
  void visit(Field* ptr)               override;
  void visit(If* ptr)                  override;
  void visit(Loop* ptr)                override;
  void visit(Break* ptr)               override;
  void visit(Cycle* ptr)               override;
  void visit(Ret* ptr)                 override;
  void visit(ExpressionStatement* ptr) override;
  void visit(Assignment* ptr)          override;
  void visit(Call* ptr)                override;
  void visit(Identifier* ptr)          override;
  void visit(Integer* ptr)             override;
  void visit(Bool* ptr)                override;
  void visit(String* ptr)              override;
};

}

void dump(Node* node) {
  Printer printer;
  node->receive(&printer);
}

void Printer::visit(Class* ptr) {
  std::cerr << "class " << ptr->name << '\n';
  for (auto& global_statement : ptr->body)
    global_statement->receive(this);
  std::cerr << "end" << '\n';
}

void Printer::visit(Method* ptr) {
  std::cerr << "method " << ptr->name << '(';
  auto iter = ptr->args.begin();
  if (iter != ptr->args.end()) {
    std::cerr << iter->first << " : ";
    iter->second->receive(this);
    ++iter;
    while (iter != ptr->args.end()) {
      std::cerr << ", ";
      std::cerr << iter->first << " : ";
      iter->second->receive(this);
      ++iter;
    }
  }
  std::cerr << ')';
  if (ptr->return_class) {
    std::cerr << " : ";
    ptr->return_class->receive(this);
  }
  std::cerr << '\n';
  for (auto& statement : ptr->body)
    statement->receive(this);
  std::cerr << "end\n";
}

void Printer::visit(Field* ptr) {
  std::cerr << ptr->name << " : ";
  ptr->cls->receive(this);
  std::cerr << '\n';
}

void Printer::visit(If* ptr) {
  std::cerr << "if ";
  ptr->condition->receive(this);
  std::cerr << '\n';
  for (auto& statement : ptr->if_body)
    statement->receive(this);
  for (auto& elif_body : ptr->elif_bodies) {
    std::cerr << "elif ";
    elif_body.first->receive(this);
    std::cerr << '\n';
    for (auto& statement : elif_body.second)
      statement->receive(this);
  }
  for (auto& statement : ptr->else_body)
    statement->receive(this);
  std::cerr << "end\n";
}

void Printer::visit(Loop* ptr) {
  std::cerr << "do\n";
  for (auto& statement : ptr->body)
    statement->receive(this);
  std::cerr << "end\n";
}

void Printer::visit(Break*) {
  std::cerr << "break\n";
}

void Printer::visit(Cycle*) {
  std::cerr << "cycle\n";
}

void Printer::visit(Ret* ptr) {
  std::cerr << "ret";
  if (ptr->value) {
    std::cerr << ' ';
    ptr->value->receive(this);
  }
  std::cerr << '\n';
}

void Printer::visit(ExpressionStatement* ptr) {
  ptr->value->receive(this);
  std::cerr << '\n';
}

void Printer::visit(Assignment* ptr) {
  ptr->left->receive(this);
  std::cerr << " = ";
  ptr->right->receive(this);
  std::cerr << '\n';
}

void Printer::visit(Call* ptr) {
  ptr->object->receive(this);
  std::cerr << '.' << ptr->name << '(';
  auto iter = ptr->args.begin();
  if (iter != ptr->args.end()) {
    (*iter)->receive(this);
    ++iter;
    while (iter != ptr->args.end()) {
      std::cerr << ", ";
      (*iter)->receive(this);
      ++iter;
    }
  }
  std::cerr << ")\n";
}

void Printer::visit(Identifier* ptr) {
  std::cerr << ptr->value;
}

void Printer::visit(Integer* ptr) {
  std::cerr << ptr->value;
}

void Printer::visit(Bool* ptr) {
  std::cerr << (ptr->value ? "true" : "false");
}

void Printer::visit(String* ptr) {
  std::cerr << '"' << ptr->value << '"';
}

}
