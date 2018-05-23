#ifndef BUCKET_AST_HXX
#define BUCKET_AST_HXX

#include <boost/core/noncopyable.hpp>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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

class Visitor {
public:
  virtual ~Visitor() = default;
  virtual void visit(Node*)                {assert(false);}
  virtual void visit(Global*)              {assert(false);}
  virtual void visit(Class*)               {assert(false);}
  virtual void visit(Method*)              {assert(false);}
  virtual void visit(Field*)               {assert(false);}
  virtual void visit(Statement*)           {assert(false);}
  virtual void visit(If*)                  {assert(false);}
  virtual void visit(Loop*)                {assert(false);}
  virtual void visit(Break*)               {assert(false);}
  virtual void visit(Cycle*)               {assert(false);}
  virtual void visit(Ret*)                 {assert(false);}
  virtual void visit(ExpressionStatement*) {assert(false);}
  virtual void visit(Expression*)          {assert(false);}
  virtual void visit(Assignment*)          {assert(false);}
  virtual void visit(Call*)                {assert(false);}
  virtual void visit(Identifier*)          {assert(false);}
  virtual void visit(Integer*)             {assert(false);}
  virtual void visit(Bool*)                {assert(false);}
};

struct Node : private boost::noncopyable {
  virtual ~Node() = default;
  virtual void receive(Visitor* visitor) {visitor->visit(this);}
};

struct Global : Node {
  std::string name;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Class final : Global {
  std::vector<std::unique_ptr<Global>> body;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Method final : Global {
  std::vector<std::pair<std::string, std::unique_ptr<Expression>>> args;
  std::unique_ptr<Expression> return_class;
  std::vector<std::unique_ptr<Statement>> body;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Field final : Global {
  std::unique_ptr<Expression> cls;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Statement : Node {
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct If final : Statement {
  std::unique_ptr<Expression> condition;
  std::vector<std::unique_ptr<Statement>> if_body;
  std::vector<std::pair<std::unique_ptr<Expression>, std::vector<std::unique_ptr<Statement>>>> elif_bodies;
  std::vector<std::unique_ptr<Statement>> else_body;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Loop final : Statement {
  std::vector<std::unique_ptr<Statement>> body;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Break final : Statement {
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Cycle final : Statement {
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Ret final : Statement {
  std::unique_ptr<Expression> value;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct ExpressionStatement final : Statement {
  std::unique_ptr<Expression> value;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Expression : Node {
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Assignment final : Expression {
  std::unique_ptr<Expression> left, right;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Call final : Expression {
  std::unique_ptr<Expression> object;
  std::string name;
  std::vector<std::unique_ptr<Expression>> args;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Identifier final : Expression {
  std::string value;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Integer final : Expression {
  std::int64_t value;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

struct Bool final : Expression {
  bool value;
  void receive(Visitor* visitor) override {visitor->visit(this);}
};

namespace details {

template <typename To>
class Caster final : public Visitor {
public:
  To* result;
private:
  void visit(Node* ptr)                override {cast(ptr);}
  void visit(Global* ptr)              override {cast(ptr);}
  void visit(Class* ptr)               override {cast(ptr);}
  void visit(Method* ptr)              override {cast(ptr);}
  void visit(Field* ptr)               override {cast(ptr);}
  void visit(Statement* ptr)           override {cast(ptr);}
  void visit(If* ptr)                  override {cast(ptr);}
  void visit(Loop* ptr)                override {cast(ptr);}
  void visit(Break* ptr)               override {cast(ptr);}
  void visit(Cycle* ptr)               override {cast(ptr);}
  void visit(Ret* ptr)                 override {cast(ptr);}
  void visit(ExpressionStatement* ptr) override {cast(ptr);}
  void visit(Expression* ptr)          override {cast(ptr);}
  void visit(Assignment* ptr)          override {cast(ptr);}
  void visit(Call* ptr)                override {cast(ptr);}
  void visit(Identifier* ptr)          override {cast(ptr);}
  void visit(Integer* ptr)             override {cast(ptr);}
  void visit(Bool* ptr)                override {cast(ptr);}
  template <typename From>
  void cast([[maybe_unused]] From* ptr) {
    if constexpr (std::is_base_of_v<To, From>)
      result = static_cast<To*>(ptr);
    else
      result = nullptr;
  }
};

}

template <typename ToPtr, typename FromPtr>
ToPtr ast_cast(FromPtr ptr)
{
  if (ptr) {
    static_assert(std::is_pointer_v<ToPtr>);
    static_assert(std::is_pointer_v<FromPtr>);
    using To   = std::remove_pointer_t<ToPtr>;
    using From = std::remove_pointer_t<FromPtr>;
    static_assert(std::is_base_of_v<Node, To>);
    static_assert(std::is_base_of_v<Node, From>);
    details::Caster<To> caster;
    const_cast<std::add_pointer_t<std::remove_const_t<From>>>(ptr)->receive(&caster);
    assert(caster.result == dynamic_cast<ToPtr>(ptr));
    return caster.result;
  }
  return nullptr;
}

template <typename ToPtr, typename FromPtr>
ToPtr checked_ast_cast(FromPtr ptr)
{
  static_assert(std::is_pointer_v<ToPtr>);
  static_assert(std::is_pointer_v<FromPtr>);
  using To   = std::remove_pointer_t<ToPtr>;
  using From = std::remove_pointer_t<FromPtr>;
  static_assert(std::is_base_of_v<Node, To>);
  static_assert(std::is_base_of_v<Node, From>);
  assert(dynamic_cast<ToPtr>(ptr));
  return static_cast<ToPtr>(ptr);
}

void dump(Node* node);

}

#endif
