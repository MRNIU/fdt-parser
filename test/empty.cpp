
// This file is a part of MRNIU/fdt-parser
// (https://github.com/MRNIU/fdt-parser).
// 
// empty.cpp for MRNIU/fdt-parser.

#include "fdt_parser.hpp"

// usage:
// ./bin/fdt_parser_test ../test/riscv64_qemu_virt.dtb
int main(int, char**) {
  auto result = FDT_PARSER::fdt_parser();
  return 0;
}
