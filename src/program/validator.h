#pragma once

#include "func.h"

namespace zvm {

  bool validate_func(
    const Func& func,
    const InterfaceTypeTable& interface_types,
    const InterfaceNameTable& interface_names);

}
