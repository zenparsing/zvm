#include "validator.h"
#include "traverse.h"

namespace zvm {

  namespace {

    struct TypeChecker {
      const InterfaceTypeTable& interface_types;

      explicit TypeChecker(const InterfaceTypeTable& interface_types) :
        interface_types {interface_types} {}

      bool can_assign_to(RegisterType source, RegisterType target);
      bool can_assign_to(const Interface& source, const Interface& target);
      bool can_assign_to(const Func& source, const Func& target);
    };

    bool TypeChecker::can_assign_to(
      RegisterType source,
      RegisterType target)
    {
      if (source == target)
        return true;

      if (source <= RegisterTypes::FirstInterfaceType)
        return false;

      if (target <= RegisterTypes::FirstInterfaceType)
        return false;

      auto iter_source = this->interface_types.find(source);
      if (iter_source == this->interface_types.end()) {
        // TODO: assert
        return false;
      }

      auto iter_target = this->interface_types.find(target);
      if (iter_target == this->interface_types.end()) {
        // TODO: assert
        return false;
      }

      return this->can_assign_to(*iter_source->second, *iter_target->second);
    }

    bool TypeChecker::can_assign_to(
      const Interface& source,
      const Interface& target)
    {
      if (target.func_map.size() > source.func_map.size())
        return false;

      for (auto& pair : target.func_map) {
        const FuncName name = pair.first;
        const Func& target_func = *pair.second;
        auto iter = source.func_map.find(name);
        if (iter == source.func_map.end()) {
          return false;
        }
        const Func& source_func = *iter->second;
        if (!this->can_assign_to(source_func, target_func)) {
          return false;
        }
      }

      return true;
    }

    bool TypeChecker::can_assign_to(
      const Func& source,
      const Func& target)
    {
      if (target.arg_count != source.arg_count)
        return false;

      if (!this->can_assign_to(source.return_type, target.return_type))
        return false;

      for (Register reg = 0; reg < target.arg_count; ++reg) {
        RegisterType target_arg = source.registers[reg];
        RegisterType source_arg = target.registers[reg];
        if (!this->can_assign_to(source_arg, target_arg))
          return false;
      }

      return true;
    }

    namespace Error {
      const char TooManyRegisters[] = "too many registers";
      const char MoreArgsThanRegisters[] = "more arguments than registers";
      const char NonScalarRegisterLoad[] = "cannot load a value into a non-scalar register";
      const char InterfaceTypeNotFound[] = "interface type does not exist";
      const char RegisterNotFound[] = "register not found";
      const char ReturnTypeMismatch[] = "register type does not match return type";
      const char BreakOutsideOfRepeat[] = "break outside of repeat";
      const char GlobalFuncNotFound[] = "global func not found";
      const char InterfaceFuncNotFound[] = "interface func not found";
      const char WrongArgumentCount[] = "wrong number of arguments";
      const char NonMatchingCall[] = "call does not match target";
    }

    struct Validator {
      const Func& func;
      const Interface& global;
      const InterfaceTypeTable& interface_types;
      TypeChecker type_checker;
      bool is_valid = true;
      bool in_repeat = false;

      Validator(
        const Func& func,
        const Interface& global,
        const InterfaceTypeTable& interface_types) :
          func {func},
          global {global},
          interface_types {interface_types},
          type_checker {interface_types} {}

      // ## TODO
      // - All paths must return matching types
      // - All registers should be assigned before used
      // - No truncation allowed for Load
      // - There will need to be some built-in error interface
      //   that we can type-check against
      // - Include diagnostics (failure messages and locations)
      // - Ability to throw on error
      // - Handle void registers

      void fail(const char* error = nullptr) {
        this->is_valid = false;
      }

      RegisterType reg_type(Register reg) {
        if (reg >= this->func.registers.size()) {
          this->fail(Error::RegisterNotFound);
          return RegisterTypes::Void;
        }
        return this->func.registers[reg];
      }

      bool validate() {
        if (this->func.arg_count > this->func.registers.size())
          this->fail(Error::MoreArgsThanRegisters);

        if (this->func.registers.size() > max_register())
          this->fail(Error::TooManyRegisters);

        traverse_block(this->func.block, *this);
        return this->is_valid;
      }

      void validate_return_reg(Register reg) {
        bool can_assign = this->type_checker.can_assign_to(
          this->reg_type(reg),
          this->func.return_type);

        if (!can_assign)
          this->fail(Error::ReturnTypeMismatch);
      }

      void validate_error_reg(Register reg) {
        auto type = this->reg_type(reg);
        // TODO
      }

      void validate_scalar_reg(Register reg) {
        if (
          auto t = this->reg_type(reg);
          t == RegisterTypes::Void || t > RegisterTypes::LastFundamentalType)
        {
          this->fail(Error::NonScalarRegisterLoad);
        }
      }

      void validate_call(
        const Func& func,
        Register target,
        const std::vector<Register> args);

      template<typename S>
      void leave_statement(const S& stmt) {}

      void enter_statement(const LoadStatement& stmt) {
        this->validate_scalar_reg(stmt.target);
      }

      void enter_statement(const CallStatement& stmt) {
        if (stmt.interface == void_register()) {
          auto iter = this->global.func_map.find(stmt.func_name);
          if (iter == this->global.func_map.end())
            return this->fail(Error::GlobalFuncNotFound);

          return this->validate_call(*iter->second, stmt.target, stmt.args);
        }

        auto iter = this->interface_types.find(this->reg_type(stmt.interface));
        if (iter == this->interface_types.end())
          return this->fail(Error::InterfaceTypeNotFound);

        const Interface& interface = *iter->second;

        auto func_iter = interface.func_map.find(stmt.func_name);
        if (func_iter == interface.func_map.end())
          return this->fail(Error::InterfaceFuncNotFound);

        this->validate_call(*func_iter->second, stmt.target, stmt.args);
      }

      void enter_statement(const IfStatement& stmt) {
        this->validate_scalar_reg(stmt.source);
      }

      void enter_statement(const RepeatStatement& stmt) {
        this->in_repeat = true;
      }

      template<>
      void leave_statement(const RepeatStatement& stmt) {
        this->in_repeat = false;
      }

      void enter_statement(const BreakStatement& stmt) {
        if (!this->in_repeat)
          this->fail(Error::BreakOutsideOfRepeat);
      }

      void enter_statement(const TryStatement& stmt) {
        this->validate_error_reg(stmt.target);
      }

      void enter_statement(const FinallyStatement& stmt) {}

      void enter_statement(const ReturnStatement& stmt) {
        this->validate_return_reg(stmt.source);
      }

      void enter_statement(const YieldStatement& stmt) {
        this->validate_return_reg(stmt.source);
      }

      void enter_statement(const ThrowStatement& stmt) {
        this->validate_error_reg(stmt.source);
      }
    };

    void Validator::validate_call(
      const Func& func,
      Register target,
      const std::vector<Register> args)
    {
      if (args.size() != func.arg_count)
        return this->fail(Error::WrongArgumentCount);

      std::vector<RegisterType> arg_types;
      arg_types.reserve(args.size());
      for (auto& reg : args) {
        arg_types.push_back(this->reg_type(reg));
      }

      Func expected;
      expected.arg_count = static_cast<Register>(args.size());
      expected.registers = std::move(arg_types);
      expected.return_type = this->reg_type(target);

      if (!this->type_checker.can_assign_to(func, expected)) {
        this->fail(Error::NonMatchingCall);
      }
    }

  }

  bool validate_func(
    Func& func,
    const Interface& global,
    const InterfaceTypeTable& interface_types,
    ValidationToken validation_token)
  {
    if (validation_token && func.validation_token == validation_token) {
      return true;
    }

    Validator validator {func, global, interface_types};
    if (validator.validate()) {
      func.validation_token = validation_token;
      return true;
    }

    return false;
  }

}
