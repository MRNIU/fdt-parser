
// This file is a part of MRNIU/fdt-parser
// (https://github.com/MRNIU/fdt-parser).
// Based on https://github.com/brenns10/sos
// test.cpp for MRNIU/fdt-parser.

#include <array>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "fdt_parser.hpp"

// usage:
// ./bin/fdt_parser_test ../test/riscv64_qemu_virt.dtb
int main(int, char** _argv) {
  std::ifstream input(_argv[1], std::ios::binary);
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
  assert(buffer.size() == 3810);

  std::array<uint8_t, 3810> fileArray;
  for (size_t i = 0; i < buffer.size(); i++) {
    fileArray[i] = buffer[i];
  }

  auto result = FDT_PARSER::fdt_parser((uintptr_t)fileArray.data());

  FDT_PARSER::resource_t resource_mem;
  resource_mem.type = FDT_PARSER::resource_t::MEM;
  result.find_via_prefix("memory@", &resource_mem);
  assert(strcmp(resource_mem.name, "memory@80000000") == 0);
  assert(resource_mem.mem.addr == 0x80000000);
  assert(resource_mem.mem.len == 0x8000000);

  FDT_PARSER::resource_t resource_clint;
  resource_clint.type = FDT_PARSER::resource_t::MEM;
  result.find_via_prefix("clint@", &resource_clint);
  assert(strcmp(resource_clint.name, "riscv,clint0") == 0);
  assert(resource_clint.mem.addr == 0x2000000);
  assert(resource_clint.mem.len == 0x10000);

  FDT_PARSER::resource_t resource_plic;
  resource_plic.type = FDT_PARSER::resource_t::MEM;
  result.find_via_prefix("plic@", &resource_plic);
  assert(strcmp(resource_plic.name, "riscv,plic0") == 0);
  assert(resource_plic.mem.addr == 0xC000000);
  assert(resource_plic.mem.len == 0x210000);

  return 0;
}
