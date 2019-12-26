#pragma once

#include <utility>
#include "program/func.h"

namespace zvm {

  enum class ExitKind {
    Normal,
    Break,
    Return,
    Throw,
    Yield,
  };

  template<typename Traits>
  struct InterpreterFrame {
    const Func& func;
    const Block* current_block;
    Block::const_iterator current_statement;
    std::vector<std::pair<const Block*, Block::const_iterator>> stack;
    std::vector<RegisterValue> registers;
    Register return_register = 0;
    // TODO: Error slot

    InterpreterFrame(const Func& func) :
      func {func},
      current_block {&func.block},
      current_statement {func.block.begin()}
    {
      this->registers.resize(func.registers.size());
    }

    void set_reg(Register target, RegisterValue value) {
      this->registers[target] = value;
    }

    RegisterValue get_reg(Register source) {
      return this->registers[source];
    }

    void push_block(const Block& block) {
      this->stack.push_back({
        this->current_block,
        this->current_statement,
      });

      this->current_block = &block;
      this->current_statement = block.begin();
    }

    bool ensure_next_statement() {
      while (this->current_statement == this->current_block->end()) {
        if (this->stack.empty())
          return false;

        auto& top = this->stack.back();
        this->current_block = top.first;
        this->current_statement = top.second;
        this->stack.pop_back();
      }

      return true;
    }

    ExitKind execute_statement(const LoadStatement& stmt) {
      std::cout << "Load\n";
      this->set_reg(stmt.target, stmt.value);
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const CallStatement& stmt) {
      std::cout << "Call\n";
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const IfStatement& stmt) {
      std::cout << "If\n";
      this->push_block(this->get_reg(stmt.source) == 0
        ? stmt.false_block
        : stmt.true_block);
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const RepeatStatement& stmt) {
      std::cout << "Repeat\n";
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const BreakStatement& stmt) {
      std::cout << "Break\n";
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const TryStatement& stmt) {
      std::cout << "Try\n";
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const FinallyStatement& stmt) {
      std::cout << "Finally\n";
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const ReturnStatement& stmt) {
      std::cout << "Return\n";
      this->return_register = stmt.source;
      return ExitKind::Return;
    }

    ExitKind execute_statement(const YieldStatement& stmt) {
      std::cout << "Yield\n";
      return ExitKind::Normal;
    }

    ExitKind execute_statement(const ThrowStatement& stmt) {
      std::cout << "Throw\n";
      return ExitKind::Normal;
    }

    ExitKind execute() {
      ExitKind exit = ExitKind::Normal;

      while (true) {
        auto& stmt = **(this->current_statement++);

        using Kind = StatementKind;
        switch (stmt.kind) {
          case Kind::Load:
            exit = this->execute_statement(cast_statement<LoadStatement>(stmt));
            break;
          case Kind::Call:
            exit = this->execute_statement(cast_statement<CallStatement>(stmt));
            break;
          case Kind::If:
            exit = this->execute_statement(cast_statement<IfStatement>(stmt));
            break;
          case Kind::Repeat:
            exit = this->execute_statement(cast_statement<RepeatStatement>(stmt));
            break;
          case Kind::Break:
            exit = this->execute_statement(cast_statement<BreakStatement>(stmt));
            break;
          case Kind::Try:
            exit = this->execute_statement(cast_statement<TryStatement>(stmt));
            break;
          case Kind::Finally:
            exit = this->execute_statement(cast_statement<FinallyStatement>(stmt));
            break;
          case Kind::Return:
            exit = this->execute_statement(cast_statement<ReturnStatement>(stmt));
            break;
          case Kind::Yield:
            exit = this->execute_statement(cast_statement<YieldStatement>(stmt));
            break;
          case Kind::Throw:
            exit = this->execute_statement(cast_statement<ThrowStatement>(stmt));
            break;
          default:
            // TODO
            break;
        }

        if (exit != ExitKind::Normal)
          return exit;

        if (!this->ensure_next_statement())
          break;
      }

      return ExitKind::Return;
    }

  };

}
