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

#include "tree_transform.hxx"

void ElifRemover::visit(ast::Class& ast_class)
{
  for (auto& ast_statement : ast_class.statements)
    ast::dispatch(*this, *ast_statement);
}

void ElifRemover::visit(ast::Method& ast_method)
{
  for (auto& ast_statement : ast_method.statements)
    ast::dispatch(*this, *ast_statement);
}

void ElifRemover::visit(ast::If& ast_if)
{
  // visit else body
  for (auto& ast_statement : ast_if.else_body)
    ast::dispatch(*this, *ast_statement);

  // while there are elif statements
  while (ast_if.elif_bodies.size()) {
    // create a new if statement
    auto new_ast_if = std::make_unique<ast::If>();
    // visit the last elif body
    for (auto& ast_statement : ast_if.elif_bodies.back().second)
      ast::dispatch(*this, *ast_statement);
    // set the new if condition and body equal to the last elif statement and
    // then delete that last elif statement
    new_ast_if->condition = std::move(ast_if.elif_bodies.back().first);
    new_ast_if->if_body = std::move(ast_if.elif_bodies.back().second);
    ast_if.elif_bodies.pop_back();
    // move the else body of 'ast_if' to 'new_ast_if'
    std::swap(new_ast_if->else_body, ast_if.else_body);
    // add 'new_ast_if' to the else body of 'ast_if'
    ast_if.else_body.push_back(std::move(new_ast_if));
  }
  // visit if body
  for (auto& ast_statement : ast_if.if_body)
    ast::dispatch(*this, *ast_statement);
}

void ElifRemover::visit(ast::InfiniteLoop& ast_infinite_loop) {
  for (auto& ast_statement : ast_infinite_loop.body)
    ast::dispatch(*this, *ast_statement);
}

void ElifRemover::visit(ast::PreTestLoop& ast_pre_test_loop)
{
  for (auto& ast_statement : ast_pre_test_loop.body)
    ast::dispatch(*this, *ast_statement);
  for (auto& ast_statement : ast_pre_test_loop.else_body)
    ast::dispatch(*this, *ast_statement);
}

void ElifRemover::visit(ast::Break&) {}
void ElifRemover::visit(ast::Cycle&) {}
void ElifRemover::visit(ast::Ret&) {}
void ElifRemover::visit(ast::Declaration&) {}
void ElifRemover::visit(ast::ExpressionStatement&) {}
