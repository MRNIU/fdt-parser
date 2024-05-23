
// This file is a part of MRNIU/fdt-parser
// (https://github.com/MRNIU/fdt-parser).
// Based on https://github.com/brenns10/sos
// test.cpp for MRNIU/fdt-parser.

#include <fstream>
#include <iostream>

#include "fdt_parser.hpp"

/// TODO
int main(int, char** _argv) {
  std::ifstream input(_argv[1], std::ios::binary);
  std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
  printf("\ndatalen: %ld\n", buffer.size());

  std::array<uint8_t, 3810> fileArray;

  for (size_t i = 0; i < buffer.size(); i++) {
    fileArray[i] = buffer[i];
    // printf("%X ", buffer[i]);
  }

  auto aaa = FDT_PARSER::fdt_parser((uintptr_t)fileArray.data());

  FDT_PARSER::resource_t resource;
  // 设置 resource 基本信息
  resource.type = FDT_PARSER::resource_t::MEM;
  resource.name = "test";
  aaa.find_via_prefix("memory@", &resource);
  std::cout << resource << std::endl;

  return 0;
}
