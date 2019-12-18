#include <string>
#include <iostream>

#include "program/validator.h"

using namespace zvm;

void test_validator() {
  Func func {
    1,
    {
      RegisterTypes::Bool,
      RegisterTypes::Int32,
      RegisterTypes::UInt64,
    },
    RegisterTypes::Bool,
    {
      new LoadStatement(1, 123),
      new LoadStatement(1, 456),
      new IfStatement(0, {
        new ReturnStatement(0),
      }, {
        new ReturnStatement(0),
      }),
    },
  };

  std::cout << validate_func(func, {}, {});
}

int main() {
  test_validator();
  return 0;
}
