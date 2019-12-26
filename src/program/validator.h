#pragma once

#include "func.h"

namespace zvm {

  bool validate_func(
    Func& func,
    const Interface& global,
    const InterfaceTypeTable& interface_types,
    ValidationToken validation_token = 0);

}
