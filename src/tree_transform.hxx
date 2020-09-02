// Copyright (C) 2020  Claire Hansel
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

#ifndef BUCKET_TREE_TRANSFORM_HXX
#define BUCKET_TREE_TRANSFORM_HXX

#include "abstract_syntax_tree.hxx"

class ElifRemover final : public ast::Visitor {
private:
  void visit(ast::Class&) override;
  void visit(ast::Method&) override;
  void visit(ast::If&) override;
  void visit(ast::InfiniteLoop&) override;
  void visit(ast::PreTestLoop&) override;
  void visit(ast::Break&) override;
  void visit(ast::Cycle&) override;
  void visit(ast::Ret&) override;
  void visit(ast::Declaration&) override;
  void visit(ast::ExpressionStatement&) override;
};

#endif
