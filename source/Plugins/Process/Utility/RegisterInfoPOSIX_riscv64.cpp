//===-- RegisterInfoPOSIX_riscv64.cpp -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===---------------------------------------------------------------------===//

#include <cassert>
#include <stddef.h>

#include "lldb/lldb-defines.h"
#include "llvm/Support/Compiler.h"

#include "RegisterInfoPOSIX_riscv64.h"

#define GPR_OFFSET(idx) ((idx)*8)
#define GPR_OFFSET_NAME(reg)                                                   \
  (LLVM_EXTENSION offsetof(RegisterInfoPOSIX_riscv64::GPR, reg))

#define FPU_OFFSET(idx) ((idx)*16 + sizeof(RegisterInfoPOSIX_riscv64::GPR))
#define FPU_OFFSET_NAME(reg)                                                   \
  (LLVM_EXTENSION offsetof(RegisterInfoPOSIX_riscv64::FPU, reg) +                \
   sizeof(RegisterInfoPOSIX_riscv64::GPR))

#define EXC_OFFSET_NAME(reg)                                                   \
  (LLVM_EXTENSION offsetof(RegisterInfoPOSIX_riscv64::EXC, reg) +                \
   sizeof(RegisterInfoPOSIX_riscv64::GPR) +                                      \
   sizeof(RegisterInfoPOSIX_riscv64::FPU))
#define DBG_OFFSET_NAME(reg)                                                   \
  (LLVM_EXTENSION offsetof(RegisterInfoPOSIX_riscv64::DBG, reg) +                \
   sizeof(RegisterInfoPOSIX_riscv64::GPR) +                                      \
   sizeof(RegisterInfoPOSIX_riscv64::FPU) +                                      \
   sizeof(RegisterInfoPOSIX_riscv64::EXC))

#define DEFINE_DBG(reg, i)                                                     \
  #reg, NULL,                                                                  \
      sizeof(((RegisterInfoPOSIX_riscv64::DBG *) NULL)->reg[i]),                 \
              DBG_OFFSET_NAME(reg[i]), lldb::eEncodingUint, lldb::eFormatHex,  \
                              {LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM,       \
                               LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM,       \
                               dbg_##reg##i },                                 \
                               NULL, NULL, NULL, 0
#define REG_CONTEXT_SIZE                                                       \
  (sizeof(RegisterInfoPOSIX_riscv64::GPR) +                                      \
   sizeof(RegisterInfoPOSIX_riscv64::FPU) +                                      \
   sizeof(RegisterInfoPOSIX_riscv64::EXC))

#define DECLARE_REGISTER_INFOS_RISCV64_STRUCT
#include "RegisterInfos_riscv64.h"
#undef DECLARE_REGISTER_INFOS_RISCV64_STRUCT

static const lldb_private::RegisterInfo *
GetRegisterInfoPtr(const lldb_private::ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::riscv32:
  case llvm::Triple::riscv64:
    return g_register_infos_riscv64_le;
  default:
    assert(false && "Unhandled target architecture.");
    return nullptr;
  }
}

// Number of register sets provided by this context.
enum {
  k_num_gpr_registers = gpr_x31 - gpr_x0 + 1,
  k_num_register_sets = 1
};

// RISC-V64 general purpose registers.
static const uint32_t g_gpr_regnums_riscv64[] = {
    gpr_x0,  gpr_ra,   gpr_sp,  gpr_x3,
    gpr_x4,  gpr_x5,   gpr_x6,  gpr_x7,
    gpr_fp,  gpr_x9,   gpr_x10, gpr_x11,
    gpr_x12, gpr_x13,  gpr_x14, gpr_x15,
    gpr_x16, gpr_x17,  gpr_x18, gpr_x19,
    gpr_x20, gpr_x21,  gpr_x22, gpr_x23,
    gpr_x24, gpr_x25,  gpr_x26, gpr_x27,
    gpr_x28, gpr_x29,   gpr_x30,  gpr_x31,
    LLDB_INVALID_REGNUM};

static_assert(((sizeof g_gpr_regnums_riscv64 / sizeof g_gpr_regnums_riscv64[0]) -
               1) == k_num_gpr_registers,
              "g_gpr_regnums_riscv64 has wrong number of register infos");

// Register sets for RISC-V64.
static const lldb_private::RegisterSet g_reg_sets_riscv64[k_num_register_sets] = {
    {"General Purpose Registers", "gpr", k_num_gpr_registers,
     g_gpr_regnums_riscv64}};

static uint32_t
GetRegisterInfoCount(const lldb_private::ArchSpec &target_arch) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::riscv32:
  case llvm::Triple::riscv64:
    return static_cast<uint32_t>(sizeof(g_register_infos_riscv64_le) /
                                 sizeof(g_register_infos_riscv64_le[0]));
  default:
    assert(false && "Unhandled target architecture.");
    return 0;
  }
}

RegisterInfoPOSIX_riscv64::RegisterInfoPOSIX_riscv64(
    const lldb_private::ArchSpec &target_arch)
    : lldb_private::RegisterInfoAndSetInterface(target_arch),
      m_register_info_p(GetRegisterInfoPtr(target_arch)),
      m_register_info_count(GetRegisterInfoCount(target_arch)) {
}

uint32_t RegisterInfoPOSIX_riscv64::GetRegisterCount() const {
  return k_num_gpr_registers;
}

size_t RegisterInfoPOSIX_riscv64::GetGPRSize() const {
  return sizeof(struct RegisterInfoPOSIX_riscv64::GPR);
}

size_t RegisterInfoPOSIX_riscv64::GetFPRSize() const {
  return sizeof(struct RegisterInfoPOSIX_riscv64::FPU);
}

const lldb_private::RegisterInfo *
RegisterInfoPOSIX_riscv64::GetRegisterInfo() const {
  return m_register_info_p;
}

size_t RegisterInfoPOSIX_riscv64::GetRegisterSetCount() const {
  return k_num_register_sets - 1;
}

size_t RegisterInfoPOSIX_riscv64::GetRegisterSetFromRegisterIndex(
    uint32_t reg_index) const {
  if (reg_index <= gpr_x31)
    return GPRegSet;
  return LLDB_INVALID_REGNUM;
}

const lldb_private::RegisterSet *
RegisterInfoPOSIX_riscv64::GetRegisterSet(size_t set_index) const {
  if (set_index < GetRegisterSetCount())
    return &g_reg_sets_riscv64[set_index];
  return nullptr;
}
