//===-- NativeRegisterContextLinux_riscv64.h -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#if defined(__riscv) || __riscv_xlen == 64

#ifndef lldb_NativeRegisterContextLinux_riscv64_h
#define lldb_NativeRegisterContextLinux_riscv64_h

#include "Plugins/Process/Linux/NativeRegisterContextLinux.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_riscv64.h"

#include <asm/ptrace.h>

namespace lldb_private {
namespace process_linux {

class NativeProcessLinux;

class NativeRegisterContextLinux_riscv64 : public NativeRegisterContextLinux {
public:
  NativeRegisterContextLinux_riscv64(const ArchSpec &target_arch,
                                   NativeThreadProtocol &native_thread);

  uint32_t GetRegisterSetCount() const override;

  uint32_t GetUserRegisterCount() const override;

  const RegisterSet *GetRegisterSet(uint32_t set_index) const override;

  Status ReadRegister(const RegisterInfo *reg_info,
                      RegisterValue &reg_value) override;

  Status WriteRegister(const RegisterInfo *reg_info,
                       const RegisterValue &reg_value) override;

  Status ReadAllRegisterValues(lldb::DataBufferSP &data_sp) override;

  Status WriteAllRegisterValues(const lldb::DataBufferSP &data_sp) override;

  void InvalidateAllRegisters() override;

  std::vector<uint32_t>
  GetExpeditedRegisters(ExpeditedRegs expType) const override;

  bool RegisterOffsetIsDynamic() const override { return true; }

  Status GetHardwareBreakHitIndex(uint32_t &bp_index,
                                  lldb::addr_t trap_addr) override;

  Status GetWatchpointHitIndex(uint32_t &wp_index,
                               lldb::addr_t trap_addr) override;

  lldb::addr_t GetWatchpointHitAddress(uint32_t wp_index) override;

  lldb::addr_t GetWatchpointAddress(uint32_t wp_index) override;

  uint32_t GetWatchpointSize(uint32_t wp_index);

  bool WatchpointIsEnabled(uint32_t wp_index);

  // Debug register type select
  enum DREGType { eDREGTypeWATCH = 0, eDREGTypeBREAK };

protected:

  Status ReadGPR() override;

  Status WriteGPR() override;

  Status ReadFPR() override;

  Status WriteFPR() override;

  void *GetGPRBuffer() override { return &m_gpr_riscv64; }

  // GetGPRBufferSize returns sizeof riscv64 GPR ptrace buffer, it is different
  // from GetGPRSize which returns sizeof RegisterInfoPOSIX_riscv64::GPR.
  size_t GetGPRBufferSize() { return sizeof(m_gpr_riscv64); }

  void *GetFPRBuffer() override { return &m_fpr; }

  size_t GetFPRSize() override { return sizeof(m_fpr); }

private:
  bool m_gpr_is_valid;
  bool m_fpu_is_valid;

  struct user_regs_struct m_gpr_riscv64; // 64-bit general purpose registers.

  RegisterInfoPOSIX_riscv64::FPU
      m_fpr; // floating-point registers including extended register sets.

  // Debug register info for hardware breakpoints and watchpoints management.
  struct DREG {
    lldb::addr_t address;  // Breakpoint/watchpoint address value.
    lldb::addr_t hit_addr; // Address at which last watchpoint trigger exception
                           // occurred.
    lldb::addr_t real_addr; // Address value that should cause target to stop.
    uint32_t control;       // Breakpoint/watchpoint control value.
    uint32_t refcount;      // Serves as enable/disable and reference counter.
  };

  struct DREG m_hbr_regs[16]; // RISC-V native linux hardware breakpoints
  struct DREG m_hwp_regs[16]; // RISC-V native linux hardware watchpoints

  uint32_t m_max_hwp_supported;
  uint32_t m_max_hbp_supported;

  bool IsGPR(unsigned reg) const;

  bool IsFPR(unsigned reg) const;

  uint32_t CalculateFprOffset(const RegisterInfo *reg_info) const;

  RegisterInfoPOSIX_riscv64 &GetRegisterInfo() const;
};

} // namespace process_linux
} // namespace lldb_private

#endif // #ifndef lldb_NativeRegisterContextLinux_riscv64_h

#endif // defined(__riscv) || __riscv_xlen == 64
