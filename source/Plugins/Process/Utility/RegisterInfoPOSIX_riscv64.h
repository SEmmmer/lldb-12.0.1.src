//===-- RegisterInfoPOSIX_riscv64.h -----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLDB_SOURCE_PLUGINS_PROCESS_UTILITY_REGISTERINFOPOSIX_RISCV64_H
#define LLDB_SOURCE_PLUGINS_PROCESS_UTILITY_REGISTERINFOPOSIX_RISCV64_H

#include "RegisterInfoAndSetInterface.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/lldb-private.h"
#include <map>

class RegisterInfoPOSIX_riscv64
    : public lldb_private::RegisterInfoAndSetInterface {
public:
  enum { GPRegSet = 0 };

  LLVM_PACKED_START
  struct GPR {
    uint64_t zero;
    uint64_t ra;
    uint64_t sp;
    uint64_t gp;
    uint64_t tp;
    uint64_t t[7]; //t0-6
    uint64_t s[12]; //fp/s0-s11
    uint64_t a[8]; //a0-7
  };
  LLVM_PACKED_END

  struct FPU {
    uint64_t f[32];
    uint32_t fcsr;
  };

  RegisterInfoPOSIX_riscv64(const lldb_private::ArchSpec &target_arch);

  size_t GetGPRSize() const override;

  size_t GetFPRSize() const override;

  const lldb_private::RegisterInfo *GetRegisterInfo() const override;

  uint32_t GetRegisterCount() const override;

  const lldb_private::RegisterSet *
  GetRegisterSet(size_t reg_set) const override;

  size_t GetRegisterSetCount() const override;

  size_t GetRegisterSetFromRegisterIndex(uint32_t reg_index) const override;

private:
  typedef std::map<uint32_t, std::vector<lldb_private::RegisterInfo>>
      per_vq_register_infos;

  per_vq_register_infos m_per_vq_reg_infos;

  const lldb_private::RegisterInfo *m_register_info_p;
  uint32_t m_register_info_count;
};

#endif
