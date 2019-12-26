#pragma once

#include "func.h"

namespace zvm {

  namespace {

    template<typename T>
    struct TraversalMapper {
      T& visitor;

      explicit TraversalMapper(T& visitor) : visitor {visitor} {}

      template<typename S, typename F>
      void visit(S& stmt, F fn) {
        this->visitor.enter_statement(stmt);
        fn();
        this->visitor.leave_statement(stmt);
      }

      void visit_block(const Block& block) {
        traverse_block(block, this->visitor);
      }

      template<typename S>
      void operator()(const S& stmt) {
        this->visit(stmt, []() {});
      }

      template<>
      void operator()(const IfStatement& stmt) {
        this->visit(stmt, [&]() {
          this->visit_block(stmt.true_block);
          this->visit_block(stmt.false_block);
        });
      }

      template<>
      void operator()(const RepeatStatement& stmt) {
        this->visit(stmt, [&]() {
          this->visit_block(stmt.block);
        });
      }

      template<>
      void operator()(const TryStatement& stmt) {
        this->visit(stmt, [&]() {
          this->visit_block(stmt.try_block);
          this->visit_block(stmt.catch_block);
        });
      }

      template<>
      void operator()(const FinallyStatement& stmt) {
        this->visit(stmt, [&]() {
          this->visit_block(stmt.block);
          this->visit_block(stmt.finally_block);
        });
      }

    };

  }

  template<typename S, typename F>
  auto map_statement(S& stmt, F& fn) {
    using Kind = StatementKind;
    switch (stmt.kind) {
      case Kind::Load:
        return fn(cast_statement<LoadStatement>(stmt));
      case Kind::Call:
        return fn(cast_statement<CallStatement>(stmt));
      case Kind::If:
        return fn(cast_statement<IfStatement>(stmt));
      case Kind::Repeat:
        return fn(cast_statement<RepeatStatement>(stmt));
      case Kind::Break:
        return fn(cast_statement<BreakStatement>(stmt));
      case Kind::Try:
        return fn(cast_statement<TryStatement>(stmt));
      case Kind::Finally:
        return fn(cast_statement<FinallyStatement>(stmt));
      case Kind::Return:
        return fn(cast_statement<ReturnStatement>(stmt));
      case Kind::Yield:
        return fn(cast_statement<YieldStatement>(stmt));
      case Kind::Throw:
        return fn(cast_statement<ThrowStatement>(stmt));
    }
  }

  template<typename V>
  void traverse_block(const Block& block, V& visitor) {
    TraversalMapper<V> mapper {visitor};
    for (auto& stmt : block) {
      map_statement(*stmt, mapper);
    }
  }

}
