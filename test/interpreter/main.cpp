#include <string>
#include <iostream>

#include <unordered_set>

#include "program/func.h"
#include "interpreter/interpreter.h"

using namespace zvm;

template<typename T>
struct Allocator {
  std::unordered_set<T*> pointers;

  Allocator() {}

  Allocator(const Allocator& other) = delete;
  Allocator& operator=(const Allocator& other) = delete;

  ~Allocator() {
    for (T* ptr : pointers) {
      delete ptr;
    }
  }

  template<typename U, typename ...Args>
  U* create(Args ...args) {
    auto* ptr = new U(std::forward<Args>(args)...);
    pointers.insert(ptr);
    return ptr;
  }

  template<typename U>
  U* insert(U* ptr) {
    pointers.insert(ptr);
    return ptr;
  }

  void destroy(T* ptr) {
    if (auto erased = pointers.erase(ptr); erased > 0) {
      delete ptr;
    }
  }
};

struct InterpreterTraits {};

int main() {
  Allocator<Statement> allocator;
  Func func;

  func.arg_count = 1;

  func.registers = {
    RegisterTypes::Bool,
    RegisterTypes::Int32,
    RegisterTypes::Int32,
    RegisterTypes::UInt64,
  };

  func.return_type = RegisterTypes::Bool;

  func.block = make_block({
    allocator.create<LoadStatement>(1, 123),
    allocator.create<LoadStatement>(2, 456),
    allocator.create<IfStatement>(0, make_block({
      allocator.create<ReturnStatement>(1),
    }), make_block({
      allocator.create<ReturnStatement>(2),
    })),
  });

  Interface global;

  InterpreterFrame<InterpreterTraits> frame {func};
  auto exit = frame.execute();

  std::cout
    << "result: " << static_cast<int>(exit)
    << "/" << frame.get_reg(frame.return_register)
    << "\n";

  return 0;
}
