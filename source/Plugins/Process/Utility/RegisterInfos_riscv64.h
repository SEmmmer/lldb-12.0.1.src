//===-- RegisterInfos_riscv64.h -----------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifdef DECLARE_REGISTER_INFOS_RISCV64_STRUCT

#include <stddef.h>

#include "lldb/lldb-defines.h"
#include "lldb/lldb-enumerations.h"
#include "lldb/lldb-private.h"

#include "Utility/RISCV64_DWARF_Registers.h"

#ifndef GPR_OFFSET
#error GPR_OFFSET must be defined before including this header file
#endif

#ifndef GPR_OFFSET_NAME
#error GPR_OFFSET_NAME must be defined before including this header file
#endif

#ifndef FPU_OFFSET
#error FPU_OFFSET must be defined before including this header file
#endif

#ifndef FPU_OFFSET_NAME
#error FPU_OFFSET_NAME must be defined before including this header file
#endif

#ifndef EXC_OFFSET_NAME
#error EXC_OFFSET_NAME must be defined before including this header file
#endif

#ifndef DBG_OFFSET_NAME
#error DBG_OFFSET_NAME must be defined before including this header file
#endif

#ifndef DEFINE_DBG
#error DEFINE_DBG must be defined before including this header file
#endif

// Offsets for a little-endian layout of the register context
#define GPR_W_PSEUDO_REG_ENDIAN_OFFSET 0
#define FPU_S_PSEUDO_REG_ENDIAN_OFFSET 0
#define FPU_D_PSEUDO_REG_ENDIAN_OFFSET 0

enum {
  gpr_x0 = 0,
  gpr_x1,
  gpr_x2,
  gpr_x3,
  gpr_x4,
  gpr_x5,
  gpr_x6,
  gpr_x7,
  gpr_x8,
  gpr_x9,
  gpr_x10,
  gpr_x11,
  gpr_x12,
  gpr_x13,
  gpr_x14,
  gpr_x15,
  gpr_x16,
  gpr_x17,
  gpr_x18,
  gpr_x19,
  gpr_x20,
  gpr_x21,
  gpr_x22,
  gpr_x23,
  gpr_x24,
  gpr_x25,
  gpr_x26,
  gpr_x27,
  gpr_x28,
  gpr_x29,
  gpr_x30,
  gpr_x31,
  gpr_ra = gpr_x1,
  gpr_sp = gpr_x2,
  gpr_fp = gpr_x8,

  k_num_registers
};

// Generates register kinds array with DWARF, EH frame and generic kind
#define MISC_KIND(reg, type, generic_kind)                                     \
  {                                                                            \
    riscv64_dwarf::reg, generic_kind, LLDB_INVALID_REGNUM,   \
        type##_##reg                                                           \
  }

// Generates register kinds array for vector registers
#define GPR64_KIND(reg, generic_kind) MISC_KIND(reg, gpr, generic_kind)

// Defines a 64-bit general purpose register
#define DEFINE_GPR64(reg, generic_kind)                                        \
  {                                                                            \
    #reg, nullptr, 8, GPR_OFFSET(gpr_##reg), lldb::eEncodingUint,              \
        lldb::eFormatHex, GPR64_KIND(reg, generic_kind), nullptr, nullptr,     \
        nullptr, 0                                                             \
  }

// Defines a 64-bit general purpose register
#define DEFINE_GPR64_ALT(reg, alt, generic_kind)                               \
  {                                                                            \
    #reg, #alt, 8, GPR_OFFSET(gpr_##reg), lldb::eEncodingUint,                 \
        lldb::eFormatHex, GPR64_KIND(reg, generic_kind), nullptr, nullptr,     \
        nullptr, 0                                                             \
  }

// Defines miscellaneous status and control registers like fcsr
#define DEFINE_MISC_REGS(reg, size, TYPE, lldb_kind)                           \
  {                                                                            \
    #reg, nullptr, size, TYPE##_OFFSET_NAME(reg), lldb::eEncodingUint,         \
        lldb::eFormatHex, MISC_##TYPE##_KIND(lldb_kind), nullptr, nullptr,     \
        nullptr, 0                                                             \
  }

static lldb_private::RegisterInfo g_register_infos_riscv64_le[] = {
    // DEFINE_GPR64(name, GENERIC KIND)
    DEFINE_GPR64(x0, LLDB_INVALID_REGNUM),
    DEFINE_GPR64_ALT(ra, x1, LLDB_REGNUM_GENERIC_RA),
    DEFINE_GPR64_ALT(sp, x2, LLDB_REGNUM_GENERIC_SP),
    DEFINE_GPR64(x3, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x4, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x5, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x6, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x7, LLDB_INVALID_REGNUM),
    DEFINE_GPR64_ALT(fp, x8, LLDB_REGNUM_GENERIC_FP),
    DEFINE_GPR64(x9, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x10, LLDB_REGNUM_GENERIC_ARG1),
    DEFINE_GPR64(x11, LLDB_REGNUM_GENERIC_ARG2),
    DEFINE_GPR64(x12, LLDB_REGNUM_GENERIC_ARG3),
    DEFINE_GPR64(x13, LLDB_REGNUM_GENERIC_ARG4),
    DEFINE_GPR64(x14, LLDB_REGNUM_GENERIC_ARG5),
    DEFINE_GPR64(x15, LLDB_REGNUM_GENERIC_ARG6),
    DEFINE_GPR64(x16, LLDB_REGNUM_GENERIC_ARG7),
    DEFINE_GPR64(x17, LLDB_REGNUM_GENERIC_ARG8),
    DEFINE_GPR64(x18, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x19, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x20, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x21, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x22, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x23, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x24, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x25, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x26, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x27, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x28, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x29, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x30, LLDB_INVALID_REGNUM),
    DEFINE_GPR64(x31, LLDB_INVALID_REGNUM),
    // clang-format on
};

#endif // DECLARE_REGISTER_INFOS_RISCV64_STRUCT
