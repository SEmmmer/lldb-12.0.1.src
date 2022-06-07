//===-- NativeRegisterContextLinux_riscv64.cpp ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#if defined(__riscv) || __riscv_xlen == 64

#include "NativeRegisterContextLinux_riscv64.h"


#include "lldb/Host/common/NativeProcessProtocol.h"
#include "lldb/Utility/DataBufferHeap.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/RegisterValue.h"
#include "lldb/Utility/Status.h"

#include "Plugins/Process/Linux/NativeProcessLinux.h"
#include "Plugins/Process/Linux/Procfs.h"
#include "Plugins/Process/POSIX/ProcessPOSIXLog.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_riscv64.h"

// System includes - They have to be included after framework includes because
// they define some macros which collide with variable names in other modules
#include <sys/socket.h>
// NT_PRSTATUS and NT_FPREGSET definition
#include <elf.h>

#define REG_CONTEXT_SIZE (GetGPRSize() + GetFPRSize())

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::process_linux;

std::unique_ptr<NativeRegisterContextLinux>
NativeRegisterContextLinux::CreateHostNativeRegisterContextLinux(
    const ArchSpec &target_arch, NativeThreadProtocol &native_thread) {
  switch (target_arch.GetMachine()) {
  case llvm::Triple::riscv64:
    return std::make_unique<NativeRegisterContextLinux_riscv64>(target_arch,
                                                               native_thread);
  default:
    llvm_unreachable("have no register context for architecture");
  }
}

NativeRegisterContextLinux_riscv64::NativeRegisterContextLinux_riscv64(
    const ArchSpec &target_arch, NativeThreadProtocol &native_thread)
    : NativeRegisterContextRegisterInfo(
          native_thread, new RegisterInfoPOSIX_riscv64(target_arch)) {
  ::memset(&m_fpr, 0, sizeof(m_fpr));
  ::memset(&m_gpr_riscv64, 0, sizeof(m_gpr_riscv64));
  ::memset(&m_hwp_regs, 0, sizeof(m_hwp_regs));
  ::memset(&m_hbr_regs, 0, sizeof(m_hbr_regs));

  // 16 is just a maximum value, query hardware for actual watchpoint count
  m_max_hwp_supported = 16;
  m_max_hbp_supported = 16;

  m_gpr_is_valid = false;
  m_fpu_is_valid = false;
}

RegisterInfoPOSIX_riscv64 &
NativeRegisterContextLinux_riscv64::GetRegisterInfo() const {
  return static_cast<RegisterInfoPOSIX_riscv64 &>(*m_register_info_interface_up);
}

uint32_t NativeRegisterContextLinux_riscv64::GetRegisterSetCount() const {
  return GetRegisterInfo().GetRegisterSetCount();
}

const RegisterSet *
NativeRegisterContextLinux_riscv64::GetRegisterSet(uint32_t set_index) const {
  return GetRegisterInfo().GetRegisterSet(set_index);
}

uint32_t NativeRegisterContextLinux_riscv64::GetUserRegisterCount() const {
  uint32_t count = 0;
  for (uint32_t set_index = 0; set_index < GetRegisterSetCount(); ++set_index)
    count += GetRegisterSet(set_index)->num_registers;
  return count;
}

Status
NativeRegisterContextLinux_riscv64::ReadRegister(const RegisterInfo *reg_info,
                                               RegisterValue &reg_value) {
  Status error;

  if (!reg_info) {
    error.SetErrorString("reg_info NULL");
    return error;
  }

  const uint32_t reg = reg_info->kinds[lldb::eRegisterKindLLDB];

  if (reg == LLDB_INVALID_REGNUM)
    return Status("no lldb regnum for %s", reg_info && reg_info->name
                                               ? reg_info->name
                                               : "<unknown register>");

  uint8_t *src;
  uint32_t offset = LLDB_INVALID_INDEX32;

  if (IsGPR(reg)) {
    error = ReadGPR();
    if (error.Fail())
      return error;

    offset = reg_info->byte_offset;
    assert(offset < GetGPRSize());
    src = (uint8_t *)GetGPRBuffer() + offset;

  } else if (IsFPR(reg)) {
    error = ReadFPR();
    if (error.Fail())
      return error;

    offset = CalculateFprOffset(reg_info);
    assert(offset < GetFPRSize());
    src = (uint8_t *)GetFPRBuffer() + offset;
  } else
    return Status("failed - register wasn't recognized to be a GPR or an FPR, "
                  "write strategy unknown");

  reg_value.SetFromMemoryData(reg_info, src, reg_info->byte_size,
                              eByteOrderLittle, error);

  return error;
}

Status NativeRegisterContextLinux_riscv64::WriteRegister(
    const RegisterInfo *reg_info, const RegisterValue &reg_value) {
  Status error;

  if (!reg_info)
    return Status("reg_info NULL");

  const uint32_t reg = reg_info->kinds[lldb::eRegisterKindLLDB];

  if (reg == LLDB_INVALID_REGNUM)
    return Status("no lldb regnum for %s", reg_info->name != nullptr
                                               ? reg_info->name
                                               : "<unknown register>");

  uint8_t *dst = nullptr;
  uint32_t offset = LLDB_INVALID_INDEX32;
  std::vector<uint8_t> sve_reg_non_live;

  if (IsGPR(reg)) {
    error = ReadGPR();
    if (error.Fail())
      return error;

    assert(reg_info->byte_offset < GetGPRSize());
    dst = (uint8_t *)GetGPRBuffer() + reg_info->byte_offset;
    ::memcpy(dst, reg_value.GetBytes(), reg_info->byte_size);

    return WriteGPR();
  } else if (IsFPR(reg)) {
    // SVE is disabled take legacy route for FPU register access
    error = ReadFPR();
    if (error.Fail())
      return error;

    offset = CalculateFprOffset(reg_info);
    assert(offset < GetFPRSize());
    dst = (uint8_t *)GetFPRBuffer() + offset;
    ::memcpy(dst, reg_value.GetBytes(), reg_info->byte_size);

    return WriteFPR();
  }

  return Status("Failed to write register value");
}

Status NativeRegisterContextLinux_riscv64::ReadAllRegisterValues(
    lldb::DataBufferSP &data_sp) {
  Status error;

  data_sp.reset(new DataBufferHeap(REG_CONTEXT_SIZE, 0));

  error = ReadGPR();
  if (error.Fail())
    return error;

  error = ReadFPR();
  if (error.Fail())
    return error;

  uint8_t *dst = data_sp->GetBytes();
  ::memcpy(dst, GetGPRBuffer(), GetGPRSize());
  dst += GetGPRSize();
  ::memcpy(dst, GetFPRBuffer(), GetFPRSize());

  return error;
}

Status NativeRegisterContextLinux_riscv64::WriteAllRegisterValues(
    const lldb::DataBufferSP &data_sp) {
  Status error;

  if (!data_sp) {
    error.SetErrorStringWithFormat(
        "NativeRegisterContextLinux_x86_64::%s invalid data_sp provided",
        __FUNCTION__);
    return error;
  }

  if (data_sp->GetByteSize() != REG_CONTEXT_SIZE) {
    error.SetErrorStringWithFormat(
        "NativeRegisterContextLinux_x86_64::%s data_sp contained mismatched "
        "data size, expected %" PRIu64 ", actual %" PRIu64,
        __FUNCTION__, REG_CONTEXT_SIZE, data_sp->GetByteSize());
    return error;
  }

  uint8_t *src = data_sp->GetBytes();
  if (src == nullptr) {
    error.SetErrorStringWithFormat("NativeRegisterContextLinux_x86_64::%s "
                                   "DataBuffer::GetBytes() returned a null "
                                   "pointer",
                                   __FUNCTION__);
    return error;
  }
  ::memcpy(GetGPRBuffer(), src, GetRegisterInfoInterface().GetGPRSize());

  error = WriteGPR();
  if (error.Fail())
    return error;

  src += GetRegisterInfoInterface().GetGPRSize();
  ::memcpy(GetFPRBuffer(), src, GetFPRSize());

  error = WriteFPR();
  if (error.Fail())
    return error;

  return error;
}

bool NativeRegisterContextLinux_riscv64::IsGPR(unsigned reg) const {
  if (GetRegisterInfo().GetRegisterSetFromRegisterIndex(reg) ==
      RegisterInfoPOSIX_riscv64::GPRegSet)
    return true;
  return false;
}

bool NativeRegisterContextLinux_riscv64::IsFPR(unsigned reg) const {
  return false;
}

Status NativeRegisterContextLinux_riscv64::GetHardwareBreakHitIndex(
    uint32_t &bp_index, lldb::addr_t trap_addr) {
  Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_BREAKPOINTS));

  LLDB_LOGF(log, "NativeRegisterContextLinux_riscv64::%s()", __FUNCTION__);

  lldb::addr_t break_addr;

  for (bp_index = 0; bp_index < m_max_hbp_supported; ++bp_index) {
    break_addr = m_hbr_regs[bp_index].address;

    if ((m_hbr_regs[bp_index].control & 0x1) && (trap_addr == break_addr)) {
      m_hbr_regs[bp_index].hit_addr = trap_addr;
      return Status();
    }
  }

  bp_index = LLDB_INVALID_INDEX32;
  return Status();
}

uint32_t
NativeRegisterContextLinux_riscv64::GetWatchpointSize(uint32_t wp_index) {
  Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  switch ((m_hwp_regs[wp_index].control >> 5) & 0xff) {
  case 0x01:
    return 1;
  case 0x03:
    return 2;
  case 0x0f:
    return 4;
  case 0xff:
    return 8;
  default:
    return 0;
  }
}
bool NativeRegisterContextLinux_riscv64::WatchpointIsEnabled(uint32_t wp_index) {
  Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  if ((m_hwp_regs[wp_index].control & 0x1) == 0x1)
    return true;
  else
    return false;
}

Status NativeRegisterContextLinux_riscv64::GetWatchpointHitIndex(
    uint32_t &wp_index, lldb::addr_t trap_addr) {
  Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}, trap_addr: {1:x}", wp_index, trap_addr);

  uint32_t watch_size;
  lldb::addr_t watch_addr;

  for (wp_index = 0; wp_index < m_max_hwp_supported; ++wp_index) {
    watch_size = GetWatchpointSize(wp_index);
    watch_addr = m_hwp_regs[wp_index].address;

    if (WatchpointIsEnabled(wp_index) && trap_addr >= watch_addr &&
        trap_addr < watch_addr + watch_size) {
      m_hwp_regs[wp_index].hit_addr = trap_addr;
      return Status();
    }
  }

  wp_index = LLDB_INVALID_INDEX32;
  return Status();
}

lldb::addr_t
NativeRegisterContextLinux_riscv64::GetWatchpointAddress(uint32_t wp_index) {
  Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  if (wp_index >= m_max_hwp_supported)
    return LLDB_INVALID_ADDRESS;

  if (WatchpointIsEnabled(wp_index))
    return m_hwp_regs[wp_index].real_addr;
  else
    return LLDB_INVALID_ADDRESS;
}

lldb::addr_t
NativeRegisterContextLinux_riscv64::GetWatchpointHitAddress(uint32_t wp_index) {
  Log *log(ProcessPOSIXLog::GetLogIfAllCategoriesSet(POSIX_LOG_WATCHPOINTS));
  LLDB_LOG(log, "wp_index: {0}", wp_index);

  if (wp_index >= m_max_hwp_supported)
    return LLDB_INVALID_ADDRESS;

  if (WatchpointIsEnabled(wp_index))
    return m_hwp_regs[wp_index].hit_addr;
  else
    return LLDB_INVALID_ADDRESS;
}

Status NativeRegisterContextLinux_riscv64::ReadGPR() {
  Status error;

  if (m_gpr_is_valid)
    return error;

  struct iovec ioVec;
  ioVec.iov_base = GetGPRBuffer();
  ioVec.iov_len = GetGPRBufferSize();

  error = ReadRegisterSet(&ioVec, GetGPRBufferSize(), NT_PRSTATUS);

  if (error.Success())
    m_gpr_is_valid = true;

  return error;
}

Status NativeRegisterContextLinux_riscv64::WriteGPR() {
  Status error = ReadGPR();
  if (error.Fail())
    return error;

  struct iovec ioVec;
  ioVec.iov_base = GetGPRBuffer();
  ioVec.iov_len = GetGPRBufferSize();

  m_gpr_is_valid = false;

  return WriteRegisterSet(&ioVec, GetGPRBufferSize(), NT_PRSTATUS);
}

Status NativeRegisterContextLinux_riscv64::ReadFPR() {
  Status error;

  if (m_fpu_is_valid)
    return error;

  struct iovec ioVec;
  ioVec.iov_base = GetFPRBuffer();
  ioVec.iov_len = GetFPRSize();

  error = ReadRegisterSet(&ioVec, GetFPRSize(), NT_FPREGSET);

  if (error.Success())
    m_fpu_is_valid = true;

  return error;
}

Status NativeRegisterContextLinux_riscv64::WriteFPR() {
  Status error = ReadFPR();
  if (error.Fail())
    return error;

  struct iovec ioVec;
  ioVec.iov_base = GetFPRBuffer();
  ioVec.iov_len = GetFPRSize();

  m_fpu_is_valid = false;

  return WriteRegisterSet(&ioVec, GetFPRSize(), NT_FPREGSET);
}

void NativeRegisterContextLinux_riscv64::InvalidateAllRegisters() {
  m_gpr_is_valid = false;
  m_fpu_is_valid = false;
}

uint32_t NativeRegisterContextLinux_riscv64::CalculateFprOffset(
    const RegisterInfo *reg_info) const {
  return reg_info->byte_offset - GetGPRSize();
}

std::vector<uint32_t> NativeRegisterContextLinux_riscv64::GetExpeditedRegisters(
    ExpeditedRegs expType) const {
  std::vector<uint32_t> expedited_reg_nums =
      NativeRegisterContext::GetExpeditedRegisters(expType);

  return expedited_reg_nums;
}

#endif // defined (__riscv) || __riscv_xlen == 64
