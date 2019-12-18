#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

namespace zvm {

  enum class StatementKind {
    Load,
    Call,
    Invoke,
    If,
    Repeat,
    Break,
    Try,
    Finally,
    Return,
    Yield,
    Throw,
  };

  struct Statement {
    const StatementKind kind;
    explicit Statement(StatementKind kind) : kind(kind) {}
    virtual ~Statement() {}
  };

  template<StatementKind kind_value>
  struct TypedStatement : public Statement {
    TypedStatement() : Statement(kind_value) {}
    static constexpr StatementKind instance_kind = kind_value;
  };

  template<typename T>
  bool is_statement_type(const Statement& stmt) {
    // TODO: static assert T is subclass of Statement
    return stmt.kind == T::instance_kind;
  }

  template<typename T>
  T* as_statement_type(Statement& stmt) {
    return is_statement_type<T>(stmt)
      ? reinterpret_cast<T*>(&stmt)
      : nullptr;
  }

  template<typename T>
  const T* as_statement_type(const Statement& stmt) {
    return is_statement_type<T>(stmt)
      ? reinterpret_cast<const T*>(&stmt)
      : nullptr;
  }

  using Register = uint16_t;
  using RegisterType = uint32_t;
  using RegisterValue = uint64_t;

  constexpr Register max_register() {
    return ~0;
  }

  namespace RegisterTypes {
    enum FundamentalTypes : RegisterType {
      Void = 0,
      Bool,
      Int8,
      Int16,
      Int32,
      Int64,
      UInt8,
      UInt16,
      UInt32,
      UInt64,
      Float32,
      Float64,

      LastFundamentalType = Float64,
      FirstInterfaceType = 0x100,
    };
  }

  using FuncName = uint16_t;
  using InterfaceName = uint16_t;
  using Block = std::vector<Statement*>;

  struct LoadStatement : public TypedStatement<StatementKind::Load> {
    Register target;
    RegisterValue value;

    LoadStatement(Register target, Register value) :
      target {target},
      value {value} {}
  };

  struct CallStatement : public TypedStatement<StatementKind::Call> {
    Register target;
    InterfaceName interface_name;
    FuncName func_name;
    std::vector<Register> args;

    CallStatement(
      Register target,
      InterfaceName interface_name,
      FuncName func_name,
      std::vector<Register>&& args = {}) :
        target {target},
        interface_name {interface_name},
        func_name {func_name},
        args {std::move(args)} {}
  };

  struct InvokeStatement : public TypedStatement<StatementKind::Invoke> {
    Register target;
    Register interface;
    FuncName func_name;
    std::vector<Register> args;

    InvokeStatement(
      Register target,
      Register interface,
      FuncName func_name,
      std::vector<Register>&& args = {}) :
        target {target},
        interface {interface},
        func_name {func_name},
        args {std::move(args)} {}
  };

  struct TryStatement : public TypedStatement<StatementKind::Try> {
    Register target;
    Block try_block;
    Block catch_block;

    TryStatement(
      Register target,
      Block&& try_block = {},
      Block&& catch_block = {}) :
        target {target},
        try_block {std::move(try_block)},
        catch_block {std::move(catch_block)} {}
  };

  struct FinallyStatement : public TypedStatement<StatementKind::Finally> {
    Block block;
    Block finally_block;

    FinallyStatement(Block&& block = {}, Block&& finally_block = {}) :
      block {std::move(block)},
      finally_block {std::move(finally_block)} {}
  };

  struct IfStatement : public TypedStatement<StatementKind::If> {
    Register source;
    Block true_block;
    Block false_block;

    IfStatement(
      Register source,
      Block&& true_block = {},
      Block&& false_block = {}) :
        source {source},
        true_block {std::move(true_block)},
        false_block {std::move(false_block)} {}
  };

  struct RepeatStatement : public TypedStatement<StatementKind::Repeat> {
    Block block;

    explicit RepeatStatement(Block&& block = {}) :
      block {std::move(block)} {}
  };

  struct BreakStatement : public TypedStatement<StatementKind::Break> {
    BreakStatement() {}
  };

  struct ReturnStatement : public TypedStatement<StatementKind::Return> {
    Register source;

    explicit ReturnStatement(Register source) : source {source} {}
  };

  struct ThrowStatement : public TypedStatement<StatementKind::Throw> {
    Register source;

    explicit ThrowStatement(Register source) : source {source} {}
  };

  struct YieldStatement : public TypedStatement<StatementKind::Yield> {
    Register source;

    explicit YieldStatement(Register source) : source {source} {}
  };

  struct Func {
    Register arg_count = 0;
    std::vector<RegisterType> registers;
    RegisterType return_type = RegisterTypes::Void;
    Block block;
  };

  struct Interface {
    // TODO: Add a method for doing lookups. Eventually this should
    // support caching. How do we model objects? I suppose they are
    // a binding
    // TODO: This should be a pointer
    std::unordered_map<FuncName, const Func&> func_map;
  };

  // TODO: This should map to pointers or const refs
  using InterfaceTypeTable = std::unordered_map<RegisterType, const Interface&>;
  using InterfaceNameTable = std::unordered_map<InterfaceName, const Interface&>;

}
