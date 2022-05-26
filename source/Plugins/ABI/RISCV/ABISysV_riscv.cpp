//===-- ABISysV_riscv.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABISysV_riscv.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"

#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Value.h"
#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/Core/ValueObjectMemory.h"
#include "lldb/Core/ValueObjectRegister.h"
#include "lldb/Symbol/UnwindPlan.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Utility/ConstString.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"

using namespace lldb;
using namespace lldb_private;

LLDB_PLUGIN_DEFINE(ABISysV_riscv)

enum riscv_dwarf_regnums {
  dwarf_x0 = 0,
  dwarf_x1,
  dwarf_x2,
  dwarf_x3,
  dwarf_x4,
  dwarf_x5,
  dwarf_x6,
  dwarf_x7,
  dwarf_x8,
  dwarf_x9,
  dwarf_x10,
  dwarf_x11,
  dwarf_x12,
  dwarf_x13,
  dwarf_x14,
  dwarf_x15,
  dwarf_x16,
  dwarf_x17,
  dwarf_x18,
  dwarf_x19,
  dwarf_x20,
  dwarf_x21,
  dwarf_x22,
  dwarf_x23,
  dwarf_x24,
  dwarf_x25,
  dwarf_x26,
  dwarf_x27,
  dwarf_x28,
  dwarf_x29,
  dwarf_x30,
  dwarf_x31,
  dwarf_f0 = 32,
  dwarf_f1,
  dwarf_f2,
  dwarf_f3,
  dwarf_f4,
  dwarf_f5,
  dwarf_f6,
  dwarf_f7,
  dwarf_f8,
  dwarf_f9,
  dwarf_f10,
  dwarf_f11,
  dwarf_f12,
  dwarf_f13,
  dwarf_f14,
  dwarf_f15,
  dwarf_f16,
  dwarf_f17,
  dwarf_f18,
  dwarf_f19,
  dwarf_f20,
  dwarf_f21,
  dwarf_f22,
  dwarf_f23,
  dwarf_f24,
  dwarf_f25,
  dwarf_f26,
  dwarf_f27,
  dwarf_f28,
  dwarf_f29,
  dwarf_f30,
  dwarf_f31
};

bool ABISysV_riscv::CreateFunctionEntryUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  unwind_plan.SetRegisterKind(eRegisterKindGeneric);

  uint32_t pc_reg_num = LLDB_REGNUM_GENERIC_PC;
  uint32_t sp_reg_num = LLDB_REGNUM_GENERIC_SP;
  uint32_t ra_reg_num = LLDB_REGNUM_GENERIC_RA;

  UnwindPlan::RowSP row(new UnwindPlan::Row);

  // Define CFA as the stack pointer
  row->GetCFAValue().SetIsRegisterPlusOffset(sp_reg_num, 0);

  // Previous frames pc is in ra
  row->SetRegisterLocationToRegister(pc_reg_num, ra_reg_num, true);

  unwind_plan.AppendRow(row);
  unwind_plan.SetSourceName("riscv function-entry unwind plan");
  unwind_plan.SetSourcedFromCompiler(eLazyBoolNo);
  return true;
}

bool ABISysV_riscv::CreateDefaultUnwindPlan(UnwindPlan &unwind_plan) {
  unwind_plan.Clear();
  unwind_plan.SetRegisterKind(eRegisterKindGeneric);

  uint32_t pc_reg_num = LLDB_REGNUM_GENERIC_PC;
  uint32_t sp_reg_num = LLDB_REGNUM_GENERIC_SP;
  uint32_t ra_reg_num = LLDB_REGNUM_GENERIC_RA;

  UnwindPlan::RowSP row(new UnwindPlan::Row);

  // Define the CFA as the current stack pointer.
  row->GetCFAValue().SetIsRegisterPlusOffset(sp_reg_num, 0);
  row->SetOffset(0);

  // The previous frames pc is stored in ra.
  row->SetRegisterLocationToRegister(pc_reg_num, ra_reg_num, true);

  unwind_plan.AppendRow(row);
  unwind_plan.SetSourceName("riscv default unwind plan");
  unwind_plan.SetSourcedFromCompiler(eLazyBoolNo);
  unwind_plan.SetUnwindPlanValidAtAllInstructions(eLazyBoolNo);
  return true;
}

bool ABISysV_riscv::RegisterIsVolatile(
    const lldb_private::RegisterInfo *reg_info) {
  return !RegisterIsCalleeSaved(reg_info);
}

// See "Register Convention" in the RISC-V psABI documentation, which is
// maintained at https://github.com/riscv/riscv-elf-psabi-doc
bool ABISysV_riscv::RegisterIsCalleeSaved(
    const lldb_private::RegisterInfo *reg_info) {
  if (!reg_info)
    return false;

  bool IsCalleeSaved =
      llvm::StringSwitch<bool>(reg_info->name)
          .Cases("x1", "x2", "x8", "x9", "x18", "x19", "x20", "x21", true)
          .Cases("x22", "x23", "x24", "x25", "x26", "x27", true)
          .Cases("f8", "f9", "f18", "f19", "f20", "f21", IsHardFloatProcess())
          .Cases("f22", "f23", "f24", "f25", "f26", "f27", IsHardFloatProcess())
          .Default(false);
  return IsCalleeSaved;
}

std::pair<uint32_t, uint32_t>
ABISysV_riscv::GetEHAndDWARFNums(llvm::StringRef name) {
  if (name == "ra")
    return {LLDB_INVALID_REGNUM, riscv_dwarf_regnums::dwarf_x1};
  if (name == "sp")
    return {LLDB_INVALID_REGNUM, riscv_dwarf_regnums::dwarf_x2};
  if (name == "fp")
    return {LLDB_INVALID_REGNUM, riscv_dwarf_regnums::dwarf_x8};
  return MCBasedABI::GetEHAndDWARFNums(name);
}

uint32_t ABISysV_riscv::GetGenericNum(llvm::StringRef name) {
  return llvm::StringSwitch<uint32_t>(name)
      .Case("pc", LLDB_REGNUM_GENERIC_PC)
      .Case("ra", LLDB_REGNUM_GENERIC_RA)
      .Case("sp", LLDB_REGNUM_GENERIC_SP)
      .Case("fp", LLDB_REGNUM_GENERIC_FP)
      .Case("a0", LLDB_REGNUM_GENERIC_ARG1)
      .Case("a1", LLDB_REGNUM_GENERIC_ARG2)
      .Case("a2", LLDB_REGNUM_GENERIC_ARG3)
      .Case("a3", LLDB_REGNUM_GENERIC_ARG4)
      .Case("a4", LLDB_REGNUM_GENERIC_ARG5)
      .Case("a5", LLDB_REGNUM_GENERIC_ARG6)
      .Case("a6", LLDB_REGNUM_GENERIC_ARG7)
      .Case("a7", LLDB_REGNUM_GENERIC_ARG8)
      .Default(LLDB_INVALID_REGNUM);
}

bool ABISysV_riscv::IsHardFloatProcess() const {
  bool is_hardfloat = false;
  ProcessSP process_sp(GetProcessSP());
  if (process_sp) {
    const ArchSpec &arch(process_sp->GetTarget().GetArchitecture());
    if (arch.GetFlags() & ArchSpec::eRISCV_abi_f ||
        arch.GetFlags() & ArchSpec::eRISCV_abi_d)
      is_hardfloat = true;
  }
  return is_hardfloat;
}

ABISP
ABISysV_riscv::CreateInstance(lldb::ProcessSP process_sp,
                              const ArchSpec &arch) {
  if (arch.GetTriple().getArch() == llvm::Triple::riscv32 ||
      arch.GetTriple().getArch() == llvm::Triple::riscv64) {
    return ABISP(
        new ABISysV_riscv(std::move(process_sp), MakeMCRegisterInfo(arch),
                          arch.GetTriple().getArch() == llvm::Triple::riscv64));
  }
  return ABISP();
}

void ABISysV_riscv::Initialize() {
  PluginManager::RegisterPlugin(
      GetPluginNameStatic(), "System V ABI for riscv targets", CreateInstance);
}

void ABISysV_riscv::Terminate() {
  PluginManager::UnregisterPlugin(CreateInstance);
}

// PluginInterface protocol

lldb_private::ConstString ABISysV_riscv::GetPluginNameStatic() {
  static ConstString g_name("sysv-riscv");
  return g_name;
}

lldb_private::ConstString ABISysV_riscv::GetPluginName() {
  return GetPluginNameStatic();
}
