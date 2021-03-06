#include <string>
#include <iostream>

#include <unordered_set>
#include "program/validator.h"

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

void test_validator() {
  Allocator<Statement> allocator;

  Func func;

  func.arg_count = 1;

  func.registers = {
    RegisterTypes::Bool,
    RegisterTypes::Int32,
    RegisterTypes::UInt64,
  };

  func.return_type = RegisterTypes::Bool;

  func.block = make_block({
    allocator.create<LoadStatement>(1, 123),
    allocator.create<LoadStatement>(1, 456),
    allocator.create<IfStatement>(0, make_block({
      allocator.create<ReturnStatement>(0),
    }), make_block({
      allocator.create<ReturnStatement>(0),
    })),
  });

  Interface global;

  std::cout << validate_func(func, global, {});
}

int main() {
  test_validator();
  return 0;
}
