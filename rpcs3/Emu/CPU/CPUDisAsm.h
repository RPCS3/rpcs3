#pragma once

#include <string>

#include <memory>

#include <utility>

#include <limits>

#include <type_traits>

#include "Utilities/StrFmt.h"

enum class cpu_disasm_mode: u32 {
  dump,
  interpreter,
  normal,
  compiler_elf,
  list,
  survey_cmd_size
};

class cpu_thread;

class CPUDisAsm {
  public: CPUDisAsm(cpu_disasm_mode mode,
    const u8 * offset, u32 start_pc = 0,
      const cpu_thread * cpu = nullptr): m_mode(mode),
  m_offset(offset - start_pc),
  m_start_pc(start_pc),
  m_cpu(cpu) {}

  virtual~CPUDisAsm() =
  default;

  cpu_disasm_mode change_mode(cpu_disasm_mode mode) {
    return std::exchange(m_mode, mode);
  }

  const u8 * change_ptr(const u8 * ptr) {
    return std::exchange(m_offset, ptr);
  }

  virtual u32 disasm(u32 pc) = 0;
  virtual std::pair <
  const void * , std::size_t > get_memory_span() const = 0;
  virtual std::unique_ptr < CPUDisAsm > copy_type_erased() const = 0;

  std::string last_opcode;
  u32 dump_pc;

  protected: cpu_disasm_mode m_mode;
  const u8 * m_offset;
  const u32 m_start_pc;
  const cpu_thread * m_cpu;
  u32 m_op = 0;

  std::string FormatInt(int v) const {
    const int min = std::numeric_limits < int > ::min();

    if (v == min) {
      return fmt::format("-0x%x", static_cast < unsigned int > (v));
    }

    const auto av = std::abs(v);

    if (av < 10) {
      return fmt::format("%d", v);
    }

    return fmt::format("%s0x%x", v < 0 ? "-" : "", av);
  }

  int PadOp(std::string_view op = {}, int min_spaces = 0) const {
    return m_mode == cpu_disasm_mode::normal ? (static_cast < int > (op.size()) + min_spaces) : 10;
  }

  void format_by_mode() {
    if (m_mode == cpu_disasm_mode::dump) {
      last_opcode = fmt::format("\t%08x:\t%02x %02x %02x %02x\t%s\n", dump_pc,
        static_cast < u8 > (m_op >> 24), static_cast < u8 > (m_op >> 16),
        static_cast < u8 > (m_op >> 8), static_cast < u8 > (m_op), last_opcode);
    } else if (m_mode == cpu_disasm_mode::interpreter) {
      last_opcode.insert(0, fmt::format("[%08x] %02x %02x %02x %02x: ", dump_pc,
        static_cast < u8 > (m_op >> 24), static_cast < u8 > (m_op >> 16),
        static_cast < u8 > (m_op >> 8), static_cast < u8 > (m_op >> 0)));
    } else if (m_mode == cpu_disasm_mode::compiler_elf) {
      last_opcode += '\n';
    } else if (m_mode == cpu_disasm_mode::normal) {
      // Do nothing
    } else {
      throw std::runtime_error("Invalid disasm mode");
    }
  }

  private: CPUDisAsm & operator = (const CPUDisAsm & ) = delete;
};
