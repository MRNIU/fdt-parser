
// This file is a part of MRNIU/fdt-parser
// (https://github.com/MRNIU/fdt-parser).
// Based on https://github.com/brenns10/sos
// fdt_parser.cpp for MRNIU/fdt-parser.

#include "cstdio"

/// @todo 不用 assert
#include "cassert"
#include "cstring"
#include "fdt_parser.hpp"

namespace FDT_PARSER {

// 所有节点
fdt_parser::node_t fdt_parser::nodes[MAX_NODES];
// 节点数
size_t fdt_parser::node_t::count = 0;
// 所有 phandle
fdt_parser::phandle_map_t fdt_parser::phandle_map[MAX_NODES];
// phandle 数
size_t fdt_parser::phandle_map_t::count = 0;

fdt_parser::dtb_info_t fdt_parser::dtb_info;

fdt_parser& fdt_parser::get_instance(void) {
  /// 定义全局 fdt_parser 对象
  static fdt_parser fdt_parser;
  return fdt_parser;
}

bool fdt_parser::dtb_init(void) {
  // 头信息
  dtb_info.header = (fdt_header_t*)BOOT_INFO::boot_info_addr;
  // 魔数
  assert(be32toh(dtb_info.header->magic) == FDT_MAGIC);
  // 版本
  assert(be32toh(dtb_info.header->version) == FDT_VERSION);
  // 设置大小
  BOOT_INFO::boot_info_size = be32toh(dtb_info.header->totalsize);
  // 内存保留区
  dtb_info.reserved =
      (fdt_reserve_entry_t*)(BOOT_INFO::boot_info_addr +
                             be32toh(dtb_info.header->off_mem_rsvmap));
  // 数据区
  dtb_info.data =
      BOOT_INFO::boot_info_addr + be32toh(dtb_info.header->off_dt_struct);
  // 字符区
  dtb_info.str =
      BOOT_INFO::boot_info_addr + be32toh(dtb_info.header->off_dt_strings);
  // 检查保留内存
  dtb_mem_reserved();
  // 初始化 map
  bzero(nodes, sizeof(nodes));
  bzero(phandle_map, sizeof(phandle_map));
  // 初始化节点的基本信息
  dtb_iter(DT_ITER_BEGIN_NODE | DT_ITER_END_NODE | DT_ITER_PROP, dtb_init_cb,
           nullptr);
  // 中断信息初始化，因为需要查找 phandle，所以在基本信息初始化完成后进行
  dtb_iter(DT_ITER_PROP, dtb_init_interrupt_cb, nullptr);
// #define DEBUG
#ifdef DEBUG
  // 输出所有信息
  for (size_t i = 0; i < nodes[0].count; i++) {
    std::cout << nodes[i].path << ": " << std::endl;
    for (size_t j = 0; j < nodes[i].prop_count; j++) {
      printf("%s: ", nodes[i].props[j].name);
      for (size_t k = 0; k < nodes[i].props[j].len / 4; k++) {
        printf("0x%X ", be32toh(((uint32_t*)nodes[i].props[j].addr)[k]));
      }
      printf("\n");
    }
  }
#undef DEBUG
#endif
  return true;
}

}  // namespace FDT_PARSER
