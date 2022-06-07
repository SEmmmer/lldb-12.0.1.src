//===-- RISCV64_DWARF_Registers.h -------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_UTILITY_RISCV64_DWARF_REGISTERS_H
#define LLDB_SOURCE_UTILITY_RISCV64_DWARF_REGISTERS_H

#include "lldb/lldb-private.h"

namespace riscv64_dwarf {

enum {
  x0 = 0,
  x1,
  x2,
  x3,
  x4,
  x5,
  x6,
  x7,
  x8,
  x9,
  x10,
  x11,
  x12,
  x13,
  x14,
  x15,
  x16,
  x17,
  x18,
  x19,
  x20,
  x21,
  x22,
  x23,
  x24,
  x25,
  x26,
  x27,
  x28,
  x29,
  x30,
  x31,
  ra = x1,
  sp = x2,
  fp = x8,
};

} // namespace riscv64_dwarf

#endif // LLDB_SOURCE_UTILITY_RISCV64_DWARF_REGISTERS_H
